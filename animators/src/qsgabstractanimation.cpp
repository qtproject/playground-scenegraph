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

QSGAbstractAnimation::QSGAbstractAnimation(QQuickItem *parent)
  : QQuickItem(parent)
  , m_running(false)
  , m_paused(false)
  , m_alwaysRunToEnd(false)
  , m_loops(1)
  , m_target(0)
{
}

QObject* QSGAbstractAnimation::target()
{
    return m_target;
}

void QSGAbstractAnimation::setTarget(QObject* a)
{
    if (m_target != a) {
        m_target = a;
        emit targetChanged();
    }
}

int QSGAbstractAnimation::loops()
{
    return m_loops;
}

void QSGAbstractAnimation::setLoops(int a)
{
    if (m_loops != a) {
        m_loops = a;
        emit loopCountChanged(m_loops);
    }
}

bool QSGAbstractAnimation::isRunning()
{
    return m_running;
}

void QSGAbstractAnimation::setRunning(bool a)
{
    if (m_running != a) {
        m_running = a;
        emit runningChanged(m_running);
    }
}

bool QSGAbstractAnimation::isPaused()
{
    return m_paused;
}

void QSGAbstractAnimation::setPaused(bool a)
{
    if (m_paused != a) {
        m_paused = a;
        emit pausedChanged(m_paused);
    }
}

bool QSGAbstractAnimation::alwaysRunToEnd()
{
    return m_alwaysRunToEnd;
}

void QSGAbstractAnimation::setAlwaysRunToEnd(bool a)
{
    if (m_alwaysRunToEnd != a) {
        m_alwaysRunToEnd = a;
        emit alwaysRunToEndChanged(m_alwaysRunToEnd);
    }
}

void QSGAbstractAnimation::complete()
{
    emit completed();
    setRunning(false);
}

void QSGAbstractAnimation::prepare(bool)
{
}
