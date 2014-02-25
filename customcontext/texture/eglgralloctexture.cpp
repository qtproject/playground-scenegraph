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

#include "eglgralloctexture.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
#include <QtCore/qdebug.h>

#include <QtQuick/QQuickWindow>

#include <android/hardware/gralloc.h>
#include <android/system/window.h>


#include <EGL/egl.h>
#include <EGL/eglext.h>

// Taken from libhybris
#define container_of(ptr, type, member) ({                  \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})

#ifndef QSG_NO_RENDER_TIMING
static bool qsg_render_timing = !qgetenv("QSG_RENDER_TIMING").isEmpty();
static QElapsedTimer qsg_renderer_timer;
#endif

namespace CustomContext {

gralloc_module_t *gralloc = 0;
alloc_device_t *alloc = 0;

extern "C" {
    typedef void (* _glEGLImageTargetTexture2DOES)(GLenum target, EGLImageKHR image);
    typedef EGLImageKHR (* _eglCreateImageKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attribs);
    typedef EGLBoolean (* _eglDestroyImageKHR)(EGLDisplay dpy, EGLImageKHR image);
}
_glEGLImageTargetTexture2DOES glEGLImageTargetTexture2DOES = 0;
_eglCreateImageKHR eglCreateImageKHR = 0;
_eglDestroyImageKHR eglDestroyImageKHR = 0;

void initialize()
{
    hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **) &gralloc);
    gralloc_open((const hw_module_t *) gralloc, &alloc);

#ifdef CUSTOMCONTEXT_DEBUG
    qDebug("EglGrallocTexture: initializing gralloc=%p, alloc=%p", gralloc, alloc);
#endif

    glEGLImageTargetTexture2DOES = (_glEGLImageTargetTexture2DOES) eglGetProcAddress("glEGLImageTargetTexture2DOES");
    eglCreateImageKHR = (_eglCreateImageKHR) eglGetProcAddress("eglCreateImageKHR");
    eglDestroyImageKHR = (_eglDestroyImageKHR) eglGetProcAddress("eglDestroyImageKHR");
}

NativeBuffer::NativeBuffer(const QImage &image)
{
    m_hasAlpha = image.hasAlphaChannel();

    format = HAL_PIXEL_FORMAT_BGRA_8888;
    usage = GRALLOC_USAGE_SW_READ_RARELY | GRALLOC_USAGE_SW_WRITE_RARELY | GRALLOC_USAGE_HW_TEXTURE;
    width = image.width();
    height = image.height();
    stride = 0;
    handle = 0;
    common.incRef = _incRef;
    common.decRef = _decRef;

#ifdef CUSTOMCONTEXT_DEBUG
    QElapsedTimer timer;
    timer.start();
#endif

    char *data = 0;

    if (alloc->alloc(alloc, width,  height, format, usage, &handle, &stride)) {
        qDebug() << "alloc failed...";
        return;
    }

#ifdef CUSTOMCONTEXT_DEBUG
    quint64 allocTime = timer.elapsed();
#endif

    if (gralloc->lock(gralloc, handle, GRALLOC_USAGE_SW_WRITE_RARELY, 0, 0, width, height, (void **) &data) || data == 0) {
        qDebug() << "gralloc lock failed...";
        release();
        return;
    }

#ifdef CUSTOMCONTEXT_DEBUG
    quint64 lockTime = timer.elapsed();
#endif

    int dbpl = stride * 4;
    if (dbpl == image.bytesPerLine()) {
        memcpy(data, image.constBits(), image.byteCount());
    } else {
        int h = image.height();
        int bpl = qMin(dbpl, image.bytesPerLine());
        for (int y=0; y<h; ++y)
            memcpy(data + y * dbpl, image.scanLine(y), bpl);
    }

#ifdef CUSTOMCONTEXT_DEBUG
    quint64 copyTime = timer.elapsed();
#endif

    if (gralloc->unlock(gralloc, handle)) {
        qDebug() << "gralloc unlock failed...";
        release();
        return;
    }

#ifdef CUSTOMCONTEXT_DEBUG
    quint64 unlockTime  = timer.elapsed();
#endif

    m_eglImage = eglCreateImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY),
                                   EGL_NO_CONTEXT,
                                   EGL_NATIVE_BUFFER_ANDROID,
                                   (ANativeWindowBuffer *) this,
                                   0);

    if (!m_eglImage)
        qFatal("no egl image...");

#ifdef CUSTOMCONTEXT_DEBUG
    quint64 imageTime = timer.elapsed();
#endif

#ifdef CUSTOMCONTEXT_DEBUG
    qDebug("EglGrallocTexture: created gralloc buffer: %d x %d, handle=%p, stride=%d/%d (%d), time: alloc=%d, lock=%d, copy=%d, unlock=%d, eglImage=%d",
           width, height,
           handle,
           stride, stride * 4, image.bytesPerLine(),
           (int) allocTime,
           (int) (lockTime - allocTime),
           (int) (copyTime - lockTime),
           (int) (unlockTime - copyTime),
           (int) (imageTime - unlockTime));
#endif
}

NativeBuffer::~NativeBuffer()
{
#ifdef CUSTOMCONTEXT_DEBUG
    qDebug() << "EglGrallocTexture: native buffer released.." << (void *) this;
#endif
    release();
}

void NativeBuffer::releaseEglImage()
{
    eglDestroyImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), m_eglImage);
    m_eglImage = 0;
}

void NativeBuffer::_incRef(android_native_base_t* base)
{
    NativeBuffer *b = container_of(base, NativeBuffer, common);
    b->m_ref.ref();
}

void NativeBuffer::_decRef(android_native_base_t* base)
{
    NativeBuffer *b = container_of(base, NativeBuffer, common);
    if (!b->m_ref.deref())
        delete b;
}

void NativeBuffer::release()
{
    alloc->free(alloc, handle);
    handle = 0;
}

EglGrallocTexture::EglGrallocTexture(NativeBuffer *buffer)
    : m_id(0)
    , m_buffer(buffer)
    , m_bound(false)
{
    Q_ASSERT(buffer);
#ifdef CUSTOMCONTEXT_DEBUG
    qDebug() << "EglGrallocTexture: created" << QSize(buffer->width, buffer->height) << buffer << (void *) (buffer ? buffer->eglImage() : 0) << (void *) (buffer ? buffer->handle : 0);
#endif
}

EglGrallocTexture::~EglGrallocTexture()
{
    if (m_id)
        glDeleteTextures(1, &m_id);
}

void EglGrallocTexture::bind()
{
    glBindTexture(GL_TEXTURE_2D, textureId());
    updateBindOptions(!m_bound);

    if (!m_bound) {
        m_bound = true;

#ifdef CUSTOMCONTEXT_DEBUG
        qsg_render_timing = true;
#endif
        if (Q_UNLIKELY(qsg_render_timing))
            qsg_renderer_timer.start();

        while (glGetError() != GL_NO_ERROR); // Clear pending errors
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_buffer->eglImage());

        int error;
        while ((error = glGetError()) != GL_NO_ERROR)
            qDebug() << "Error after glEGLImageTargetTexture2DOES" << hex << error;

        if (Q_UNLIKELY(qsg_render_timing))
            qDebug("   - eglgralloctexture(%dx%d) bind=%d ms", m_buffer->width, m_buffer->height, (int) qsg_renderer_timer.elapsed());
    }
}

int EglGrallocTexture::textureId() const
{
    if (m_id == 0)
        glGenTextures(1, &m_id);
    return m_id;
}

QSize EglGrallocTexture::textureSize() const
{
    return QSize(m_buffer->width, m_buffer->height);
}

bool EglGrallocTexture::hasAlphaChannel() const
{
    return m_buffer->hasAlpha();
}

bool EglGrallocTexture::hasMipmaps() const
{
    return false;
}

EglGrallocTextureFactory::EglGrallocTextureFactory(NativeBuffer *buffer)
    : m_buffer(buffer)
{
#ifdef CUSTOMCONTEXT_DEBUG
    qDebug("EglGrallocTexture: creating texture factory size=%dx%d, buffer=%p", buffer->width, buffer->height, buffer);
#endif
}

EglGrallocTextureFactory::~EglGrallocTextureFactory()
{
    m_buffer->releaseEglImage();
}

QSGTexture *EglGrallocTextureFactory::createTexture(QQuickWindow *window) const
{
    return new EglGrallocTexture(m_buffer);
}

QSize EglGrallocTextureFactory::textureSize() const
{
    return QSize(m_buffer->width, m_buffer->height);
}

int EglGrallocTextureFactory::textureByteCount() const
{
    return m_buffer->stride * m_buffer->height;
}

QImage EglGrallocTextureFactory::image() const
{
    void *data = 0;
    if (gralloc->lock(gralloc, m_buffer->handle, GRALLOC_USAGE_SW_READ_RARELY, 0, 0, m_buffer->width, m_buffer->height, (void **) &data) || data == 0) {
        qDebug("EglGrallocTextureFactory::image(): lock failed, cannot get image data");
        return QImage();
    }

    QImage content = QImage((const uchar *) data, m_buffer->width, m_buffer->height, m_buffer->stride * 4,
                            m_buffer->hasAlpha() ? QImage::Format_ARGB32_Premultiplied
                                                 : QImage::Format_RGB32).copy();

    if (gralloc->unlock(gralloc, m_buffer->handle)) {
        qDebug("EglGrallocTextureFactory::image(): unlock failed");
        return QImage();
    }

    return content;
}

EglGrallocTextureFactory *EglGrallocTextureFactory::create(const QImage &image)
{
    if (image.width() * image.height() < 500 * 500)
        return 0;

    if (gralloc == 0) {
        initialize();
        if (!gralloc)
            return 0;
    }

    NativeBuffer *buffer = new NativeBuffer(image);
    if (buffer && !buffer->handle) {
#ifdef CUSTOMCONTEXT_DEBUG
        qDebug("EglGrallocTextureFactory: failed to allocate native buffer for image: %d x %d", image.width(), image.height());
#endif
        delete buffer;
        return 0;
    }
    return new EglGrallocTextureFactory(buffer);
}

}
