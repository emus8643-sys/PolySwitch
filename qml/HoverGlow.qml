// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
import QtQuick.Shapes

Item {
    id: halo
    required property var points
    property color glowColor: "#ff163b"
    property string style: "neon"
    property real radius: 24
    property real strength: 100
    property real breath: 1
    readonly property bool luminous: style==="ice"
    readonly property real intensity: strength/100*(style==="pulse"?breath:1)
    readonly property real padding: luminous ? 176 : 80
    readonly property var bounds: {
        if (!points || points.length<3) return Qt.rect(0,0,0,0);
        let left=Infinity,top=Infinity,right=-Infinity,bottom=-Infinity;
        for(const p of points){left=Math.min(left,p.x);right=Math.max(right,p.x);top=Math.min(top,p.y);bottom=Math.max(bottom,p.y);}
        return Qt.rect(left,top,right-left,bottom-top);
    }
    x: bounds.x-padding; y: bounds.y-padding
    width: bounds.width+2*padding; height: bounds.height+2*padding
    visible: points && points.length>=3 && strength>0
    readonly property var localPoints: {
        const result=[];
        for(const p of points)result.push(Qt.point(p.x-halo.x,p.y-halo.y));
        if(result.length)result.push(result[0]);
        return result;
    }
    SequentialAnimation on breath {
        running: halo.visible && halo.style==="pulse"
        loops: Animation.Infinite
        NumberAnimation { to: 0.45; duration: 900; easing.type: Easing.InOutSine }
        NumberAnimation { to: 1; duration: 900; easing.type: Easing.InOutSine }
    }
    GlowStroke {
        anchors.fill: parent
        points: halo.localPoints
        visible: halo.luminous
        glowColor: halo.style==="sunlight" ? "#ffb52e" : "#49baff"
        lineWidth: 13
        spread: halo.radius*2.5
        intensity: halo.intensity*0.75
        opacity: halo.intensity*0.65
    }
    GlowStroke {
        anchors.fill: parent
        visible: halo.style!=="sunlight"
        points: halo.localPoints
        glowColor: halo.glowColor
        lineWidth: halo.luminous ? 6 : 2+3*halo.intensity
        spread: halo.radius
        intensity: halo.intensity
        opacity: halo.intensity
    }
    GlowStroke {
        anchors.fill: parent
        points: halo.localPoints
        visible: halo.luminous
        glowColor: halo.style==="sunlight" ? "#fffbee" : "#f1fdff"
        lineWidth: 2.5
        spread: 8
        intensity: halo.intensity
        opacity: halo.intensity
    }
    readonly property var rays: halo.style==="sunlight" ? sunlight.segments : []
    SunlightHalo {
        id: sunlight
        visible: halo.style==="sunlight"
        points: halo.localPoints.slice(0,-1)
        radius: halo.radius
        strength: halo.strength
    }
}
