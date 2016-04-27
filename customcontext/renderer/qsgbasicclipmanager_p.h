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

#ifndef QSGBASICCLIPMANAGER_P_H
#define QSGBASICCLIPMANAGER_P_H

#include <private/qsgcontext_p.h>
#include <private/qsgshadersourcebuilder_p.h>

#include <QOpenGLShaderProgram>

class QSGBasicClipManager : public QObject
{
    Q_OBJECT
public:
    enum ClipTypeBit
    {
        NoClip = 0x00,
        ScissorClip = 0x01,
        StencilClip = 0x02
    };
    Q_DECLARE_FLAGS(ClipType, ClipTypeBit)

    QSGBasicClipManager(QSGRenderContext *context)
        : QObject(context)
        , m_program(0)
        , m_currentStencilValue(0)
        , m_clipMatrixId(0)
        , m_clipType(NoClip)
    {
        connect(context, SIGNAL(invalidated()), this, SLOT(invalidate()));
    }

    static QSGBasicClipManager *from(QSGRenderContext *context)
    {
        const QString name = QStringLiteral("qsg_basic_clipmanager");
        QSGBasicClipManager *cm = context->findChild<QSGBasicClipManager*>(name);
        if (!cm) {
            cm = new QSGBasicClipManager(context);
            cm->setObjectName(name);
        }
        return cm;
    }

    void setDeviceRect(const QRect &deviceRect) { m_deviceRect = deviceRect; }
    void setProjectionMatrix(const QMatrix4x4 &matrix) { m_projectionMatrix = matrix; }

    template<typename ClipRenderer, typename ShaderStateTracker>
    void activate(const QSGClipNode *clip, ClipRenderer *clipRenderer, ShaderStateTracker *tracker, QOpenGLFunctions *gl);

    void reset(QOpenGLFunctions *gl) {
        if (m_clipType & StencilClip)
            gl->glDisable(GL_STENCIL_TEST);
        if (m_clipType & ScissorClip)
            gl->glDisable(GL_SCISSOR_TEST);
        m_clipType = NoClip;
        m_currentClip = 0;
    }

    ClipType clipType() const { return m_clipType; }
    const QSGClipNode *currentClip() const { return m_currentClip; }

public Q_SLOTS:
    void invalidate() {
        delete m_program;
        m_program = 0;
    }

private:
    QOpenGLShaderProgram *m_program;
    QRect m_currentScissorRect;
    int m_currentStencilValue;
    int m_clipMatrixId;
    ClipType m_clipType;
    const QSGClipNode *m_currentClip;

    QRect m_deviceRect;
    QMatrix4x4 m_projectionMatrix;
};

template <typename ClipRenderer, typename ShaderStateTracker>
void QSGBasicClipManager::activate(const QSGClipNode *clip,
                                   ClipRenderer *clipRenderer,
                                   ShaderStateTracker *tracker,
                                   QOpenGLFunctions *gl)
{
    if (m_currentClip == clip)
        return;

    m_clipType = NoClip;
    m_currentClip = clip;
    if (!clip) {
        gl->glDisable(GL_STENCIL_TEST);
        gl->glDisable(GL_SCISSOR_TEST);
        return;
    }

    gl->glDisable(GL_SCISSOR_TEST);

    m_currentStencilValue = 0;
    m_currentScissorRect = QRect();

    while (clip) {
        QMatrix4x4 m = m_projectionMatrix;
        if (clip->matrix())
            m *= *clip->matrix();

        bool isRectangleWithNoPerspective = clip->isRectangular()
                && qFuzzyIsNull(m(3, 0)) && qFuzzyIsNull(m(3, 1));
        bool noRotate = qFuzzyIsNull(m(0, 1)) && qFuzzyIsNull(m(1, 0));
        bool isRotate90 = qFuzzyIsNull(m(0, 0)) && qFuzzyIsNull(m(1, 1));

        if (isRectangleWithNoPerspective && (noRotate || isRotate90)) {
            QRectF bbox = clip->clipRect();
            qreal invW = 1 / m(3, 3);
            qreal fx1, fy1, fx2, fy2;
            if (noRotate) {
                fx1 = (bbox.left() * m(0, 0) + m(0, 3)) * invW;
                fy1 = (bbox.bottom() * m(1, 1) + m(1, 3)) * invW;
                fx2 = (bbox.right() * m(0, 0) + m(0, 3)) * invW;
                fy2 = (bbox.top() * m(1, 1) + m(1, 3)) * invW;
            } else {
                Q_ASSERT(isRotate90);
                fx1 = (bbox.bottom() * m(0, 1) + m(0, 3)) * invW;
                fy1 = (bbox.left() * m(1, 0) + m(1, 3)) * invW;
                fx2 = (bbox.top() * m(0, 1) + m(0, 3)) * invW;
                fy2 = (bbox.right() * m(1, 0) + m(1, 3)) * invW;
            }

            if (fx1 > fx2)
                qSwap(fx1, fx2);
            if (fy1 > fy2)
                qSwap(fy1, fy2);

            GLint ix1 = qRound((fx1 + 1) * m_deviceRect.width() * qreal(0.5));
            GLint iy1 = qRound((fy1 + 1) * m_deviceRect.height() * qreal(0.5));
            GLint ix2 = qRound((fx2 + 1) * m_deviceRect.width() * qreal(0.5));
            GLint iy2 = qRound((fy2 + 1) * m_deviceRect.height() * qreal(0.5));

            if (!(m_clipType & ScissorClip)) {
                m_currentScissorRect = QRect(ix1, iy1, ix2 - ix1, iy2 - iy1);
                gl->glEnable(GL_SCISSOR_TEST);
                m_clipType |= ScissorClip;
            } else {
                m_currentScissorRect &= QRect(ix1, iy1, ix2 - ix1, iy2 - iy1);
            }
            gl->glScissor(m_currentScissorRect.x(), m_currentScissorRect.y(),
                          m_currentScissorRect.width(), m_currentScissorRect.height());
        } else {

            if (!(m_clipType & StencilClip)) {
                tracker->deactivate();
                if (!m_program) {
                    m_program = new QOpenGLShaderProgram();
                    QSGShaderSourceBuilder::initializeProgramFromFiles(
                        m_program,
                        QStringLiteral(":/scenegraph/shaders/stencilclip.vert"),
                        QStringLiteral(":/scenegraph/shaders/stencilclip.frag"));
                    m_program->bindAttributeLocation("vCoord", 0);
                    m_program->link();
                    m_clipMatrixId = m_program->uniformLocation("matrix");
                }

                gl->glClearStencil(0);
                gl->glClear(GL_STENCIL_BUFFER_BIT);
                gl->glEnable(GL_STENCIL_TEST);
                gl->glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                gl->glDepthMask(GL_FALSE);

                m_program->bind();
                tracker->setEnabledVertexAttribArrays(1, gl);

                m_clipType |= StencilClip;
            }

            gl->glStencilFunc(GL_EQUAL, m_currentStencilValue, 0xff); // stencil test, ref, test mask
            gl->glStencilOp(GL_KEEP, GL_KEEP, GL_INCR); // stencil fail, z fail, z pass
            m_program->setUniformValue(m_clipMatrixId, m);

            clipRenderer->renderClip(clip);

            ++m_currentStencilValue;
        }

        clip = clip->clipList();
    }

    if (m_clipType & StencilClip) {
        gl->glStencilFunc(GL_EQUAL, m_currentStencilValue, 0xff); // stencil test, ref, test mask
        gl->glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); // stencil fail, z fail, z pass
        gl->glColorMask(true, true, true, true);
    } else {
        gl->glDisable(GL_STENCIL_TEST);
    }
}

#endif
