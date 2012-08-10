/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of %MODULE%.
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

#include <QtQml/qqmlextensionplugin.h>
#include "qsganimatoritem.h"
#include "qsganimatorshadereffect.h"
#include "qsgpauseanimation.h"
#include "qsgpropertyanimation.h"
#include "qsgsequentialanimation.h"
#include "qsgparallelanimation.h"
#include "qsgnumberanimation.h"
#include "qsgcoloranimation.h"
#include "qsgvector3danimation.h"

class QSGAnimatorPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface/1.0")
public:
    virtual void registerTypes(const char *uri)
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("Animators"));
        Q_UNUSED(uri);
        qmlRegisterType<QSGAnimatorItem>(uri, 1, 0, "Item");
        qmlRegisterType<QSGAnimatorShaderEffect>(uri, 1, 0, "ShaderEffect");
        qmlRegisterType<QSGPauseAnimation>(uri, 1, 0, "PauseAnimation");
        qmlRegisterType<QSGPropertyAnimation>(uri, 1, 0, "PropertyAnimation");
        qmlRegisterType<QSGColorAnimation>(uri, 1, 0, "ColorAnimation");
        qmlRegisterType<QSGNumberAnimation>(uri, 1, 0, "NumberAnimation");
        qmlRegisterType<QSGVector3DAnimation>(uri, 1, 0, "Vector3DAnimation");
        qmlRegisterType<QSGSequentialAnimation>(uri, 1, 0, "SequentialAnimation");
        qmlRegisterType<QSGParallelAnimation>(uri, 1, 0, "ParallelAnimation");
    }
};

#include "qsganimatorplugin.moc"
