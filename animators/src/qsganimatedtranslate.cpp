/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qsganimatedtranslate.h"
#include "qsganimatedproperty.h"
#include "qsganimatorcontroller.h"
#include <QtGui>

QSGAnimatedTranslate::QSGAnimatedTranslate(QSGAnimatorController* controller, QObject* qmlObject) : QSGAnimatedTransform(controller, qmlObject)
{
    m_x = new QSGAnimatedProperty(m_qmlObject, "x", 0.0);
    m_y = new QSGAnimatedProperty(m_qmlObject, "y", 0.0);

    // controller takes owneship
    controller->registerProperty(m_x);
    controller->registerProperty(m_y);

    if (qmlObject) {
        setX(qmlObject->property("x"));
        setY(qmlObject->property("y"));
    }
}

void QSGAnimatedTranslate::setX(QVariant x)
{
    m_x->setValue(x);
}

void QSGAnimatedTranslate::setY(QVariant y)
{
    m_y->setValue(y);
}

void QSGAnimatedTranslate::applyTo(QMatrix4x4 &m)
{
    m.translate(m_x->value().toReal(), m_y->value().toReal());
}
