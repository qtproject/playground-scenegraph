/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Scenegraph Playground module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CONTEXT_H
#define CONTEXT_H

#include <private/qsgcontext_p.h>
#include <QtGui/QOpenGLShaderProgram>

#include "atlastexture.h"

namespace CustomContext
{


struct DitherProgram
{
    DitherProgram()
        : id_size(0)
        , id_texture(0)
        , prepared(false)
    {
    }

    void prepare();
    void draw(int width, int height);

    QOpenGLShaderProgram program;
    int id_size;
    GLuint id_texture;
    uint prepared : 1;
};

class Context : public QSGContext
{
    Q_OBJECT
public:
    explicit Context(QObject *parent = 0);

    void initialize(QOpenGLContext *context);
    void invalidate();
    void renderNextFrame(QSGRenderer *renderer, GLuint fbo);

    QAnimationDriver *createAnimationDriver(QObject *parent);
    QSGRenderer *createRenderer();

    QSurfaceFormat defaultSurfaceFormat() const;
    QSGTexture *createTexture(const QImage &image) const;
    QQuickTextureFactory *createTextureFactory(const QImage &image);

    void setDitherEnabled(bool enabled);

public slots:
    void handleInvalidated();

private:

    TextureAtlasManager m_atlas_manager;
    DitherProgram *dither;
    int samples;
    uint useCustomRenderer : 1;
    uint useCustomAnimationDriver : 1;
    uint useDefaultRectangles : 1;
    uint useMultisampling : 1;
    uint useDefaultTextures : 1;
    uint isCompositor : 1;
    uint useDithering : 1;
};

} // namespace

#endif // CONTEXT_H
