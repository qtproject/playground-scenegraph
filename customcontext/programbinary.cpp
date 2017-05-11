/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Scenegraph Playground module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "context.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QFile>
#include <QtCore/QSaveFile>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QElapsedTimer>

#include <QtQuick/qsgmaterial.h>

#include <EGL/egl.h>

/*
 * The programbinary support makes use of the GL_OES_get_program_binary
 * extension to cache programs binaries for use in the scenegraph.
 *
 * Program binaries are by default stored under "/tmp/$USER-qsg-bs-store".
 * The user can override this location using an environment variable.
 * Program binaries are written and read with user privileges, as
 * is the directory in which they are stored.
 *
 * Filenames are generated based on a SHA1 from the source code.
 *
 * Issues:
 * - If there are conflicting SHA1's there will be problems, but in
 *   real life, this will not happen.
 * - If multiple users use the same cache locations there will be
 *   read/write issues. Don't do that, it isn't meant to be supported.
 */

#define GL_PROGRAM_BINARY_LENGTH 0x8741

#ifndef QT_OPENGL_ES_3
extern "C" {
    typedef void (* _glGetProgramBinary)(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);
    typedef void (* _glProgramBinary)(GLuint program, GLenum binaryFormat, const void *binary, GLint length);
}
static _glGetProgramBinary glGetProgramBinary;
static _glProgramBinary glProgramBinary;
#endif

namespace CustomContext {

class QSGMaterialShader_Accessor : public QSGMaterialShader {
public:

    static QSGMaterialShader_Accessor *from(QSGMaterialShader *s) {
        return (QSGMaterialShader_Accessor *) s;
    }
    const char *vertexCode() const { return vertexShader(); }
    const char *fragmentCode() const { return fragmentShader(); }
};

struct ProgramBinary
{
    QByteArray key;
    QByteArray blob;
    uint format;
    QByteArray vsh;
    QByteArray fsh;
};

int get_env_int(const char *name, int defaultValue)
{
    QByteArray v = qgetenv(name);
    bool ok;
    int value = v.toInt(&ok);
    if (!ok)
        value = defaultValue;
    return value;
}

class ProgramBinaryStore
{
public:
    ProgramBinaryStore()
        : m_hash(QCryptographicHash::Sha1)
    {
        m_maxShaderCount = get_env_int("QSG_PROGRAM_BINARY_LIMIT", 512);

        m_location = QString::fromLocal8Bit(qgetenv("QSG_PROGRAM_BINARY_STORE"));
        if (m_location.isEmpty()) {
            QString base = QString::fromLocal8Bit(qgetenv("XDG_RUNTIME_DIR"));
            if (base.isEmpty())
                base = QDir().tempPath() + QStringLiteral("/.") + QString::fromLocal8Bit(qgetenv("USER")) + QStringLiteral("-");
            else
                base += QStringLiteral("/");
            m_location = base + "qsg-pb-store";
        }
        QDir().mkpath(m_location);
#ifdef CUSTOMCONTEXT_DEBUG
        qDebug() << "Customcontext: binary shaders stored in:" << m_location;
#endif

#ifndef QT_OPENGL_ES_3
        glGetProgramBinary = (_glGetProgramBinary) eglGetProcAddress("glGetProgramBinaryOES");
        glProgramBinary = (_glProgramBinary) eglGetProcAddress("glProgramBinaryOES");

        Q_ASSERT(glGetProgramBinary && glProgramBinary);
#endif
    }

    static ProgramBinaryStore *self() {
        if (!instance)
            instance = new ProgramBinaryStore();
        return instance;
    }

    QByteArray key(QSGMaterialShader *shader, const char *v, const char *f);
    QString fileName(const QByteArray &key) { return m_location + QStringLiteral("/") + QString::fromLatin1(key); }
    ProgramBinary *lookup(const QByteArray key);
    void insert(ProgramBinary *shader);
    void purge(const QByteArray &key);

    void sanityCheck();
#if QT_VERSION >= 0x050800
    void compileAndInsert(QSGDefaultRenderContext *rc, const QByteArray &key, QSGMaterialShader *s, QSGMaterial *m, const char *v, const char *f);
#else
    void compileAndInsert(QSGRenderContext *rc, const QByteArray &key, QSGMaterialShader *s, QSGMaterial *m, const char *v, const char *f);
#endif

private:
    QCryptographicHash m_hash;
    QHash<QByteArray, ProgramBinary *> m_store;
    QMutex m_mutex;

    QString m_location;

    int m_maxShaderCount;

    static ProgramBinaryStore *instance;
};

ProgramBinaryStore *ProgramBinaryStore::instance = 0;

QByteArray ProgramBinaryStore::key(QSGMaterialShader *shader, const char *v, const char *f)
{
    QSGMaterialShader_Accessor *s = QSGMaterialShader_Accessor::from(shader);
    const char *vsh = v ? v : s->vertexCode();
    const char *fsh = f ? f : s->fragmentCode();
    return QCryptographicHash::hash(QByteArray(vsh) + QByteArray(fsh), QCryptographicHash::Sha1).toHex();
}

ProgramBinary *ProgramBinaryStore::lookup(const QByteArray key)
{
    QMutexLocker lock(&m_mutex);

    ProgramBinary *s = m_store.value(key);
    if (s)
        return s;

    QFile file(fileName(key));
    if (file.open(QFile::ReadOnly)) {
        QDataStream stream(&file);
        s = new ProgramBinary();
        s->key = key;
        stream >> s->format;
        stream >> s->blob;
        m_store.insert(key, s);
        return s;
    }

    return 0;
}

void ProgramBinaryStore::insert(ProgramBinary *shader)
{
    m_mutex.lock();

    m_store.insert(shader->key, shader);

    QSaveFile file(fileName(shader->key));
    if (file.open(QFile::WriteOnly)) {
        QDataStream stream(&file);
        stream << shader->format;
        stream << shader->blob;
        file.commit();
    }

    m_mutex.unlock();

    sanityCheck();
}

void ProgramBinaryStore::purge(const QByteArray &key)
{
    QMutexLocker lock(&m_mutex);

    delete m_store.take(key);

    QFile file(fileName(key));
    if (file.exists())
        file.remove();
}
#if QT_VERSION >= 0x050800
void ProgramBinaryStore::compileAndInsert(QSGDefaultRenderContext *rc, const QByteArray &key, QSGMaterialShader *s, QSGMaterial *m, const char *v, const char *f)
#else
void ProgramBinaryStore::compileAndInsert(QSGRenderContext *rc, const QByteArray &key, QSGMaterialShader *s, QSGMaterial *m, const char *v, const char *f)
#endif
{
    // Use the baseclass impl to do the actual compilation
#if QT_VERSION >= 0x050800
    rc->QSGDefaultRenderContext::compileShader(s, m, v, f);
#else
    rc->QSGRenderContext::compile(s, m, v, f);
#endif
    QOpenGLShaderProgram *p = s->program();
    if (p->isLinked()) {
        ProgramBinary *b = new ProgramBinary;
        int length = 0;
        int id = p->programId();
        QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
        funcs->glGetProgramiv(id, GL_PROGRAM_BINARY_LENGTH, &length);
        b->blob.resize(length);
        glGetProgramBinary(id, b->blob.size(), &length, &b->format, b->blob.data());
        b->key = key;
        if (length != b->blob.size()) {
#ifdef CUSTOMCONTEXT_DEBUG
            qDebug() << " - glGetProgramBinary did not return expected results...";
#endif
            delete b;
        } else {
            insert(b);
        }
    }
}

#if QT_VERSION >= 0x050800
void RenderContext::compileShader(QSGMaterialShader *shader, QSGMaterial *material, const char *vertex, const char *fragment)
#else
void RenderContext::compile(QSGMaterialShader *shader, QSGMaterial *material, const char *vertex, const char *fragment)
#endif
{
    Q_ASSERT(QOpenGLContext::currentContext()->extensions().contains("GL_OES_get_program_binary"));

    // We cannot cache shaders which have custom compilation
    if (material->flags() & QSGMaterial::CustomCompileStep) {
#if QT_VERSION >= 0x050800
        QSGDefaultRenderContext::compileShader(shader, material, vertex, fragment);
#else
        QSGRenderContext::compile(shader, material, vertex, fragment);
#endif
        return;
    }

    ProgramBinaryStore *store = ProgramBinaryStore::self();
    QByteArray key = store->key(shader, vertex, fragment);
    ProgramBinary *binary = store->lookup(key);

    if (!binary) {
#ifdef CUSTOMCONTEXT_DEBUG
        qDebug() << " - program binary not found, compiling and inserting" << key;
#endif
        store->compileAndInsert(this, key, shader, material, vertex, fragment);

    } else {
#ifdef CUSTOMCONTEXT_DEBUG
        qDebug() << " - found stored program binary" << key;
#endif
        QOpenGLShaderProgram *p = shader->program();
        // The program might not have an id yet. addShader(0) will trick it into making one.
        if (p->programId() == 0)
            p->addShader(0);
        // Upload precompiled binary
        glProgramBinary(p->programId(), binary->format, binary->blob.data(), binary->blob.size());
        p->link();
        if (!p->isLinked()) {
#ifdef CUSTOMCONTEXT_DEBUG
            qDebug() << " - program binary" << key << "failed to link, purging and recreating";
#endif
            // If it failed, purge the binary from the store and compile and insert a new one.
            store->purge(key);
            store->compileAndInsert(this, key, shader, material, vertex, fragment);
        }
    }
}

static bool sortByAccessTime(const QFileInfo &a, const QFileInfo &b)
{
    return a.lastRead() < b.lastRead();
}

/* The primary goal of the sanity check is that we should not
 * pollute the file system with an infinite amount of shaders,
 * even if each one is fairly small.
 *
 * Pollution can quite easily happen because of QML and ShaderEffect.
 * For instance, if an application has a shader effect editor which
 * changes the fragment/vertex strings every few seconds the file system
 * will fill up with thousands of unused shaders. Better to nuke them.
 *
 * If an app has "many" shaders check if the amount on disk is
 * more than maxShaderCount. If so, delete them so we are left with
 * maxShaderCount/2. We also nuke the store, as the programs
 * themselves are anyway cached by the renderers and recovering them
 * from disk again is cheapish.
 *
 * Multiple processes with different maxShaderCount will work against
 * different counts, so they might end up conflicting, but the default
 * value is set high enough for this to not be a problem and since the
 * store is user-local. If he/she changes it to a low value, it is their
 * problem.
 */
void ProgramBinaryStore::sanityCheck()
{
    if (m_store.size() > m_maxShaderCount) {
        QDir dir(m_location);
        QList<QFileInfo> infos = dir.entryInfoList(QDir::Files);
#ifdef CUSTOMCONTEXT_DEBUG
        qDebug() << " - BinaryProgramStore has" << m_store.size() << "entries in process," << infos.size() << "on disk..." << m_maxShaderCount;
#endif
        if (infos.size()) {
            std::sort(&infos.first(), &infos.last() + 1, sortByAccessTime);
            int firstToKeep = infos.size() - m_maxShaderCount / 2;
            for (int i=0; i<firstToKeep; ++i)
                purge(infos.at(i).fileName().toLocal8Bit());
        }
    }
}

}
