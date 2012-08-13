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

#include "qsgtoplevelanimator.h"
#include "qsganimatorcontroller.h"
#include <QDebug>

QSGTopLevelAnimator::QSGTopLevelAnimator()
    : QSGAbstractAnimator(0, 0, 0, 0)
{
}

void QSGTopLevelAnimator::advance(qreal t)
{
    QSGAbstractAnimator::advance(t);

    for (int i = 0; i < m_animators.count(); i++) {
        m_animators.at(i)->advance(t);
    }
}

qreal QSGTopLevelAnimator::sync(bool topLevelRunning, qreal startTime)
{
    QSGAbstractAnimator::sync(topLevelRunning, startTime);

    qreal duration = 0.0;
    for (int i = 0; i < m_animators.count(); i++) {
        qreal childAnimationDuration = m_animators.at(i)->sync(topLevelRunning, startTime);
        duration = qMax(duration, childAnimationDuration);
    }
    duration *= m_loops;
    return duration;
}
