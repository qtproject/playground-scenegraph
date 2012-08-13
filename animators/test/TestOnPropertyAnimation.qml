import QtQuick 2.0
import Animators 1.0 as Rt

Rectangle {
    id: root
    width: 320
    height: 480
    color: "black"
    property var out33in60: [ 0.33, 0.0, 0.40, 1.0, 1.0, 1.0 ]
    property var out60in33: [ 0.60, 0.0, 0.67, 1.0, 1.0, 1.0 ]

    Timer {
        repeat: false
        interval: 1000
        running: true
        onTriggered: {
            renderThreadItem.rotation = 360
            mainThreadItem.rotation = 360
        }
    }

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

        Rt.NumberAnimation on rotation {
            duration: 6000
            from: 0
            to: 360
            target: renderThreadItem // NOTE: target must be specified here.
            property: "rotation" // NOTE: property must be specified here again.
            easing.type: Easing.Bezier
            easing.bezierCurve: out60in33
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

        NumberAnimation on rotation {
            duration: 6000
            from: 0
            to: 360
            easing.type: Easing.Bezier
            easing.bezierCurve: out60in33
        }
    }

    Control {
        id: control
        anchors.bottom: parent.bottom
        running: true
    }
}
