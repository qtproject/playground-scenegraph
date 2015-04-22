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

#ifndef QSGSIMPLERENDERER_H
#define QSGSIMPLERENDERER_H

#include <private/qdatabuffer_p.h>

#include <private/qsgrenderer_p.h>
#include <private/qsgcontext_p.h>

#include <qbitarray.h>

#include "qsgbasicshadermanager_p.h"
#include "qsgbasicclipmanager_p.h"

namespace QSGSimpleRenderer
{

struct Element {
    QSGBasicGeometryNode *node;
    qint32 vboOffset;
    qint32 iboOffset;
};

class Renderer : public QSGRenderer, public QOpenGLFunctions
{
public:
    Renderer(QSGRenderContext *context);
    ~Renderer();

    virtual void render();
    virtual void nodeChanged(QSGNode *node, QSGNode::DirtyState state) Q_DECL_OVERRIDE;

    void buildRenderList(QSGNode *node, QSGClipNode *clip);
    void updateMatrices(QSGNode *node, QSGTransformNode *xnode);
    void updateOpacities(QSGNode *node, qreal opacity);
    void renderElements();
    void renderElement(int elementIndex);

    QDataBuffer<Element> m_renderList;
    QBitArray m_opaqueElements;
    QDataBuffer<char> m_vboData;
    QDataBuffer<char> m_iboData;

    QSet<QSGNode *> m_dirtyTransformNodes;
    QSet<QSGNode *> m_dirtyOpacityNodes;

    QSGBasicShaderManager *m_shaderManager;
    QSGBasicClipManager *m_clipManager;

    GLuint m_vboId;
    GLuint m_iboId;
    int m_vboSize;
    int m_iboSize;

    bool m_rebuild;
    bool m_useDepthBuffer;
    bool m_dirtyOpaqueElements;
};

}

#endif // QSGSIMPLERENDERER_H
