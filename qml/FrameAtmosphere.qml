// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
Item {
    id: frames
    required property var polygons
    property color tint: "#ff183d"
    property real strength: 0.55
    property real spread: 18
    property string style: "neon"
    property real breath: 1
    readonly property real intensity: strength*(style==="pulse"?breath:1)
    SequentialAnimation on breath {
        running: frames.visible && frames.style==="pulse"
        loops: Animation.Infinite
        NumberAnimation { to:0.35;duration:1200;easing.type:Easing.InOutSine }
        NumberAnimation { to:1;duration:1200;easing.type:Easing.InOutSine }
    }
    readonly property var frameData: {
        const edges=[],corners=[],seenEdges={},seenCorners={};
        let left=Infinity,top=Infinity,right=-Infinity,bottom=-Infinity;
        function key(p){return Math.round(p.x)+","+Math.round(p.y);}
        for(const polygon of polygons)for(let i=0;i<polygon.length;i++){
            const a=polygon[i],b=polygon[(i+1)%polygon.length],ka=key(a),kb=key(b);
            const edgeKey=ka<kb?ka+";"+kb:kb+";"+ka;
            if(!seenEdges[edgeKey]){seenEdges[edgeKey]=true;edges.push({a:Qt.point(a.x,a.y),b:Qt.point(b.x,b.y)});}
            if(!seenCorners[ka]){seenCorners[ka]=true;corners.push(Qt.point(a.x,a.y));}
            left=Math.min(left,a.x);right=Math.max(right,a.x);top=Math.min(top,a.y);bottom=Math.max(bottom,a.y);
        }
        return {edges:edges,corners:corners,bounds:corners.length?Qt.rect(left,top,right-left,bottom-top):Qt.rect(0,0,0,0)};
    }
    CornerFlare {
        visible: frames.frameData.corners.length>0
        width: Math.min(frames.width*0.8,frames.frameData.bounds.width*1.6)
        height: 66
        x: frames.frameData.bounds.x+(frames.frameData.bounds.width-width)/2
        y: frames.frameData.bounds.y+frames.frameData.bounds.height-8
        mode: 2
        spread: width*0.32
        strength: frames.intensity*0.85
        tint: frames.tint
    }
    Repeater {
        model: frames.frameData.edges
        EdgeLight {
            required property var modelData
            startPoint: modelData.a; endPoint: modelData.b
            tint: frames.tint
            spread: frames.spread
            soft: frames.style==="soft"
            sunlight: frames.style==="radiant"
            strength: frames.intensity
        }
    }
    Repeater {
        model: frames.frameData.corners
        CornerFlare {
            required property var modelData
            x: modelData.x-width/2; y: modelData.y-height/2
            tint: frames.tint
            visible: frames.style!=="soft"
            strength: frames.intensity*1.25
            mode: frames.style==="radiant"?1:0
            spread: frames.spread*(frames.style==="radiant"?1.5:1.1)
        }
    }
}
