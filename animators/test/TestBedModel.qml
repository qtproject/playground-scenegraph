import QtQuick 2.0

ListModel {
    id: testcaseModel
    ListElement { name: "TestPosition.qml"; group: "Item properties"; last: false }
    ListElement { name: "TestScale.qml"; group: "Item properties"; last: false }
    ListElement { name: "TestOpacity.qml"; group: "Item properties"; last: false }
    ListElement { name: "TestRotation.qml"; group: "Item properties"; last: false }
    ListElement { name: "TestTransformTranslate.qml"; group: "Item properties"; last: false }
    ListElement { name: "TestTransformScale.qml"; group: "Item properties"; last: false }
    ListElement { name: "TestTransformRotatation.qml"; group: "Item properties"; last: false }
    ListElement { name: "TestTransformRotationOriginPoint.qml"; group: "Item properties"; last: false }
    ListElement { name: "TestMultipleProperties.qml"; group: "Item properties"; last: true }
    ListElement { name: "TestShaderEffect.qml"; group: "ShaderEffect properties"; last: false }
    ListElement { name: "TestShaderEffectOpacity.qml"; group: "ShaderEffect properties"; last: true }
    ListElement { name: "TestPropertyAnimation.qml"; group: "Animations"; last: false }
    ListElement { name: "TestNumberAnimation.qml"; group: "Animations"; last: false }
    ListElement { name: "TestParallelAnimation.qml"; group: "Animations"; last: false }
    ListElement { name: "TestSequentialAnimation.qml"; group: "Animations"; last: true }
    ListElement { name: "TestMultipleAnimation.qml"; group: "Animations"; last: false }
    ListElement { name: "TestSequentialAnimationAlwaysRunToEnd.qml"; group: "Animations"; last: false }
    ListElement { name: "TestNumberAnimationAlwaysRunToEnd.qml"; group: "Animations"; last: true }
}
