// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
import org.kde.kwin as KWin

Rectangle {
    id: preview
    required property string windowId
    required property string title
    required property var preferences
    property var face: ({})
    property bool active: true
    property int resolution: 640
    // Match the polygon's physical UV aspect, avoiding stretched windows.
    readonly property var layout: polygonLayout()
    width: resolution
    height: resolution / layout.aspect
    color: preferences.faceColor
    clip: true
    readonly property real pixelScale: resolution / 640
    readonly property bool titleOnly: !active
    function polygonLayout() {
        if (!face.vertices || face.vertices.length < 3) return {aspect:1,x:0,y:0,w:1,h:1};
        const n=face.normal;
        const preferred=face.up || (Math.abs(n.y)<0.949999988079071?Qt.vector3d(0,1,0):Qt.vector3d(0,0,-1));
        const up=preferred.minus(n.times(preferred.dotProduct(n))).normalized();
        const right=up.crossProduct(n).normalized();
        const points=face.vertices.map(p=>Qt.point(p.minus(face.center).dotProduct(right),p.minus(face.center).dotProduct(up)));
        const xs=points.map(p=>p.x), ys=points.map(p=>p.y);
        const minX=Math.min(...xs),maxX=Math.max(...xs),minY=Math.min(...ys),maxY=Math.max(...ys);
        const w=Math.max(0.001,maxX-minX),h=Math.max(0.001,maxY-minY);
        const cx=-minX/w,cy=1-(-minY/h);
        const uv=points.map(p=>Qt.point((p.x-minX)/w,1-(p.y-minY)/h));
        function inside(x,y) {
            let sign=0;
            for(let i=0;i<uv.length;i++) {
                const a=uv[i],b=uv[(i+1)%uv.length];
                const c=(b.x-a.x)*(y-a.y)-(b.y-a.y)*(x-a.x);
                if(Math.abs(c)<1e-8)continue;
                if(sign && sign*c<0)return false;
                sign=c;
            }
            return true;
        }
        // A centered safe rectangle stays inside triangular/pentagonal faces.
        let lo=0,hi=1;
        for(let i=0;i<20;i++) {
            const half=(lo+hi)/4;
            const ok=inside(cx-half,cy-half)&&inside(cx+half,cy-half)&&inside(cx-half,cy+half)&&inside(cx+half,cy+half);
            if(ok)lo=(lo+hi)/2;else hi=(lo+hi)/2;
        }
        const size=lo*0.96;
        return {aspect:w/h,x:cx-size/2,y:cy-size/2,w:size,h:size};
    }
    Item {
        id: content
        readonly property bool fit: preview.preferences.previewMode !== "fill" || preview.titleOnly
        readonly property real pad: preview.preferences.previewPadding/100
        x: (fit?preview.layout.x:0)*preview.width + pad*preview.width
        y: (fit?preview.layout.y:0)*preview.height + pad*preview.height
        width: Math.max(1,(fit?preview.layout.w:1)*preview.width-2*pad*preview.width)
        height: Math.max(1,(fit?preview.layout.h:1)*preview.height-2*pad*preview.height)
        Item {
            id: viewport
            width: parent.width
            height: preview.preferences.previewMode !== "fill" && preview.preferences.showFaceTitle && !preview.titleOnly ? Math.max(1,parent.height-titleBar.height) : parent.height
            y: preview.preferences.previewMode !== "fill" && preview.preferences.showFaceTitle && preview.preferences.titlePosition === "top" && !preview.titleOnly ? titleBar.height : 0
            clip: true
            visible: !preview.titleOnly
            opacity: preview.preferences.previewOpacity/100
            KWin.WindowThumbnail {
                id: thumbnail
                readonly property real ratio: client && client.width>0 && client.height>0 ? client.width/client.height : 1.6
                readonly property bool fill: preview.preferences.previewMode === "fill"
                width: fill ? Math.max(viewport.width,viewport.height*ratio) : viewport.width
                height: fill ? Math.max(viewport.height,viewport.width/ratio) : viewport.height
                anchors.centerIn: parent
                wId: preview.active ? preview.windowId : ""
                visible: client !== null
            }
        }
        Rectangle {
            id: titleBar
            width: parent.width
            height: preview.titleOnly ? parent.height : Math.min(parent.height,preview.preferences.titleSize*preview.pixelScale*1.9)
            y: preview.titleOnly || preview.preferences.titlePosition === "top" ? 0 : preview.preferences.titlePosition === "center" ? (parent.height-height)/2 : parent.height-height
            visible: preview.titleOnly || preview.preferences.showFaceTitle
            color: Qt.rgba(base.r,base.g,base.b,preview.preferences.titleBackgroundOpacity/100)
            readonly property color base: preview.preferences.titleBackgroundColor
            Text {
                anchors.fill: parent
                anchors.margins: 4*preview.pixelScale
                text: preview.title
                color: preview.preferences.titleColor
                font.pixelSize: preview.preferences.titleSize*preview.pixelScale
                font.bold: preview.preferences.titleBold
                wrapMode: Text.Wrap
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
        Text {
            anchors.centerIn: viewport
            width: viewport.width
            text: "Preview unavailable"
            color: preview.preferences.titleColor
            font.pixelSize: 18*preview.pixelScale
            horizontalAlignment: Text.AlignHCenter
            visible: preview.active && thumbnail.client === null
        }
    }
    Rectangle {
        anchors.fill: parent
        visible: preview.preferences.glassFrames && !preview.titleOnly
        opacity: preview.preferences.glassSheen/100
        gradient: Gradient {
            GradientStop { position:0; color:"#d0e9ff" }
            GradientStop { position:0.17; color:"#00ffffff" }
            GradientStop { position:0.66; color:"#00ffffff" }
            GradientStop { position:1; color:"#b0000814" }
        }
    }
}
