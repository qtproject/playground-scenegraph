/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of %MODULE%.
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

#include "qsganimatornode.h"
#include "qsganimatorcontroller.h"

#define MIN_OPACITY 0.001
#define MAX_OPACITY 0.999

QSGAnimatorNode::QSGAnimatorNode(QQuickItem *item)
    : m_item(item)
    , m_controller(0)
    , m_transformNode(0)
    , m_opacityNode(0)
{
    m_controller = new QSGAnimatorController(item);
}

QSGAnimatorNode::~QSGAnimatorNode()
{
    delete m_controller;
}

void QSGAnimatorNode::setTransformNode(QSGTransformNode *n)
{
    m_transformNode = n;
}

void QSGAnimatorNode::setOpacityNode(QSGOpacityNode *n)
{
    m_opacityNode = n;
}

void QSGAnimatorNode::preprocess()
{
    QSGNode::preprocess();
    if (m_controller->isInitialized()) {
        if (m_transformNode) {
            qreal x = m_controller->registeredProperty("x")->value().toReal();
            qreal y = m_controller->registeredProperty("y")->value().toReal();
            QMatrix4x4 m = m_controller->transformMatrix();
            QPointF transformOrigin = m_controller->registeredProperty("transformOriginPoint")->value().toPointF();
            qreal scale = m_controller->registeredProperty("scale")->value().toReal();
            qreal rotation = m_controller->registeredProperty("rotation")->value().toReal();
            m.translate(transformOrigin.x(), transformOrigin.y());
            m.translate(x, y);
            m.scale(scale);
            m.rotate(rotation, 0, 0, 1);
            m.translate(-transformOrigin.x(), -transformOrigin.y());
            m_transformNode->setMatrix(m);

            if (m_controller->isUpdating())
                m_transformNode->markDirty(QSGNode::DirtyMatrix);
        }

        if (m_opacityNode) {
            qreal opacity = m_controller->registeredProperty("opacity")->value().toReal();
            m_opacityNode->setOpacity(qMin(qreal(MAX_OPACITY), qMax(qreal(MIN_OPACITY), opacity)));

            if (m_controller->isUpdating())
                m_opacityNode->markDirty(QSGNode::DirtyOpacity);
        }
    }
}

QSGAnimatorController& QSGAnimatorNode::controller()
{
    return *m_controller;
}

