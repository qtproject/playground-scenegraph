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

#ifndef QSGPROPERTYANIMATION_H
#define QSGPROPERTYANIMATION_H

#include "qsgabstractanimation.h"

class QSGPropertyAnimation : public QSGAbstractAnimation
{
    Q_OBJECT
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(QVariant from READ from WRITE setFrom NOTIFY fromChanged)
    Q_PROPERTY(QVariant to READ to WRITE setTo NOTIFY toChanged)
    Q_PROPERTY(QEasingCurve easing READ easing WRITE setEasing NOTIFY easingChanged)
    Q_PROPERTY(QObject* target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QString property READ property WRITE setProperty NOTIFY propertyChanged)
    Q_PROPERTY(QString properties READ properties WRITE setProperties NOTIFY propertiesChanged)

public:
    QSGPropertyAnimation(QObject *parent = 0);

    int duration();
    void setDuration(int);

    QVariant from();
    void setFrom(QVariant);

    QVariant to();
    void setTo(QVariant);

    const QEasingCurve& easing();
    void setEasing(const QEasingCurve&);

    QObject* target();
    void setTarget(QObject *);

    const QString& property();
    void setProperty(QString);

    const QString& properties();
    void setProperties(QString);

    virtual QAbstractAnimationJob* transition(QQuickStateActions &actions,
                            QQmlProperties &modified,
                            TransitionDirection direction,
                            QObject *defaultTarget = 0);

    void prepareTransition(QQuickStateActions &actions,
                           QQmlProperties &modified,
                           TransitionDirection direction,
                           QObject *defaultTarget);

public Q_SLOTS:
    virtual void complete();
    virtual void prepare(bool);

Q_SIGNALS:
    void durationChanged(int);
    void fromChanged(QVariant);
    void toChanged(QVariant);
    void easingChanged(const QEasingCurve &);
    void targetChanged();
    void propertyChanged();
    void propertiesChanged();

private:
    int m_duration;
    QVariant m_from;
    QVariant m_to;
    QEasingCurve m_easing;
    QObject *m_target;
    QString m_property;
    QString m_properties;
    Q_DISABLE_COPY(QSGPropertyAnimation)
};

#endif // QSGPROPERTYANIMATION_H
