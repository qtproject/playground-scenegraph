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

#include <QtCore/qdebug.h>
#include <QtCore/QCoreApplication>

#include "hybristexture.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QThread>


#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "drawhelper.h"

#ifndef QSG_NO_RENDER_TIMING
static bool qsg_render_timing = !qgetenv("QSG_RENDER_TIMING").isEmpty();
static QElapsedTimer qsg_renderer_timer;
#endif

// from hybris_nativebuffer.h in libhybris
#define HYBRIS_USAGE_SW_READ_RARELY     0x00000002
#define HYBRIS_USAGE_SW_WRITE_RARELY    0x00000020
#define HYBRIS_USAGE_HW_TEXTURE         0x00000100
#define HYBRIS_PIXEL_FORMAT_RGBA_8888   1
#define HYBRIS_PIXEL_FORMAT_BGRA_8888   5

#define EGL_NATIVE_BUFFER_HYBRIS             0x3140


namespace CustomContext {

extern "C" {
    typedef EGLBoolean (EGLAPIENTRYP _eglHybrisCreateNativeBuffer)(EGLint width, EGLint height, EGLint usage, EGLint format, EGLint *stride, EGLClientBuffer *buffer);
    typedef EGLBoolean (EGLAPIENTRYP _eglHybrisLockNativeBuffer)(EGLClientBuffer buffer, EGLint usage, EGLint l, EGLint t, EGLint w, EGLint h, void **vaddr);
    typedef EGLBoolean (EGLAPIENTRYP _eglHybrisUnlockNativeBuffer)(EGLClientBuffer buffer);
    typedef EGLBoolean (EGLAPIENTRYP _eglHybrisReleaseNativeBuffer)(EGLClientBuffer buffer);

    typedef void (EGLAPIENTRYP _glEGLImageTargetTexture2DOES)(GLenum target, EGLImageKHR image);
    typedef EGLImageKHR (EGLAPIENTRYP _eglCreateImageKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attribs);
    typedef EGLBoolean (EGLAPIENTRYP _eglDestroyImageKHR)(EGLDisplay dpy, EGLImageKHR image);
}

_glEGLImageTargetTexture2DOES glEGLImageTargetTexture2DOES = 0;
_eglCreateImageKHR eglCreateImageKHR = 0;
_eglDestroyImageKHR eglDestroyImageKHR = 0;

_eglHybrisCreateNativeBuffer eglHybrisCreateNativeBuffer = 0;
_eglHybrisLockNativeBuffer eglHybrisLockNativeBuffer = 0;
_eglHybrisUnlockNativeBuffer eglHybrisUnlockNativeBuffer = 0;
_eglHybrisReleaseNativeBuffer eglHybrisReleaseNativeBuffer = 0;

static void initialize()
{
    glEGLImageTargetTexture2DOES = (_glEGLImageTargetTexture2DOES) eglGetProcAddress("glEGLImageTargetTexture2DOES");
    eglCreateImageKHR = (_eglCreateImageKHR) eglGetProcAddress("eglCreateImageKHR");
    eglDestroyImageKHR = (_eglDestroyImageKHR) eglGetProcAddress("eglDestroyImageKHR");
    eglHybrisCreateNativeBuffer = (_eglHybrisCreateNativeBuffer) eglGetProcAddress("eglHybrisCreateNativeBuffer");
    eglHybrisLockNativeBuffer = (_eglHybrisLockNativeBuffer) eglGetProcAddress("eglHybrisLockNativeBuffer");
    eglHybrisUnlockNativeBuffer = (_eglHybrisUnlockNativeBuffer) eglGetProcAddress("eglHybrisUnlockNativeBuffer");
    eglHybrisReleaseNativeBuffer = (_eglHybrisReleaseNativeBuffer) eglGetProcAddress("eglHybrisReleaseNativeBuffer");
}

NativeBuffer::NativeBuffer(const QImage &image)
{
    hasAlpha = image.hasAlphaChannel();
    const QImage::Format iformat = image.format();
    format = iformat == QImage::Format_RGBA8888_Premultiplied
            || iformat == QImage::Format_RGBX8888
            || iformat == QImage::Format_RGBA8888
            ? HYBRIS_PIXEL_FORMAT_RGBA_8888
            : HYBRIS_PIXEL_FORMAT_BGRA_8888;
    int usage = HYBRIS_USAGE_SW_READ_RARELY | HYBRIS_USAGE_SW_WRITE_RARELY | HYBRIS_USAGE_HW_TEXTURE;
    width = image.width();
    height = image.height();
    stride = 0;
    buffer = 0;

#ifdef CUSTOMCONTEXT_DEBUG
    QElapsedTimer timer;
    timer.start();
#endif

    char *data = 0;

    EGLint status;

    status = eglHybrisCreateNativeBuffer(width, height, usage, format, &stride, &buffer);
    if (status != EGL_TRUE || !buffer) {
        qDebug() << "eglHybrisCreateNativeBuffer failed, error:" << hex << eglGetError();
        return;
    }

#ifdef CUSTOMCONTEXT_DEBUG
    quint64 allocTime = timer.elapsed();
#endif

    status = eglHybrisLockNativeBuffer(buffer, HYBRIS_USAGE_SW_WRITE_RARELY, 0, 0, width, height, (void **) &data);
    if (status != EGL_TRUE || data == 0) {
        qDebug() << "eglHybrisLockNativeBuffer failed, error:" << hex << eglGetError();
        release();
        return;
    }

#ifdef CUSTOMCONTEXT_DEBUG
    quint64 lockTime = timer.elapsed();
#endif

    bool simpleCopy = iformat == QImage::Format_ARGB32_Premultiplied
            || iformat == QImage::Format_RGB32
            || iformat == QImage::Format_RGBA8888_Premultiplied
            || iformat == QImage::Format_RGBX8888;

    const int dbpl = stride * 4;
    const int h = image.height();
    const int w = image.width();
    if (simpleCopy) {
        if (dbpl == image.bytesPerLine()) {
            memcpy(data, image.constBits(), image.byteCount());
        } else {
            int bpl = qMin(dbpl, image.bytesPerLine());
            for (int y=0; y<h; ++y)
                memcpy(data + y * dbpl, image.constScanLine(y), bpl);
        }
    } else {
        if (iformat == QImage::Format_ARGB32) {
            for (int y=0; y<h; ++y) {
                const uint *src = (const uint *) image.constScanLine(y);
                uint *dst = (uint *) (data + dbpl * y);
                for (int x=0; x<w; ++x)
                    dst[x] = PREMUL(src[x]);
            }
        } else { // QImage::Format_RGBA88888 (non-premultiplied)
            for (int y=0; y<h; ++y) {
                const uchar *src = image.constScanLine(y);
                uchar *dst = (uchar *) (data + dbpl * y);
                for (int x=0; x<w; ++x) {
                    int p = x<<2;
                    uint a = src[p + 3];
                    dst[p  ] = qt_div_255(src[p  ] * a);
                    dst[p+1] = qt_div_255(src[p+1] * a);
                    dst[p+2] = qt_div_255(src[p+2] * a);
                    dst[p+3] = a;
                }
            }
        }
    }

#ifdef CUSTOMCONTEXT_DEBUG
    quint64 copyTime = timer.elapsed();
#endif

    if (!eglHybrisUnlockNativeBuffer(buffer)) {
        qDebug() << "eglHybrisUnlockNativeBuffer failed, error:" << hex << eglGetError();
        release();
        return;
    }

#ifdef CUSTOMCONTEXT_DEBUG
    quint64 unlockTime  = timer.elapsed();
#endif

    eglImage = eglCreateImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY),
                                   EGL_NO_CONTEXT,
                                   EGL_NATIVE_BUFFER_HYBRIS,
                                   buffer,
                                   0);

    if (!eglImage) {
        qDebug() << "eglCreateImageKHR failed, error:" << hex << eglGetError();
        release();
        return;
    }

#ifdef CUSTOMCONTEXT_DEBUG
    quint64 imageTime = timer.elapsed();
#endif

#ifdef CUSTOMCONTEXT_DEBUG
    qDebug("HybrisTexture: created buffer: %d x %d, buffer=%p, stride=%d/%d (%d), time: alloc=%d, lock=%d, copy=%d, unlock=%d, eglImage=%d",
           width, height,
           buffer,
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
    qDebug() << "HybrisTexture: native buffer released.." << (void *) this;
#endif
    release();
}

void NativeBuffer::release()
{
    if (eglImage) {
        eglDestroyImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), eglImage);
        eglImage = 0;
    }
    if (buffer) {
        eglHybrisReleaseNativeBuffer(buffer);
        buffer = 0;
    }
}

NativeBuffer *NativeBuffer::create(const QImage &image)
{
    if (image.width() * image.height() < 500 * 500 || image.depth() != 32)
        return 0;

    if (!eglHybrisCreateNativeBuffer) {
        initialize();
        if (!eglHybrisCreateNativeBuffer)
            return 0;
    }

    NativeBuffer *buffer = new NativeBuffer(image);
    if (buffer && !buffer->buffer) {
#ifdef CUSTOMCONTEXT_DEBUG
        qDebug("HybrisTexture: failed to allocate native buffer for image: %d x %d", image.width(), image.height());
#endif
        delete buffer;
        return 0;
    }

    return buffer;
}

HybrisTexture::HybrisTexture(QSharedPointer<NativeBuffer> buffer)
    : m_id(0)
    , m_buffer(buffer)
    , m_bound(false)
{
    Q_ASSERT(buffer);
#ifdef CUSTOMCONTEXT_DEBUG
    qDebug() << "HybrisTexture: created" << QSize(buffer->width, buffer->height) << buffer << (void *) (buffer ? buffer->eglImage : 0) << (void *) (buffer ? buffer->buffer : 0);
#endif
}

HybrisTexture::~HybrisTexture()
{
    if (m_id)
        glDeleteTextures(1, &m_id);
}

void HybrisTexture::bind()
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
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_buffer->eglImage);

        int error;
        while ((error = glGetError()) != GL_NO_ERROR)
            qDebug() << "Error after glEGLImageTargetTexture2DOES" << hex << error;

        if (Q_UNLIKELY(qsg_render_timing))
            qDebug("   - Hybristexture(%dx%d) bind=%d ms", m_buffer->width, m_buffer->height, (int) qsg_renderer_timer.elapsed());
    }
}

int HybrisTexture::textureId() const
{
    if (m_id == 0)
        glGenTextures(1, &m_id);
    return m_id;
}

QSize HybrisTexture::textureSize() const
{
    return QSize(m_buffer->width, m_buffer->height);
}

bool HybrisTexture::hasAlphaChannel() const
{
    return m_buffer->hasAlpha;
}

bool HybrisTexture::hasMipmaps() const
{
    return false;
}

HybrisTexture *HybrisTexture::create(const QImage &image)
{
    NativeBuffer *buffer = NativeBuffer::create(image);
    if (!buffer)
        return 0;
    HybrisTexture *texture = new HybrisTexture(QSharedPointer<NativeBuffer>(buffer));
    return texture;
}

HybrisTextureFactory::HybrisTextureFactory(NativeBuffer *buffer)
    : m_buffer(buffer)
{
#ifdef CUSTOMCONTEXT_DEBUG
    qDebug("HybrisTexture: creating texture factory size=%dx%d, buffer=%p", buffer->width, buffer->height, buffer);
#endif
}

HybrisTextureFactory::~HybrisTextureFactory()
{
}

QSGTexture *HybrisTextureFactory::createTexture(QQuickWindow *) const
{
    return new HybrisTexture(m_buffer);
}

QSize HybrisTextureFactory::textureSize() const
{
    return QSize(m_buffer->width, m_buffer->height);
}

int HybrisTextureFactory::textureByteCount() const
{
    return m_buffer->stride * m_buffer->height;
}

QImage HybrisTextureFactory::image() const
{
    void *data = 0;
    if (!eglHybrisLockNativeBuffer(m_buffer->buffer, HYBRIS_USAGE_SW_READ_RARELY, 0, 0, m_buffer->width, m_buffer->height, (void **) &data) || data == 0) {
        qDebug() << "HybrisTextureFactory::image(): lock failed, cannot get image data; error:" << hex << eglGetError();
        return QImage();
    }

    QImage content = QImage((const uchar *) data, m_buffer->width, m_buffer->height, m_buffer->stride * 4,
                            m_buffer->hasAlpha ? QImage::Format_ARGB32_Premultiplied
                                                 : QImage::Format_RGB32).copy();

    if (!eglHybrisUnlockNativeBuffer(m_buffer->buffer)) {
        qDebug() << "HybrisTextureFactory::image(): unlock failed; error:" << hex << eglGetError();
        return QImage();
    }

    return content;
}

HybrisTextureFactory *HybrisTextureFactory::create(const QImage &image)
{
    NativeBuffer *buffer = NativeBuffer::create(image);
    return buffer ? new HybrisTextureFactory(buffer) : 0;
}

}
