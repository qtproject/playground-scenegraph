/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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

#include "atlastexture.h"

#include <QtCore/QVarLengthArray>
#include <QtCore/QElapsedTimer>

#include <QtGui/QOpenGLContext>

#include <private/qsgtexture_p.h>

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif


#ifndef QSG_NO_RENDERER_TIMING
static bool qsg_render_timing = !qgetenv("QML_RENDERER_TIMING").isEmpty();
#endif

namespace CustomContext
{

int get_env_int(const char *name, int defaultValue)
{
    QByteArray content = qgetenv(name);

    bool ok = false;
    int value = content.toInt(&ok);
    return ok ? value : defaultValue;
}

TextureAtlasManager::TextureAtlasManager()
    : m_primary_atlas(0)
    , m_secondary_atlas(0)
{
    int primary = get_env_int("CustomContext_PRIMARY_ATLAS_SIZE", 1024);
    int secondary = get_env_int("CustomContext_SECONDARY_ATLAS_SIZE", 2048);

    m_atlas_size_limit = get_env_int("CustomContext_ATLAS_SIZE_LIMIT", 256);
    m_primary_atlas_size = QSize(primary, primary);
    m_secondary_atlas_size = QSize(secondary, secondary);
}


TextureAtlasManager::~TextureAtlasManager()
{
    invalidate();
}

void TextureAtlasManager::invalidate()
{
    delete m_primary_atlas;
    m_primary_atlas = 0;
    delete m_secondary_atlas;
    m_secondary_atlas = 0;
}

void TextureAtlasManager::preload()
{
    m_primary_atlas = new TextureAtlas(m_primary_atlas_size);
    m_primary_atlas->bind();
}

QSGTexture *TextureAtlasManager::create(const QImage &image)
{
    QSGTexture *t = 0;
    if (image.width() < m_atlas_size_limit && image.height() < m_atlas_size_limit) {

        // If the secondary atlas is larger, try to allocate in that one, so we get as many
        // images as possible batched together.
        if (m_secondary_atlas && m_secondary_atlas_size.width() > m_primary_atlas_size.width()) {
            t = m_secondary_atlas->create(image);
            if (t)
                return t;
        }

        if (!m_primary_atlas)
            m_primary_atlas = new TextureAtlas(m_primary_atlas_size);
        t = m_primary_atlas->create(image);
        if (t)
            return t;

        if (!m_secondary_atlas)
            m_secondary_atlas = new TextureAtlas(m_secondary_atlas_size);
        t = m_secondary_atlas->create(image);
        if (t)
            return t;
    }

    return t;
}

TextureAtlas::TextureAtlas(const QSize &size)
    : m_allocator(size)
    , m_texture_id(0)
    , m_size(size)
    , m_allocated(false)
{
    const char *ext = (const char *) glGetString(GL_EXTENSIONS);

#ifdef QT_OPENGL_ES
    if (strstr(ext, "GL_EXT_bgra")
            || strstr(ext, "GL_EXT_texture_format_BGRA8888")
            || strstr(ext, "GL_IMG_texture_format_BGRA8888")) {
        m_internalFormat = m_externalFormat = GL_BGRA;
    } else {
        m_internalFormat = m_externalFormat = GL_RGBA;
    }
#else
    m_internalFormat = GL_RGBA;
    m_externalFormat = GL_BGRA;
#endif

    m_use_bgra_fallback = !qgetenv("CustomContext_USE_BGRA_FALLBACK").isEmpty();
}

TextureAtlas::~TextureAtlas()
{
    if (m_texture_id)
        glDeleteTextures(1, &m_texture_id);
}


AtlasTexture *TextureAtlas::create(const QImage &image)
{
    QRect rect = m_allocator.allocate(QSize(image.width() + 2, image.height() + 2));
    if (rect.width() > 0 && rect.height() > 0) {
        AtlasTexture *t = new AtlasTexture(this, rect, image);
        m_pending_uploads << t;
        return t;
    }
    return 0;
}


int TextureAtlas::textureId() const
{
    if (!m_texture_id) {
        Q_ASSERT(QOpenGLContext::currentContext());
        glGenTextures(1, &const_cast<TextureAtlas *>(this)->m_texture_id);
    }

    return m_texture_id;
}

static void swizzleBGRAToRGBA(QImage *image)
{
    const int width = image->width();
    const int height = image->height();
    uint *p = (uint *) image->bits();
    int stride = image->bytesPerLine() / 4;
    for (int i = 0; i < height; ++i) {
        for (int x = 0; x < width; ++x)
            p[x] = ((p[x] << 16) & 0xff0000) | ((p[x] >> 16) & 0xff) | (p[x] & 0xff00ff00);
        p += stride;
    }
}

void TextureAtlas::upload(AtlasTexture *texture)
{
    const QImage &image = texture->image();
    const QRect &r = texture->atlasSubRect();

    QImage tmp(r.width(), r.height(), QImage::Format_ARGB32_Premultiplied);
    {
        QPainter p(&tmp);
        p.setCompositionMode(QPainter::CompositionMode_Source);

        int w = r.width();
        int h = r.height();
        int iw = image.width();
        int ih = image.height();

        p.drawImage(1, 1, image);
        p.drawImage(1, 0, image, 0, 0, iw, 1);
        p.drawImage(1, h - 1, image, 0, ih - 1, iw, 1);
        p.drawImage(0, 1, image, 0, 0, 1, ih);
        p.drawImage(w - 1, 1, image, iw - 1, 0, 1, ih);
        p.drawImage(0, 0, image, 0, 0, 1, 1);
        p.drawImage(0, h - 1, image, 0, ih - 1, 1, 1);
        p.drawImage(w - 1, 0, image, iw - 1, 0, 1, 1);
        p.drawImage(w - 1, h - 1, image, iw - 1, ih - 1, 1, 1);
    }

    if (m_externalFormat == GL_RGBA)
        swizzleBGRAToRGBA(&tmp);
    glTexSubImage2D(GL_TEXTURE_2D, 0, r.x(), r.y(), r.width(), r.height(), m_externalFormat, GL_UNSIGNED_BYTE, tmp.constBits());
}

void TextureAtlas::uploadBgra(AtlasTexture *texture)
{
    const QRect &r = texture->atlasSubRect();
    QImage image = texture->image();

    if (image.format() != QImage::Format_ARGB32_Premultiplied
            || image.format() != QImage::Format_RGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }

    QVarLengthArray<quint32, 512> tmpBits(qMax(image.width() + 2, image.height() + 2));
    int iw = image.width();
    int ih = image.height();
    int bpl = image.bytesPerLine() / 4;
    const quint32 *src = (const quint32 *) image.constBits();
    quint32 *dst = tmpBits.data();

    // top row, padding corners
    dst[0] = src[0];
    memcpy(dst + 1, src, iw * sizeof(quint32));
    dst[1 + iw] = src[iw-1];
    glTexSubImage2D(GL_TEXTURE_2D, 0, r.x(), r.y(), iw + 2, 1, m_externalFormat, GL_UNSIGNED_BYTE, dst);

    // bottom row, padded corners
    const quint32 *lastRow = src + bpl * (ih - 1);
    dst[0] = lastRow[0];
    memcpy(dst + 1, lastRow, iw * sizeof(quint32));
    dst[1 + iw] = lastRow[iw-1];
    glTexSubImage2D(GL_TEXTURE_2D, 0, r.x(), r.y() + ih + 1, iw + 2, 1, m_externalFormat, GL_UNSIGNED_BYTE, dst);

    // left column
    for (int i=0; i<ih; ++i)
        dst[i] = src[i * bpl];
    glTexSubImage2D(GL_TEXTURE_2D, 0, r.x(), r.y() + 1, 1, ih, m_externalFormat, GL_UNSIGNED_BYTE, dst);

    // right column
    for (int i=0; i<ih; ++i)
        dst[i] = src[i * bpl + iw - 1];
    glTexSubImage2D(GL_TEXTURE_2D, 0, r.x() + iw + 1, r.y() + 1, 1, ih, m_externalFormat, GL_UNSIGNED_BYTE, dst);

    // Inner part of the image....
    glTexSubImage2D(GL_TEXTURE_2D, 0, r.x() + 1, r.y() + 1, r.width() - 2, r.height() - 2, m_externalFormat, GL_UNSIGNED_BYTE, src);

}

bool TextureAtlas::bind()
{
    bool forceUpdate = false;
    if (!m_allocated) {
        m_allocated = true;

        while (glGetError() != GL_NO_ERROR) ;

        glGenTextures(1, &m_texture_id);
        glBindTexture(GL_TEXTURE_2D, m_texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, m_internalFormat, m_size.width(), m_size.height(), 0, m_externalFormat, GL_UNSIGNED_BYTE, 0);

#if 0
        QImage pink(m_size.width(), m_size.height(), QImage::Format_RGB32);
        pink.fill(0xffff00ff);
        glTexImage2D(GL_TEXTURE_2D, 0, m_internalFormat, m_size.width(), m_size.height(), 0, m_externalFormat, GL_UNSIGNED_BYTE, pink.constBits());
#endif

        GLenum errorCode = glGetError();
        if (errorCode == GL_OUT_OF_MEMORY) {
            qDebug("CustomContext: texture atlas allocation failed, out of memory");
            glDeleteTextures(1, &m_texture_id);
            m_texture_id = 0;
        } else if (errorCode != GL_NO_ERROR) {
            qDebug("CustomContext: texture atlas allocation failed, code=%x", errorCode);
            glDeleteTextures(1, &m_texture_id);
            m_texture_id = 0;
        }
        forceUpdate = true;
    } else {
        glBindTexture(GL_TEXTURE_2D, m_texture_id);
    }

    if (m_texture_id == 0)
        return false;

    // Upload all pending images..
    for (int i=0; i<m_pending_uploads.size(); ++i) {

#ifndef QSG_NO_RENDERER_TIMING
        QElapsedTimer timer;
        if (qsg_render_timing)
            timer.start();
#endif

        if (m_externalFormat == GL_BGRA &&
                !m_use_bgra_fallback) {
            uploadBgra(m_pending_uploads.at(i));
        } else {
            upload(m_pending_uploads.at(i));
        }
#ifndef QSG_NO_RENDERER_TIMING
        if (qsg_render_timing) {
            printf("   - atlastexture(%dx%d), uploaded in %d ms\n",
                   m_pending_uploads.at(i)->image().width(),
                   m_pending_uploads.at(i)->image().height(),
                   (int) timer.elapsed());
        }
#endif
    }


    m_pending_uploads.clear();

    return forceUpdate;
}

void TextureAtlas::remove(AtlasTexture *t)
{
    QRect atlasRect = t->atlasSubRect();
    bool ok = m_allocator.deallocate(atlasRect);

    m_pending_uploads.removeOne(t);

    // ### Remove debug output...
    if (!ok)
        qDebug() << "TextureAtlas: failed to deallocate" << t << atlasRect;
}



AtlasTexture::AtlasTexture(TextureAtlas *atlas, const QRect &textureRect, const QImage &image)
    : QSGTexture()
    , m_allocated_rect(textureRect)
    , m_image(image)
    , m_atlas(atlas)
    , m_nonatlas_texture(0)
{
    m_allocated_rect_without_padding = m_allocated_rect.adjusted(1, 1, -1, -1);
    float w = atlas->size().width();
    float h = atlas->size().height();

    m_texture_coords_rect = QRectF(m_allocated_rect_without_padding.x() / w,
                                   m_allocated_rect_without_padding.y() / h,
                                   m_allocated_rect_without_padding.width() / w,
                                   m_allocated_rect_without_padding.height() / h);
}

AtlasTexture::~AtlasTexture()
{
    m_atlas->remove(this);
    if (m_nonatlas_texture)
        delete m_nonatlas_texture;
}

void AtlasTexture::bind()
{
    m_atlas->bind();
    updateBindOptions(true);
}

QSGTexture *AtlasTexture::removedFromAtlas() const
{
    if (!m_nonatlas_texture) {
        m_nonatlas_texture = new QSGPlainTexture();
        m_nonatlas_texture->setImage(m_image);
    }
    return m_nonatlas_texture;
}

}
