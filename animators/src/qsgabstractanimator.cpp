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

#include "qsgabstractanimator.h"
#include "qsgabstractanimation.h"
#include "qsganimatorcontroller.h"
#include <QDebug>

QSGAbstractAnimator::QSGAbstractAnimator(QSGAnimatorController* controller, QSGAbstractAnimator *parent, QSGAbstractAnimation *qmlObject, qreal startTime)
  : m_parent(parent)
  , m_controller(controller)
  , m_startTime(startTime)
  , m_stopTime(0)
  , m_elapsed(0)
  , m_duration(0)
  , m_loops(1)
  , m_running(false)
  , m_paused(false)
  , m_alwaysRunToEnd(false)
  , m_runningToEnd(false)
  , m_hasControl(false)
  , m_stopping(false)
  , m_done(false)
  , m_qmlObject(qmlObject)
{
    copyQmlObjectData();
    if (m_parent)
        m_parent->registerAnimator(this);
}

QSGAbstractAnimator::~QSGAbstractAnimator()
{
    for (int i = 0; i < m_animators.count(); i++) {
        m_animators.at(i)->setParent(0);
        delete m_animators.at(i);
    }

    if (m_parent)
        m_parent->unregisterAnimator(this);
}

void QSGAbstractAnimator::advance(qreal t)
{
    if ((!m_running || m_paused) && !m_runningToEnd)
        return;

    m_elapsed += t;

    if (m_hasControl && m_elapsed > m_startTime + m_duration * m_loops) {
        m_stopping = true;
    }

    if (m_runningToEnd && m_elapsed > m_stopTime) {
#ifdef ANIMATORS_DEBUG
        qDebug() << "QSGAbstractAnimator::advance running to end, stopping soon. m_elapsed = " << m_elapsed << " m_stopTime = " << m_stopTime;
#endif
        m_runningToEnd = false;
        m_done = true;
        m_stopTime = 0.0;
        m_elapsed = m_stopTime;
    }
}

void QSGAbstractAnimator::copyQmlObjectData()
{
    if (m_qmlObject) {
        m_loops = m_qmlObject->loops();
        m_running = m_qmlObject->isRunning();
        m_paused = m_qmlObject->isPaused();
        m_alwaysRunToEnd = m_qmlObject->alwaysRunToEnd();
    }
}

qreal QSGAbstractAnimator::sync(bool topLevelRunning, qreal startTime)
{
    m_startTime = startTime;
    bool wasRunning = m_running;

    copyQmlObjectData();
    if (m_qmlObject) {
        m_running = m_qmlObject->isRunning() || topLevelRunning;
        m_hasControl = m_qmlObject->isRunning() && !topLevelRunning;
    }

    // Normal stop request detected from QML side.
    if (!wasRunning && m_running && !m_runningToEnd) {
        m_elapsed = 0.0;
    }

    // Stop request from QML side detected, but must run to the end. Calculate stoptime.
    if (m_alwaysRunToEnd && wasRunning && !m_running && m_duration > 0 && !m_runningToEnd) {
        m_runningToEnd = true;
        m_stopTime = m_startTime + qMax(1, 1 + int((m_elapsed - m_startTime) / m_duration)) * m_duration;
#ifdef ANIMATORS_DEBUG
        qDebug() << "QSGAbstractAnimator::sync m_stopTime = " << m_stopTime << " m_elapsed = " << m_elapsed;
#endif
    }

    // Running, cancel possible run to the end sequence etc.
    if (m_running) {
        m_done = false;
        m_runningToEnd = false;
        m_stopTime = 0.0;
    }

    // Stop because running to the end of the animation duration.
    if (m_qmlObject && m_running && m_stopping) {
        m_stopping = false;
        m_running = false;
        m_done = true;
#ifdef ANIMATORS_DEBUG
        qDebug() << "QSGAbstractAnimator::sync stopping because end of animation reached.";
#endif
    }

    // If done, let QML side know about it.
    if (m_qmlObject && m_done) {
        m_done = false;
#ifdef ANIMATORS_DEBUG
        qDebug() << "QSGAbstractAnimator::sync calling complete";
#endif
        int count = m_animators.count();
        for (int i = 0; i < count; i++) {
            QObject *o = m_animators.at(i)->qmlObject();
            if (o)
                QMetaObject::invokeMethod(o, "complete", Qt::QueuedConnection);
        }
        QMetaObject::invokeMethod(m_qmlObject, "complete", Qt::QueuedConnection);
    }

    return 0.0;
}

void QSGAbstractAnimator::registerAnimator(QSGAbstractAnimator *a)
{
    if (!m_animators.contains(a)) {
        m_animators.append(a);
#ifdef ANIMATORS_DEBUG
        qDebug() << " ";
        qDebug() << "QSGAbstractAnimator::registerAnimator - " << a;
#endif
    }
}

void QSGAbstractAnimator::unregisterAnimator(QSGAbstractAnimator *a)
{
    m_animators.removeAll(a);
#ifdef ANIMATORS_DEBUG
    qDebug() << "QSGAbstractAnimator::unregisterAnimator - " << a;
#endif
}

QSGAbstractAnimator* QSGAbstractAnimator::parent()
{
    return m_parent;
}

void QSGAbstractAnimator::setParent(QSGAbstractAnimator *a)
{
    m_parent = a;
}


QSGAbstractAnimation* QSGAbstractAnimator::qmlObject()
{
    return m_qmlObject;
}

void QSGAbstractAnimator::setQmlObject(QSGAbstractAnimation* a)
{
    m_qmlObject = a;
}

int QSGAbstractAnimator::startTime()
{
    return m_startTime;
}

void QSGAbstractAnimator::setStartTime(int a)
{
    m_startTime = a;
}

qreal QSGAbstractAnimator::elapsed()
{
    return m_elapsed;
}

void QSGAbstractAnimator::setElapsed(qreal a)
{
    m_elapsed = a;
}

int QSGAbstractAnimator::duration()
{
    return m_duration;
}

void QSGAbstractAnimator::setDuration(int a)
{
    m_duration = a;
}

int QSGAbstractAnimator::loops()
{
    return m_loops;
}

void QSGAbstractAnimator::setLoops(int a)
{
    m_loops = a;
}

bool QSGAbstractAnimator::running()
{
    return m_running;
}

void QSGAbstractAnimator::setRunning(bool a)
{
    m_running = a;
}

bool QSGAbstractAnimator::paused()
{
    return m_paused;
}

void QSGAbstractAnimator::setPaused(bool a)
{
    m_paused = a;
}

bool QSGAbstractAnimator::alwaysRunToEnd()
{
    return m_alwaysRunToEnd;
}

void QSGAbstractAnimator::setAlwaysRunToEnd(bool a)
{
    m_alwaysRunToEnd = a;
}

bool QSGAbstractAnimator::isActive()
{
    if (m_running || m_runningToEnd)
        return true;

    bool childRunning = false;

    for (int i = 0; i < m_animators.count(); i++)
        childRunning |= m_animators.at(i)->isActive();

    return childRunning;
}

bool QSGAbstractAnimator::isUpdating()
{
    if ((m_running && !m_paused) || m_runningToEnd)
        return true;

    bool childRunning = false;

    for (int i = 0; i < m_animators.count(); i++)
        childRunning |= m_animators.at(i)->isUpdating();

    return childRunning;
}
