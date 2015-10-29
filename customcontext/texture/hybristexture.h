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

#ifndef HYBRISTEXTURE_H
#define HYBRISTEXTURE_H

#include <QtQuick/QSGTexture>
#include <QtQuick/QQuickTextureFactory>
#include <QSharedPointer>

#include <QtGui/qopengl.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

class QQuickWindow;

namespace CustomContext {

class NativeBuffer
{
public:
    NativeBuffer(const QImage &image);
    ~NativeBuffer();

    static NativeBuffer *create(const QImage &image);

    void release();

    EGLClientBuffer buffer;
    EGLImageKHR eglImage;
    int width;
    int height;
    int stride;
    int format;
    bool hasAlpha;
};

class HybrisTexture : public QSGTexture
{
    Q_OBJECT
public:
    HybrisTexture(QSharedPointer<NativeBuffer> buffer);
    ~HybrisTexture();

    virtual int textureId() const;
    virtual QSize textureSize() const;
    virtual bool hasAlphaChannel() const;
    virtual bool hasMipmaps() const;
    virtual void bind();

    static HybrisTexture *create(const QImage &image);

private:
    mutable GLuint m_id;
    QSharedPointer<NativeBuffer> m_buffer;
    bool m_bound;
};

class HybrisTextureFactory : public QQuickTextureFactory
{
    Q_OBJECT
public:
    HybrisTextureFactory(NativeBuffer *buffer);
    ~HybrisTextureFactory();

    virtual QSGTexture *createTexture(QQuickWindow *window) const;
    virtual QSize textureSize() const;
    virtual int textureByteCount() const;
    virtual QImage image() const;

    static HybrisTextureFactory *create(const QImage &image);

private:
    QSharedPointer<NativeBuffer> m_buffer;
};

}

#endif // HYBRISTEXTURE_H
