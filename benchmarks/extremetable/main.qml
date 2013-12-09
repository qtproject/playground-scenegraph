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

    gradient: Gradient {
        GradientStop { position: 0; color: "steelblue" }
        GradientStop { position: 1; color: "black" }
    }

    GridView {
        id: grid

        model: 100000

        anchors.fill: parent

        cellWidth: 50
        cellHeight: 20

        clip: true

        delegate: Item {

            width: grid.cellWidth
            height: grid.cellHeight

            Rectangle {
                anchors.fill: parent
                anchors.margins: 2

                gradient: Gradient {
                    GradientStop { position: 0; color: "white"; }
                    GradientStop { position: 1; color: "lightsteelblue" }
                }

                Text {
                    text: index
                    anchors.centerIn: parent
                }
            }
        }
    }

    Rectangle {
        id: box
        color: Qt.rgba(1, 1, 1, 0.5);
        border.width: 5
        border.color: Qt.rgba(0, 0, 0, 0.5);
        radius: 20

        width: label.width + 50
        height: label.height + 50

        clip: true;

        SequentialAnimation on x {
            NumberAnimation { to: root.width - box.width; duration: 8047 }
            NumberAnimation { to: 0; duration: 8047 }
            loops: Animation.Infinite
        }

        SequentialAnimation on y {
            NumberAnimation { from: 0; to: root.height - box.height; duration: 5713 }
            NumberAnimation { from: root.height - box.height; to: 0; duration: 5713 }
            loops: Animation.Infinite
        }

        Text {
            id: label
            anchors.centerIn: parent
            text: "This is Qt!\nIsn't it neat?"
            font.pixelSize: 60
            style: Text.Raised
            color: "black"
        }
    }

}
