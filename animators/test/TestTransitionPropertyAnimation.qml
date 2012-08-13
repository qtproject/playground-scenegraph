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
            console.log("onTriggered: changing test state.")
            renderThreadItem.state = "moved"
            mainThreadItem.state = "moved"
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

        states: State {
              name: "moved"
              PropertyChanges { target: renderThreadItem; rotation: 360 }
        }

        transitions: Transition {
            Rt.NumberAnimation {
                target: renderThreadItem
                easing.type: Easing.Bezier
                easing.bezierCurve: out60in33
                duration: 6000
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

        states: State {
              name: "moved"
              PropertyChanges { target: mainThreadItem; rotation: 360 }
        }

        transitions: Transition {
            NumberAnimation {
                target: mainThreadItem
                property: "rotation"
                easing.type: Easing.Bezier
                easing.bezierCurve: out60in33
                duration: 6000
            }
        }
    }

    Control {
        id: control
        anchors.bottom: parent.bottom
        running: true
    }
}
