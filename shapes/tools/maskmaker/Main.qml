/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Scene Graph Playground module of the Qt Toolkit.
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
import Qt.labs.shapes 1.0

Item {

    id: root;

    width: Math.max(600, imageRoot.width + 200);
    height: Math.max(600, imageRoot.height + 200);

    property bool viewFiltered: true
    property real aspectRatio: width / height
    property string writeMode: "interior"       // interior, exterior, none

    property var exterior: []
    property var interior: []

    property int boxSize: 4

    Checkers
    {
        anchors.fill: parent
    }

    Item {
        id: imageRoot

        visible: image.status == Image.Ready

        anchors.centerIn: parent
        width: image.width
        height: image.height

        Image {
            id: image
            property real aspectRatio: width / height
            source: "file:" + engine.source;
        }

        ShaderEffect {
            anchors.fill: parent;

            visible: root.viewFiltered

            blending: false
            property variant source: image
            fragmentShader:
                "
                uniform lowp sampler2D source;
                varying highp vec2 qt_TexCoord0;
                void main() {
                    lowp float a = texture2D(source, qt_TexCoord0).a;
                    if (a == 0.0)
                        gl_FragColor = vec4(0, 0, 0, 1);
                    else if (a == 1.0)
                        gl_FragColor = vec4(0.4, 0.4, 0.4, 1);
                    else
                        gl_FragColor = vec4(1, 1, 1, 1);
                }
                "
        }
    }

    Canvas {
        id: overlay

        anchors.fill: parent;

        function drawDots(ctx, list, color) {
            ctx.beginPath();
            for (var si=0; si<list.length; ++si) {
                var subpath = list[si];
                for (var i=0; i<subpath.length; ++i) {
                    var p = subpath[i];
                    ctx.ellipse(p.x - 6, p.y - 6, 13, 13);
                }
            }
            ctx.fillStyle = color
            ctx.fill();
            ctx.strokeStyle = "white"
            ctx.stroke();
        }

        function xform(pt) {
            return { x: pt.x * imageRoot.width + imageRoot.x,
                     y: pt.y * imageRoot.height + imageRoot.y };
        }

        function drawShapes(ctx, list, color) {
            for (var si=0; si<list.length; ++si) {
                var subpath = list[si];
                if (subpath.length >= 3) {
                    for (var i=0; i<subpath.length; ++i) {
                        var p = subpath[i];
                        if (i == 0)
                            ctx.moveTo(p.x, p.y);
                        else
                            ctx.lineTo(p.x, p.y);
                    }
                }
                ctx.closePath();
            }
            ctx.fillStyle = color;
            ctx.fill();
            ctx.stroke()
        }

        onPaint: {
            var ctx = overlay.getContext("2d");

            ctx.clearRect(0, 0, overlay.width, overlay.height);
            ctx.reset();

            ctx.translate(imageRoot.x, imageRoot.y);

            ctx.lineCap = "butt"
            ctx.beginPath();
            ctx.globalAlpha = 0.7
            ctx.strokeStyle = "white";
            ctx.fillRule = Qt.OddEvenFill
            ctx.lineWidth = 1;
            drawShapes(ctx, root.interior, "#aaaaff");
            if (root.exterior.length > 0 && root.exterior[0].length > 2)
                drawShapes(ctx, root.exterior, "#ffaaaa");

            ctx.lineWidth = 2
            ctx.globalAlpha = 1.0;
            drawDots(ctx, root.interior, "blue");
            drawDots(ctx, root.exterior, "red");

        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: {
            helpOverlay.hidden = true;
            var list = root.writeMode == "interior" ? root.interior : root.exterior;
            var pt = { x: mouse.x - imageRoot.x, y: mouse.y - imageRoot.y };

            if (list.length == 0) {
                list.push( [ pt ]);
            } else {
                var subpath = list[list.length - 1];
                if (subpath.length == 0) {
                    subpath.push(pt)
                } else {
                    var s = subpath[0];
                    if (Math.abs(s.x - mouse.x + imageRoot.x) < 20 && Math.abs(s.y - mouse.y + imageRoot.x) < 20 && subpath.length >= 2) {
                        subpath.push(s);
                        list.push([]);
                    } else {
                        subpath.push(pt);
                    }
                }
            }
            overlay.requestPaint()
        }
    }

    TriangleSet {
        id: triangleSet

        function add(list) {
            for (var si=0; si<list.length; ++si) {
                var subpath = list[si];
                if (subpath.length > 0)
                    moveTo(subpath[0].x / imageRoot.width, subpath[0].y / imageRoot.height);
                for (var i=1; i<subpath.length; ++i)
                    lineTo(subpath[i].x / imageRoot.width, subpath[i].y / imageRoot.height);
                closeSubpath()
            }
        }

        function save() {
            if (root.interior.length == 0 || root.interior[0].length < 3
                    || root.exterior.length == 0 || root.exterior[0].length < 3) {
                helpOverlay.hidden = false;
                return;
            }

            beginPathConstruction();
            add(root.interior);
            clipToRect(0, 0, 1, 1)
            finishPathConstruction();
            var or = saveFile(engine.source + ".opaquemask");

            if (or == "") {
                print("saved " + engine.source + ".opaquemask ok...");
            } else {
                print("saving " + engine.source + ".opaquemask failed: " + or);
                return;
            }

            beginPathConstruction();
            add(root.interior);
            add(root.exterior);
            clipToRect(0, 0, 1, 1);
            finishPathConstruction();
            var ar = saveFile(engine.source + ".alphamask");
            if (ar == "") {
                print("saved " + engine.source + ".alphamask ok...");
            } else {
                print("saving " + engine.source + ".alphamask failed: " + ar);
                return;
            }

            print("My work here is done...\nGood bye!")
            Qt.quit();
        }
    }

    Keys.onSpacePressed: root.viewFiltered = !root.viewFiltered;
    Keys.onPressed: {
        if (event.key == Qt.Key_I) {
            root.writeMode = "interior"
        } else if (event.key == Qt.Key_E) {
            root.writeMode = "exterior"
        } else if (event.key == Qt.Key_F1) {
            helpOverlay.hidden = !helpOverlay.hidden;
        } else if (event.key == Qt.Key_Escape) {
            Qt.quit();
        } else if (event.key == Qt.Key_Backspace) {
            root.interior = []
            root.exterior = []
            overlay.requestPaint();
        } else if (event.key == Qt.Key_Enter || event.key == Qt.Key_Return) {
            triangleSet.save()
        }
    }

    Rectangle {

        id: helpOverlay

        x: parent.width / 2 - width / 2;
        y: parent.height / 2- height / 2;

        width: label.width + 40
        height: label.height + 40

        gradient: Gradient {
            GradientStop { position: 0; color: "white" }
            GradientStop { position: 1; color: "gray" }
        }
        property bool hidden: false
        opacity: hidden ? 0.0 : 1.0
        Behavior on opacity { NumberAnimation { duration: 100 } }
        border.color: "black"
        radius: 10

        enabled: !hidden

        Text {
            id: label
            font.family: "courier"
            text: image.status != Image.Ready ?
                      "Missing source image. Did you forget to specify it?" :
"
Usage:

Click to add points to the polygon for interior and exterior.
The interior polygon should mark that which is fully opaque,
visible as gray when the filter is active. The exterior should
be outside all partially translucent pixels, visible as white
when the filter is active. You need at least three nodes in both
interior and exterior before writing the triangles set to disk.

Shortcuts:

E:          Add to exterior " + (root.writeMode == "exterior" ? "*" : "") + "
I:          Add to interior " + (root.writeMode == "interior" ? "*" : "") + "
Backspace:  Clear polygons and start over...
F1:         Toggle this help
Enter:      Save polygons as MaskedImage metadata
Space:      Toggle image filter " + (root.viewFiltered ? "*" : "") + "
Escape:     Exit...
"
            anchors.centerIn: parent;

        }
    }


    focus: true


}
