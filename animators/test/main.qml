import QtQuick 2.0

Rectangle {
    id: main
    width: 320
    height: 480
    color: "black"

    property string currentTest: ""
    property bool renderThreadAnimationsDisabled: false
    property bool mainThreadAnimationsDisabled: false

    Loader {
        id: testLoader
        anchors.left: menuView.right
        width: 320
        height: 480
    }

    Component {
        id: sectionHeading
        Text {
            id: sectionText
            text: section
            anchors.left: parent.left
            anchors.leftMargin: 10
            height: 30
            width: parent.width
            color: "white"
            verticalAlignment: Text.AlignVCenter
            font.family: "Nokia Pure Text"
            font.bold: true
            font.pixelSize: 18
        }
    }

    Rectangle {
        id: menuView
        width: parent.width
        height: parent.height
        color: "black"

        ListView {
            id: testCaseList

            anchors.top: parent.top
            anchors.topMargin: 10
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 40
            anchors.left: parent.left
            anchors.right: parent.right

            model: TestBedModel {}

            section.property: "group"
            section.criteria: ViewSection.FullString
            section.delegate: sectionHeading

            Component.onCompleted: contentY = 0

            delegate: Item {
                    width: testCaseList.width
                    height: 50

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 5
                        color: "#2e2e2e"
                    }

                    Text {
                        id: delegateText;
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        text: name.slice(4, name.indexOf("."))
                        width: parent.width
                        height: 50
                        font.family: "Nokia Pure Text"
                        font.pixelSize: 16
                        verticalAlignment: Text.AlignVCenter
                        color: "#e6e6e6"
                    }
                    MouseArea {
                        id: delegateMouseArea
                        anchors.fill: parent;
                        onClicked: {
                            main.state = "testRunning"
                            testLoader.source = name
                            currentTest = name.slice(4, name.indexOf("."))
                        }
                    }
            }
        }
    }

    states: State {
        name: "testRunning"
        PropertyChanges { target: menuView; x: -width }
    }

    transitions: [
        Transition {
            to: "testRunning"
            SequentialAnimation {
                NumberAnimation {
                    property: "x"
                    duration: 200
                }
                PropertyAction {
                    target: menuView
                    property: "visible"
                    value: false
                }
            }
        },
        Transition {
            to: ""
            SequentialAnimation {
                PropertyAction {
                    target: menuView
                    property: "visible"
                    value: true
                }
                NumberAnimation {
                    property: "x"
                    duration: 200
                }
                PropertyAction {
                    target: testLoader
                    property: "source"
                    value: ""
                }
            }
        }
    ]

    Rectangle {
        id: mainControl
        anchors.bottom: parent.bottom
        property bool keepalive: false
        visible: main.state == ""

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
                width: 90
                height: 40
                color: "#3e3e3e"
                Text {
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    anchors.fill: parent
                    text: !mainControl.keepalive ? "Keepalive" : "No keepalive"
                    color: "white"
                    font.pixelSize: 14
                    font.family: "Nokia Pure Text"
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            mainControl.keepalive = !mainControl.keepalive
                        }
                    }
                }
                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.bottom
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: "Keepalive"
                    color: "red"
                    font.pixelSize: 10
                    opacity: mainControl.keepalive ? 0.5 : 0.0
                    NumberAnimation on opacity {
                        running: mainControl.keepalive
                        from: 0.5
                        to: 0.1
                        duration: 2000
                        loops: Animation.Infinite
                    }
                }
            }
            Rectangle {
                width: 90
                height: 40
                color: "#3e3e3e"
                Text {
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    anchors.fill: parent
                    text: renderThreadAnimationsDisabled ? "Enable RT" : "Disable RT"
                    color: "white"
                    font.pixelSize: 14
                    font.family: "Nokia Pure Text"
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            renderThreadAnimationsDisabled = !renderThreadAnimationsDisabled
                        }
                    }
                }
                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.bottom
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: "RT disabled"
                    color: "red"
                    font.pixelSize: 10
                    visible: renderThreadAnimationsDisabled
                }
            }
            Rectangle {
                width: 90
                height: 40
                color: "#3e3e3e"
                Text {
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    anchors.fill: parent
                    text: mainThreadAnimationsDisabled ? "Enable MT" : "Disable MT"
                    color: "white"
                    font.pixelSize: 14
                    font.family: "Nokia Pure Text"
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            mainThreadAnimationsDisabled = !mainThreadAnimationsDisabled
                        }
                    }
                }
                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.bottom
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: "RT disabled"
                    color: "red"
                    font.pixelSize: 10
                    visible: mainThreadAnimationsDisabled
                }
            }
        }
    }
}
