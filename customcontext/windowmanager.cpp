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

#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QAnimationDriver>

#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

#include <QtQuick/QQuickWindow>
#include <private/qquickwindow_p.h>

#include <QtQuick/private/qsgrenderer_p.h>

#include "windowmanager.h"

#define NON_VISUAL_TIMER_INTERVAL int(1000 / QGuiApplication::primaryScreen()->refreshRate())

#ifdef CUSTOMCONTEXT_DEBUG
#define WM_DEBUG_LEVEL_1
#endif
//#define WM_DEBUG_LEVEL_2

#if defined (WM_DEBUG_LEVEL_2)
#  define WMDEBUG1(x) qDebug("%s : %4d - %s", __FILE__, __LINE__, x);
#  define WMDEBUG(x) qDebug("%s : %4d - %s", __FILE__, __LINE__, x);
#elif defined (WM_DEBUG_LEVEL_1)
#  define WMDEBUG1(x) qDebug("%s : %4d - %s", __FILE__, __LINE__, x);
#  define WMDEBUG(x)
#else
#  define WMDEBUG1(x)
#  define WMDEBUG(x)
#endif

#define QQUICK_WINDOW_TIMING
#ifdef QQUICK_WINDOW_TIMING
static bool qquick_window_timing = !qgetenv("QML_WINDOW_TIMING").isEmpty();
static QTime threadTimer;
static int syncTime;
static int renderTime;
static int sinceLastTime;
#endif

bool WindowManager::fakeRendering = false;

extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

const QEvent::Type WM_Show              = QEvent::Type(QEvent::User + 1);
const QEvent::Type WM_Hide              = QEvent::Type(QEvent::User + 2);
const QEvent::Type WM_SyncAndAdvance    = QEvent::Type(QEvent::User + 3);
const QEvent::Type WM_RequestSync       = QEvent::Type(QEvent::User + 4);
const QEvent::Type WM_NotifyDoneRender  = QEvent::Type(QEvent::User + 5);
const QEvent::Type WM_TryRelease        = QEvent::Type(QEvent::User + 6);
const QEvent::Type WM_ReleaseHandled    = QEvent::Type(QEvent::User + 7);
const QEvent::Type WM_Grab              = QEvent::Type(QEvent::User + 8);
const QEvent::Type WM_GrabResult        = QEvent::Type(QEvent::User + 9);
const QEvent::Type WM_EnterWait         = QEvent::Type(QEvent::User + 10);
const QEvent::Type WM_Polish            = QEvent::Type(QEvent::User + 11);

template <typename T> T *windowFor(const QList<T> list, QQuickWindow *window)
{
    for (int i=0; i<list.size(); ++i) {
        const T &t = list.at(i);
        if (t.window == window)
            return const_cast<T *>(&t);
    }
    return 0;
}


class WMwindowEvent : public QEvent
{
public:
    WMwindowEvent(QQuickWindow *c, QEvent::Type type) : QEvent(type), window(c) { }
    QQuickWindow *window;
};


class WMShowEvent : public WMwindowEvent
{
public:
    WMShowEvent(QQuickWindow *c) : WMwindowEvent(c, WM_Show), size(c->size()) { }
    QSize size;
};


class WMGrabResultEvent : public WMwindowEvent
{
public:
    WMGrabResultEvent(QQuickWindow *window, const QImage &r) : WMwindowEvent(window, WM_GrabResult), result(r) { }
    QImage result;
};

/*!
 * A note on wayland: Wayland receives buffer_release on the Main thread
 * while swapBuffers will block until it gets a new buffer. The render
 * thread would for this reason be very deadlock prone but contains a
 * number of workarounds and the two threads will never lock each other,
 * except for the short duration during the "sync" which is deadlock safe.
 */

class RenderThread : public QThread
{
    Q_OBJECT
public:
    RenderThread(WindowManager *w)
        : wm(w)
        , gl(0)
        , sg(QSGContext::createDefaultContext())
        , pendingUpdate(false)
        , sleeping(false)
        , animationRunning(false)
        , sceneChanged(false)
        , shouldExit(false)
        , allowMainThreadProcessing(true)
        , inSync(true)
    {
        sg->moveToThread(this);
    }

    void invalidateOpenGL(QQuickWindow *window);
    void initializeOpenGL();

    bool event(QEvent *);
    void run();

    void syncAndRender();

    void lockGuiFromRender(QEvent::Type eventType)
    {
        mutex.lock();
        allowMainThreadProcessing = false;
        QCoreApplication::postEvent(wm, new QEvent(eventType));
        waitCondition.wait(&mutex);
    }

    void unlockGuiFromRender()
    {
        waitCondition.wakeOne();
        allowMainThreadProcessing = true;
        mutex.unlock();
    }

public slots:
    void animationStarted() {
        WMDEBUG("    Render: animations started...");
        animationRunning = true;
        if (sleeping)
            exit();
    }

    void animationStopped() {
        WMDEBUG("    Render: animations stopped...");
        animationRunning = false;
    }

    void sceneWasChanged() {
        sceneChanged = true;
    }

public:
    WindowManager *wm;
    QOpenGLContext *gl;
    QSGContext *sg;

    QEventLoop eventLoop;

    uint pendingUpdate : 1;
    uint sleeping : 1;
    uint animationRunning : 1;
    uint sceneChanged : 1;

    volatile bool shouldExit;

    volatile bool allowMainThreadProcessing;
    volatile bool inSync : 1;

    QMutex mutex;
    QWaitCondition waitCondition;

    struct Window {
        QQuickWindow *window;
        QSize size;
        uint sceneChanged : 1;
    };
    QList<Window> m_windows;
};

bool RenderThread::event(QEvent *e)
{
    switch ((int) e->type()) {

    case WM_Show: {
        WMDEBUG1("    Render: got show event");
        WMShowEvent *se = static_cast<WMShowEvent *>(e);

        if (windowFor(m_windows, se->window)) {
            WMDEBUG1("    Render: window already added...");
            return true;
        }

        Window window;
        window.window = se->window;
        window.size = se->size;
        m_windows << window;
        pendingUpdate = true;
        if (sleeping)
            exit();
        return true; }

    case WM_Hide: {
        WMDEBUG1("    Render: got hide..");
        WMwindowEvent *ce = static_cast<WMwindowEvent *>(e);
        for (int i=0; i<m_windows.size(); ++i) {
            if (m_windows.at(i).window == ce->window) {
                WMDEBUG1("    Render: removed one...");
                m_windows.removeAt(i);
                break;
            }
        }

        if (sleeping && m_windows.size())
            exit();

        return true; }

    case WM_RequestSync:
        WMDEBUG("    Render: got sync request event");
        if (sleeping)
            exit();
        pendingUpdate = true;
        return true;

    case WM_NotifyDoneRender:
        WMDEBUG1("    Render: got notify done render");
        // By the time the notify render is handled, we are done rendering so
        // we just need to ping-pong the event back to the wm...
        QCoreApplication::postEvent(wm, new QEvent(WM_NotifyDoneRender));
        return true;

    case WM_TryRelease:
        WMDEBUG1("    Render: got request to release resources");
        if (m_windows.size() == 0) {
            lockGuiFromRender(WM_EnterWait);
            WMDEBUG1("    Render: setting exit flag and invalidating GL");
            shouldExit = true;
            invalidateOpenGL(static_cast<WMwindowEvent *>(e)->window);
            if (sleeping)
                exit();
            unlockGuiFromRender();
        } else {
            WMDEBUG1("    Render: not releasing anything because we have active windows...");
        }
        QCoreApplication::postEvent(wm, new QEvent(WM_ReleaseHandled));
        return true;

    case WM_Grab: {
        WMDEBUG1("    Render: got grab event");
        WMwindowEvent *ce = static_cast<WMwindowEvent *>(e);
        Window *w = windowFor(m_windows, ce->window);
        QImage grabbed;
        if (w) {
            gl->makeCurrent(ce->window);
            QQuickWindowPrivate::get(ce->window)->renderSceneGraph(w->size);
            grabbed = qt_gl_read_framebuffer(w->size, false, false);
        }
        WMDEBUG1("    Render: posting grab result back to GUI");
        QCoreApplication::postEvent(wm, new WMGrabResultEvent(ce->window, grabbed));
        return true;
    }

    default:
        break;
    }
    return QThread::event(e);
}

void RenderThread::invalidateOpenGL(QQuickWindow *window)
{
    WMDEBUG1("    Render: trying to invalidate OpenGL");

    if (!gl)
        return;

    if (!window) {
        qWarning("WindowManager:RenderThread: no window to make current...");
        return;
    }

    // We're not doing any cleanup in this case...
    if (window->isPersistentSceneGraph()) {
        WMDEBUG1("    Render: persistent SG, avoiding cleanup");
        return;
    }

    gl->makeCurrent(window);

    QQuickWindowPrivate *dd = QQuickWindowPrivate::get(window);
    dd->cleanupNodesOnShutdown();

    sg->invalidate();
    gl->doneCurrent();
    WMDEBUG1("    Render: invalidated scenegraph..");

    if (!window->isPersistentOpenGLContext()) {
        delete gl;
        gl = 0;
        WMDEBUG1("    Render: invalidated OpenGL");
    } else {
        WMDEBUG1("    Render: persistent GL, avoiding cleanup");
    }
}

void RenderThread::initializeOpenGL()
{
    WMDEBUG1("    Render: initializing OpenGL");
    QQuickWindow *win = m_windows.at(0).window;

    gl = new QOpenGLContext();
    // Pick up the surface format from one of them
    gl->setFormat(win->requestedFormat());
    gl->create();
    if (!gl->makeCurrent(win))
        qWarning("QQuickWindow: makeCurrent() failed...");
    sg->initialize(gl);
}


void RenderThread::syncAndRender()
{
#ifdef QQUICK_WINDOW_TIMING
    if (qquick_window_timing)
        sinceLastTime = threadTimer.restart();
#endif

    WMDEBUG("    Render: about to lock");
    lockGuiFromRender(WM_SyncAndAdvance);
    pendingUpdate = false;
    inSync = true;
    WMDEBUG("    Render: performing sync");
    int skippedRenders = 0;
    for (int i=0; i<m_windows.size(); ++i) {
        Window &w = const_cast<Window &>(m_windows.at(i));
        gl->makeCurrent(w.window);
        QQuickWindowPrivate *d = QQuickWindowPrivate::get(w.window);

        bool hasRenderer = d->renderer != 0;
        sceneChanged = !hasRenderer || wm->checkAndResetForceUpdate(w.window);
        d->syncSceneGraph(); // May trigger sceneWasChanged...
        w.sceneChanged = sceneChanged;

        if (!sceneChanged)
            ++skippedRenders;

        // If there was no renderer before, this was the first render pass and
        // we register for the sceneGraphChanged signal..
        if (!hasRenderer)
            connect(d->renderer, SIGNAL(sceneGraphChanged()), this, SLOT(sceneWasChanged()));
    }

    // When we are skipping renders, animations we are not tracking vsync, so the
    // increment in the animation driver will be wrong, causing a drift over time.
    // Make this knowledge public so the animation driver can pick it up.
    WindowManager::fakeRendering = skippedRenders == m_windows.size();

    inSync = false;
    unlockGuiFromRender();
    WMDEBUG("    Render: sync done, on to rendering..");

#ifdef QQUICK_WINDOW_TIMING
    if (qquick_window_timing)
        syncTime = threadTimer.elapsed();
#endif

    for (int i=0; i<m_windows.size(); ++i) {
        Window &w = const_cast<Window &>(m_windows.at(i));
        QQuickWindowPrivate *d = QQuickWindowPrivate::get(w.window);
        if (w.sceneChanged) {
            gl->makeCurrent(w.window);
            d->renderSceneGraph(w.size);
#ifdef QQUICK_WINDOW_TIMING
            if (qquick_window_timing && i == 0)
                renderTime = threadTimer.elapsed();
#endif
            gl->swapBuffers(w.window);
            d->fireFrameSwapped();
        }
    }
    WMDEBUG("    Render: rendering done");

    if (WindowManager::fakeRendering)
        msleep(16);

#ifdef QQUICK_WINDOW_TIMING
        if (qquick_window_timing)
            qDebug("window Time: sinceLast=%d, sync=%d, render=%d, swap=%d%s",
                   sinceLastTime,
                   syncTime,
                   renderTime - syncTime,
                   threadTimer.elapsed() - renderTime,
                   WindowManager::fakeRendering ? ", fake frame" : "");
#endif
}

void RenderThread::run()
{
    WMDEBUG1("    Render: running...");
    while (!shouldExit) {

        // perform sync
        if (m_windows.size() > 0) {
            if (!gl)
                initializeOpenGL();
            if (!sg->isReady())
                sg->initialize(gl);
            syncAndRender();
        }

        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);

        if (!shouldExit
            && ((!animationRunning && !pendingUpdate) || m_windows.size() == 0)) {
            WMDEBUG("    Render: going to sleep...")
            sleeping = true;
            exec();
            sleeping = false;
        }

    }

    if (gl) {
        WMDEBUG1("    Render: we have a GL context when shutting down... eeek...");
    }

    WMDEBUG1("    Render: exited..");
}

WindowManager::WindowManager()
    : renderPassScheduled(false)
    , renderPassDone(false)
    , m_animation_timer(0)
    , m_releases_requested(0)
{
    m_thread = new RenderThread(this);
    m_thread->moveToThread(m_thread);

    m_animation_driver = m_thread->sg->createAnimationDriver(this);

    connect(m_animation_driver, SIGNAL(started()), m_thread, SLOT(animationStarted()));
    connect(m_animation_driver, SIGNAL(stopped()), m_thread, SLOT(animationStopped()));
    connect(m_animation_driver, SIGNAL(started()), this, SLOT(animationStarted()));
    connect(m_animation_driver, SIGNAL(stopped()), this, SLOT(animationStopped()));

    m_animation_driver->install();
    WMDEBUG1("GUI: window manager created...");
}

QSGContext *WindowManager::sceneGraphContext() const
{
    return m_thread->sg;
}

bool WindowManager::anyoneShowing()
{
    for (int i=0; i<m_windows.size(); ++i) {
        QQuickWindow *c = m_windows.at(i).window;
        if (c->isVisible() && c->isExposed())
            return true;
    }
    return false;
}

void WindowManager::animationStarted()
{
    WMDEBUG("GUI: animations started");
    if (!anyoneShowing() && m_animation_timer == 0)
        m_animation_timer = startTimer(NON_VISUAL_TIMER_INTERVAL);
}

void WindowManager::animationStopped()
{
    WMDEBUG("GUI: animations stopped");
    if (!anyoneShowing()) {
        killTimer(m_animation_timer);
        m_animation_timer = 0;
    }
}

/*!
 * Adds this window to the list of tracked windowes in this window manager. show() does
 * not trigger rendering to start, that happens in expose.
 */

void WindowManager::show(QQuickWindow *window)
{
    WMDEBUG1("GUI: show()");

    Window win;
    win.window = window;
    win.pendingUpdate = false;
    win.pendingExpose = false;
    win.pendingForceUpdate = true;
    m_windows << win;
}

/*!
 * Removes this window from the list of tracked windowes in this window manager. hide()
 * will trigger obscure, which in turn will stop rendering.
 */

void WindowManager::hide(QQuickWindow *window)
{
    WMDEBUG1("GUI: hide()");

    if (window->isExposed())
        handleObscurity(window);

    for (int i=0; i<m_windows.size(); ++i) {
        if (m_windows.at(i).window == window) {
            m_windows.removeAt(i);
            break;
        }
    }

}


/*!
 * Shutdown is a bit tricky as we would like to block the GUI with a clean mutex
 * until render thread has exited and cleaned up, but because wayland can enter
 * a deadlock when we block GUI while doing a swapBuffer in Render, we need
 * to do this rather ugly thing
 */
void WindowManager::windowDestroyed(QQuickWindow *window)
{
    WMDEBUG1("GUI: windowDestroyed()");
    if (!m_thread->isRunning())
        return;

    renderPassDone = false;

    handleObscurity(window);
    releaseResources();
    hide(window);
    waitForReleaseComplete();
    while (m_windows.size() == 0 && m_thread->isRunning()) {
        RenderThread::msleep(1);
        QCoreApplication::processEvents();
    }

    WMDEBUG1("GUI: done with windowDestroyed()");
}


void WindowManager::exposureChanged(QQuickWindow *window)
{
    WMDEBUG1("GUI: exposureChanged()");
    if (windowFor(m_windows, window) == 0)
        return;

    if (window->isExposed()) {
        handleExposure(window);
    } else {
        handleObscurity(window);
    }
}

/*!
 * Will post an event to the render thread that this window should
 * start to render.
 *
 * If we are waiting for a releaseResources, the render thread could be
 * in a state of shutting down, so we only register that this window has
 * an exposure change which we will replay later when the render thread
 * is in a more predictable state.
 */
void WindowManager::handleExposure(QQuickWindow *window)
{
    WMDEBUG1("GUI: handleExposure");
    if (m_releases_requested) {
        WMDEBUG1("GUI: --- using workaround");
        windowFor(m_windows, window)->pendingExpose = true;
        return;
    }

    // Because we are going to bind a GL context to it, make sure it
    // is created.
    if (!window->handle())
        window->create();

    QCoreApplication::postEvent(m_thread, new WMShowEvent(window));

    // If the render thread is about to shut down, wait for it to
    // finish before trying to restart it..
    while (m_thread->shouldExit && m_thread->isRunning())
        RenderThread::yieldCurrentThread();

    // Start render thread if it is not running
    if (!m_thread->isRunning()) {
        m_thread->shouldExit = false;
        m_thread->animationRunning = m_animation_driver->isRunning();

        WMDEBUG1("GUI: - starting render thread...");
        m_thread->start();

    } else {
        WMDEBUG1("GUI: - render thread already running");
    }

    // Kill non-visual animation timer if it is running
    if (m_animation_timer) {
        killTimer(m_animation_timer);
        m_animation_timer = 0;
    }

}

/*!
 * This function posts an event to the render thread to remove the window
 * from the list of windowses to render.
 *
 * It also starts up the non-vsync animation tick if no more windows
 * are showing.
 *
 * If we are waiting for a releaseResources, the render thread could be
 * in a state of shutting down, so we only register that this window has
 * an exposure change which we will replay later when the render thread
 * is in a more predictable state.
 */
void WindowManager::handleObscurity(QQuickWindow *window)
{
    WMDEBUG1("GUI: handleObscurity");
    if (m_releases_requested) {
        WMDEBUG1("GUI: --- using workaround");
        windowFor(m_windows, window)->pendingExpose = true;
        return;
    }

    if (m_thread->isRunning())
        QCoreApplication::postEvent(m_thread, new WMwindowEvent(window, WM_Hide));

    if (!anyoneShowing() && m_animation_driver->isRunning() && m_animation_timer == 0) {
        m_animation_timer = startTimer(NON_VISUAL_TIMER_INTERVAL);
    }
}


/*!
 * Called whenever the QML scene has changed. Will post an event
 * to the render thread that a sync and render is needed.
 */
void WindowManager::maybeUpdate(QQuickWindow *window)
{
    WMDEBUG("GUI: maybeUpdate called");
    if (m_thread->inSync) {
        m_thread->pendingUpdate = true;
        return;
    }

    Window *w = windowFor(m_windows, window);
    if (!w || w->pendingUpdate || !m_thread->isRunning())
        return;

    w->pendingUpdate = true;

    if (renderPassScheduled)
        return;
    renderPassScheduled = true;

    WMDEBUG("GUI: posting update");
    QCoreApplication::postEvent(m_thread, new QEvent(WM_RequestSync));
}

/*!
 * Called when the QQuickWindow should be repainted..
 */
void WindowManager::update(QQuickWindow *window)
{
    WMDEBUG("Gui: update called");
    maybeUpdate(window);
    Window *w = windowFor(m_windows, window);
    w->pendingForceUpdate = true;
}


/*!
 * Called from the render thread, while GUI is blocked to check if
 * there is a pending explicit update on this window.
 */
bool WindowManager::checkAndResetForceUpdate(QQuickWindow *window)
{
    Window *w = windowFor(m_windows, window);
    if (!w)
        return false;
    bool force = w->pendingForceUpdate;
    w->pendingForceUpdate = false;
    return force;
}


volatile bool *WindowManager::allowMainThreadProcessing()
{
    return &m_thread->allowMainThreadProcessing;
}


void WindowManager::waitForReleaseComplete()
{
    WMDEBUG1("GUI: waiting for release");
    while (m_releases_requested) {
        RenderThread::yieldCurrentThread();
        QCoreApplication::processEvents();
    }
    WMDEBUG1("GUI: done waiting for release");
}


void WindowManager::replayDelayedEvents()
{
    WMDEBUG1("GUI: replaying delayed events");
    for (int i=0; i<m_windows.size(); ++i) {
        Window &w = const_cast<Window &>(m_windows.at(i));
        if (w.window->isExposed())
            handleExposure(w.window);
        else
            handleObscurity(w.window);
        w.pendingExpose = false;
    }
    WMDEBUG1("GUI: done replaying delayed events");
}

/*!
 * Release resources will post an event to the render thread to
 * free up the SG and GL resources and exists the render thread.
 * It also increments the m_releases_requested, which is used to
 * to track if the render thread is in a state of "potentially
 * shutting down". When the release event has been handled in the
 * render thread, it posts a WM_ReleaseHandled back to GUI.
 * This counter is used in expose and obscure to decide if
 *  it is safe to start / stop the thread.
 */
void WindowManager::releaseResources()
{
    WMDEBUG1("GUI: releaseResources requested...");

    bool shuttingDown = false;
    m_thread->mutex.lock();
    shuttingDown = m_thread->shouldExit;
    m_thread->mutex.unlock();

    if (m_thread->isRunning() && m_windows.size() && !shuttingDown) {
        WMDEBUG1("GUI: posting release request to render thread");
        QCoreApplication::postEvent(m_thread, new WMwindowEvent(m_windows.at(0).window, WM_TryRelease));
        ++m_releases_requested;
    }
}

static void wakeAndWait(RenderThread *t)
{
    WMDEBUG("GUI: locking in gui");
    t->mutex.lock();
    t->waitCondition.wakeOne();
    WMDEBUG("GUI: woke up render, now waiting...");
    t->waitCondition.wait(&t->mutex);
    t->mutex.unlock();
}

bool WindowManager::event(QEvent *e)
{
    switch ((int) e->type()) {

    case QEvent::Timer:
        if (static_cast<QTimerEvent *>(e)->timerId() == m_animation_timer) {
            WMDEBUG("Gui: ticking non-visual animation");
            m_animation_driver->advance();
        }
        break;

    case WM_EnterWait:
        WMDEBUG("GUI: got request to enter wait");
        wakeAndWait(m_thread);
        break;

    case WM_SyncAndAdvance: {
#ifdef QQUICK_WINDOW_TIMING
        QElapsedTimer timer;
        int polishTime;
        if (qquick_window_timing)
            timer.start();
#endif
        WMDEBUG("GUI: got sync and animate request, polishing");
        // Polish as the last thing we do before we allow the sync to take place
        for (int i=0; i<m_windows.size(); ++i) {
            const Window &w = m_windows.at(i);
            QQuickWindowPrivate *d = QQuickWindowPrivate::get(w.window);
            d->polishItems();
        }
#ifdef QQUICK_WINDOW_TIMING
        if (qquick_window_timing)
            polishTime = timer.elapsed();
#endif
        wakeAndWait(m_thread);
        WMDEBUG("GUI: clearing update flags...");
        // The lock down the GUI thread and wake render for doing the sync
        renderPassScheduled = false;
        for (int i=0; i<m_windows.size(); ++i) {
            m_windows[i].pendingUpdate = false;
        }
        if (m_animation_driver->isRunning()) {
            m_animation_driver->advance();
            WMDEBUG("GUI: animations advanced..");
        }
#ifdef QQUICK_WINDOW_TIMING
        if (qquick_window_timing)
            qDebug(" - polish=%d, animation=%d", polishTime, (int) (timer.elapsed() - polishTime));
#endif
        WMDEBUG("GUI: sync and animate done...");
        return true;
    }

    case WM_NotifyDoneRender:
        WMDEBUG("GUI: render done notification...");
        renderPassDone = true;
        return true;

    case WM_GrabResult: {
        WMGrabResultEvent *ge = static_cast<WMGrabResultEvent *>(e);
        m_grab_results[ge->window] = ge->result;
        return true; }

    case WM_ReleaseHandled:
        m_releases_requested--;
        if (m_releases_requested == 0)
            replayDelayedEvents();
        break;

    default:
        break;
    }

    return QObject::event(e);
}


QImage WindowManager::grab(QQuickWindow *window)
{
    if (!m_thread->isRunning())
        return QImage();

    if (!window->handle())
        window->create();

    waitForReleaseComplete();

    QCoreApplication::postEvent(m_thread, new WMwindowEvent(window, WM_Grab));

    while (!m_grab_results.contains(window)) {
        RenderThread::msleep(10);
        QCoreApplication::processEvents();
    }

    QImage result = m_grab_results[window];
    m_grab_results.remove(window);
    return result;
}

void WindowManager::wakeup()
{
    if (!m_thread->isRunning() || !m_windows.size())
        return;

    maybeUpdate(m_windows.at(0).window);
}


#include "windowmanager.moc"
