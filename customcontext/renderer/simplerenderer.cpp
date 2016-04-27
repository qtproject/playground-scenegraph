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

#include "simplerenderer.h"

#include <private/qsgnodeupdater_p.h>

#include <QtGui/QOpenGLFunctions_1_0>
#include <QtGui/QOpenGLFunctions_3_2_Core>

namespace QSGSimpleRenderer {

#define QSGNODE_TRAVERSE(NODE) for (QSGNode *child = NODE->firstChild(); child; child = child->nextSibling())

#define DECLARE_DEBUG_VAR(variable) \
    static bool debug_ ## variable() \
    { static bool value = qgetenv("QSG_RENDERER_DEBUG").contains(QT_STRINGIFY(variable)); return value; }
DECLARE_DEBUG_VAR(build)
DECLARE_DEBUG_VAR(change)
DECLARE_DEBUG_VAR(render)
DECLARE_DEBUG_VAR(nodepthbuffer)

static inline int size_of_type(GLenum type)
{
    static int sizes[] = {
        sizeof(char),
        sizeof(unsigned char),
        sizeof(short),
        sizeof(unsigned short),
        sizeof(int),
        sizeof(unsigned int),
        sizeof(float),
        2,
        3,
        4,
        sizeof(double)
    };
    Q_ASSERT(type >= GL_BYTE && type <= 0x140A); // the value of GL_DOUBLE
    return sizes[type - GL_BYTE];
}


class DummyUpdater : public QSGNodeUpdater
{
public:
    void updateState(QSGNode *) { };
};

Renderer::Renderer(QSGRenderContext *ctx)
    : QSGRenderer(ctx)
    , m_renderList(16)
    , m_vboData(1024)
    , m_iboData(256)
    , m_vboId(0)
    , m_iboId(0)
    , m_vboSize(0)
    , m_iboSize(0)
    , m_rebuild(true)
{
    m_shaderManager = QSGBasicShaderManager::from(ctx);
    m_clipManager = QSGBasicClipManager::from(ctx);
    m_useDepthBuffer = QOpenGLContext::currentContext()->format().depthBufferSize() > 0
                       && !debug_nodepthbuffer();

    setNodeUpdater(new DummyUpdater());

    initializeOpenGLFunctions();
    glGenBuffers(1, &m_vboId);
    glGenBuffers(1, &m_iboId);
}

Renderer::~Renderer()
{
    glDeleteBuffers(1, &m_vboId);
    glDeleteBuffers(1, &m_iboId);
}

void Renderer::nodeChanged(QSGNode *node, QSGNode::DirtyState state)
{
    if (Q_UNLIKELY(debug_change())) {
        QDebug debug = qDebug();
        debug << "dirty:";
        if (state & QSGNode::DirtyGeometry)
            debug << "Geometry";
        if (state & QSGNode::DirtyMaterial)
            debug << "Material";
        if (state & QSGNode::DirtyMatrix)
            debug << "Matrix";
        if (state & QSGNode::DirtyNodeAdded)
            debug << "Added";
        if (state & QSGNode::DirtyNodeRemoved)
            debug << "Removed";
        if (state & QSGNode::DirtyOpacity)
            debug << "Opacity";
        if (state & QSGNode::DirtySubtreeBlocked)
            debug << "SubtreeBlocked";
        if (state & QSGNode::DirtyForceUpdate)
            debug << "ForceUpdate";

        // when removed, some parts of the node could already have been destroyed
        // so don't debug it out.
        if (state & QSGNode::DirtyNodeRemoved)
            debug << (void *) node << node->type();
        else
            debug << node;
    }

    if (state & (QSGNode::DirtyNodeAdded
                 | QSGNode::DirtyNodeRemoved
                 | QSGNode::DirtySubtreeBlocked
                 | QSGNode::DirtyGeometry
                 | QSGNode::DirtyForceUpdate)) {
        m_rebuild = true;
    }
    if (state & QSGNode::DirtyMatrix)
        m_dirtyTransformNodes << node;
    if (state & QSGNode::DirtyOpacity)
        m_dirtyOpacityNodes << node;
    if (state & QSGNode::DirtyMaterial)
        m_dirtyOpaqueElements = true;

    QSGRenderer::nodeChanged(node, state);
}

class BasicGeometryNode_Accessor : public QSGNode
{
public:
    QSGGeometry *m_geometry;

    int m_reserved_start_index;
    int m_reserved_end_index;

    const QMatrix4x4 *m_matrix;
    const QSGClipNode *m_clip_list;
};


void Renderer::buildRenderList(QSGNode *node, QSGClipNode *clip)
{
    if (node->isSubtreeBlocked())
        return;

    if (node->type() == QSGNode::GeometryNodeType || node->type() == QSGNode::ClipNodeType) {
        QSGBasicGeometryNode *gn = static_cast<QSGBasicGeometryNode *>(node);
        QSGGeometry *g = gn->geometry();

        Element e;
        e.node = gn;

        if (g->vertexCount() > 0) {
            e.vboOffset = m_vboSize;
            int vertexSize = g->sizeOfVertex() * g->vertexCount();
            m_vboSize += vertexSize;
            m_vboData.resize(m_vboSize);
            memcpy(m_vboData.data() + e.vboOffset, g->vertexData(), vertexSize);
        } else {
            e.vboOffset = -1;
        }

        if (g->indexCount() > 0) {
            e.iboOffset = m_iboSize;
            int indexSize = g->sizeOfIndex() * g->indexCount();
            m_iboSize += indexSize;
            m_iboData.resize(m_iboSize);
            memcpy(m_iboData.data() + e.iboOffset, g->indexData(), indexSize);
        } else {
            e.iboOffset = -1;
        }

        m_renderList.add(e);

        static_cast<BasicGeometryNode_Accessor *>(node)->m_clip_list = clip;
        if (node->type() == QSGNode::ClipNodeType)
            clip = static_cast<QSGClipNode *>(node);
    }

    QSGNODE_TRAVERSE(node)
        buildRenderList(child, clip);
}

void Renderer::updateMatrices(QSGNode *node, QSGTransformNode *xform)
{
    if (node->isSubtreeBlocked())
        return;

    if (node->type() == QSGNode::TransformNodeType) {
        QSGTransformNode *tn = static_cast<QSGTransformNode *>(node);
        if (xform)
            tn->setCombinedMatrix(xform->combinedMatrix() * tn->matrix());
        else
            tn->setCombinedMatrix(tn->matrix());
        QSGNODE_TRAVERSE(node)
            updateMatrices(child, tn);

    } else {
        if (node->type() == QSGNode::GeometryNodeType || node->type() == QSGNode::ClipNodeType) {
           static_cast<BasicGeometryNode_Accessor *>(node)->m_matrix = xform ? &xform->combinedMatrix() : 0;
        }
        QSGNODE_TRAVERSE(node)
            updateMatrices(child, xform);
    }
}

void Renderer::updateOpacities(QSGNode *node, qreal inheritedOpacity)
{
    if (node->isSubtreeBlocked())
        return;

    if (node->type() == QSGNode::OpacityNodeType) {
        QSGOpacityNode *on = static_cast<QSGOpacityNode *>(node);
        qreal combined = inheritedOpacity * on->opacity();
        on->setCombinedOpacity(combined);
        QSGNODE_TRAVERSE(node)
            updateOpacities(child, combined);
    } else {
        if (node->type() == QSGNode::GeometryNodeType) {
            QSGGeometryNode *gn = static_cast<QSGGeometryNode *>(node);
            gn->setInheritedOpacity(inheritedOpacity);
        }
        QSGNODE_TRAVERSE(node)
            updateOpacities(child, inheritedOpacity);
    }
}

/* Search through the node set and remove nodes that are leaves of other
   nodes in the same set.
 */
QSet<QSGNode *> qsg_simplerenderer_updateRoots(const QSet<QSGNode *> &nodes, QSGRootNode *root)
{
    QSet<QSGNode *> result = nodes;
    foreach (QSGNode *node, nodes) {
        QSGNode *n = node;
        while (n != root) {
            if (result.contains(n)) {
                result.remove(node);
                break;
            }
            n = n->parent();
        }
    }
    return result;
}


void Renderer::render()
{
    if (Q_UNLIKELY(debug_render()))
        qDebug("render: %s", m_rebuild ? "rebuild" : "retained");

    glBindBuffer(GL_ARRAY_BUFFER, m_vboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iboId);

    if (m_rebuild) {
        // This changes everything, so discard all cached states
        m_rebuild = false;
        m_dirtyOpacityNodes.clear();
        m_dirtyOpacityNodes.insert(rootNode());
        m_dirtyTransformNodes.clear();
        m_dirtyTransformNodes.insert(rootNode());
        m_renderList.reset();
        m_vboData.reset();
        m_iboData.reset();
        m_vboSize = 0;
        m_iboSize = 0;
        buildRenderList(rootNode(), 0);

        glBufferData(GL_ARRAY_BUFFER, m_vboSize, m_vboData.data(), GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_iboSize, m_iboData.data(), GL_STATIC_DRAW);

        if (Q_UNLIKELY(debug_build())) {
            qDebug("RenderList: %d elements in total", m_renderList.size());
            for (int i=0; i<m_renderList.size(); ++i) {
                const Element &e = m_renderList.at(i);
                qDebug() << " - " << e.vboOffset << e.iboOffset << e.node;
            }
        }
    }

    if (m_dirtyTransformNodes.size()) {
        foreach (QSGNode *node, qsg_simplerenderer_updateRoots(m_dirtyTransformNodes, rootNode())) {

            // First find the parent transform so we have the accumulated
            // matrix up until this point.
            QSGTransformNode *xform = 0;
            QSGNode *n = node;
            if (n->type() == QSGNode::TransformNodeType)
                n = node->parent();
            while (n != rootNode() && n->type() != QSGNode::TransformNodeType)
                n = n->parent();
            if (n != rootNode())
                xform = static_cast<QSGTransformNode *>(n);

            // Then update in the subtree
            updateMatrices(node, xform);
        }
    }

    if (m_dirtyOpacityNodes.size()) {
        foreach (QSGNode *node, qsg_simplerenderer_updateRoots(m_dirtyOpacityNodes, rootNode())) {

            // First find the parent opacity node so we have the accumulated
            // matrix up until this point.
            qreal opacity = 1;
            QSGNode *n = node;
            if (n->type() == QSGNode::OpacityNodeType)
                n = node->parent();
            while (n != rootNode() && n->type() != QSGNode::OpacityNodeType)
                n = n->parent();
            if (n != rootNode())
                opacity = static_cast<QSGOpacityNode *>(n)->combinedOpacity();

            // Then update in the subtree
            updateOpacities(node, opacity);
        }
        m_dirtyOpaqueElements = true;
    }

    if (m_dirtyOpaqueElements && m_useDepthBuffer) {
        m_dirtyOpaqueElements = false;
        m_opaqueElements.clear();
        m_opaqueElements.resize(m_renderList.size());
        for (int i=0; i<m_renderList.size(); ++i) {
            const Element &e = m_renderList.at(i);
            if (e.node->type() == QSGNode::GeometryNodeType) {
                const QSGGeometryNode *gn = static_cast<QSGGeometryNode *>(e.node);
                if (gn->inheritedOpacity() > 0.999 && ((gn->activeMaterial()->flags() & QSGMaterial::Blending) == 0))
                    m_opaqueElements.setBit(i);
            }
        }
    }

    m_clipManager->setDeviceRect(deviceRect());
    m_clipManager->setProjectionMatrix(projectionMatrix());
    renderElements();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Renderer::renderElements()
{
    QRect r = viewportRect();
    glViewport(r.x(), deviceRect().bottom() - r.bottom(), r.width(), r.height());
    glClearColor(clearColor().redF(), clearColor().greenF(), clearColor().blueF(), clearColor().alphaF());
    glDisable(GL_CULL_FACE);
    glColorMask(true, true, true, true);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    if (m_useDepthBuffer) {
        // First do the opaque ones..
        glDisable(GL_BLEND);
        glClearDepthf(1);
        glDepthMask(true);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
    }

    bindable()->clear(clearMode());
    m_current_projection_matrix = projectionMatrix();

    if (m_useDepthBuffer) {
        // First do opaque...
        // The algorithm is quite simple. We traverse the list back-to-from
        // and for every item, we start a second traversal from this point
        // and draw all elements which have identical material. Then we clear
        // the bit for this in the rendered list so we don't draw it again
        // when we come to that index.
        QBitArray rendered = m_opaqueElements;
        for (int i=m_renderList.size()-1; i>=0; --i) {
            if (rendered.testBit(i)) {
                renderElement(i);
                for (int j=i-1; j>=0; --j) {
                    if (rendered.testBit(j)) {
                        const QSGGeometryNode *gni = static_cast<QSGGeometryNode *>(m_renderList.at(i).node);
                        const QSGGeometryNode *gnj = static_cast<QSGGeometryNode *>(m_renderList.at(j).node);
                        if (gni->clipList() == gnj->clipList()
                            && gni->inheritedOpacity() == gnj->inheritedOpacity()
                            && gni->geometry()->drawingMode() == gnj->geometry()->drawingMode()
                            && gni->geometry()->attributes() == gnj->geometry()->attributes()) {
                            const QSGMaterial *ami = gni->activeMaterial();
                            const QSGMaterial *amj = gnj->activeMaterial();
                            if (ami->type() == amj->type()
                                && ami->flags() == amj->flags()
                                && ami->compare(amj) == 0) {
                                renderElement(j);
                                rendered.clearBit(j);
                            }
                        }
                    }
                }
            }
        }
        glDepthMask(false);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // Then the alpha ones..
    for (int i=0; i<m_renderList.size(); ++i) {
        if (m_renderList.at(i).node->type() == QSGNode::GeometryNodeType
            && (!m_useDepthBuffer || (!m_opaqueElements.testBit(i)))) {
            renderElement(i);
        }
    }

    m_clipManager->reset(this);
    m_shaderManager->endFrame(this);
}

struct ClipRenderer
{
    ClipRenderer(Renderer *r, int geoPos)
        : renderer(r)
        , geometryPos(geoPos)
        , lastClipPos(geoPos)
    {
    }

    void renderClip(const QSGClipNode *clip)
    {
        while (renderer->m_renderList.at(--lastClipPos).node != clip) {
            Q_ASSERT(lastClipPos >= 0);
        }
        const Element &ce = renderer->m_renderList.at(lastClipPos);
        Q_ASSERT(ce.node == clip);

        const QSGGeometry *g = clip->geometry();
        Q_ASSERT(g->attributeCount() == 1);
        Q_ASSERT(g->attributes()[0].tupleSize == 2);
        Q_ASSERT(g->attributes()[0].type == GL_FLOAT);

        renderer->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, g->sizeOfVertex(), (void *) (qintptr) ce.vboOffset);
        if (clip->geometry()->indexCount() > 0) {
            Q_ASSERT(ce.iboOffset >= 0);
            renderer->glDrawElements(g->drawingMode(), g->indexCount(), g->indexType(), (void *) (qintptr) ce.iboOffset);
        } else {
            renderer->glDrawArrays(g->drawingMode(), 0, g->vertexCount());
        }
    }

    Renderer *renderer;
    int geometryPos;
    int lastClipPos;
};

void Renderer::renderElement(int elementIndex)
{
    const Element &e = m_renderList.at(elementIndex);
    Q_ASSERT(e.node->type() == QSGNode::GeometryNodeType);

    if (e.vboOffset < 0)
        return;

    const QSGGeometryNode *gn = static_cast<QSGGeometryNode *>(e.node);
    if (Q_UNLIKELY(debug_render()))
        qDebug() << " - element:" << elementIndex << gn << e.vboOffset << e.iboOffset << gn->inheritedOpacity() << gn->clipList();

    if (gn->inheritedOpacity() < 0.001) // pretty much invisible, don't draw it
        return;

    const QSGGeometry *g = gn->geometry();
    QSGMaterial *m = gn->activeMaterial();

    QSGMaterialShader::RenderState::DirtyStates dirtyState = QSGMaterialShader::RenderState::DirtyMatrix;
    if (m_useDepthBuffer) {
        m_current_projection_matrix = projectionMatrix();
        qreal scale = 1.0 / m_renderList.size();
        m_current_projection_matrix(2, 2) = scale;
        m_current_projection_matrix(2, 3) = 1.0f - (elementIndex + 1) * scale;
    }
    m_current_model_view_matrix = gn->matrix() ? *gn->matrix() : QMatrix4x4();
    m_current_determinant = m_current_model_view_matrix.determinant();
    if (gn->inheritedOpacity() != m_current_opacity) {
        m_current_opacity = gn->inheritedOpacity();
        dirtyState |= QSGMaterialShader::RenderState::DirtyOpacity;
    }
    if (m_clipManager->currentClip() != gn->clipList()) {
        ClipRenderer renderer(this, elementIndex);
        m_clipManager->activate(gn->clipList(), &renderer, m_shaderManager, this);
    }
    m_shaderManager->activate(m, dirtyState, this, this);

    const int currentAttributeCount = m_shaderManager->currentAttributeCount();
    if (g->attributeCount() < currentAttributeCount) {
        qWarning() << "render: Material requires" << currentAttributeCount
                   << "attributes, but geometry only provides" << g->attributeCount()
                   << "; skipping:" << gn;
        return;
    }

    int offset = 0;
    for (int i=0; i<currentAttributeCount; ++i) {
        const QSGGeometry::Attribute &a = g->attributes()[i];
        GLboolean normalize = a.type != GL_FLOAT && a.type != GL_DOUBLE;
        glVertexAttribPointer(a.position, a.tupleSize, a.type, normalize, g->sizeOfVertex(), (void *) (qintptr) (offset + e.vboOffset));
        offset += a.tupleSize * size_of_type(a.type);
    }

    if (g->drawingMode() == GL_LINE_STRIP || g->drawingMode() == GL_LINE_LOOP || g->drawingMode() == GL_LINES)
        glLineWidth(g->lineWidth());
#if !defined(QT_OPENGL_ES_2)
    else if (!QOpenGLContext::currentContext()->isOpenGLES() && g->drawingMode() == GL_POINTS) {
        QOpenGLFunctions_1_0 *gl1funcs = 0;
        QOpenGLFunctions_3_2_Core *gl3funcs = 0;
        if (QOpenGLContext::currentContext()->format().profile() == QSurfaceFormat::CoreProfile)
            gl3funcs = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_2_Core>();
        else
            gl1funcs = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_1_0>();
        Q_ASSERT(gl1funcs || gl3funcs);
        if (gl1funcs)
            gl1funcs->glPointSize(g->lineWidth());
        else
            gl3funcs->glPointSize(g->lineWidth());
    }
#endif

    if (e.iboOffset >= 0) {
        glDrawElements(g->drawingMode(), g->indexCount(), g->indexType(), (void *) (qintptr) e.iboOffset);
    } else {
        glDrawArrays(g->drawingMode(), 0, g->vertexCount());
    }

}

};
