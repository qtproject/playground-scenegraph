/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Scene Graph Playground module of the Qt Toolkit.
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

#include "qsgpolygon.h"

#include <QtQuick/qsgnode.h>
#include <QtQuick/qsgflatcolormaterial.h>

// #define QSGPOLYGON_DEBUG

class QSGPolygonNode : public QSGGeometryNode
{
public:
    QSGPolygonNode()
        : g(QSGGeometry::defaultAttributes_Point2D(), 0, 0)
    {
        setGeometry(&g);
        setMaterial(&m);
        setFlags(OwnsGeometry | OwnsMaterial, false);
    }

    QSGGeometry g;
    QSGFlatColorMaterial m;
};

QSGPolygon::QSGPolygon()
    : m_triangleSet(0)
    , m_color(Qt::white)
    , m_colorWasChanged(false)
    , m_triangleSetWasChanged(false)
{
    setFlag(ItemHasContents, true);
}

void QSGPolygon::setTriangleSet(QSGTriangleSet *set)
{
    if (set == m_triangleSet)
        return;

    if (m_triangleSet)
        disconnect(m_triangleSet, SIGNAL(changed()), this, SLOT(update()));
    m_triangleSet = set;
    connect(m_triangleSet, SIGNAL(changed()), this, SLOT(update()));

    m_triangleSetWasChanged = true;

    emit triangleSetChanged(m_triangleSet);
    update();
}

void QSGPolygon::setColor(const QColor &color)
{
    if (color == m_color)
        return;

    m_color = color;

    m_colorWasChanged = true;
    emit colorChanged(m_color);
    update();
}

QSGNode *QSGPolygon::updatePaintNode(QSGNode *old, UpdatePaintNodeData *)
{
    QSGPolygonNode *n = static_cast<QSGPolygonNode *>(old);

    if (m_triangleSet == 0 || !m_triangleSet->isValid() || !m_color.isValid() || m_color.alpha() == 0) {
        delete n;
        return 0;
    }

    if (!n)
        n = new QSGPolygonNode();

    if (m_triangleSetWasChanged) {
        QSGGeometry *g = &n->g;
        g->allocate(m_triangleSet->vertices2D().size(), m_triangleSet->indices().size());
        g->setDrawingMode(m_triangleSet->drawingMode());

        memcpy(g->vertexData(), m_triangleSet->vertices2D().constData(), g->sizeOfVertex() * g->vertexCount());
        memcpy(g->indexData(), m_triangleSet->indices().constData(), g->sizeOfIndex() * g->indexCount());

#ifdef QSGPOLYGON_DEBUG
        for (int i=0; i<g->vertexCount(); ++i)
            qDebug() << "Polygon Vertex:" <<  g->vertexDataAsPoint2D()[i].x << g->vertexDataAsPoint2D()[i].y;
        for (int i=0; i<g->indexCount(); ++i)
            qDebug() << "Polygon Index:" << g->indexDataAsUShort()[i];
#endif

        n->markDirty(QSGNode::DirtyGeometry);
        m_triangleSetWasChanged = false;
    }

    if (m_colorWasChanged) {
        n->m.setColor(m_color);
        m_colorWasChanged = false;
        n->markDirty(QSGNode::DirtyMaterial);
    }

    return n;
}

