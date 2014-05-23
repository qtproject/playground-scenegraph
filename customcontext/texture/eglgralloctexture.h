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

#ifndef EGLGRALLOCTEXTURE_H
#define EGLGRALLOCTEXTURE_H

#include <QtQuick/QSGTexture>
#include <QtQuick/QQuickTextureFactory>

#include <QtGui/qopengl.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <android/system/window.h>

class QQuickWindow;

namespace CustomContext {

class NativeBuffer : public ANativeWindowBuffer
{
public:
    NativeBuffer(const QImage &image);
    ~NativeBuffer();

    static void _incRef(android_native_base_t *base);
    static void _decRef(android_native_base_t *base);

    void release();

    EGLImageKHR eglImage() const { return m_eglImage; }
    void releaseEglImage();

    bool hasAlpha() const { return m_hasAlpha; }

    static NativeBuffer *create(const QImage &image);

private:
    QAtomicInt m_ref;
    EGLImageKHR m_eglImage;
    bool m_hasAlpha;
};

class EglGrallocTexture : public QSGTexture
{
    Q_OBJECT
public:
    EglGrallocTexture(NativeBuffer *buffer);
    ~EglGrallocTexture();

    virtual int textureId() const;
    virtual QSize textureSize() const;
    virtual bool hasAlphaChannel() const;
    virtual bool hasMipmaps() const;
    virtual void bind();

    static EglGrallocTexture *create(const QImage &image);

private:
    mutable GLuint m_id;
    NativeBuffer *m_buffer;
    bool m_bound;
    bool m_ownsBuffer;
};

class EglGrallocTextureFactory : public QQuickTextureFactory
{
    Q_OBJECT
public:
    EglGrallocTextureFactory(NativeBuffer *buffer);
    ~EglGrallocTextureFactory();

    virtual QSGTexture *createTexture(QQuickWindow *window) const;
    virtual QSize textureSize() const;
    virtual int textureByteCount() const;
    virtual QImage image() const;

    static EglGrallocTextureFactory *create(const QImage &image);

private:
    NativeBuffer *m_buffer;
};

}

#endif // EGLGRALLOCTEXTURE_H
