/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "qsganimatedrotation.h"
#include "qsganimatedproperty.h"
#include "qsganimatorcontroller.h"
#include <QtGui>

QSGAnimatedRotation::QSGAnimatedRotation(QSGAnimatorController* controller, QObject* qmlObject) : QSGAnimatedTransform(controller, qmlObject)
{
    m_angle = new QSGAnimatedProperty(m_qmlObject, "angle", 0.0);
    m_axisX = new QSGAnimatedProperty(m_qmlObject, "axis.x", 0.0);
    m_axisY = new QSGAnimatedProperty(m_qmlObject, "axis.y", 0.0);
    m_axisZ = new QSGAnimatedProperty(m_qmlObject, "axis.z", 1.0);
    m_originX = new QSGAnimatedProperty(m_qmlObject, "origin.x", 0.0);
    m_originY = new QSGAnimatedProperty(m_qmlObject, "origin.y", 0.0);

    // controller takes owneship
    controller->registerProperty(m_angle);
    controller->registerProperty(m_axisX);
    controller->registerProperty(m_axisY);
    controller->registerProperty(m_axisZ);
    controller->registerProperty(m_originX);
    controller->registerProperty(m_originY);

    if (qmlObject) {
        setAngle(qmlObject->property("angle"));
        setAxis(qmlObject->property("axis"));
        setOrigin(qmlObject->property("origin"));
    }
}

void QSGAnimatedRotation::setAngle(QVariant v)
{
    m_angle->setValue(v);
}

void QSGAnimatedRotation::setAxis(QVariant v)
{
    if (v.isValid() && v.type() == QVariant::Vector3D){
        QVector3D vec = qvariant_cast<QVector3D>(v);
        m_axisX->setValue(vec.x());
        m_axisY->setValue(vec.y());
        m_axisZ->setValue(vec.z());
    }
}

void QSGAnimatedRotation::setAxisX(QVariant v)
{
    m_axisX->setValue(v);
}

void QSGAnimatedRotation::setAxisY(QVariant v)
{
    m_axisY->setValue(v);
}

void QSGAnimatedRotation::setAxisZ(QVariant v)
{
    m_axisZ->setValue(v);
}

void QSGAnimatedRotation::setOrigin(QVariant v)
{
    if (v.isValid() && v.type() == QVariant::Vector3D){
        QVector3D vec = qvariant_cast<QVector3D>(v);
        m_originX->setValue(vec.x());
        m_originY->setValue(vec.y());
    }
}

void QSGAnimatedRotation::setOriginX(QVariant v)
{
    m_originX->setValue(v);
}

void QSGAnimatedRotation::setOriginY(QVariant v)
{
    m_originY->setValue(v);
}

void QSGAnimatedRotation::applyTo(QMatrix4x4 &m)
{
    m.translate(m_originX->value().toReal(), m_originY->value().toReal());
    m.rotate(m_angle->value().toReal(), m_axisX->value().toReal(), m_axisY->value().toReal(), m_axisZ->value().toReal());
    m.translate(-m_originX->value().toReal(), -m_originY->value().toReal());
}

