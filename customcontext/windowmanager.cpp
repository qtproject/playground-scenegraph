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


int get_env_int(const char *name, int defaultValue)
{
    QByteArray content = qgetenv(name);

    bool ok = false;
    int value = content.toInt(&ok);
    return ok ? value : defaultValue;
}

#define QQUICK_WINDOW_TIMING
#ifdef QQUICK_WINDOW_TIMING
static bool qquick_window_timing = !qgetenv("QML_WINDOW_TIMING").isEmpty();
static QTime threadTimer;
static int syncTime;
static int renderTime;
static int sinceLastTime;
#endif

extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

const QEvent::Type WM_Show              = QEvent::Type(QEvent::User + 1);
const QEvent::Type WM_Hide              = QEvent::Type(QEvent::User + 2);
const QEvent::Type WM_LockAndSync       = QEvent::Type(QEvent::User + 3);
const QEvent::Type WM_RequestSync       = QEvent::Type(QEvent::User + 4);
const QEvent::Type WM_RequestRepaint    = QEvent::Type(QEvent::User + 5);
const QEvent::Type WM_Resize            = QEvent::Type(QEvent::User + 6);
const QEvent::Type WM_TryRelease        = QEvent::Type(QEvent::User + 7);
const QEvent::Type WM_UpdateLater       = QEvent::Type(QEvent::User + 8);
const QEvent::Type WM_Grab              = QEvent::Type(QEvent::User + 9);
const QEvent::Type WM_AdvanceAnimations = QEvent::Type(QEvent::User + 11);

template <typename T> T *windowFor(const QList<T> list, QQuickWindow *window)
{
    for (int i=0; i<list.size(); ++i) {
        const T &t = list.at(i);
        if (t.window == window)
            return const_cast<T *>(&t);
    }
    return 0;
}


class WMWindowEvent : public QEvent
{
public:
    WMWindowEvent(QQuickWindow *c, QEvent::Type type) : QEvent(type), window(c) { }
    QQuickWindow *window;
};


class WMResizeEvent : public WMWindowEvent
{
public:
    WMResizeEvent(QQuickWindow *c, const QSize &s) : WMWindowEvent(c, WM_Resize), size(s) { }
    QSize size;
};


class WMShowEvent : public WMWindowEvent
{
public:
    WMShowEvent(QQuickWindow *c) : WMWindowEvent(c, WM_Show), size(c->size()) { }
    QSize size;
};


class WMGrabEvent : public WMWindowEvent
{
public:
    WMGrabEvent(QQuickWindow *c, QImage *result) : WMWindowEvent(c, WM_Grab), image(result) {}
    QImage *image;
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
        , pendingUpdate(0)
        , sleeping(false)
        , animationRunning(false)
        , guiIsLocked(false)
        , shouldExit(false)
        , allowMainThreadProcessing(true)
        , animationRequestsPending(0)
    {
        sg->moveToThread(this);
    }


    void invalidateOpenGL(QQuickWindow *window);
    void initializeOpenGL();

    bool event(QEvent *);
    void run();

    void syncAndRender();
    void sync();

    void requestRepaint()
    {
        if (sleeping)
            exit();
        if (m_windows.size() > 0)
            pendingUpdate |= RepaintRequest;
    }

public slots:
    void animationStarted() {
        WMDEBUG("    Render: animationStarted()");
        animationRunning = true;
        if (sleeping)
            exit();
    }

    void animationStopped() {
        WMDEBUG("    Render: animationStopped()");
        animationRunning = false;
    }

public:
    enum UpdateRequest {
        SyncRequest         = 0x01,
        RepaintRequest      = 0x02
    };

    WindowManager *wm;
    QOpenGLContext *gl;
    QSGContext *sg;

    QEventLoop eventLoop;

    uint pendingUpdate : 2;
    uint sleeping : 1;
    uint animationRunning : 1;

    volatile bool guiIsLocked;
    volatile bool shouldExit;

    volatile bool allowMainThreadProcessing;
    volatile int animationRequestsPending;

    QMutex mutex;
    QWaitCondition waitCondition;

    QElapsedTimer m_timer;

    struct Window {
        QQuickWindow *window;
        QSize size;
    };
    QList<Window> m_windows;
};

bool RenderThread::event(QEvent *e)
{
    switch ((int) e->type()) {

    case WM_Show: {
        WMDEBUG1("    Render: WM_Show");
        WMShowEvent *se = static_cast<WMShowEvent *>(e);

        if (windowFor(m_windows, se->window)) {
            WMDEBUG1("    Render:  - window already added...");
            return true;
        }

        Window window;
        window.window = se->window;
        window.size = se->size;
        m_windows << window;
        return true; }

    case WM_Hide: {
        WMDEBUG1("    Render: WM_Hide");
        WMWindowEvent *ce = static_cast<WMWindowEvent *>(e);
        for (int i=0; i<m_windows.size(); ++i) {
            if (m_windows.at(i).window == ce->window) {
                WMDEBUG1("    Render:  - removed one...");
                m_windows.removeAt(i);
                break;
            }
        }

        if (sleeping && m_windows.size())
            exit();

        return true; }

    case WM_RequestSync:
        WMDEBUG("    Render: WM_RequestSync");
        if (sleeping)
            exit();
        if (m_windows.size() > 0)
            pendingUpdate |= SyncRequest;
        return true;

    case WM_Resize: {
        WMDEBUG("    Render: WM_Resize");
        WMResizeEvent *re = static_cast<WMResizeEvent *>(e);
        Window *w = windowFor(m_windows, re->window);
        w->size = re->size;
        // No need to wake up here as we will get a sync shortly.. (see WindowManager::resize());
        return true; }

    case WM_TryRelease:
        WMDEBUG1("    Render: WM_TryRelease");
        mutex.lock();
        if (m_windows.size() == 0) {
            WMDEBUG1("    Render:  - setting exit flag and invalidating GL");
            shouldExit = true;
            invalidateOpenGL(static_cast<WMWindowEvent *>(e)->window);
            if (sleeping)
                exit();
        } else {
            WMDEBUG1("    Render:  - not releasing anything because we have active windows...");
        }
        waitCondition.wakeOne();
        mutex.unlock();
        return true;

    case WM_Grab: {
        WMDEBUG1("    Render: WM_Grab");
        WMGrabEvent *ce = static_cast<WMGrabEvent *>(e);
        Window *w = windowFor(m_windows, ce->window);
        if (w) {
            gl->makeCurrent(ce->window);
            QQuickWindowPrivate::get(ce->window)->renderSceneGraph(w->size);
            *ce->image = qt_gl_read_framebuffer(w->size, false, false);
        }
        WMDEBUG1("    Render:  - waking gui to handle grab result");
        mutex.lock();
        waitCondition.wakeOne();
        mutex.unlock();
        return true;
    }

    default:
        break;
    }
    return QThread::event(e);
}

void RenderThread::invalidateOpenGL(QQuickWindow *window)
{
    WMDEBUG1("    Render: invalidateOpenGL()");

    if (!gl)
        return;

    if (!window) {
        qWarning("WindowManager:RenderThread: no window to make current...");
        return;
    }

    // We're not doing any cleanup in this case...
    if (window->isPersistentSceneGraph()) {
        WMDEBUG1("    Render:  - persistent SG, avoiding cleanup");
        return;
    }

    gl->makeCurrent(window);

    QQuickWindowPrivate *dd = QQuickWindowPrivate::get(window);
    dd->cleanupNodesOnShutdown();

    sg->invalidate();
    gl->doneCurrent();
    WMDEBUG1("    Render:  - invalidated scenegraph..");

    if (!window->isPersistentOpenGLContext()) {
        delete gl;
        gl = 0;
        WMDEBUG1("    Render:  - invalidated OpenGL");
    } else {
        WMDEBUG1("    Render:  - persistent GL, avoiding cleanup");
    }
}

void RenderThread::initializeOpenGL()
{
    WMDEBUG1("    Render: initializeOpenGL()");
    QWindow *win = m_windows.at(0).window;
    bool temp = false;

    // Workaround for broken expose logic... We should not get an
    // expose when the size of a window is invalid, but we sometimes do.
    // On Mac this leads to harmless, yet annoying, console warnings
    if (m_windows.at(0).size.isEmpty()) {
        temp = true;
        win = new QWindow();
        win->setFormat(m_windows.at(0).window->requestedFormat());
        win->setSurfaceType(QWindow::OpenGLSurface);
        win->setGeometry(0, 0, 64, 64);
        win->create();
    }

    gl = new QOpenGLContext();
    // Pick up the surface format from one of them
    gl->setFormat(win->requestedFormat());
    gl->create();
    if (!gl->makeCurrent(win))
        qWarning("QQuickWindow: makeCurrent() failed...");
    sg->initialize(gl);

    if (temp) {
        delete win;
    }
}

/*!
 * Makes an attemt to lock the GUI thread so we can perform the synchronizaiton
 * of the QML scene's state into the scene graph.
 *
 * We return the number of views that did not trigger any changes.
 */
void RenderThread::sync()
{
    WMDEBUG("    Render: sync()");
    mutex.lock();

    Q_ASSERT_X(guiIsLocked, "RenderThread::sync()", "sync triggered on bad terms as gui is not already locked...");
    pendingUpdate = 0;

    for (int i=0; i<m_windows.size(); ++i) {
        Window &w = const_cast<Window &>(m_windows.at(i));
        if (w.size.width() == 0 || w.size.height() == 0) {
            WMDEBUG("    Render:  - window has bad size, waiting...");
            continue;
        }
        gl->makeCurrent(w.window);
        QQuickWindowPrivate *d = QQuickWindowPrivate::get(w.window);
        d->syncSceneGraph();
    }

    WMDEBUG("    Render:  - unlocking after sync");

    waitCondition.wakeOne();
    mutex.unlock();
}


void RenderThread::syncAndRender()
{
#ifdef QQUICK_WINDOW_TIMING
    if (qquick_window_timing)
        sinceLastTime = threadTimer.restart();
#endif
    WMDEBUG("    Render: syncAndRender()");

    // This animate request will get there after the sync
    if (animationRunning && animationRequestsPending < 2) {
        WMDEBUG("    Render:  - posting animate to gui..");
        ++animationRequestsPending;
        QCoreApplication::postEvent(wm, new QEvent(WM_AdvanceAnimations));

    }

    if (pendingUpdate & SyncRequest) {
        WMDEBUG("    Render:  - update pending, doing sync");
        sync();
    }

#ifdef QQUICK_WINDOW_TIMING
    if (qquick_window_timing)
        syncTime = threadTimer.elapsed();
#endif

    for (int i=0; i<m_windows.size(); ++i) {
        Window &w = const_cast<Window &>(m_windows.at(i));
        QQuickWindowPrivate *d = QQuickWindowPrivate::get(w.window);
        if (!d->renderer || w.size.width() == 0 || w.size.height() == 0) {
            WMDEBUG("    Render:  - Window not yet ready, skipping render...");
            continue;
        }
        gl->makeCurrent(w.window);
        d->renderSceneGraph(w.size);
#ifdef QQUICK_WINDOW_TIMING
        if (qquick_window_timing && i == 0)
            renderTime = threadTimer.elapsed();
#endif
        gl->swapBuffers(w.window);
        d->fireFrameSwapped();
    }
    WMDEBUG("    Render:  - rendering done");

#ifdef QQUICK_WINDOW_TIMING
        if (qquick_window_timing)
            qDebug("window Time: sinceLast=%d, sync=%d, first render=%d, after final swap=%d",
                   sinceLastTime,
                   syncTime,
                   renderTime - syncTime,
                   threadTimer.elapsed() - renderTime);
#endif
}

void RenderThread::run()
{
    WMDEBUG1("    Render: run()");
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
            && ((!animationRunning && pendingUpdate == 0) || m_windows.size() == 0)) {
            WMDEBUG("    Render: enter event loop (going to sleep)");
            sleeping = true;
            exec();
            sleeping = false;
        }

    }

    Q_ASSERT_X(!gl, "RenderThread::run()", "The OpenGL context should be cleaned up before exiting the render thread...");

    WMDEBUG1("    Render: rund() completed...");
}

WindowManager::WindowManager()
    : m_animation_timer(0)
    , m_update_timer(0)
{
    m_thread = new RenderThread(this);
    m_thread->moveToThread(m_thread);

    m_animation_driver = m_thread->sg->createAnimationDriver(this);

    m_exhaust_delay = get_env_int("QML_EXHAUST_DELAY", 5);

    connect(m_animation_driver, SIGNAL(started()), m_thread, SLOT(animationStarted()));
    connect(m_animation_driver, SIGNAL(stopped()), m_thread, SLOT(animationStopped()));
    connect(m_animation_driver, SIGNAL(started()), this, SLOT(animationStarted()));
    connect(m_animation_driver, SIGNAL(stopped()), this, SLOT(animationStopped()));

    m_animation_driver->install();
    WMDEBUG1("GUI: WindowManager() created");
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
    WMDEBUG("GUI: animationStarted()");
    if (!anyoneShowing() && m_animation_timer == 0)
        m_animation_timer = startTimer(NON_VISUAL_TIMER_INTERVAL);
}

void WindowManager::animationStopped()
{
    WMDEBUG("GUI: animationStopped()");
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

    releaseResources();

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

    hide(window);

    WMDEBUG1("GUI:  - done with windowDestroyed()");
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

    // Because we are going to bind a GL context to it, make sure it
    // is created.
    if (!window->handle())
        window->create();

    QCoreApplication::postEvent(m_thread, new WMShowEvent(window));

    // Start render thread if it is not running
    if (!m_thread->isRunning()) {
        m_thread->shouldExit = false;
        m_thread->animationRunning = m_animation_driver->isRunning();

        WMDEBUG1("GUI: - starting render thread...");
        m_thread->start();

    } else {
        WMDEBUG1("GUI: - render thread already running");
    }

    polishAndSync();

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
 */
void WindowManager::handleObscurity(QQuickWindow *window)
{
    WMDEBUG1("GUI: handleObscurity");
    if (m_thread->isRunning())
        QCoreApplication::postEvent(m_thread, new WMWindowEvent(window, WM_Hide));

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
    Q_ASSERT_X(QThread::currentThread() == QCoreApplication::instance()->thread() || m_thread->guiIsLocked,
               "QQuickItem::update()",
               "Function can only be called from GUI thread or during QQuickItem::updatePaintNode()");

    WMDEBUG("GUI: maybeUpdate...");
    Window *w = windowFor(m_windows, window);
    if (!w || w->pendingUpdate || !m_thread->isRunning()) {
        return;
    }

    // Call this function from the Gui thread later as startTimer cannot be
    // called from the render thread.
    if (QThread::currentThread() == m_thread) {
        WMDEBUG("GUI:  - on render thread, posting update later");
        QCoreApplication::postEvent(this, new WMWindowEvent(window, WM_UpdateLater));
        return;
    }


    w->pendingUpdate = true;

    if (m_update_timer > 0) {
        return;
    }

    WMDEBUG("GUI:  - posting update");
    m_update_timer = startTimer(m_exhaust_delay, Qt::PreciseTimer);
}

/*!
 * Called when the QQuickWindow should be repainted..
 */
void WindowManager::update(QQuickWindow *window)
{
    if (QThread::currentThread() == m_thread) {
        WMDEBUG("Gui: update called on render thread");
        m_thread->requestRepaint();
        return;
    }

    WMDEBUG("Gui: update called");
    maybeUpdate(window);
}


volatile bool *WindowManager::allowMainThreadProcessing()
{
    return &m_thread->allowMainThreadProcessing;
}


/*!
 * Release resources will post an event to the render thread to
 * free up the SG and GL resources and exists the render thread.
 */
void WindowManager::releaseResources()
{
    WMDEBUG1("GUI: releaseResources requested...");

    m_thread->mutex.lock();
    if (m_thread->isRunning() && m_windows.size() && !m_thread->shouldExit) {
        WMDEBUG1("GUI:  - posting release request to render thread");
        QCoreApplication::postEvent(m_thread, new WMWindowEvent(m_windows.at(0).window, WM_TryRelease));
        m_thread->waitCondition.wait(&m_thread->mutex);
    }
    m_thread->mutex.unlock();
}

void WindowManager::polishAndSync()
{
    if (!anyoneShowing())
        return;

#ifdef QQUICK_WINDOW_TIMING
    QElapsedTimer timer;
    int polishTime;
    int waitTime;
    if (qquick_window_timing)
        timer.start();
#endif
    WMDEBUG("GUI: polishAndSync()");
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

    WMDEBUG("GUI:  - clearing update flags...");
    for (int i=0; i<m_windows.size(); ++i) {
        m_windows[i].pendingUpdate = false;
    }

    WMDEBUG("GUI:  - lock for sync...");
    m_thread->mutex.lock();
    m_thread->guiIsLocked = true;
    QEvent *event = new QEvent(WM_RequestSync);

    QCoreApplication::postEvent(m_thread, event);
    WMDEBUG("GUI:  - wait for sync...");
#ifdef QQUICK_WINDOW_TIMING
    if (qquick_window_timing)
        waitTime = timer.elapsed();
#endif
    m_thread->waitCondition.wait(&m_thread->mutex);
    m_thread->guiIsLocked = false;
    m_thread->mutex.unlock();
    WMDEBUG("GUI:  - unlocked after sync...");

#ifdef QQUICK_WINDOW_TIMING
    if (qquick_window_timing)
        qDebug(" - polish=%d, wait=%d, sync=%d", polishTime, waitTime - polishTime, int(timer.elapsed() - waitTime));
#endif
}

bool WindowManager::event(QEvent *e)
{
    switch ((int) e->type()) {

    case QEvent::Timer:
        if (static_cast<QTimerEvent *>(e)->timerId() == m_animation_timer) {
            WMDEBUG("Gui: QEvent::Timer -> non-visual animation");
            m_animation_driver->advance();
        } else if (static_cast<QTimerEvent *>(e)->timerId() == m_update_timer) {
            WMDEBUG("Gui: QEvent::Timer -> polishAndSync()");
            killTimer(m_update_timer);
            m_update_timer = 0;
            polishAndSync();
        }
        return true;

    case WM_UpdateLater: {
        QQuickWindow *window = static_cast<WMWindowEvent *>(e)->window;
        // The window might have gone away...
        if (windowFor(m_windows, window))
            maybeUpdate(window);
        return true; }

    case WM_AdvanceAnimations:
        --m_thread->animationRequestsPending;
        WMDEBUG("GUI: WM_AdvanceAnimations");
        if (m_animation_driver->isRunning()) {
#ifdef QQUICK_CANVAS_TIMING
            QElapsedTimer timer;
            timer.start();
#endif
            m_animation_driver->advance();
            WMDEBUG("GUI:  - animations advanced..");
#ifdef QQUICK_CANVAS_TIMING
            if (qquick_canvas_timing)
                qDebug(" - animation: %d", (int) timer.elapsed());
#endif
        }
        return true;

    default:
        break;
    }

    return QObject::event(e);
}


QImage WindowManager::grab(QQuickWindow *window)
{
    WMDEBUG1("GUI: grab");
    if (!m_thread->isRunning())
        return QImage();

    if (!window->handle())
        window->create();

    QImage result;
    m_thread->mutex.lock();
    QCoreApplication::postEvent(m_thread, new WMGrabEvent(window, &result));
    m_thread->waitCondition.wait(&m_thread->mutex);
    m_thread->mutex.unlock();

    return result;
}

void WindowManager::wakeup()
{
}

void WindowManager::resize(QQuickWindow *w, const QSize &size)
{
    WMDEBUG1("GUI: resize");

    if (!m_thread->isRunning() || !m_windows.size() || !w->isExposed() || windowFor(m_windows, w) == 0) {
        return;
    }

    if (size.width() == 0 || size.height() == 0)
        return;

    WMDEBUG("GUI:  - posting resize event...");
    WMResizeEvent *e = new WMResizeEvent(w, size);
    QCoreApplication::postEvent(m_thread, e);

    polishAndSync();
}

#include "windowmanager.moc"
