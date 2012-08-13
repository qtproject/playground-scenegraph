/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of %MODULE%.
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

#include "qsganimatedscale.h"
#include "qsganimatedproperty.h"
#include "qsganimatorcontroller.h"
#include <QtGui>

QSGAnimatedScale::QSGAnimatedScale(QSGAnimatorController* controller, QObject* qmlObject) : QSGAnimatedTransform(controller, qmlObject)
{
    m_xScale = new QSGAnimatedProperty(m_qmlObject, "xScale", 1.0);
    m_yScale = new QSGAnimatedProperty(m_qmlObject, "yScale", 1.0);
    m_originX = new QSGAnimatedProperty(m_qmlObject, "origin.x", 0.0);
    m_originY = new QSGAnimatedProperty(m_qmlObject, "origin.y", 0.0);

    // controller takes ownership
    controller->registerProperty(m_xScale);
    controller->registerProperty(m_yScale);
    controller->registerProperty(m_originX);
    controller->registerProperty(m_originY);

    if (qmlObject) {
        setXScale(qmlObject->property("xScale"));
        setYScale(qmlObject->property("yScale"));
        setOrigin(qmlObject->property("origin"));
    }
}

void QSGAnimatedScale::setOrigin(QVariant v)
{
    if (v.isValid() && v.type() == QVariant::Vector3D){
        QVector3D vec = qvariant_cast<QVector3D>(v);
        m_originX->setValue(vec.x());
        m_originY->setValue(vec.y());
    }
}

void QSGAnimatedScale::setXScale(QVariant v)
{
    m_xScale->setValue(v);
}

void QSGAnimatedScale::setYScale(QVariant v)
{
    m_yScale->setValue(v);
}

void QSGAnimatedScale::setOriginX(QVariant v)
{
    m_originX->setValue(v);
}

void QSGAnimatedScale::setOriginY(QVariant v)
{
    m_originY->setValue(v);
}

void QSGAnimatedScale::applyTo(QMatrix4x4 &m)
{
    m.translate(m_originX->value().toReal(), m_originY->value().toReal());
    m.scale(m_xScale->value().toReal(), m_yScale->value().toReal());
    m.translate(-m_originX->value().toReal(), -m_originY->value().toReal());
}

