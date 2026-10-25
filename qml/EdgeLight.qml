// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
Item {
    id: edge
    required property point startPoint
    required property point endPoint
    property color tint: "#ff183d"
    property real spread: 18
    property real strength: 0.5
    property bool sunlight: false
    property bool soft: false
    readonly property real length: Math.hypot(endPoint.x-startPoint.x,endPoint.y-startPoint.y)
    readonly property real padding: Math.max(32,spread*2.8)
    x: startPoint.x-padding; y: startPoint.y-padding
    width: length+2*padding; height: 2*padding
    transform: Rotation { origin.x: edge.padding; origin.y: edge.padding; angle: Math.atan2(edge.endPoint.y-edge.startPoint.y,edge.endPoint.x-edge.startPoint.x)*180/Math.PI }
    ShaderEffect {
        anchors.fill: parent
        property real lineLength: edge.length
        property real spread: edge.spread
        property real margin: edge.padding
        property vector2d resolution: Qt.vector2d(width,height)
        property color tint: edge.tint
        property real mode: edge.soft?2:edge.sunlight?1:0
        property real strength: edge.strength
        fragmentShader: "shaders/edge.frag.qsb"
    }
}
