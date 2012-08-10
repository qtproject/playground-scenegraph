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


#include "qsganimatoritem.h"
#include "qsganimatorcontroller.h"
#include "qsganimatornode.h"
#include <QtQuick/QQuickItem>
#include <QDebug>

QSGAnimatorItem::QSGAnimatorItem(QQuickItem *parent)
    : QQuickItem(parent)
    , m_animatorNode(0)
{
    setFlag(ItemHasContents);
}

QSGAnimatorItem::~QSGAnimatorItem()
{
}

QSGNode *QSGAnimatorItem::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
    QSGTransformNode *transformNode = static_cast<QSGTransformNode*>(QQuickItemPrivate::get(this)->itemNode());
    QSGOpacityNode *opacityNode = static_cast<QSGOpacityNode*>(QQuickItemPrivate::get(this)->opacityNode());

    if (!node) {
        m_animatorNode = new QSGAnimatorNode(this);
        m_animatorNode->setFlag(QSGNode::UsePreprocess, true);
    }

    if (m_animatorNode) {
        m_animatorNode->controller().sync();
        m_animatorNode->setTransformNode(transformNode);
        m_animatorNode->setOpacityNode(opacityNode);
    }

    return m_animatorNode;
}
