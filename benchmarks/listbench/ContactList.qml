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

import QtQuick 2.0

Rectangle {
    id: root

    property int itemHeight: 60
    width: itemHeight * 3
    height: parent.height
    color: "transparent"
    border.width: 1
    border.color: "darkgray"

    ListModel {
        id: contacts
        ListElement {
            image: "avatar_female_gray_on_clear_200x200.png"
            name: "Jane Doe"
            occupation: "Test person"
        }
        ListElement {
            image: "avatar_male_gray_on_clear_200x200.png"
            name: "John Doe"
            occupation: "Test person"
        }
    }

    ListView {
        id: view
        SequentialAnimation {
            running: true
            NumberAnimation {
                target: view
                property: "contentY"
                from: 0
                to: 10 * itemHeight
                duration: 20000
            }
            NumberAnimation {
                target: view
                property: "contentY"
                to: 0
                duration: 20000
            }
            loops: Animation.Infinite
        }
        x: 1
        width: parent.width - 1
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        model: 40
        delegate: Rectangle {
            property variant item: contacts.get(index % 2)
            color: (index % 2) == 0 ? "white" : "lightgray"
            width: view.width
            height: itemHeight

            Image {
                x: 10

                anchors.verticalCenter: parent.verticalCenter

                source: item.image
                smooth: true
                width: itemHeight * 0.9
                height: itemHeight * 0.9
                sourceSize: Qt.size(itemHeight * 0.9, itemHeight * 0.9)
            }

            Column {
                x: 10 + itemHeight * 0.94
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    text: item.name
                    font.pixelSize: itemHeight * 0.25
                }

                Text {
                    text: item.occupation
                    font.italic: true
                    font.weight: Font.Light
                    font.pixelSize: itemHeight * 0.25
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom

                width: view.width
                height: 1

                color: "darkgray"
            }
        }
    }
}

