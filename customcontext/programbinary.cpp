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
extern "C" {
    typedef void (* _glGetProgramBinary)(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);
    typedef void (* _glProgramBinary)(GLuint program, GLenum binaryFormat, const void *binary, GLint length);
}
static _glGetProgramBinary glGetProgramBinary;
static _glProgramBinary glProgramBinary;

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

class ProgramBinaryStore
{
public:
    ProgramBinaryStore()
        : m_hash(QCryptographicHash::Sha1)
    {
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

        glGetProgramBinary = (_glGetProgramBinary) eglGetProcAddress("glGetProgramBinaryOES");
        glProgramBinary = (_glProgramBinary) eglGetProcAddress("glProgramBinaryOES");

        Q_ASSERT(glGetProgramBinary && glProgramBinary);
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

    void compileAndInsert(QSGRenderContext *rc, const QByteArray &key, QSGMaterialShader *s, QSGMaterial *m, const char *v, const char *f);

private:
    QCryptographicHash m_hash;
    QHash<QByteArray, ProgramBinary *> m_store;
    QMutex m_mutex;

    QString m_location;

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
    QMutexLocker lock(&m_mutex);

    m_store.insert(shader->key, shader);

    QSaveFile file(fileName(shader->key));
    if (file.open(QFile::WriteOnly)) {
        QDataStream stream(&file);
        stream << shader->format;
        stream << shader->blob;
        file.commit();
    }
}

void ProgramBinaryStore::purge(const QByteArray &key)
{
    QMutexLocker lock(&m_mutex);

    delete m_store.take(key);

    QFile file(fileName(key));
    if (file.exists())
        file.remove();
}

void ProgramBinaryStore::compileAndInsert(QSGRenderContext *rc, const QByteArray &key, QSGMaterialShader *s, QSGMaterial *m, const char *v, const char *f)
{
    // Use the baseclass impl to do the actual compilation
    rc->QSGRenderContext::compile(s, m, v, f);
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

void RenderContext::compile(QSGMaterialShader *shader, QSGMaterial *material, const char *vertex, const char *fragment)
{
    Q_ASSERT(QOpenGLContext::currentContext()->extensions().contains("GL_OES_get_program_binary"));

    // We cannot cache shaders which have custom compilation
    if (material->flags() & QSGMaterial::CustomCompileStep) {
        QSGRenderContext::compile(shader, material, vertex, fragment);
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

}
