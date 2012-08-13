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

#include "qsgsequentialanimation.h"
#include <private/qpauseanimationjob_p.h>

QSGSequentialAnimation::QSGSequentialAnimation(QObject *parent)
    : QSGAbstractAnimation(parent)
{
    connect(this, SIGNAL(runningChanged(bool)), SLOT(prepare(bool)));
}

void QSGSequentialAnimation::prepare(bool v)
{
    int count = children().count();
    for (int i = 0; i < count; i++) {
        QSGAbstractAnimation *a = qobject_cast<QSGAbstractAnimation*>(children().at(i));
        if (a)
            a->prepare(v);
    }
}

QAbstractAnimationJob* QSGSequentialAnimation::transition(QQuickStateActions &actions,
                        QQmlProperties &modified,
                        TransitionDirection direction,
                        QObject *defaultTarget)
{
    if (!isRunning())
        m_transitionRunning = true;

    prepareTransition(actions, modified, direction, defaultTarget);

    QPauseAnimationJob *job = new QPauseAnimationJob();
    job->setLoopCount(loops());
    job->setDuration(-1);
    return job;
}

void QSGSequentialAnimation::prepareTransition(QQuickStateActions &actions,
                                             QQmlProperties &modified,
                                             TransitionDirection direction,
                                             QObject *defaultTarget)
{
    if (direction != Forward)
        qWarning("QSGSequentialAnimation::prepareTransition - Backward transition not yet supported.");

    if (actions.count() > 0) {
        registerToHost(actions.at(0).property.object());
    }

    int count = children().count();
    for (int i = 0; i < count; i++) {
        QSGAbstractAnimation* a = dynamic_cast<QSGAbstractAnimation*> (children().at(i));
        if (a)
            a->prepareTransition(actions, modified, direction, defaultTarget);
    }
}
