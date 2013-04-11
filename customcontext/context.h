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

#ifdef CUSTOMCONTEXT_DITHER
#include "renderhooks/ordereddither2x2.h"
#endif

#ifdef CUSTOMCONTEXT_ATLASTEXTURE
#include "atlastexture.h"
#endif

#ifdef CUSTOMCONTEXT_THREADUPLOADTEXTURE
#include "threaduploadtexture.h"
#endif



namespace CustomContext
{



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

private:

    int m_sampleCount;
    uint m_useMultisampling : 1;
    uint m_depthBuffer : 1;

#ifdef CUSTOMCONTEXT_MATERIALPRELOAD
    bool m_materialPreloading;
#endif

#ifdef CUSTOMCONTEXT_DITHER
    bool m_dither;
    OrderedDither2x2 *m_ditherProgram;
#endif

#ifdef CUSTOMCONTEXT_OVERLAPRENDERER
    bool m_overlapRenderer;
    QOpenGLShaderProgram m_clipProgram;
    int m_clipMatrixID;
#endif

#ifdef CUSTOMCONTEXT_ANIMATIONDRIVER
    bool m_animationDriver;
#endif

#ifdef CUSTOMCONTEXT_SWAPLISTENINGANIMATIONDRIVER
    bool m_swapListeningAnimationDriver;
#endif

#ifdef CUSTOMCONTEXT_ATLASTEXTURE
    TextureAtlasManager m_atlasManager;
    bool m_atlasTexture;
#endif

#ifdef CUSTOMCONTEXT_MACTEXTURE
    bool m_macTexture;
#endif

#ifdef CUSTOMCONTEXT_THREADUPLOADTEXTURE
    ThreadUploadTextureManager m_threadUploadManager;
    bool m_threadUploadTexture;
#endif

#ifdef CUSTOMCONTEXT_NONPRESERVEDTEXTURE
    bool m_nonPreservedTexture;
    friend class NonPreservedTextureFactory;
#endif



};

} // namespace

#endif // CONTEXT_H
