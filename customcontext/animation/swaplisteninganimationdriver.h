#ifndef SWAPLISTENINGANIMATIONDRIVER_H
#define SWAPLISTENINGANIMATIONDRIVER_H

#include <QtCore/QAnimationDriver>
#include <QtCore/QElapsedTimer>

class SwapListeningAnimationDriver : public QAnimationDriver
{
    Q_OBJECT

public:
    SwapListeningAnimationDriver();

    void start();
    void advance();
    qint64 elapsed() const;

    void startListening();

public slots:
    void swapped();
    void updateStableVSync(qreal vsync) { m_stableVsync = vsync; }

signals:
    void stableVSyncChanged(qreal vsync);

private:
    QElapsedTimer m_timer;

    float m_stableVsync;
    float m_currentTime;

    struct Stabilizer {
        QElapsedTimer timer;
        bool initialized;
        int count;
        int requiredCount;
        int lastTime;
        int lowerBound;
        int upperBound;

        void reset();
    } m_stabilizer;
};

#endif // SWAPLISTENINGANIMATIONDRIVER_H
