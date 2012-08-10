import QtQuick 2.0
import Animators 1.0 as Rt

Rectangle {
    id: root
    width: 320
    height: 480
    color: "black"
    property var out33in60: [ 0.33, 0.0, 0.40, 1.0, 1.0, 1.0 ]
    property var out60in33: [ 0.60, 0.0, 0.67, 1.0, 1.0, 1.0 ]

    Rt.Item {
        id: renderThreadItem
        width: parent.width
        height: parent.height / 2
        visible: !renderThreadAnimationsDisabled

        Text {
            anchors.centerIn: parent
            text: "Render thread animation"
            smooth: true
            color: "white"
            font.pixelSize: 20
        }

        Rt.SequentialAnimation {
            id: testAnimation
            running: control.running
            paused: control.paused
            loops: 3
            Rt.NumberAnimation {
                loops: 1
                running: false
                target: renderThreadItem
                property: "scale"
                from: 1.0
                to: 2.0
                duration: 3*2000
                easing.type: Easing.Bezier
                easing.bezierCurve: out60in33
            }
            Rt.PauseAnimation { duration: 2000 }
            Rt.NumberAnimation {
                loops: 1
                running: false
                target: renderThreadItem
                property: "scale"
                from: 2.0
                to: 1.0
                duration: 3*2000
                easing.type: Easing.Bezier
                easing.bezierCurve: out60in33
            }
        }
    }

    Item {
        id: mainThreadItem
        y: parent.height / 2
        width: parent.width
        height: parent.height / 2
        visible: !mainThreadAnimationsDisabled

        Text {
            anchors.centerIn: parent
            text: "Main thread animation"
            smooth: true
            color: "white"
            font.pixelSize: 20
        }

        SequentialAnimation {
            id: referenceAnimation
            running: control.running && !mainThreadAnimationsDisabled
            paused: control.paused
            loops: 3
            NumberAnimation {
                loops: 1
                running: false
                target: mainThreadItem
                property: "scale"
                from: 1.0
                to: 2.0
                duration: 3*2000
                easing.type: Easing.Bezier
                easing.bezierCurve: out60in33
            }
            PauseAnimation { duration: 2000 }
            NumberAnimation {
                loops: 1
                running: false
                target: mainThreadItem
                property: "scale"
                from: 2.0
                to: 1.0
                duration: 3*2000
                easing.type: Easing.Bezier
                easing.bezierCurve: out60in33
            }
        }
    }

    Control {
        id: control
        anchors.bottom: parent.bottom
        running: true
    }
}
