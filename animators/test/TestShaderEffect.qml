import QtQuick 2.0
import Animators 1.0 as Rt

Rectangle {
    id: root
    width: 320
    height: 480
    color: "black"
    property var out33in60: [ 0.33, 0.0, 0.40, 1.0, 1.0, 1.0 ]
    property var out60in33: [ 0.60, 0.0, 0.67, 1.0, 1.0, 1.0 ]

    Rt.ShaderEffect {
        id: effect
        width: parent.width
        height: parent.height / 2
        visible: !renderThreadAnimationsDisabled

        property variant source: ShaderEffectSource {
            sourceItem: Rectangle { width: 100; height: 100; color: "red" }
            hideSource: true
        }
        property real c: 0.0

        fragmentShader:
            "
            varying highp vec2 qt_TexCoord0;
            uniform sampler2D source;
            uniform highp float c;

            void main() {
                lowp vec4 color = texture2D(source, qt_TexCoord0);
                gl_FragColor = mix(color, vec4(0.4), c);
            }
            "

        Text {
            anchors.centerIn: parent
            text: "Render thread animation"
            smooth: true
            color: "white"
            font.pixelSize: 20
        }

        Rt.SequentialAnimation {
            id: testAnimation
            running: control.running && !renderThreadAnimationsDisabled
            paused: control.paused
            loops: 10

            Rt.NumberAnimation {
                loops: 1
                target: effect
                property: "scale"
                from: 1.0
                to: 0.8
                duration: 1500
            }

            Rt.SequentialAnimation {
                loops: 2
                Rt.NumberAnimation {
                    loops: 1
                    target: effect
                    property: "c"
                    from: 0.0
                    to: 1.0
                    duration: 1500
                }
                Rt.NumberAnimation {
                    loops: 1
                    target: effect
                    property: "c"
                    from: 1.0
                    to: 0.0
                    duration: 1500
                }
            }

            Rt.NumberAnimation {
                loops: 1
                target: effect
                property: "scale"
                from: 0.8
                to: 1.0
                duration: 1500
            }

        }
    }

    ShaderEffect {
        id: refeffect
        y: parent.height / 2 + 5
        width: parent.width
        height: parent.height / 2
        visible: !mainThreadAnimationsDisabled
        property var source: ShaderEffectSource {
            sourceItem: Rectangle { width: 100; height: 100; color: "red" }
            hideSource: true
        }
        property real c: 0.0

        fragmentShader:
            "
            varying highp vec2 qt_TexCoord0;
            uniform sampler2D source;
            uniform highp float c;

            void main() {
                lowp vec4 color = texture2D(source, qt_TexCoord0);
                gl_FragColor = mix(color, vec4(0.4), c);
            }
            "
        Text {
            anchors.centerIn: parent
            text: "Main thread animation"
            smooth: true
            color: "white"
            font.pixelSize: 20
        }

        SequentialAnimation {
            id: refAnimation
            running: control.running && !mainThreadAnimationsDisabled
            paused: control.paused
            loops: 10

            NumberAnimation {
                loops: 1
                target: refeffect
                property: "scale"
                from: 1.0
                to: 0.8
                duration: 1500
            }

            SequentialAnimation {
                loops: 2
                NumberAnimation {
                    loops: 1
                    target: refeffect
                    property: "c"
                    from: 0.0
                    to: 1.0
                    duration: 1500
                }
                NumberAnimation {
                    loops: 1
                    target: refeffect
                    property: "c"
                    from: 1.0
                    to: 0.0
                    duration: 1500
                }
            }

            NumberAnimation {
                loops: 1
                target: refeffect
                property: "scale"
                from: 0.8
                to: 1.0
                duration: 1500
            }
        }
    }

        Control {
            id: control
            anchors.bottom: parent.bottom
            running: true
        }
    }
