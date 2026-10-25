// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
ShaderEffect {
    property real spread: 24
    property real strength: 0.8
    property real mode: 0
    property color tint: "#ff183d"
    property vector2d resolution: Qt.vector2d(width,width)
    width: spread*5
    height: width
    fragmentShader: "shaders/flare.frag.qsb"
}
