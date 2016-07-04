/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Sceneground Playground.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSGBASICSHADERMANAGER_P_H
#define QSGBASICSHADERMANAGER_P_H

#include <qsgmaterial.h>
#include <QtGui/qopenglfunctions.h>

#if QT_VERSION >= 0x050800
#include <private/qsgdefaultcontext_p.h>
#include <private/qsgdefaultrendercontext_p.h>
#else
#include <private/qsgcontext_p.h>
#endif
#include <private/qsgrenderer_p.h>

class QSGDefaultRenderContext;

class QSGBasicShaderManager : public QObject
{
    Q_OBJECT
public:
    QSGBasicShaderManager(QSGRenderContext *context)
        : QObject(context)
#if QT_VERSION >= 0x050800
        , m_context(static_cast<QSGDefaultRenderContext *>(context))
#else
        , m_context(context)
#endif
        , m_currentShader(0)
        , m_currentMaterial(0)
        , m_previousMaterial(0)
        , m_currentAttributeCount(0)
    {
        connect(context, SIGNAL(invalidated()), this, SLOT(invalidate()));
    }

    static QSGBasicShaderManager *from(QSGRenderContext *context)
    {
        const QString name = QStringLiteral("qsg_basic_shadermanager");
        QSGBasicShaderManager *sm = context->findChild<QSGBasicShaderManager *>(name);
        if (!sm) {
            sm = new QSGBasicShaderManager(context);
            sm->setObjectName(name);
        }
        return sm;
    }

    QSGMaterialShader *prepare(const QSGMaterial *material)
    {
        Q_ASSERT(material);

        QSGMaterialType *type = material->type();
        QSGMaterialShader *shader = m_shaders.value(type);
        if (shader)
            return shader;

        shader = static_cast<QSGMaterialShader *>(material->createShader());
#if QT_VERSION >= 0x050800
        m_context->compileShader(shader, const_cast<QSGMaterial *>(material));
        m_context->initializeShader(shader);
#else
        m_context->compile(shader, const_cast<QSGMaterial *>(material));
        m_context->initialize(shader);
#endif

        m_shaders[type] = shader;

        return shader;
    }

    static int countAttributes(QSGMaterialShader *shader)
    {
        const char * const *names = shader->attributeNames();
        Q_ASSERT(names);
        int count = 0;
        while (names[count])
            ++count;
        return count;
    }

    void activate(const QSGMaterial *m, QSGMaterialShader::RenderState::DirtyStates states, QSGRenderer *renderer, QOpenGLFunctions *gl)
    {
        Q_ASSERT(m);
        QSGMaterialShader *shader = prepare(m);
        Q_ASSERT(shader);

        m_previousMaterial = m_currentMaterial;
        m_currentMaterial = m;

        if (shader != m_currentShader) {
            if (m_currentShader)
                m_currentShader->deactivate();
            m_currentShader = shader;
            m_currentShader->program()->bind();
            m_currentShader->activate();
            m_previousMaterial = 0;
            states |=(QSGMaterialShader::RenderState::DirtyOpacity | QSGMaterialShader::RenderState::DirtyMatrix);
            setEnabledVertexAttribArrays(countAttributes(m_currentShader), gl);
        }

        m_currentShader->updateState(renderer->state(states),
                                     const_cast<QSGMaterial *>(m_currentMaterial),
                                     const_cast<QSGMaterial *>(m_previousMaterial));
    }

    void setEnabledVertexAttribArrays(int howMany, QOpenGLFunctions *gl) {
        for (int i=m_currentAttributeCount; i<howMany; ++i)
            gl->glEnableVertexAttribArray(i);
        for (int i=m_currentAttributeCount-1; i>=howMany; --i)
            gl->glDisableVertexAttribArray(i);
        m_currentAttributeCount = howMany;
    }

    void deactivate() {
        if (m_currentShader) {
            m_currentShader->deactivate();
            m_currentShader = 0;
        }
        m_previousMaterial = 0;
        m_currentMaterial = 0;
    }

    QSGMaterialShader *currentShader() const { return m_currentShader; }
    int currentAttributeCount() const { return m_currentAttributeCount; }

    void endFrame(QOpenGLFunctions *gl) {
        // Reset shader and potential shader state
        if (m_currentShader) {
            m_currentShader->deactivate();
            m_currentShader = 0;
        }
        m_currentMaterial = 0;
        m_previousMaterial = 0;
        // Disable all attribute arrays.
        while (m_currentAttributeCount > 0)
            gl->glDisableVertexAttribArray(--m_currentAttributeCount);
    }

public Q_SLOTS:
    void invalidate() {
        qDeleteAll(m_shaders.values());
        m_shaders.clear();
    }

private:
    QHash<QSGMaterialType *, QSGMaterialShader *> m_shaders;

#if QT_VERSION >= 0x050800
    QSGDefaultRenderContext *m_context;
#else
    QSGRenderContext *m_context;
#endif

    QSGMaterialShader *m_currentShader;
    const QSGMaterial *m_currentMaterial;
    const QSGMaterial *m_previousMaterial;
    int m_currentAttributeCount;
};

#endif
