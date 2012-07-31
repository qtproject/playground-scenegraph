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
#include "animationdriver.h"
#include "overlaprenderer.h"

#include "QtCore/qabstractanimation.h"
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qmath.h>

#ifndef DESKTOP_BUILD
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

#include <QtGui>

namespace CustomContext
{

void DitherProgram::prepare()
{
    if (prepared)
        return;
    prepared = true;

    program.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                    "attribute highp vec2 pos;\n"
                                    "attribute highp vec2 texCoord;\n"
                                    "varying highp vec2 tex;\n"
                                    "void main() {\n"
                                    "    gl_Position = vec4(pos.x, pos.y, 0, 1);\n"
                                    "    tex = texCoord;\n"
                                    "}");
    program.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                    "uniform lowp sampler2D texture;"
                                    "varying highp vec2 tex;\n"
                                    "void main() {\n"
                                    "    gl_FragColor = texture2D(texture, tex);\n"
                                    "}\n");

    program.bindAttributeLocation("pos", 0);
    program.bindAttributeLocation("texCoord", 1);

    program.link();
    program.setUniformValue("texture", 0);

    glGenTextures(1, &id_texture);
    glBindTexture(GL_TEXTURE_2D, id_texture);

    int data[4] = {
        0x00000000,
        0x02020202,
        0x03030303,
        0x01010101
    };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void DitherProgram::draw(int width, int height)
{
    program.bind();
    program.enableAttributeArray(0);
    program.enableAttributeArray(1);

    float posData[] = { -1, 1, 1, 1, -1, -1, 1, -1 };
    program.setAttributeArray(0, GL_FLOAT, posData, 2);

    float w = width/2;
    float h = height/2;
    float texData[] = { 0, 0, w, 0, 0, h, w, h };
    program.setAttributeArray(1, GL_FLOAT, texData, 2);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, id_texture);

    glBlendFunc(GL_ONE, GL_ONE);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    program.disableAttributeArray(0);
    program.disableAttributeArray(1);
    program.release();
}



Context::Context(QObject *parent)
    : QSGContext(parent)
    , dither(0)
    , samples(0)
    , useCustomRenderer(false)
    , useCustomAnimationDriver(false)
    , useMultisampling(false)
{
    useMultisampling = qgetenv("QML_NO_MULTISAMPLING").isEmpty();
    if (useMultisampling) {
        samples = 4;
        QByteArray overrideSamples = qgetenv("QML_MULTISAMPLE_COUNT");
        bool ok;
        int override = overrideSamples.toInt(&ok);
        if (ok)
            samples = override;
    }

    useCustomRenderer = qgetenv("QML_DEFAULT_RENDERER").isEmpty();
    useCustomAnimationDriver = qgetenv("QML_DEFAULT_ANIMATION_DRIVER").isEmpty();
    useDefaultTextures = !qgetenv("QML_DEFAULT_TEXTURES").isEmpty();

    useDithering = !qgetenv("QML_FULLSCREEN_DITHER").isEmpty();

    connect(this, SIGNAL(invalidated()), this, SLOT(handleInvalidated()));

#ifdef CUSTOMCONTEXT_DEBUG
    qDebug("CustomContext created:");
    qDebug(" - multisampling: %s, samples=%d", useMultisampling ? "yes" : "no", samples);
    qDebug(" - renderer: %s", useCustomRenderer ? "overlap" : "default");
    qDebug(" - animation driver: %s", useCustomAnimationDriver ? "custom" : "default");
    qDebug(" - textures: %s", useDefaultTextures ? "default" : "atlas" );
    qDebug(" - dithering: %s", useDithering ? "yes" : "no");
#endif

}



void Context::initialize(QOpenGLContext *context)
{

    dither = new DitherProgram;

#if defined(EGL_WL_request_client_buffer_format) && !defined(DESKTOP_BUILD)
    EGLDisplay display = eglGetCurrentDisplay();
    const char *extensions = eglQueryString(display, EGL_EXTENSIONS);
    if (strstr(extensions, "EGL_WL_request_client_buffer_format") != 0) {
        eglRegisterBufferFormatCallbackWL(display, bufferFormatChanged, this);
    }
#endif

#ifdef CUSTOMCONTEXT_DEBUG
    qDebug("CustomContext: initialized..");
    qDebug(" - OpenGL extensions: %s", glGetString(GL_EXTENSIONS));
#if defined(EGL_WL_request_client_buffer_format) && !defined(DESKTOP_BUILD)
    qDebug(" - EGL extensions: %s\n", extensions);
#endif
#endif

    QSGContext::initialize(context);
}

void Context::invalidate()
{
    QSGContext::invalidate();

    delete dither;
    dither = 0;

    m_atlas_manager.invalidate();
}

void Context::setDitherEnabled(bool enabled)
{
    useDithering = enabled;
}


QSGTexture *Context::createTexture(const QImage &image) const
{
    if (useDefaultTextures)
        return QSGContext::createTexture(image);
    else
        return const_cast<Context *>(this)->m_atlas_manager.create(image);
}

QSurfaceFormat Context::defaultSurfaceFormat() const
{
    QSurfaceFormat format;
    format.setStencilBufferSize(8);
    if (useMultisampling)
        format.setSamples(samples);
    return format;
}

QSGRenderer *Context::createRenderer()
{
    if (useCustomRenderer)
        return new OverlapRenderer::Renderer(this);
    return QSGContext::createRenderer();
}


QAnimationDriver *Context::createAnimationDriver(QObject *parent)
{
    if (useCustomAnimationDriver)
        return new AnimationDriver();
    else
        return QSGContext::createAnimationDriver(parent);
}


QQuickTextureFactory *Context::createTextureFactory(const QImage &image)
{
    return 0;
}


void Context::renderNextFrame(QSGRenderer *renderer, GLuint fbo)
{
    QSGContext::renderNextFrame(renderer, fbo);

    if (useDithering) {
        QSurface *s = glContext()->surface();
        QSize size = static_cast<QWindow *>(s)->size();
        dither->prepare();
        dither->draw(size.width(), size.height());
    }
}

void Context::handleInvalidated()
{
    m_atlas_manager.invalidate();
}

}; // namespace
