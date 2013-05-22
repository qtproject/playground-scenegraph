/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Scenegraph Playground module of the Qt Toolkit.
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

#include "qsgsequentialanimator.h"
#include "qsgsequentialanimation.h"
#include "qsganimatorcontroller.h"
#include <QDebug>

QSGSequentialAnimator::QSGSequentialAnimator(QSGAnimatorController* controller, QSGAbstractAnimator *parent, QSGAbstractAnimation *qmlObject, qreal startTime)
    : QSGAbstractAnimator(controller, parent, qmlObject, startTime)
{
}

void QSGSequentialAnimator::advance(qreal t)
{
    if ((!m_running || m_paused) && !m_runningToEnd)
        return;

    QSGAbstractAnimator::advance(t);

    if (m_elapsed > m_startTime && ((m_elapsed < m_startTime + m_loops * m_duration) || (m_loops < 0))) {
        qreal tx = int(m_elapsed - m_startTime) % int(m_duration);
        for (int i = 0; i < m_animators.count(); i++) {
            QSGAbstractAnimator *child = m_animators.at(i);
            int childStartTime = child->startTime();
            int childDuration = child->duration();
            int childLoops = child->loops();

            qreal cx = int(m_startTime + tx - childStartTime) % int(childDuration);
            if (m_startTime + tx > childStartTime && m_startTime + tx < childStartTime + childDuration * childLoops) {
                child->setElapsed(childStartTime + cx);
                child->advance();
            }
        }
    }
}

qreal QSGSequentialAnimator::sync(bool topLevelRunning, qreal startTime)
{
    QSGAbstractAnimator::sync(topLevelRunning, startTime);

    qreal duration = 0.0;
    for (int i = 0; i < m_animators.count(); i++)
        duration += m_animators.at(i)->sync(topLevelRunning || (m_running && !m_paused) || m_runningToEnd, startTime + duration);

    duration *= m_loops;
    return duration;
}
