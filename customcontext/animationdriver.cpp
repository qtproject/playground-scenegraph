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

#include "animationdriver.h"

#include <QDebug>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <private/qguiapplication_p.h>

#include "windowmanager.h"

namespace CustomContext {

float get_env_float(const char *name, float defaultValue)
{
    QByteArray content = qgetenv(name);

    bool ok = false;
    float value = content.toFloat(&ok);
    return ok ? value : defaultValue;
}


AnimationDriver::AnimationDriver()
    : m_stable_vsync(-1)
    , m_current_animation_time(0)
    , m_current_animation_delay(0)
    , m_current_animation_catchup(0)
{
    m_last_updated.invalidate();
    m_timer.invalidate();

    // threshold for doing a non-smooth jump is 250ms
    m_threshold_for_catchup = get_env_float("QML_ANIMATION_DRIVER_CATCHUP_THRESHOLD", 250);

    // Try to catch up over 20 frames.
    m_catchup_ratio = get_env_float("QML_ANIMATION_DRIVER_CATCHUP_RATIO", 0.05);
}

void AnimationDriver::maybeUpdateDelta()
{
    // No point in updating it too frequently...
    if (m_last_updated.elapsed() < 30000 && m_last_updated.isValid())
        return;

    m_stable_vsync = 1000.0 / QGuiApplication::primaryScreen()->refreshRate();
    m_last_updated.restart();
}

qint64 AnimationDriver::elapsed() const
{
    if (WindowManager::fakeRendering || !isRunning() || m_stable_vsync < -1)
        return startTime() + m_timer.elapsed();
    else
        return startTime() + m_current_animation_time;
}

void AnimationDriver::start()
{
    QAnimationDriver::start();
    m_timer.restart();
    m_current_animation_time = 0;
    m_current_animation_delay = 0;
    m_current_animation_catchup = 0;
}


void AnimationDriver::advance()
{
    maybeUpdateDelta();

    if (WindowManager::fakeRendering || m_stable_vsync < 0) {
        m_current_animation_time = m_timer.elapsed();
    } else {

        m_current_animation_time += m_stable_vsync + m_current_animation_catchup;
        m_current_animation_delay -= m_current_animation_catchup;

        if (m_current_animation_delay < m_current_animation_catchup*0.5) {
            // Caught up after a dropped frame
            m_current_animation_delay = 0.0;
            m_current_animation_catchup = 0.0;
        }

        if (m_current_animation_time < m_timer.elapsed()) {
            m_current_animation_delay = m_timer.elapsed() - m_current_animation_time;
#ifdef CUSTOMCONTEXT_DEBUG
            qDebug("CustomContext: animation needs compensation, calculated=%f, real=%f", (float) m_current_animation_time, (float) m_timer.elapsed());
#endif
            m_current_animation_time = qFloor((m_timer.elapsed() / m_stable_vsync) + 1) * m_stable_vsync;

            if (m_current_animation_delay > m_threshold_for_catchup) {
                // If we are far behind, don't try the smooth catchup, just jump
                m_current_animation_time += m_current_animation_delay;
                m_current_animation_delay = 0.0;
            }
            m_current_animation_catchup = m_current_animation_delay * m_catchup_ratio; // Catch up over 20 frames
        }
    }

    QAnimationDriver::advanceAnimation(startTime() + m_current_animation_time);
}

}
