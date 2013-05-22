/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of %MODULE%.
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

#ifndef QSGANIMATEDSCALE_H
#define QSGANIMATEDSCALE_H

#include <QtCore>
#include "qsganimatedtransform.h"

class QSGAnimatedProperty;

class QSGAnimatedScale : public QSGAnimatedTransform
{
public:
    QSGAnimatedScale(QSGAnimatorController *, QObject *);
    virtual void applyTo(QMatrix4x4 &);
    void setXScale(QVariant);
    void setYScale(QVariant);
    void setOrigin(QVariant);
    void setOriginX(QVariant);
    void setOriginY(QVariant);

private:
    QSGAnimatedProperty *m_originX;
    QSGAnimatedProperty *m_originY;
    QSGAnimatedProperty *m_yScale;
    QSGAnimatedProperty *m_xScale;
};

#endif // QSGANIMATEDSCALE_H
