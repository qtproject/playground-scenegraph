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

#ifndef QSGANIMATOR_H
#define QSGANIMATOR_H

#include <QtCore>

class QSGAnimatorController;
class QSGAbstractAnimation;

class QSGAbstractAnimator
{
public:
    QSGAbstractAnimator();
    QSGAbstractAnimator(QSGAnimatorController *, QSGAbstractAnimator *, QSGAbstractAnimation *, qreal);
    virtual ~QSGAbstractAnimator();

    virtual void advance(qreal t = 0.0);
    virtual qreal sync(bool topLevelRunning, qreal startTime = 0.0);

    void registerAnimator(QSGAbstractAnimator *);
    void unregisterAnimator(QSGAbstractAnimator *);

    QSGAbstractAnimator* parent();
    void setParent(QSGAbstractAnimator *);

    QSGAbstractAnimation* qmlObject();
    void setQmlObject(QSGAbstractAnimation *);

    int startTime();
    void setStartTime(int);

    qreal elapsed();
    void setElapsed(qreal);

    int duration();
    void setDuration(int);

    int loops();
    void setLoops(int);

    bool running();
    void setRunning(bool);

    bool paused();
    void setPaused(bool);

    bool alwaysRunToEnd();
    void setAlwaysRunToEnd(bool);

    bool isActive();
    bool isUpdating();

private:
    void copyQmlObjectData();

protected:
    QSGAbstractAnimator *m_parent;
    QSGAnimatorController* m_controller;
    QList<QSGAbstractAnimator*> m_animators;
    int m_startTime;
    int m_stopTime;
    qreal m_elapsed;
    int m_duration;
    int m_loops;
    bool m_running;
    bool m_paused;
    bool m_alwaysRunToEnd;
    bool m_runningToEnd;
    bool m_hasControl;
    bool m_stopping;
    bool m_done;

private:
    QSGAbstractAnimation *m_qmlObject;
};

#endif // QSGANIMATOR_H
