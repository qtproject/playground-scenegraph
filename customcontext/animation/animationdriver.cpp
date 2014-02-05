/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2014 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
** Contact: http://www.qt-project.org/legal
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

#include "animationdriver.h"

#include <QDebug>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <private/qguiapplication_p.h>

#define THRESHOLD_FOR_HARD_SWITCH 100
#define THRESHOLD_FOR_SOFT_SWITCH 100
#define CATCHUP_RATE 1

namespace CustomContext {

AnimationDriver::AnimationDriver()
    : m_vsyncDelta(-1)
    , m_animationTime(0)
    , m_timeMode(PredictedTime)
    , m_badFrameTime(-1)
{
    m_queryVsyncTimer.invalidate();
    m_animationTimer.invalidate();
}

void AnimationDriver::maybeUpdateDelta()
{
    // No point in updating it too frequently...
    if (m_queryVsyncTimer.elapsed() < 30000 && m_queryVsyncTimer.isValid())
        return;
    m_vsyncDelta = 1000.0 / QGuiApplication::primaryScreen()->refreshRate();
    m_queryVsyncTimer.restart();

#ifdef CUSTOMCONTEXT_DEBUG
    qDebug("CustomContext: AnimationDriver uses vsync=%f", m_vsyncDelta);
#endif
}

qint64 AnimationDriver::elapsed() const
{
    if (!isRunning() || m_vsyncDelta < 0 || m_timeMode == CurrentTime)
        return startTime() + m_animationTimer.elapsed();
    else
        return startTime() + m_animationTime;
}

void AnimationDriver::start()
{
    QAnimationDriver::start();
    m_animationTimer.restart();
    m_animationTime = 0;
    m_badFrameTime = -1;
}

void AnimationDriver::advance()
{
    maybeUpdateDelta();
    float currentTime = m_animationTimer.elapsed();

    if (m_vsyncDelta < 0) {
        m_animationTime = currentTime;
        QAnimationDriver::advanceAnimation(startTime() + m_animationTime);
        return;
    }

    m_animationTime += m_vsyncDelta;
    float drift = currentTime - m_animationTime;

    bool badFrame = drift > m_vsyncDelta * 0.1; // We're lagging behind, tolerate a 10% error compared to vsync.

    // If we are drifting behind with this much, we jump and force time-based
    if (drift > THRESHOLD_FOR_HARD_SWITCH) {
//        printf("animation driver crossed threshold for hard switch... drift=%f, animationTime=%f, currentTime=%f\n",
//               drift, m_animationTime, currentTime);
        m_timeMode = CurrentTime;
        m_animationTime = m_animationTimer.elapsed();

    } else if (m_timeMode == PredictedTime) {
        if (badFrame) {
            if (m_badFrameTime > 0 && currentTime - m_badFrameTime < THRESHOLD_FOR_SOFT_SWITCH) {
//                printf("animation lagged twice recently: first=%f, currentTime=%f, drift=%f\n",
//                       m_badFrameTime, currentTime, drift);
                m_timeMode = CurrentTime;
                m_animationTime = currentTime;
            }
            m_badFrameTime = currentTime;

        } else if (drift > CATCHUP_RATE) {
            // Slowly catch up when we are lagging behind.
            m_animationTime += CATCHUP_RATE;

        } else if (drift < - 2 * m_vsyncDelta) {
            // This will only happen if the m_vsyncDelta value is higher than
            // the actual vsync delta. When it is, we will accumulate a drift
            // ahead of current time which can result in jumps when switching
            // to CurrentTime.
            m_animationTime -= CATCHUP_RATE;
        }

    } else if (m_timeMode == CurrentTime) {
        if (badFrame) {
            m_badFrameTime = currentTime;
        } else if (m_badFrameTime > 0 && currentTime - m_badFrameTime > THRESHOLD_FOR_SOFT_SWITCH) {
//            printf("animation had good frames for a while: last-bad=%f, currentTime=%f, animTime=%f, drift=%f\n",
//                   m_badFrameTime, currentTime, m_animationTime, drift);
            m_timeMode = PredictedTime;
        }
        m_animationTime = currentTime;
    }

//    printf(" - (%s) advancing by: startTime=%d, animTime=%d, current=%d %s\n",
//           m_timeMode == PredictedTime ? "pre" : "cur",
//           (int) startTime(), (int) m_animationTime, (int) currentTime, badFrame ? "*bad*" : "");

    QAnimationDriver::advanceAnimation(startTime() + m_animationTime);
}

}
