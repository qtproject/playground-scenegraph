#include "swaplisteninganimationdriver.h"

#include <QtCore/qmath.h>

#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

#include <QtQuick/QQuickWindow>


float get_env_int(const char *name, int defaultValue)
{
    QByteArray content = qgetenv(name);

    bool ok = false;
    float value = content.toInt(&ok);
    return ok ? value : defaultValue;
}

SwapListeningAnimationDriver::SwapListeningAnimationDriver()
    : QAnimationDriver()
{
    m_stableVsync = 0;
    m_currentTime = 0;

    m_stabilizer.initialized = false;
    m_stabilizer.lowerBound = get_env_int("CUSTOMCONTEXT_SWAPLISTENINGANIMATIONDRIVER_LOWERBOUND", 15);
    m_stabilizer.upperBound = get_env_int("CUSTOMCONTEXT_SWAPLISTENINGANIMATIONDRIVER_UPPERBOUND", 18);
    m_stabilizer.requiredCount = get_env_int("CUSTOMCONTEXT_SWAPLISTENINGANIMATIONDRIVER_REQUIRED", 100);

#ifdef CUSTOMCONTEXT_DEBUG
    qDebug(" --- SwapListeningAnimationDriver: lowerBound: %d", m_stabilizer.lowerBound);
    qDebug(" --- SwapListeningAnimationDriver: upperBound: %d", m_stabilizer.upperBound);
    qDebug(" --- SwapListeningAnimationDriver: requiredCount: %d", m_stabilizer.requiredCount);
#endif
}

void SwapListeningAnimationDriver::startListening()
{
    QWindow *focusWindow = QGuiApplication::focusWindow();
    if (!focusWindow) {
        return;
    }

    QQuickWindow *window = qobject_cast<QQuickWindow *>(focusWindow);
    if (!window) {
        return;
    }
    m_stabilizer.initialized = true;
    m_stabilizer.reset();

    connect(window, SIGNAL(frameSwapped()), this, SLOT(swapped()), Qt::DirectConnection);
    connect(this, SIGNAL(stableVSyncChanged(qreal)), this, SLOT(updateStableVSync(qreal)));
}

void SwapListeningAnimationDriver::Stabilizer::reset()
{
    count = 0;
    timer.start();
    lastTime = 0;

}

// Note: this function is called on the render thread..
void SwapListeningAnimationDriver::swapped()
{
    int current = m_stabilizer.timer.elapsed();
    int delta = current - m_stabilizer.lastTime;
    if (delta < m_stabilizer.lowerBound || delta > m_stabilizer.upperBound) {
        m_stabilizer.reset();
    } else {
        ++m_stabilizer.count;
        if (m_stabilizer.count > m_stabilizer.requiredCount) {
            qreal vsync = current / (qreal) m_stabilizer.count;
#ifdef CUSTOMCONTEXT_DEBUG
            if (m_stableVsync == 0)
                qDebug(" --- Swap Animation Driver: Stable VSync found %.2f\n", vsync);
#endif
            emit stableVSyncChanged(vsync);
            m_stabilizer.reset();
        } else {
            m_stabilizer.lastTime = current;
        }
    }
}

void SwapListeningAnimationDriver::start()
{
    if (!m_stabilizer.initialized)
        startListening();

    QAnimationDriver::start();
    m_currentTime = 0;
    m_timer.restart();
}

qint64 SwapListeningAnimationDriver::elapsed() const
{
    if (!isRunning() || m_stableVsync <= 0)
        return m_timer.elapsed();
    else
        return m_currentTime;
}

void SwapListeningAnimationDriver::advance()
{
    if (m_stableVsync > 0) {
        m_currentTime += m_stableVsync;
        if (m_currentTime + m_stableVsync < m_timer.elapsed()) {
//            qDebug(" --- Swap Animation Driver: compensated animationTime=%d, actualTime=%d",
//                   (int) m_currentTime,
//                   (int) m_timer.elapsed());
            m_currentTime = qFloor((m_timer.elapsed() / m_stableVsync) + 1) * m_stableVsync;
        }
    } else {
        m_currentTime = m_timer.elapsed();
    }

    QAnimationDriver::advanceAnimation(m_currentTime);
}
