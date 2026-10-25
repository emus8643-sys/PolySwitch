// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
Item {
    id: sun
    required property var points
    property real radius: 24
    property real strength: 100
    readonly property var segments: {
        const result=[];
        if(points)for(let i=0;i<points.length;i++)result.push({a:Qt.point(points[i].x,points[i].y),b:Qt.point(points[(i+1)%points.length].x,points[(i+1)%points.length].y)});
        return result;
    }
    readonly property int northCorner: {
        let index=0;
        for(let i=1;i<points.length;i++)if(points[i].y<points[index].y)index=i;
        return index;
    }
    Repeater {
        model: sun.segments
        EdgeLight {
            required property var modelData
            startPoint: modelData.a; endPoint: modelData.b
            sunlight: true
            tint: "#ffe18b"
            spread: sun.radius*1.55
            strength: sun.strength/100
        }
    }
    Repeater {
        model: sun.points
        CornerFlare {
            required property var modelData
            required property int index
            mode: 1
            tint: "#ffc968"
            spread: sun.radius*(index===sun.northCorner?2.6:1.5)
            strength: sun.strength/100*(index===sun.northCorner?1:0.8)
            x: modelData.x-width/2; y: modelData.y-height/2
        }
    }
}
