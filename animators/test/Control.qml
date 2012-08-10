import QtQuick 2.0

Rectangle {
    id: control

    property bool testing: false
    property bool running: false
    property bool paused: false
    property bool blocking: false

    onBlockingChanged: {
        blockerStopTimer.running = blocking
    }

    Timer {
        id: blockerStopTimer
        interval: 20000
        repeat: false
        running: false
        onTriggered: control.blocking = false
    }

    width: 320
    height: 65
    color: "#2e2e2e"

    Rectangle {
        width: parent.width
        height: 4
        gradient: Gradient {
                GradientStop { position: 1.0; color: "#2e2e2e" }
                GradientStop { position: 0.0; color: "black" }
            }
    }

    Row {
        x: 10
        y: 10
        spacing: 5
        Rectangle {
            width: 70
            height: 40
            color: "#3e3e3e"
            Text {
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                anchors.fill: parent
                text: "Back"
                color: "white"
                font.pixelSize: 14
                font.family: "Nokia Pure Text"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        control.testing = !control.testing
                        control.running = false
                        control.paused = false
                        control.blocking = false
                        main.state = ""
                    }
                }
            }
        }

        Rectangle {
            id: pauseButton
            width: 70
            height: 40
            color: "#3e3e3e"
            Text {
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                anchors.fill: parent
                text: control.paused ? "Continue" : "Pause"
                color: "white"
                font.pixelSize: 14
                font.family: "Nokia Pure Text"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        control.paused = !control.paused
                    }
                }
            }
            Text {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.bottom
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: "Paused"
                color: "red"
                font.pixelSize: 10
                visible: control.paused
            }
        }

        Rectangle {
            id: stopButton
            width: 70
            height: 40
            color: "#3e3e3e"
            Text {
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                anchors.fill: parent
                text: control.running ? "Stop" : "Start"
                color: "white"
                font.pixelSize: 14
                font.family: "Nokia Pure Text"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        control.running = !control.running
                    }
                }
            }
            Text {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.bottom
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: "Running"
                color: "red"
                font.pixelSize: 10
                visible: control.running
            }
        }

        Rectangle {
            id: blockButton
            width: 70
            height: 40
            color: "#3e3e3e"
            Text {
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                anchors.fill: parent
                text: control.blocking ? "No block" : "Block"
                color: "white"
                font.pixelSize: 14
                font.family: "Nokia Pure Text"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        control.blocking = !control.blocking
                    }
                }
            }
            Text {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.bottom
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: "Blocking"
                color: "red"
                font.pixelSize: 10
                visible: control.blocking
            }
        }
    }

    Timer {
        id: blockerTimer
        interval: 200
        repeat: true
        running: control.blocking
        onTriggered: blocker()
        function blocker() {
            var date = new Date();
            while (new Date().getTime() - date.getTime() < 100) {
                var x = 1 + 2;
            }
        }
    }
}
