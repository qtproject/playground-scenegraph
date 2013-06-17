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
        m_value = m_qmlObject->property(m_name.toLatin1().constData());
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
        QVariant v = m_qmlObject->property(p.toLatin1().constData());
        if (v.isValid()) {
            QVector3D vec = qvariant_cast<QVector3D>(v);
            m_value = vec.x();
            qDebug() << m_name << " = " << m_value;
        }
    } else if (p.contains(".y")) {
        QVariant v = m_qmlObject->property(p.toLatin1().constData());
        if (v.isValid()) {
            QVector3D vec = qvariant_cast<QVector3D>(v);
            m_value = vec.y();
            qDebug() << m_name << " = " << m_value;
        }
    } else if (p.contains(".z")) {
        QVariant v = m_qmlObject->property(p.toLatin1().constData());
        if (v.isValid()) {
            QVector3D vec = qvariant_cast<QVector3D>(v);
            m_value = vec.z();
            qDebug() << m_name << " = " << m_value;
        }
    } else {
        m_value = m_qmlObject->property(m_name.toLatin1().constData());
    }

    m_changed = false;
}
