/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qsganimatorcontroller.h"
#include "qsganimatoritem.h"
#include "qsganimatedtranslate.h"
#include "qsganimatedscale.h"
#include "qsganimatedrotation.h"
#include "qsgpropertyanimator.h"
#include "qsgsequentialanimator.h"
#include "qsgparallelanimator.h"
#include "qsgpauseanimator.h"

#include "qsgpropertyanimation.h"
#include "qsgsequentialanimation.h"
#include "qsgparallelanimation.h"
#include "qsgpauseanimation.h"
#include <QtQuick>
#include <QtQuick/private/qquickitem_p.h>

float get_env_float(const char *name, float defaultValue)
{
    QByteArray content = qgetenv(name);
    bool ok = false;
    float value = content.toFloat(&ok);
    return ok ? value : defaultValue;
}

QSGAnimatorController::QSGAnimatorController(QQuickItem *item)
    : m_item(item)
    , m_initialized(false)
    , m_frameCounter(0)
    , m_stableVsync(0)
    , m_currentAnimationTime(0.0)
    , m_currentAnimationDelay(0)
    , m_currentAnimationCatchup(0)
    , m_thresholdForCatchup(250.0)
    , m_catchupRatio(0.05)

{
    m_thresholdForCatchup = get_env_float("QML_ANIMATION_DRIVER_CATCHUP_THRESHOLD", 250);
    m_catchupRatio = get_env_float("QML_ANIMATION_DRIVER_CATCHUP_RATIO", 0.05);

#ifdef ANIMATORS_DEBUG
    qDebug() << "QSGAnimatorController::QSGAnimatorController() VSYNC: " << m_stableVsync;
#endif

    connect(item->window(), SIGNAL(beforeRendering()), SLOT(advance()),Qt::DirectConnection);
}

QSGAnimatorController::~QSGAnimatorController()
{
    int tc = m_transform.count();
    for (int i = 0; i < tc; i++)
        delete m_transform.at(i);

    QList<QSGAnimatedProperty*> properties = m_registeredProperties.values();
    int pc = properties.count();
    for (int i = 0; i < pc; i++) {
        delete properties.at(i);
    }
}

bool QSGAnimatorController::isInitialized()
{
    return m_initialized;
}

bool QSGAnimatorController::isUpdating()
{
    return m_topLevelAnimator.isUpdating();
}

void QSGAnimatorController::advance()
{
    qreal previousTime = m_currentAnimationTime;
    qint64 t = m_timer.elapsed();

    if (m_frameCounter == 0) {
        t = 0;
        m_timer.restart();
        m_currentAnimationTime = 0.0;
        previousTime = 0.0;
        m_frameCounter++;
        m_currentAnimationDelay = 0.0;
        m_currentAnimationCatchup = 0.0;
    }

    if (m_topLevelAnimator.isUpdating()) {
        if (m_stableVsync > 0) {
            m_currentAnimationTime += (m_stableVsync + m_currentAnimationCatchup);
            m_currentAnimationDelay -= m_currentAnimationCatchup;

            if (m_currentAnimationDelay < m_currentAnimationCatchup * 0.5) {
                m_currentAnimationDelay = 0.0;
                m_currentAnimationCatchup = 0.0;
            }

            if (m_currentAnimationTime < t) {
                m_currentAnimationDelay = t - m_currentAnimationTime;
                m_currentAnimationTime = qFloor((t / m_stableVsync) + 1) * m_stableVsync;

                if (m_currentAnimationDelay > m_thresholdForCatchup) {
                    m_currentAnimationTime += m_currentAnimationDelay;
                    m_currentAnimationDelay = 0.0;
                }
                m_currentAnimationCatchup = m_currentAnimationDelay * m_catchupRatio;
            }

        } else {
            m_currentAnimationTime = t;
        }

        m_topLevelAnimator.advance(m_currentAnimationTime - previousTime);
    } else {
        m_frameCounter = 0;
    }
}

void QSGAnimatorController::sync()
{
    if (!m_initialized) {
        createProperties();
        m_initialized = true;

        if (m_topLevelAnimator.isUpdating())
            m_item->update();

        return;
    }

    // How to handle reading back property values from qml...
    // for now just avoid it when *any* animator is active.
    bool syncProperties = !m_topLevelAnimator.isActive();

    m_topLevelAnimator.sync(false, 0.0);

    if (syncProperties) {
        QList<QSGAnimatedProperty*> properties = m_registeredProperties.values();
        for (int i = 0; i < properties.count(); i++)
            properties.at(i)->sync();
    }

    if (m_topLevelAnimator.isUpdating())
        m_item->update();
}

void QSGAnimatorController::registerProperty(QSGAnimatedProperty *p)
{
    QString key = QString::number((qint64)p->qmlObject()) + "_" + p->name();

    if (!m_registeredProperties.contains(key))
         m_registeredProperties.insert(key, p);

}

void QSGAnimatorController::unregisterProperty(QSGAnimatedProperty *p)
{
    QString key = QString::number((quint64)p->qmlObject()) + "_" + p->name();
    m_registeredProperties.remove(key);
}

QSGAnimatedProperty *QSGAnimatorController::registeredProperty(QString name, QObject *o)
{
    QObject* qmlObject = 0;

    if (o)
        qmlObject = o;
    else
        qmlObject = m_item;

    QString key = QString::number((quint64)qmlObject) + "_" + name;
    return m_registeredProperties.value(key);
}

QMatrix4x4 QSGAnimatorController::transformMatrix()
{
    QMatrix4x4 m;
    int count = m_transform.count();
    for (int i = 0; i < count; i++) {
        m_transform.at(i)->applyTo(m);
    }
    return m;
}

void QSGAnimatorController::createProperties()
{
    int propertyCount = m_item->metaObject()->propertyCount();
    for (int i = 0; i < propertyCount; i++) {
        QString name = m_item->metaObject()->property(i).name();
        QVariant v = m_item->property(name.toLatin1().constData());
        QSGAnimatedProperty *p = new QSGAnimatedProperty(m_item, name, v);
        registerProperty(p);
    }

    QQuickItemPrivate *o = QQuickItemPrivate::get(m_item);

    if (!o)
        return;

    QQmlListProperty<QQuickTransform> list = m_item->transform();
    int tc = o->transform_count(&list);
    for (int i = 0; i < tc; i++) {
        QQuickTransform *t = o->transform_at(&list, i);

        // Not possible to qobject_cast QQuickTransform (not exported), so we use metaobject.
        QString n = QString(t->metaObject()->className());
        if (n == "QQuickTranslate") {
            QSGAnimatedTranslate *tr = new QSGAnimatedTranslate(this, t);
            m_transform.append(tr);
        }
        else if (n == "QQuickScale") {
            QSGAnimatedScale *tr = new QSGAnimatedScale(this, t);
            m_transform.append(tr);
        }
        else if (n == "QQuickRotation") {
            QSGAnimatedRotation *tr = new QSGAnimatedRotation(this, t);
            m_transform.append(tr);
        }
    }
}

void QSGAnimatorController::registerAnimation(QSGAbstractAnimation *animation)
{
    qreal duration = createAnimators(&m_topLevelAnimator, animation, false, 0.0);
    m_topLevelAnimator.setDuration(duration);
}

qreal QSGAnimatorController::createAnimators(QSGAbstractAnimator *controller, QObject *o, bool topLevelRunning, qreal startTime)
{
    if (!o)
        return 0.0;

    QString n = QString(o->metaObject()->className());
    qreal duration = 0.0;

    if (n == "QSGPropertyAnimation" || n == "QSGNumberAnimation" || n == "QSGColorAnimation" || n == "QSGVector3DAnimation") {
        QSGPropertyAnimation *propertyAnimation = qobject_cast<QSGPropertyAnimation*> (o);
        QSGPropertyAnimator *animator = new QSGPropertyAnimator(this, controller, propertyAnimation, startTime);
        animator->setRunning(animator->running() || topLevelRunning);
        duration += propertyAnimation->duration();
        duration *= propertyAnimation->loops();
        QObject::connect(propertyAnimation, SIGNAL(runningChanged(bool)), m_item, SLOT(update()));
        QObject::connect(propertyAnimation, SIGNAL(pausedChanged(bool)), m_item, SLOT(update()));
    } else if (n == "QSGSequentialAnimation") {
        QSGAbstractAnimation *sequentialAnimation = qobject_cast<QSGAbstractAnimation*> (o);
        QSGSequentialAnimator *animator = new QSGSequentialAnimator(this, controller, sequentialAnimation, startTime);
        animator->setRunning(animator->running() || topLevelRunning);
        int count = sequentialAnimation->children().count();
        for (int i = 0; i < count; i++) {
            duration += createAnimators(animator, sequentialAnimation->children().at(i), sequentialAnimation->isRunning() || topLevelRunning, startTime + duration);
        }
        animator->setDuration(duration);
        duration *= sequentialAnimation->loops();
        QObject::connect(sequentialAnimation, SIGNAL(runningChanged(bool)), m_item, SLOT(update()));
        QObject::connect(sequentialAnimation, SIGNAL(pausedChanged(bool)), m_item, SLOT(update()));
    } else if (n == "QSGParallelAnimation") {
        QSGParallelAnimation *parallelAnimation = qobject_cast<QSGParallelAnimation*> (o);
        QSGParallelAnimator *animator = new QSGParallelAnimator(this, controller, parallelAnimation, startTime);
        animator->setRunning(animator->running() || topLevelRunning);
        int count = parallelAnimation->children().count();
        for (int i = 0; i < count; i++) {
            qreal childAnimationDuration = createAnimators(animator, parallelAnimation->children().at(i), parallelAnimation->isRunning() || topLevelRunning, startTime);
            duration = qMax(duration, childAnimationDuration);
        }
        animator->setDuration(duration);
        duration *= parallelAnimation->loops();
        QObject::connect(parallelAnimation, SIGNAL(runningChanged(bool)), m_item, SLOT(update()));
        QObject::connect(parallelAnimation, SIGNAL(pausedChanged(bool)), m_item, SLOT(update()));
    } else if (n == "QSGPauseAnimation") {
        QSGPauseAnimation *pauseAnimation = qobject_cast<QSGPauseAnimation*> (o);
        QSGPauseAnimator *animator = new QSGPauseAnimator(this, controller, pauseAnimation, startTime);
        animator->setRunning(animator->running() || topLevelRunning);
        duration = pauseAnimation->duration();
        duration *= pauseAnimation->loops();
        QObject::connect(pauseAnimation, SIGNAL(runningChanged(bool)), m_item, SLOT(update()));
        QObject::connect(pauseAnimation, SIGNAL(pausedChanged(bool)), m_item, SLOT(update()));
    }
    return duration;
}

QQuickItem* QSGAnimatorController::item()
{
    return m_item;
}
