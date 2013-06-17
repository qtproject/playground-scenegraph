/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef ATLASTEXTURE_H
#define ATLASTEXTURE_H

#include <QtCore/QSize>

#include <QtGui/qopengl.h>

#include <QtQuick/QSGTexture>
#include <QtQuick/private/qsgtexture_p.h>
#include <QtQuick/private/qsgareaallocator_p.h>

namespace CustomContext
{

class AtlasTexture;
class TextureAtlas;

class TextureAtlasManager
{
public:
    TextureAtlasManager();
    ~TextureAtlasManager();

    QSGTexture *create(const QImage &image);
    void invalidate();
    void preload();

private:
    TextureAtlas *m_primary_atlas;
    TextureAtlas *m_secondary_atlas;

    QSize m_primary_atlas_size;
    QSize m_secondary_atlas_size;
    int m_atlas_size_limit;
};

class TextureAtlas
{
public:
    TextureAtlas(const QSize &size);
    ~TextureAtlas();

    void initialize();

    int textureId() const;
    bool bind();

    void upload(AtlasTexture *texture);
    void uploadBgra(AtlasTexture *texture);

    AtlasTexture *create(const QImage &image);
    void remove(AtlasTexture *t);

    QSize size() const { return m_size; }

private:
    QSGAreaAllocator m_allocator;
    GLuint m_texture_id;
    QSize m_size;
    QList<AtlasTexture *> m_pending_uploads;

    GLuint m_internalFormat;
    GLuint m_externalFormat;

    uint m_allocated : 1;
    uint m_use_bgra_fallback: 1;
};

class AtlasTexture : public QSGTexture
{
    Q_OBJECT
public:
    AtlasTexture(TextureAtlas *atlas, const QRect &textureRect, const QImage &image);
    ~AtlasTexture();

    int textureId() const { return m_atlas->textureId(); }
    QSize textureSize() const { return m_allocated_rect_without_padding.size(); }
    bool hasAlphaChannel() const { return m_image.hasAlphaChannel(); }
    bool hasMipmaps() const { return false; }

    QRectF normalizedTextureSubRect() const { return m_texture_coords_rect; }

    QRect atlasSubRect() const { return m_allocated_rect; }
    QRect atlasSubRectWithoutPadding() const { return m_allocated_rect_without_padding; }

    bool isAtlasTexture() const { return true; }

    QSGTexture *removedFromAtlas() const;

    const QImage &image() const { return m_image; }

    void bind();

private:
    QRect m_allocated_rect_without_padding;
    QRect m_allocated_rect;
    QRectF m_texture_coords_rect;

    QImage m_image;

    TextureAtlas *m_atlas;

    mutable QSGPlainTexture *m_nonatlas_texture;

};

}

#endif // ATLASTEXTURE_H
