/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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

import QtQuick 2.0

Rectangle {

    id: root

    width: 1500
    height: 900

    property real itemSize: 20
    property int rows: height / itemSize - 2;
    property int cols: width / itemSize - 2;

    gradient: Gradient {
        GradientStop { position: 0; color: "steelblue" }
        GradientStop { position: 1; color: "black" }
    }

    Repeater {

        model: root.rows * root.cols

        onModelChanged: if (model != 0) print("Populating with " + model + " items...");

        Image {
            width: root.itemSize
            height: root.itemSize

            sourceSize: Qt.size(width, height);
            source: "icon" + (1 + index % 4) + ".png"

            property int xPos: ((index % cols) + 1) * root.itemSize;
            property int yPos: (Math.floor(index / cols) + 1) * root.itemSize;
            x: xPos + shift
            y: yPos + shift

            property real shift: -10
            SequentialAnimation on shift {
                PauseAnimation { duration: index * 3; }
                SequentialAnimation {
                    NumberAnimation { to: 10; duration: 1000; easing.type: Easing.InOutSine }
                    NumberAnimation { to: -10; duration: 1000; easing.type: Easing.InOutSine }
                    loops: Animation.Infinite
                }
            }


        }
    }

}
