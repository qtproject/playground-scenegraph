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

#include "qsgpropertyanimator.h"
#include "qsgpropertyanimation.h"
#include "qsganimatorcontroller.h"
#include <QDebug>

QSGPropertyAnimator::QSGPropertyAnimator(QSGAnimatorController* controller, QSGAbstractAnimator *parent, QSGPropertyAnimation *qmlObject, qreal startTime)
    : QSGAbstractAnimator(controller, parent, qmlObject, startTime)
    , m_qmlObject(qmlObject)
    , m_target(0)
    , m_property("")
    , m_from(0.0)
    , m_to(0.0)
{
    copyQmlObjectData();
}

void QSGPropertyAnimator::updateProperty(QObject *target, const QString& p)
{
    QSGAnimatedProperty *ap = m_controller->registeredProperty(p, target);
    if (ap && m_duration > 0) {
        if (m_elapsed > m_startTime && ((m_elapsed < m_startTime + m_loops * m_duration) || (m_loops < 0))) {

            QVariant value = ap->value();
            qreal tx = int(m_elapsed - m_startTime) % int(m_duration);

            switch (value.type()) {
            case QMetaType::Double:
                value = QVariant(m_from.toReal() + (m_to.toReal() - m_from.toReal()) * m_easing.valueForProgress(tx / m_duration));
                break;
            case QMetaType::QColor:
                {
                QColor from = qvariant_cast<QColor>(m_from);
                QColor to = qvariant_cast<QColor>(m_to);
                QColor result = qvariant_cast<QColor>(value);
                result.setRed(from.red() + (to.red() - from.red()) * m_easing.valueForProgress(tx / m_duration));
                result.setGreen(from.green() + (to.green() - from.green()) * m_easing.valueForProgress(tx / m_duration));
                result.setBlue(from.blue() + (to.blue() - from.blue()) * m_easing.valueForProgress(tx / m_duration));
                result.setAlpha(from.alpha() + (to.alpha() - from.alpha()) * m_easing.valueForProgress(tx / m_duration));
                value = result;
                break;
                }
            case QMetaType::Int:
                value = QVariant(m_from.toInt() + (m_to.toInt() - m_from.toInt()) * m_easing.valueForProgress(tx / m_duration));
                break;
            case QMetaType::QSize:
                {
                QSize from = m_from.toSize();
                QSize to = m_to.toSize();
                QSize result = value.toSize();
                result.setWidth(from.width() + (to.width() - from.width()) * m_easing.valueForProgress(tx / m_duration));
                result.setHeight(from.height() + (to.height() - from.height()) * m_easing.valueForProgress(tx / m_duration));
                value = result;
                break;
                }
            case QMetaType::QSizeF:
                {
                QSizeF from = m_from.toSize();
                QSizeF to = m_to.toSize();
                QSizeF result = value.toSize();
                result.setWidth(from.width() + (to.width() - from.width()) * m_easing.valueForProgress(tx / m_duration));
                result.setHeight(from.height() + (to.height() - from.height()) * m_easing.valueForProgress(tx / m_duration));
                value = result;
                break;
                }
            case QMetaType::QPoint:
                {
                QPoint from = m_from.toPoint();
                QPoint to = m_to.toPoint();
                QPoint result = value.toPoint();
                result.setX(from.x() + (to.x() - from.x()) * m_easing.valueForProgress(tx / m_duration));
                result.setY(from.y() + (to.y() - from.y()) * m_easing.valueForProgress(tx / m_duration));
                value = result;
                break;
                }
            case QMetaType::QPointF:
                {
                QPointF from = m_from.toPointF();
                QPointF to = m_to.toPointF();
                QPointF result = value.toPointF();
                result.setX(from.x() + (to.x() - from.x()) * m_easing.valueForProgress(tx / m_duration));
                result.setY(from.y() + (to.y() - from.y()) * m_easing.valueForProgress(tx / m_duration));
                value = result;
                break;
                }
            case QMetaType::QRect:
                {
                QRect from = m_from.toRect();
                QRect to = m_to.toRect();
                QRect result = value.toRect();
                result.setX(from.x() + (to.x() - from.x()) * m_easing.valueForProgress(tx / m_duration));
                result.setY(from.y() + (to.y() - from.y()) * m_easing.valueForProgress(tx / m_duration));
                result.setWidth(from.width() + (to.width() - from.width()) * m_easing.valueForProgress(tx / m_duration));
                result.setHeight(from.height() + (to.height() - from.height()) * m_easing.valueForProgress(tx / m_duration));
                value = result;
                break;
                }
            case QMetaType::QRectF:
                {
                QRectF from = m_from.toRectF();
                QRectF to = m_to.toRectF();
                QRectF result = value.toRectF();
                result.setX(from.x() + (to.x() - from.x()) * m_easing.valueForProgress(tx / m_duration));
                result.setY(from.y() + (to.y() - from.y()) * m_easing.valueForProgress(tx / m_duration));
                result.setWidth(from.width() + (to.width() - from.width()) * m_easing.valueForProgress(tx / m_duration));
                result.setHeight(from.height() + (to.height() - from.height()) * m_easing.valueForProgress(tx / m_duration));
                value = result;
                break;
                }
            case QMetaType::QVector3D:
                {
                QVector3D from = qvariant_cast<QVector3D>(m_from);
                QVector3D to = qvariant_cast<QVector3D>(m_to);
                QVector3D result = qvariant_cast<QVector3D>(value);
                result.setX(from.x() + (to.x() - from.x()) * m_easing.valueForProgress(tx / m_duration));
                result.setY(from.y() + (to.y() - from.y()) * m_easing.valueForProgress(tx / m_duration));
                result.setZ(from.z() + (to.z() - from.z()) * m_easing.valueForProgress(tx / m_duration));
                value = result;
                break;
                }
            default:
                break;
            }
            ap->setValue(value);
        }
    }
}

void QSGPropertyAnimator::advance(qreal t)
{
    if ((!m_running || m_paused) && !m_runningToEnd)
        return;

    QSGAbstractAnimator::advance(t);

    if (!m_target)
        return;

    QStringList properties;
    if (!m_properties.isEmpty()) {
        properties = m_properties.split(",");
        for (int i = 0; i < properties.count(); i++)
            updateProperty(m_target, properties[i].trimmed());
    } else {
        updateProperty(m_target, m_property);
    }
}

qreal QSGPropertyAnimator::sync(bool topLevelRunning, qreal startTime)
{
    QSGAbstractAnimator::sync(topLevelRunning, startTime);
    copyQmlObjectData();
    return m_duration;
}

void QSGPropertyAnimator::copyQmlObjectData()
{
    if (m_qmlObject) {
        m_target = m_qmlObject->target();
        m_property = m_qmlObject->property();
        m_properties = m_qmlObject->properties();
        m_from = m_qmlObject->from();
        m_to = m_qmlObject->to();
        m_duration = m_qmlObject->duration();
        m_easing = m_qmlObject->easing();
    }
}


QObject* QSGPropertyAnimator::target()
{
    return m_target;
}

void QSGPropertyAnimator::setTarget(QObject* a)
{
    m_target = a;
}

const QString& QSGPropertyAnimator::property()
{
    return m_property;
}

void QSGPropertyAnimator::setProperty(QString a)
{
    m_property = a;
}

QVariant QSGPropertyAnimator::from()
{
    return m_from;
}

void QSGPropertyAnimator::setFrom(QVariant a)
{
    m_from = a;
}

QVariant QSGPropertyAnimator::to()
{
    return m_to;
}

void QSGPropertyAnimator::setTo(QVariant a)
{
    m_to = a;
}

const QEasingCurve& QSGPropertyAnimator::easing()
{
    return m_easing;
}

void QSGPropertyAnimator::setEasing(const QEasingCurve& a)
{
    m_easing = a;
}
