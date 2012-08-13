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

#include "qsgabstractanimation.h"
#include "qsganimatorhost.h"
#include <private/qpauseanimationjob_p.h>

QSGAbstractAnimation::QSGAbstractAnimation(QObject *parent)
  : QQuickAnimationGroup(parent)
  , m_registered(false)
  , m_transitionRunning(false)
{
}

void QSGAbstractAnimation::complete()
{
    m_transitionRunning = false;
}

void QSGAbstractAnimation::prepare(bool)
{
}

bool QSGAbstractAnimation::isTransitionRunning()
{
    return m_transitionRunning;
}

void QSGAbstractAnimation::registerToHost(QObject* target)
{
    if (m_registered)
        return;

    QObject *host = target;
    QObject *hostParent = host ? host->parent() : 0;

    while (host && !host->inherits("QSGAnimatorHost")) {
        host = hostParent;
        hostParent = host ? host->parent() : 0;
    }

    QObject *animation = this;
    QObject *animationParent = parent();

    while (animationParent && animationParent->inherits("QSGAbstractAnimation")) {
        animation = animationParent;
        animationParent = animation ? animation->parent() : 0;
    }

    if (host && host->inherits("QSGAnimatorHost")) {
        QSGAnimatorHost* h = dynamic_cast<QSGAnimatorHost*> (host);
        QSGAbstractAnimation *a = dynamic_cast<QSGAbstractAnimation*> (animation);
        h->registerAnimation(a);
        m_registered = true;
    }
    else {
        qWarning() << "QSGAbstractAnimation::registerToHost() unable to register, " << target << " (or its parent) is not QSGAnimatorHost.";
    }
}
