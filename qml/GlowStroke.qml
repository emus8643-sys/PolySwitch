// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
import QtQuick.Shapes
import QtQuick.Effects

Shape {
    id: stroke
    required property var points
    property color glowColor: "white"
    property real lineWidth: 3
    property real spread: 24
    property real intensity: 1
    preferredRendererType: Shape.CurveRenderer
    antialiasing: true
    layer.enabled: true
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: stroke.glowColor
        shadowOpacity: stroke.intensity
        shadowBlur: 1
        blurMax: 64
        blurMultiplier: stroke.spread / 64
        shadowHorizontalOffset: 0
        shadowVerticalOffset: 0
    }
    ShapePath {
        strokeColor: stroke.glowColor
        strokeWidth: stroke.lineWidth
        joinStyle: ShapePath.RoundJoin
        fillColor: "transparent"
        PathPolyline { path: stroke.points }
    }
}
