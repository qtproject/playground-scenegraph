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

#ifndef QSGABSTRACTANIMATION_H
#define QSGABSTRACTANIMATION_H

#include <QtCore>
#include <QtQuick>

class QSGAbstractAnimation : public QQuickItem
{
    Q_OBJECT
    Q_ENUMS(Loops)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool paused READ isPaused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool alwaysRunToEnd READ alwaysRunToEnd WRITE setAlwaysRunToEnd NOTIFY alwaysRunToEndChanged)
    Q_PROPERTY(int loops READ loops WRITE setLoops NOTIFY loopCountChanged)
    Q_PROPERTY(QObject* target READ target WRITE setTarget NOTIFY targetChanged)

public:
    QSGAbstractAnimation(QQuickItem *parent = 0);

    enum Loops { Infinite = -2 };

    bool isRunning();
    void setRunning(bool);

    bool isPaused();
    void setPaused(bool);

    bool alwaysRunToEnd();
    void setAlwaysRunToEnd(bool);

    int loops();
    void setLoops(int);

    QObject* target();
    void setTarget(QObject*);

public Q_SLOTS:
    virtual void complete();
    virtual void prepare(bool);

Q_SIGNALS:
    void completed();
    void runningChanged(bool);
    void pausedChanged(bool);
    void alwaysRunToEndChanged(bool);
    void loopCountChanged(int);
    void targetChanged();

protected:
    bool m_running;
    bool m_paused;
    bool m_alwaysRunToEnd;
    int m_loops;
    QObject* m_target;

private:
    Q_DISABLE_COPY(QSGAbstractAnimation)

};

#endif // QSGABSTRACTANIMATION_H
