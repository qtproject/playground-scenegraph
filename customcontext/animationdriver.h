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

#ifndef ANIMATIONDRIVER_H
#define ANIMATIONDRIVER_H

#include <QVector>
#include <QAnimationDriver>
#include <qelapsedtimer.h>
#include <qmath.h>

#define FRAME_SMOOTH_COUNT 60 * 2

namespace CustomContext
{

class AnimationDriver : public QAnimationDriver
{
public:
    AnimationDriver();

    void start();
    void advance();

    void maybeUpdateDelta();

    virtual qint64 elapsed() const;

private:
    QElapsedTimer m_timer;
    QElapsedTimer m_last_updated;

    float m_stable_vsync;
    float m_current_animation_time;
    float m_current_animation_delay;
    float m_current_animation_catchup;

    float m_threshold_for_catchup;
    float m_catchup_ratio;
};

}

#endif // ANIMATIONDRIVER_H
