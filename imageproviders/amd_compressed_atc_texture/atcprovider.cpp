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

#include "atcprovider.h"

#include <QFile>
#include <QDebug>
#include <qopenglfunctions.h>
#include <qqmlfile.h>

struct DDSFormat {
    quint32 dwMagic;
    quint32 dwSize;
    quint32 dwFlags;
    quint32 dwHeight;
    quint32 dwWidth;
    quint32 dwLinearSize;
    quint32 dummy1;
    quint32 dwMipMapCount;
    quint32 dummy2[11];
    struct {
        quint32 dummy3[2];
        quint32 dwFourCC;
        quint32 dummy4[5];
    } ddsPixelFormat;
    quint32 dwCaps1;
    quint32 dwCaps2;
    quint32 dummy5[3];
};

#define FOURCC(ch0, ch1, ch2, ch3) ((uint)(char)(ch0) | ((uint)(char)(ch1) << 8) | ((uint)(char)(ch2) << 16) | ((uint)(char)(ch3) << 24 ))

// FourCC codes used by the DDS file
#define FOURCC_ATC_RGB                     FOURCC( 'A', 'T', 'C', ' ' )
#define FOURCC_ATC_RGBA_EXPLICIT           FOURCC( 'A', 'T', 'C', 'A' )
#define FOURCC_ATC_RGBA_INTERPOLATED       FOURCC( 'A', 'T', 'C', 'I' )

// GL_AMD_compressed_ATC_texture extension
#ifndef GL_AMD_compressed_ATC_texture
#define GL_ATC_RGB_AMD                     0x8C92
#define GL_ATC_RGBA_EXPLICIT_ALPHA_AMD     0x8C93
#define GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD 0x87EE
#endif

AtcTexture::AtcTexture()
    : m_texture_id(0), m_uploaded(false)
{
}

AtcTexture::~AtcTexture()
{
    if (m_texture_id)
        glDeleteTextures(1, &m_texture_id);
}

int AtcTexture::textureId() const
{
    if (m_texture_id == 0)
        glGenTextures(1, &const_cast<AtcTexture *>(this)->m_texture_id);
    return m_texture_id;
}

void AtcTexture::bind()
{
    if (m_uploaded && m_texture_id) {
        glBindTexture(GL_TEXTURE_2D, m_texture_id);
        return;
    }

    if (m_texture_id == 0)
        glGenTextures(1, &m_texture_id);
    glBindTexture(GL_TEXTURE_2D, m_texture_id);

#ifndef QT_NO_DEBUG
    while (glGetError() != GL_NO_ERROR) { }
#endif

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    Q_ASSERT(ctx != 0);
    ctx->functions()->glCompressedTexImage2D(GL_TEXTURE_2D, 0, m_glFormat,
                                             m_size.width(), m_size.height(), 0,
                                             m_sizeInBytes,
                                             m_data.data() + sizeof(DDSFormat));

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

#ifndef QT_NO_DEBUG
    // Gracefully fail in case of an error...
    GLuint error = glGetError();
    if (error != GL_NO_ERROR) {
        qDebug () << "glCompressedTexImage2D for compressed texture failed, error: " << error;
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &m_texture_id);
        m_texture_id = 0;
        return;
    }
#endif

    m_uploaded = true;
    updateBindOptions(true);
}

bool AtcTexture::hasAlphaChannel() const
{
    return m_glFormat != GL_ATC_RGB_AMD;
}

class QAtcTextureFactory : public QQuickTextureFactory
{
public:
    QByteArray m_data;
    QSize m_size;
    GLuint m_sizeInBytes;
    GLenum m_glFormat;

    QSize textureSize() const { return m_size; }
    int textureByteCount() const { return m_data.size(); }

    QSGTexture *createTexture(QQuickWindow *) const {
        AtcTexture *texture = new AtcTexture;
        texture->m_data = m_data;
        texture->m_size = m_size;
        texture->m_sizeInBytes = m_sizeInBytes;
        texture->m_glFormat = m_glFormat;
        return texture;
    }
};

QQuickTextureFactory *AtcProvider::requestTexture(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize);
    QAtcTextureFactory *ret = 0;

    size->setHeight(0);
    size->setWidth(0);

    QUrl url = QUrl(id);
    if (url.isRelative() && !m_baseUrl.isEmpty())
        url = m_baseUrl.resolved(url);
    QString path = QQmlFile::urlToLocalFileOrQrc(url);

    QFile file(path);
#ifdef ETC_DEBUG
    qDebug() << "requestTexture opening file: " << path;
#endif
    if (file.open(QIODevice::ReadOnly)) {
        ret = new QAtcTextureFactory;
        ret->m_data = file.readAll();
        if (!ret->m_data.isEmpty()) {
            DDSFormat *ddsFormat = NULL;
            ddsFormat = (DDSFormat *)ret->m_data.data();

            int byteSizeMultiplier = 0;
            switch (ddsFormat->ddsPixelFormat.dwFourCC) {
            case FOURCC_ATC_RGB:
               ret->m_glFormat = GL_ATC_RGB_AMD;
               byteSizeMultiplier = 8;
               break;
            case FOURCC_ATC_RGBA_EXPLICIT:
               ret->m_glFormat = GL_ATC_RGBA_EXPLICIT_ALPHA_AMD;
               byteSizeMultiplier = 16;
               break;
            case FOURCC_ATC_RGBA_INTERPOLATED:
               ret->m_glFormat = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
               byteSizeMultiplier = 16;
               break;
            default:
               qWarning( "Unsupported compressed texture format." );
               delete ret;
               return 0;
            }

            size->setHeight(ddsFormat->dwHeight);
            size->setWidth(ddsFormat->dwWidth);
            ret->m_size = *size;

            ret->m_sizeInBytes = ((ddsFormat->dwWidth+3)/4) * ((ddsFormat->dwHeight+3)/4) * byteSizeMultiplier;
        }
        else {
            delete ret;
            ret = 0;
        }
    }

    return ret;
}

void AtcProvider::setBaseUrl(const QUrl &base)
{
    m_baseUrl = base;
}
