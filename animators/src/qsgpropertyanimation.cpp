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

#include "qsgpropertyanimation.h"

#define MIN_OPACITY 0.001
#define MAX_OPACITY 0.999

QSGPropertyAnimation::QSGPropertyAnimation(QQuickItem *parent)
    : QSGAbstractAnimation(parent)
    , m_duration(0)
    , m_from(0)
    , m_to(0)
    , m_target(0)
{
    connect(this, SIGNAL(runningChanged(bool)), SLOT(prepare(bool)));
}

int QSGPropertyAnimation::duration()
{
    return m_duration;
}

void QSGPropertyAnimation::setDuration(int a)
{
    if (m_duration != a) {
        m_duration = a;
        emit durationChanged(m_duration);
    }
}

QVariant QSGPropertyAnimation::from()
{
    return m_from;
}

void QSGPropertyAnimation::setFrom(QVariant a)
{
    if (m_from != a) {
        m_from = a;
        emit fromChanged(m_from);
    }
}

QVariant QSGPropertyAnimation::to()
{
    return m_to;
}

void QSGPropertyAnimation::setTo(QVariant a)
{
    if (m_to != a) {
        m_to = a;
        emit toChanged(m_to);
    }
}

const QEasingCurve& QSGPropertyAnimation::easing()
{
    return m_easing;
}

void QSGPropertyAnimation::setEasing(const QEasingCurve& a)
{
    if (m_easing != a) {
        m_easing = a;
        emit easingChanged(m_easing);
    }
}

QObject* QSGPropertyAnimation::target()
{
    return m_target;
}

void QSGPropertyAnimation::setTarget(QObject* a)
{
    if (m_target != a) {
        m_target = a;
        emit targetChanged();
    }
}

const QString& QSGPropertyAnimation::property()
{
    return m_property;
}

void QSGPropertyAnimation::setProperty(QString a)
{
    if (m_property != a) {
        m_property = a;
        emit propertyChanged();
    }
}

const QString& QSGPropertyAnimation::properties()
{
    return m_properties;
}

void QSGPropertyAnimation::setProperties(QString a)
{
    if (m_properties != a) {
        m_properties = a;
        emit propertiesChanged();
    }
}

void QSGPropertyAnimation::complete()
{
    if (m_target) {
        QVariant v = m_to;
        QString p = m_property;
        if (m_property.contains(".x")) {
            p = m_property.left(m_property.length() - 2);
            v = m_target->property(p.toAscii().constData());
            QVector3D vec = qvariant_cast<QVector3D>(v);
            vec.setX(m_to.toReal());
            v = vec;
        } else if (m_property.contains(".y")) {
            v = m_target->property(p.toAscii().constData());
            QVector3D vec = qvariant_cast<QVector3D>(v);
            vec.setY(m_to.toReal());
            v = vec;
        } else if (m_property.contains(".z")) {
            v = m_target->property(p.toAscii().constData());
            QVector3D vec = qvariant_cast<QVector3D>(v);
            vec.setZ(m_to.toReal());
            v = vec;
        }
        m_target->setProperty(p.toAscii().constData(), v);
    }
    QSGAbstractAnimation::complete();
}

void QSGPropertyAnimation::prepare(bool v)
{
    // Make sure opacity node exists
    QQuickItem *t = qobject_cast<QQuickItem*> (m_target);
    if (t && v && m_property == "opacity") {
        if (t->opacity() < MIN_OPACITY) {
            t->setOpacity(MIN_OPACITY);
        }
        else if (t->opacity() > MAX_OPACITY) {
            t->setOpacity(MAX_OPACITY);
        }
#ifdef ANIMATORS_DEBUG
            qDebug() << "QSGPropertyAnimation::prepare opacity is now " << t->opacity();
#endif
    }
}
