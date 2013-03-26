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

#include "context.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>

#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>

#include <QtQuick/QSGFlatColorMaterial>
#include <QtQuick/QSGVertexColorMaterial>
#include <QtQuick/QSGOpaqueTextureMaterial>
#include <QtQuick/QSGTextureMaterial>
#include <private/qsgdefaultimagenode_p.h>
#include <private/qsgdefaultrectanglenode_p.h>
#include <private/qsgdistancefieldglyphnode_p_p.h>

#ifdef CUSTOMCONTEXT_ANIMATIONDRIVER
#include "animation/animationdriver.h"
#endif

#ifdef CUSTOMCONTEXT_SWAPLISTENINGANIMATIONDRIVER
#include "animation/swaplisteninganimationdriver.h"
#endif

#ifdef CUSTOMCONTEXT_OVERLAPRENDERER
#include "renderer/overlaprenderer.h"
#endif

#ifdef CUSTOMCONTEXT_MACTEXTURE
#include "mactexture.h"
#endif

#ifdef CUSTOMCONTEXT_NONPRESERVEDTEXTURE
#include "nonpreservedtexture.h"
#endif



namespace CustomContext
{

Context::Context(QObject *parent)
    : QSGContext(parent)
    , m_sampleCount(0)
    , m_useMultisampling(false)
{
    m_useMultisampling = qgetenv("CUSTOMCONTEXT_NO_MULTISAMPLE").isEmpty();
    if (m_useMultisampling) {
        m_sampleCount= 4;
        QByteArray overrideSamples = qgetenv("CUSTOMCONTEXT_MULTISAMPLE_COUNT");
        bool ok;
        int override = overrideSamples.toInt(&ok);
        if (ok)
            m_sampleCount = override;
    }

#ifdef CUSTOMCONTEXT_MATERIALPRELOAD
    m_materialPreloading = qgetenv("CUSTOMCONTEXT_NO_MATERIAL_PRELOADING").isEmpty();
#endif
    m_depthBuffer = qgetenv("CUSTOMCONTEXT_NO_DEPTH_BUFFER").isEmpty();

#ifdef CUSTOMCONTEXT_OVERLAPRENDERER
    m_overlapRenderer = qgetenv("CUSTOMCONTEXT_NO_OVERLAPRENDERER").isEmpty();
#endif

#ifdef CUSTOMCONTEXT_ANIMATIONDRIVER
    m_animationDriver = qgetenv("CUSTOMCONTEXT_NO_ANIMATIONDRIVER").isEmpty();
#endif

#ifdef CUSTOMCONTEXT_SWAPLISTENINGANIMATIONDRIVER
    m_swapListeningAnimationDriver = qgetenv("CUSTOMCONTEXT_NO_SWAPLISTENINGANIMATIONDRIVER").isEmpty();
#endif

#ifdef CUSTOMCONTEXT_ATLASTEXTURE
    m_atlasTexture = qgetenv("CUSTOMCONTEXT_NO_ATLASTEXTURE").isEmpty();
#endif

#ifdef CUSTOMCONTEXT_MACTEXTURE
    m_macTexture = qgetenv("CUSTOMCONTEXT_NO_MACTEXTURE").isEmpty();
#endif

#ifdef CUSTOMCONTEXT_DITHER
    m_dither= qgetenv("CUSTOMCONTEXT_NO_DITHER").isEmpty();
    m_ditherProgram = 0;
#endif

#ifdef CUSTOMCONTEXT_THREADUPLOADTEXTURE
    m_threadUploadTexture = qgetenv("CUSTOMCONTEXT_NO_THREADUPLOADTEXTURE").isEmpty();
    connect(this, SIGNAL(invalidated()), &m_threadUploadManager, SLOT(invalidated()), Qt::DirectConnection);
#endif

#ifdef CUSTOMCONTEXT_NONPRESERVEDTEXTURE
    m_nonPreservedTexture = qgetenv("CUSTOMCONTEXT_NO_NONPRESERVEDTEXTURE").isEmpty();
#endif



#ifdef CUSTOMCONTEXT_DEBUG
    qDebug("CustomContext created:");
    qDebug(" - multisampling: %s, samples=%d", m_useMultisampling ? "yes" : "no", m_sampleCount);
    qDebug(" - depth buffer: %s", m_depthBuffer ? "yes" : "no");

#ifdef CUSTOMCONTEXT_MATERIALPRELOAD
    qDebug(" - material preload: %s", m_materialPreloading ? "yes" : "no");
#endif
#ifdef CUSTOMCONTEXT_OVERLAPRENDERER
    qDebug(" - overlaprenderer: %s", m_overlapRenderer ? "yes" : "no");
#endif
#ifdef CUSTOMCONTEXT_SWAPLISTENINGANIMATIONDRIVER
    qDebug(" - swap listening animation driver: %s", m_swapListeningAnimationDriver ? "yes" : "no");
#endif
#ifdef CUSTOMCONTEXT_ANIMATIONDRIVER
    qDebug(" - custom animation driver: %s", m_animationDriver ? "yes" : "no");
#endif
#ifdef CUSTOMCONTEXT_ATLASTEXTURE
    qDebug(" - atlas textures: %s", m_atlasTexture ? "yes" : "no" );
#endif
#ifdef CUSTOMCONTEXT_THREADUPLOADTEXTURE
    qDebug(" - threaded texture upload: %s", m_threadUploadTexture ? "yes" : "no");
#endif
#ifdef CUSTOMCONTEXT_MACTEXTURE
    qDebug(" - mac textures: %s", m_macTexture ? "yes" : "no");
#endif
#ifdef CUSTOMCONTEXT_NONPRESERVEDTEXTURE
    qDebug(" - non preserved textures: %s", m_nonPreservedTexture ? "yes" : "no");
#endif

#ifdef CUSTOMCONTEXT_DITHER
    qDebug(" - ordered 2x2 dither: %s", m_dither ? "yes" : "no");
#endif

#endif

}



void Context::initialize(QOpenGLContext *context)
{

#ifdef CUSTOMCONTEXT_DITHER
    if (m_dither)
        m_ditherProgram = new OrderedDither2x2(context);
#endif

#ifdef CUSTOMCONTEXT_THREADUPLOADTEXTURE
    if (m_threadUploadTexture)
        m_threadUploadManager.initialized(context);
#endif

#ifdef CUSTOMCONTEXT_MATERIALPRELOAD
#ifdef CUSTOMCONTEXT_DEBUG
    QElapsedTimer prepareTimer;
    prepareTimer.start();
#endif
    if (m_materialPreloading) {
        {
            QSGVertexColorMaterial m;
            prepareMaterial(&m);
        }
        {
            QSGFlatColorMaterial m;
            prepareMaterial(&m);
        }
        {
            QSGOpaqueTextureMaterial m;
            prepareMaterial(&m);
        }
        {
            QSGTextureMaterial m;
            prepareMaterial(&m);
        }
        {
            SmoothTextureMaterial m;
            prepareMaterial(&m);
        }
        {
            SmoothColorMaterial m;
            prepareMaterial(&m);
        }
        {
            QSGDistanceFieldTextMaterial m;
            prepareMaterial(&m);
        }
    }
#endif

#ifdef CUSTOMCONTEXT_DEBUG
    qDebug("CustomContext: initialized..");
#ifdef CUSTOMCONTEXT_MATERIALPRELOAD
    if (m_materialPreloading)
        qDebug(" - Standard materials compiled in: %d ms", (int) prepareTimer.elapsed());
#endif
    qDebug(" - OpenGL extensions: %s", glGetString(GL_EXTENSIONS));
    qDebug(" - OpenGL Vendor: %s", glGetString(GL_VENDOR));
    qDebug(" - OpenGL Version: %s", glGetString(GL_VERSION));
    qDebug(" - OpenGL Renderer: %s", glGetString(GL_RENDERER));
    qDebug(" - OpenGL Shading Language Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    int textureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &textureSize);
    qDebug(" - GL Max Texture Size: %d", textureSize);
#endif


    QSGContext::initialize(context);
}

void Context::invalidate()
{
    QSGContext::invalidate();

#ifdef CUSTOMCONTEXT_DITHER
    delete m_ditherProgram;
    m_ditherProgram = 0;
#endif

#ifdef CUSTOMCONTEXT_ATLASTEXTURE
    m_atlasManager.invalidate();
#endif
}



QSurfaceFormat Context::defaultSurfaceFormat() const
{
    QSurfaceFormat format;
    format.setStencilBufferSize(8);
    if (m_depthBuffer)
        format.setDepthBufferSize(24);
    if (m_useMultisampling)
        format.setSamples(m_sampleCount);
    return format;
}



QSGTexture *Context::createTexture(const QImage &image) const
{
#ifdef CUSTOMCONTEXT_ATLASTEXTURE
    if (m_atlasTexture) {
        QSGTexture *t = const_cast<Context *>(this)->m_atlasManager.create(image);
        if (t)
            return t;
    }
#endif

#ifdef CUSTOMCONTEXT_MACTEXTURE
    if (m_macTexture)
        return new MacTexture(image);
#endif

    return QSGContext::createTexture(image);
}



QSGRenderer *Context::createRenderer()
{
#ifdef CUSTOMCONTEXT_OVERLAPRENDERER
    if (m_overlapRenderer)
        return new OverlapRenderer::Renderer(this);
#endif
    return QSGContext::createRenderer();
}



QAnimationDriver *Context::createAnimationDriver(QObject *parent)
{
#ifdef CUSTOMCONTEXT_ANIMATIONDRIVER
    if (m_animationDriver)
        return new AnimationDriver();
#endif
#ifdef CUSTOMCONTEXT_SWAPLISTENINGANIMATIONDRIVER
    if (m_swapListeningAnimationDriver)
        return new SwapListeningAnimationDriver();
#endif

   return QSGContext::createAnimationDriver(parent);
}



QQuickTextureFactory *Context::createTextureFactory(const QImage &image)
{
    Q_UNUSED(image);

#ifdef CUSTOMCONTEXT_THREADUPLOADTEXTURE
    if (m_threadUploadTexture)
        return m_threadUploadManager.create(image);
#endif

#ifdef CUSTOMCONTEXT_NONPRESERVEDTEXTURE
    if (m_nonPreservedTexture)
        return new NonPreservedTextureFactory(image, this);
#endif

    return 0;
}



void Context::renderNextFrame(QSGRenderer *renderer, GLuint fbo)
{
    QSGContext::renderNextFrame(renderer, fbo);

#ifdef CUSTOMCONTEXT_DITHER
    if (m_dither) {
        QSurface *s = glContext()->surface();
        QSize size = static_cast<QWindow *>(s)->size();
        m_ditherProgram->prepare();
        m_ditherProgram->draw(size.width(), size.height());
    }
#endif
}



} // namespace
