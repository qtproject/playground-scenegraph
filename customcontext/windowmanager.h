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

#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QtCore/QThread>
#include <QtGui/QOpenGLContext>
#include <private/qsgcontext_p.h>

#include <private/qquickwindowmanager_p.h>

class RenderThread;

class WindowManager : public QObject, public QQuickWindowManager
{
    Q_OBJECT
public:
    WindowManager();

    void show(QQuickWindow *window);
    void hide(QQuickWindow *window);

    void windowDestroyed(QQuickWindow *window);
    void exposureChanged(QQuickWindow *window);

    void handleExposure(QQuickWindow *window);
    void handleObscurity(QQuickWindow *window);

    QImage grab(QQuickWindow *);

    void resize(QQuickWindow *, const QSize &);

    void update(QQuickWindow *window);
    void maybeUpdate(QQuickWindow *window);
    volatile bool *allowMainThreadProcessing();
    QSGContext *sceneGraphContext() const;

    QAnimationDriver *animationDriver() const;

    void releaseResources();

    bool event(QEvent *);

    void wakeup();

public slots:
    void animationStarted();
    void animationStopped();

private:
    friend class RenderThread;

    bool checkAndResetForceUpdate(QQuickWindow *window);

    bool anyoneShowing();
    void initialize();

    void waitForReleaseComplete();

    void polishAndSync();

    struct Window {
        QQuickWindow *window;
        uint pendingUpdate : 1;
    };

    RenderThread *m_thread;
    QAnimationDriver *m_animation_driver;
    QList<Window> m_windows;

    int m_animation_timer;
    int m_update_timer;
    int m_exhaust_delay;
};

#endif // WINDOWMANAGER_H
