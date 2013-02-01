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

#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>

#ifdef CUSTOMCONTEXT_ANIMATIONDRIVER
#include "animation/animationdriver.h"
#endif

#ifdef CUSTOMCONTEXT_OVERLAPRENDERER
#include "renderer/overlaprenderer.h"
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

#ifdef CUSTOMCONTEXT_OVERLAPRENDERER
    m_overlapRenderer = qgetenv("CUSTOMCONTEXT_NO_OVERLAPRENDERER").isEmpty();
#endif

#ifdef CUSTOMCONTEXT_ANIMATIONDRIVER
    m_animationDriver = qgetenv("CUSTOMCONTEXT_NO_ANIMATIONDRIVER").isEmpty();
#endif

#ifdef CUSTOMCONTEXT_ATLASTEXTURE
    m_atlasTexture = qgetenv("CUSTOMCONTEXT_NO_ATLASTEXTURE").isEmpty();
#endif

#ifdef CUSTOMCONTEXT_DITHER
    m_dither= qgetenv("CUSTOMCONTEXT_NO_DITHER").isEmpty();
    m_ditherProgram = 0;
#endif

#ifdef CUSTOMCONTEXT_THREADUPLOADTEXTURE
    m_threadUploadTexture = qgetenv("CUSTOMCONTEXT_NO_THREADUPLOADTEXTURE").isEmpty();
    connect(this, SIGNAL(invalidated()), &m_threadUploadManager, SLOT(invalidated()), Qt::DirectConnection);
#endif



#ifdef CUSTOMCONTEXT_DEBUG
    qDebug("CustomContext created:");
    qDebug(" - multisampling: %s, samples=%d", m_useMultisampling ? "enabled" : "disabled", m_sampleCount);

#ifdef CUSTOMCONTEXT_OVERLAPRENDERER
    qDebug(" - overlaprenderer: %s", m_overlapRenderer ? "enabled" : "disabled");
#endif

#ifdef CUSTOMCONTEXT_ANIMATIONDRIVER
    qDebug(" - custom animation driver: %s", m_animationDriver ? "enabled" : "disabled");
#endif

#ifdef CUSTOMCONTEXT_ATLASTEXTURE
    qDebug(" - atlas textures: %s", m_atlasTexture ? "enabled" : "disabled" );
#endif

#ifdef CUSTOMCONTEXT_DITHER
    qDebug(" - ordered 2x2 dither: %s", m_dither ? "enabled" : "disabled");
#endif

#ifdef CUSTOMCONTEXT_THREADUPLOADTEXTURE
    qDebug(" - threaded texture upload: %s", m_threadUploadTexture ? "enabled" : "disabled");
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

#ifdef CUSTOMCONTEXT_DEBUG
    qDebug("CustomContext: initialized..");
    qDebug(" - OpenGL extensions: %s", glGetString(GL_EXTENSIONS));
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
    if (m_useMultisampling)
        format.setSamples(m_sampleCount);

    qDebug() << "returning surface format...";

    return format;
}



QSGTexture *Context::createTexture(const QImage &image) const
{
#ifdef CUSTOMCONTEXT_ATLASTEXTURE
    if (m_atlasTexture)
        return const_cast<Context *>(this)->m_atlasManager.create(image);
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
   return QSGContext::createAnimationDriver(parent);
}



QQuickTextureFactory *Context::createTextureFactory(const QImage &image)
{
    Q_UNUSED(image);

#ifdef CUSTOMCONTEXT_THREADUPLOADTEXTURE
    if (m_threadUploadTexture)
        return m_threadUploadManager.create(image);
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
