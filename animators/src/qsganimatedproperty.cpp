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

#include "qsganimatedproperty.h"
#include "qsganimatorcontroller.h"

QSGAnimatedProperty::QSGAnimatedProperty() :
    m_qmlObject(0)
  , m_name("")
  , m_changed(false)
{
}

QSGAnimatedProperty::QSGAnimatedProperty(QObject *qmlObject, QString name, QVariant value) :
      m_qmlObject(qmlObject)
    , m_name(name)
    , m_value(value)
    , m_changed(false)
{
    if (m_qmlObject)
        m_value = m_qmlObject->property(m_name.toAscii().constData());
}

QSGAnimatedProperty::~QSGAnimatedProperty()
{
}

void QSGAnimatedProperty::setQmlObject(QObject *v)
{
    m_qmlObject = v;
}

QObject *QSGAnimatedProperty::qmlObject()
{
    return m_qmlObject;
}

void QSGAnimatedProperty::setValue(QVariant v)
{
    m_value = v;
    m_changed = true;
}

QVariant QSGAnimatedProperty::value()
{
    return m_value;
}

void QSGAnimatedProperty::setName(QString& name)
{
    m_name = name;
}

QString QSGAnimatedProperty::name()
{
    return m_name;
}

void QSGAnimatedProperty::setChanged(bool value)
{
    m_changed = value;
}

bool QSGAnimatedProperty::changed()
{
    return m_changed;
}

void QSGAnimatedProperty::sync()
{
    if (!m_qmlObject)
        return;

    QString p = m_name;
    if (p.contains(".x")) {
        p = p.left(p.length() - 2);
        QVariant v = m_qmlObject->property(p.toAscii().constData());
        if (v.isValid()) {
            QVector3D vec = qvariant_cast<QVector3D>(v);
            m_value = vec.x();
            qDebug() << m_name << " = " << m_value;
        }
    } else if (p.contains(".y")) {
        QVariant v = m_qmlObject->property(p.toAscii().constData());
        if (v.isValid()) {
            QVector3D vec = qvariant_cast<QVector3D>(v);
            m_value = vec.y();
            qDebug() << m_name << " = " << m_value;
        }
    } else if (p.contains(".z")) {
        QVariant v = m_qmlObject->property(p.toAscii().constData());
        if (v.isValid()) {
            QVector3D vec = qvariant_cast<QVector3D>(v);
            m_value = vec.z();
            qDebug() << m_name << " = " << m_value;
        }
    } else {
        m_value = m_qmlObject->property(m_name.toAscii().constData());
    }

    m_changed = false;
}
