// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
import QtQuick3D
import QtQuick.Controls as C
import org.kde.kwin as KWin
import PolySwitch.Native 1.0
import "ScreenRotation.js" as ScreenRotation

Item {
    id: root
    required property var effectBridge
    readonly property var taskEntries: effectBridge.tasks
    readonly property int nativeWindowCount: taskEntries.length
    readonly property var nativeWindowIds: taskEntries.map(task => task.id)
    readonly property var nativeWindowTitles: taskEntries.map(task => task.title)
    focus: true
    readonly property real cameraDistance: 67000 / prefs.zoomPercent
    function changeZoom(steps) {
        noteInteraction();
        effectBridge.settings.setValue("zoomPercent",Math.round(prefs.zoomPercent*Math.pow(1.1,steps)));
    }
    property string searchQuery: ""
    readonly property var searchMatches: rankSearch(searchQuery)
    function rankSearch(query) {
        const q=query.trim().toLocaleLowerCase();
        if (!q) return [];
        let matches=[];
        for (const task of taskEntries) {
            const title=task.title.toLocaleLowerCase();
            const words=title.split(/[^\p{L}\p{N}]+/u);
            const rank=title.startsWith(q)?0:words.some(word=>word.startsWith(q))?1:title.includes(q)?2:99;
            if (rank<99) matches.push({id:task.id,title:task.title,rank:rank});
        }
        matches.sort((a,b)=>a.rank-b.rank || a.title.toLocaleLowerCase().localeCompare(b.title.toLocaleLowerCase()) || a.id.localeCompare(b.id));
        return matches;
    }
    function updateSearch() {
        if (!searchQuery.trim()) { keyboardMode=false; return; }
        keyboardMode=true;
        selectedId=searchMatches.length?searchMatches[0].id:"";
        focusSelectedFace();
    }
    onSearchQueryChanged: { noteInteraction(); Qt.callLater(updateSearch); }
    function activateSelection() {
        if (searchQuery.trim() && !searchMatches.length) return;
        const id=selectedTask>=0?selectedId:nativeWindowIds[hitFace(width/2,height/2)];
        if (id) effectBridge.activateWindowId(id);
    }
    property bool keyboardMode: false
    property string selectedId: ""
    readonly property int selectedTask: nativeWindowIds.indexOf(selectedId)
    onNativeWindowCountChanged: noteInteraction()
    onTaskEntriesChanged: Qt.callLater(reconcileSelection)
    function reconcileSelection() {
        if (searchQuery.trim()) { updateSearch(); return; }
        if (selectedId && nativeWindowIds.indexOf(selectedId) < 0)
            selectedId = nativeWindowIds.length ? nativeWindowIds[0] : "";
        settingsPreviewTask = -1;
        if (keyboardMode) focusSelectedFace();
    }
    function focusSelectedFace() {
        velX=0; velY=0;
        const face = polyGeometry.faces[selectedTask];
        if (face) orientation = ScreenRotation.uprightFace(face.normal);
    }
    function stepSelection(step) {
        noteInteraction();
        const wasKeyboard=keyboardMode;
        keyboardMode = true;
        if (searchQuery.trim()) {
            if (!searchMatches.length) return;
            const at=searchMatches.findIndex(task=>task.id===selectedId);
            selectedId=searchMatches[(Math.max(0,at)+step+searchMatches.length)%searchMatches.length].id;
            focusSelectedFace(); return;
        }
        if (!nativeWindowCount) { selectedId=""; return; }
        const current = wasKeyboard && selectedTask >= 0 ? selectedTask : nearestFaceIndex();
        const index = current < 0 ? 0 : (current+step+nativeWindowCount)%nativeWindowCount;
        selectedId = nativeWindowIds[index];
        focusSelectedFace();
    }
    property real idleMilliseconds: 0
    property bool idleAligned: false
    function noteInteraction() {
        idleMilliseconds=0; idleAligned=false;
        if(effectBridge && typeof effectBridge.markInteraction==="function") effectBridge.markInteraction();
    }
    function nearestFaceIndex() {
        const inverse=Qt.quaternion(orientation.scalar,-orientation.x,-orientation.y,-orientation.z);
        const camera=ScreenRotation.rotated(inverse,Qt.vector3d(0,0,cameraDistance));
        let best=-Infinity, index=-1;
        for(let i=0;i<polyGeometry.faces.length;i++) {
            const face=polyGeometry.faces[i];
            let score=face.normal.dotProduct(camera.minus(face.center).normalized());
            if(nativeWindowCount===1 || nativeWindowCount===3 || nativeWindowCount===4)score=Math.abs(score);
            if(score>best+1e-6) {best=score;index=i;}
        }
        return index;
    }
    function alignNearestFace() {
        const index=nearestFaceIndex();
        if(index<0)return;
        selectedId=nativeWindowIds[index];
        keyboardMode=true;
        focusSelectedFace();
    }
    function advanceIdle(milliseconds) { updateIdleElapsed(idleMilliseconds+milliseconds); }
    function updateIdleElapsed(milliseconds) {
        if(!prefs.autoUpright || dragging || searchQuery.trim()) { idleMilliseconds=0; return; }
        idleMilliseconds=milliseconds;
        if(!idleAligned && idleMilliseconds>=1000) { idleAligned=true; alignNearestFace(); }
    }
    Keys.onPressed: event => {
        noteInteraction();
        if (settingsOpen) return;
        if (event.key===Qt.Key_Plus || event.key===Qt.Key_Equal) { changeZoom(1); event.accepted=true; return; }
        if (event.key===Qt.Key_Minus) { changeZoom(-1); event.accepted=true; return; }
        if (event.key===Qt.Key_0) { effectBridge.settings.setValue("zoomPercent",100); event.accepted=true; return; }
        if ([Qt.Key_Left,Qt.Key_Up,Qt.Key_Backtab].includes(event.key)) { stepSelection(-1); event.accepted=true; }
        else if ([Qt.Key_Right,Qt.Key_Down,Qt.Key_Tab].includes(event.key)) { stepSelection(1); event.accepted=true; }
        else if (event.key===Qt.Key_Return || event.key===Qt.Key_Enter) {
            activateSelection();
            event.accepted=true;
        } else if (event.text && event.text.trim() && !(event.modifiers & (Qt.ControlModifier|Qt.AltModifier|Qt.MetaModifier))) {
            searchField.text += event.text;
            searchField.forceActiveFocus();
            event.accepted=true;
        }
    }
    readonly property var prefs: effectBridge.settings.values
    onPrefsChanged: { noteInteraction(); if (prefs && !prefs.inertiaEnabled) { root.velX=0; root.velY=0; } }
    property bool settingsOpen: false
    onSettingsOpenChanged: { if (settingsOpen) { root.velX=0; root.velY=0; } }
    property int settingsPreviewTask: -1
    property bool pointerInside: false
    property point hoverPosition: Qt.point(0,0)
    readonly property int highlightedTask: searchQuery.trim() ? selectedTask : settingsOpen ? settingsPreviewTask : (keyboardMode ? selectedTask : hoveredTask)
    readonly property var hoverPolygon: {
        const revision = root.projectionRevision;
        const face = polyGeometry.faces[highlightedTask];
        if (!face || root.dragging) return [];
        return face.vertices.map(p => scene.mapFrom3DScene(body.mapPositionToScene(p)));
    }
    function updateHover() {
        if (!root.pointerInside || root.dragging || root.keyboardMode) return;
        root.hoveredTask = root.hitFace(root.hoverPosition.x, root.hoverPosition.y);
        if (root.settingsOpen && root.hoveredTask >= 0) root.settingsPreviewTask = root.hoveredTask;
    }
    property int projectionRevision: 0
    // Projection methods are imperative: refresh after scene synchronization
    // and while rotation changes, including the first rendered frame.
    Timer {
        interval: 33
        repeat: true
        running: root.visible
        onTriggered: {
            root.projectionRevision = (root.projectionRevision + 1) % 1000000;
            root.updateHover();
        }
    }
    property quaternion orientation: ScreenRotation.initialOrientation()
    function applyScreenRotation(pitchStep, yawStep) {
        root.orientation = ScreenRotation.rotate(root.orientation,pitchStep,yawStep);
    }
    property point lastPos: Qt.point(0, 0)
    property bool dragging: false
    property var dragSamples: []
    function sampleDrag(x,y) {
        const now=Date.now();
        let samples=dragSamples.filter(s => now-s.t<=100);
        samples.push({t:now,x:x,y:y});
        dragSamples=samples;
        if (samples.length>1) {
            const first=samples[0];
            const scale=16/Math.max(16,now-first.t)*0.29*prefs.rotationSpeed;
            velX=(y-first.y)*scale; velY=(x-first.x)*scale;
        }
    }
    property real velX: 0
    property real velY: 0
    property int hoveredTask: -1
    readonly property bool facePreviewsEnabled: prefs.livePreviews && prefs.previewMode !== "title"
    readonly property real rimInset: prefs.rimPercent / 100
    readonly property real previewInset: (prefs.rimPercent + prefs.edgePercent) / 100
    readonly property int faceCount: Math.max(0, Math.min(64, nativeWindowCount))
    function taskFaceIndex(task) { return task; }
    function faceTaskIndex(face) { return face < nativeWindowCount ? face : -1; }
    readonly property color accent: prefs.borderColor
    readonly property color activeGlowColor: prefs.glowStyle==="sunlight" ? "#ffe6a6" : prefs.glowStyle==="ice" ? "#a8edff" : prefs.hoverColor

    // Geometry, label anchors and picking share the same polygon definitions.
    Rectangle { anchors.fill: parent; color: root.prefs.backgroundColor }
    Image {
        objectName: "customBackground"
        anchors.fill: parent
        visible: root.prefs.backgroundEnabled && root.prefs.backgroundImage.length>0
        source: root.prefs.backgroundImage
        asynchronous: true
        cache: false
        sourceSize: Qt.size(4096,4096)
        fillMode: root.prefs.backgroundMode==="fit" ? Image.PreserveAspectFit : root.prefs.backgroundMode==="stretch" ? Image.Stretch : Image.PreserveAspectCrop
        opacity: root.prefs.backgroundOpacity/100
    }
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.width: 1
        border.color: "#32121d"
    }
    View3D {
        id: scene
        anchors.fill: parent
        camera: PerspectiveCamera {
            position: Qt.vector3d(0, 0, root.cameraDistance)
            clipNear: 1
            clipFar: 3000
        }
        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Transparent
            clearColor: root.prefs.backgroundColor
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }
        // Broad, low-contrast neutral fill from both sides keeps the mesh readable
        // through rotation; opposing warm lights keep the red tint visible.
        DirectionalLight { eulerRotation.x: -32; eulerRotation.y: -28; brightness: 0.75; color: "#d9dbe1" }
        DirectionalLight { eulerRotation.x: 30; eulerRotation.y: 145; brightness: 0.62; color: "#bbb4bc" }
        DirectionalLight { eulerRotation.x: -20; eulerRotation.y: 65; brightness: 0.50; color: "#ff5265" }
        DirectionalLight { eulerRotation.x: 24; eulerRotation.y: -115; brightness: 0.42; color: "#ff5265" }
        Model {
            id: body
            geometry: PrismGeometry {
                id: polyGeometry
                taskCount: root.faceCount
            }
            materials: PrincipledMaterial {
                baseColor: root.prefs.rimColor
                metalness: root.prefs.glassFrames ? 0.65 : 0
                roughness: root.prefs.glassFrames ? 0.24 : 0.85
                cullMode: Material.NoCulling
            }
            rotation: root.orientation
            Repeater3D {
                model: root.nativeWindowCount
                delegate: Node {
                    id: taskSurface
                    required property int index
                    readonly property var face: polyGeometry.faces[index] || ({})
                    Model {
                        geometry: FaceGeometry {
                            face: taskSurface.face
                            inset: root.rimInset
                            offset: 0.12
                            twoSided: root.nativeWindowCount < 5
                        }
                        materials: DefaultMaterial {
                            diffuseColor: root.prefs.glowEnabled && root.highlightedTask === taskSurface.index ? root.activeGlowColor : root.accent
                            lighting: DefaultMaterial.NoLighting
                            cullMode: Material.BackFaceCulling
                        }
                    }
                    Model {
                        geometry: FaceGeometry {
                            face: taskSurface.face
                            inset: root.previewInset
                            offset: 0.20
                            twoSided: root.nativeWindowCount < 5
                        }
                        materials: DefaultMaterial {
                            lighting: DefaultMaterial.NoLighting
                            cullMode: Material.BackFaceCulling
                            diffuseMap: Texture {
                                sourceItem: FacePreview {
                                    face: taskSurface.face
                                    preferences: root.prefs
                                    active: root.facePreviewsEnabled
                                    resolution: root.facePreviewsEnabled ? root.prefs.previewSize : 256
                                    windowId: String(root.nativeWindowIds[taskSurface.index] || "")
                                    title: String(root.nativeWindowTitles[taskSurface.index] || "Window closed")
                                }
                                minFilter: Texture.Linear
                                magFilter: Texture.Linear
                            }
                        }
                    }
                }
            }
        }
    }

    // Intersect actual polygons. Open rings have no cap polygons, so rays
    // can pass through their openings and select the visible inner surface.
    function hitFace(px, py) {
        if (nativeWindowCount <= 0) return -1;
        const near = body.mapPositionFromScene(scene.mapTo3DScene(Qt.vector3d(px, py, 1)));
        const far = body.mapPositionFromScene(scene.mapTo3DScene(Qt.vector3d(px, py, 1000)));
        const direction = far.minus(near);
        let bestT = Infinity, bestFace = -1;
        const faces = polyGeometry.faces;
        for (let i = 0; i < faces.length; ++i) {
            const face = faces[i];
            const denominator = face.normal.dotProduct(direction);
            if (Math.abs(denominator) < 1e-8) continue;
            const t = face.normal.dotProduct(face.center.minus(near)) / denominator;
            if (t <= 0 || t >= bestT) continue;
            const point = near.plus(direction.times(t));
            let inside = true;
            for (let j = 0; j < face.vertices.length; ++j) {
                const a = face.vertices[j];
                const b = face.vertices[(j + 1) % face.vertices.length];
                if (b.minus(a).crossProduct(point.minus(a)).dotProduct(face.normal) < -0.01) {
                    inside = false;
                    break;
                }
            }
            if (inside) { bestT = t; bestFace = i; }
        }
        return bestFace < 0 ? -1 : faceTaskIndex(bestFace);
    }

    readonly property var visibleFacePolygons: {
        const revision=root.projectionRevision;
        const pose=root.orientation;
        const camera=body.mapPositionFromScene(Qt.vector3d(0,0,root.cameraDistance));
        const result=[];
        for(let i=0;i<polyGeometry.faces.length;i++) {
            const face=polyGeometry.faces[i];
            const center=scene.mapFrom3DScene(body.mapPositionToScene(face.center));
            const facing=root.nativeWindowCount<5 ? root.hitFace(center.x,center.y)===i : face.normal.dotProduct(camera.minus(face.center))>0;
            if(!facing || center.z<=0)continue;
            const points=face.vertices.map(p=>scene.mapFrom3DScene(body.mapPositionToScene(p)));
            if(points.every(p=>p.z>0 && Number.isFinite(p.x) && Number.isFinite(p.y)))result.push(points);
        }
        return result;
    }
    FrameAtmosphere {
        objectName: "referenceFrames"
        anchors.fill: parent
        visible: root.prefs.glassFrames
        polygons: root.visibleFacePolygons
        tint: root.prefs.frameGlowColor
        style: root.prefs.frameGlowStyle
        spread: root.prefs.frameGlowSpread
        strength: root.prefs.frameGlowStrength/100
    }

    HoverGlow {
        objectName: "faceHoverGlow"
        z: 1
        points: root.prefs.glowEnabled ? root.hoverPolygon : []
        glowColor: root.activeGlowColor
        style: root.prefs.glowStyle
        radius: root.prefs.glowRadius
        strength: root.prefs.glowStrength
    }

    // Labels are projected from their real face centers into screen space.
    // Keeping text in Qt Quick (instead of face textures) maintains legibility
    // at oblique angles and requires no additional window-preview rendering.
    // On open rings, picking determines which inner/outer surfaces are visible.
    Repeater {
        model: root.nativeWindowCount
        delegate: Item {
            id: faceLabel
            objectName: "taskFaceLabel"
            required property int index
            readonly property var face: polyGeometry.faces[root.taskFaceIndex(index)]
            readonly property vector3d normal: face ? face.normal : Qt.vector3d(0, 0, 0)
            readonly property vector3d center3D: face ? face.center : Qt.vector3d(0, 0, 0)
            readonly property vector3d projected: {
                const revision = root.projectionRevision;
                return scene.mapFrom3DScene(body.mapPositionToScene(center3D));
            }
            // Use the face normal, not the picking ray, for label visibility.
            // This keeps labels visible even when the ray-picking routine rejects
            // an off-center or perspective-projected screen coordinate.
            readonly property vector3d cameraLocal: {
                const revision = root.projectionRevision;
                return body.mapPositionFromScene(Qt.vector3d(0, 0, root.cameraDistance));
            }
            readonly property real faceNormalDot: normal.dotProduct(cameraLocal.minus(center3D))
            readonly property bool facingCamera: {
                // A panel center can stay at the same screen position while
                // rotating edge-on: picking must still refresh every frame.
                const revision = root.projectionRevision;
                return root.nativeWindowCount < 5
                    ? projected.z > 0 && root.hitFace(projected.x, projected.y) === index
                    : faceNormalDot > 0;
            }
            width: Math.min(root.nativeWindowCount === 5 ? 110 : 210, Math.max(74, 340 / Math.sqrt(root.faceCount)))
            height: 43
            x: projected.x - width / 2
            y: projected.y - height / 2
            visible: (root.prefs.hoverTitles && root.highlightedTask === index) && facingCamera && projected.z > 0 && Number.isFinite(projected.x) && Number.isFinite(projected.y)
            z: 2
            Rectangle {
                anchors.fill: parent
                radius: 7
                color: "#e1111018"
                border.width: 1
                border.color: "#bdef4057"
            }
            Column {
                anchors.fill: parent
                anchors.margins: 5
                spacing: 2
                Text {
                    width: parent.width
                    text: "TASK " + (faceLabel.index + 1)
                    color: "#ff7587"
                    font.bold: true
                    font.pixelSize: 9
                    horizontalAlignment: Text.AlignHCenter
                }
                Text {
                    width: parent.width
                    text: String(root.nativeWindowTitles[faceLabel.index] || "Untitled window")
                    color: "#fff3f5"
                    font.pixelSize: 12
                    font.bold: true
                    elide: Text.ElideMiddle
                    maximumLineCount: 1
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }

    // Wall-clock ticks come from one native timer, independent of which
    // display drives Qt Quick's animation clock or its refresh rate.
    function advanceInertia(milliseconds) {
        if (!prefs.inertiaEnabled || dragging) return;
        if (Math.abs(velX)+Math.abs(velY)<=0.09) { velX=0; velY=0; return; }
        const steps=milliseconds/16;
        const decay=Math.pow(0.96,steps);
        const distance=(1-decay)/(1-0.96);
        applyScreenRotation(velX*distance,velY*distance);
        velX*=decay; velY*=decay;
    }
    Connections {
        target: root.effectBridge
        function onAnimationTick(milliseconds) { root.advanceInertia(milliseconds); }
        function onIdleElapsed(milliseconds) { root.updateIdleElapsed(milliseconds); }
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton | Qt.LeftButton
        hoverEnabled: true
        onWheel: wheel => {
            const steps=wheel.angleDelta.y ? wheel.angleDelta.y/120 : wheel.pixelDelta.y/60;
            if (steps) root.changeZoom(steps);
            wheel.accepted=true;
        }
        onEntered: { root.noteInteraction(); root.pointerInside = true; root.hoverPosition = Qt.point(mouseX,mouseY); root.updateHover(); }
        onExited: { root.pointerInside = false; root.hoveredTask = -1; }
        onPressed: mouse => {
            root.noteInteraction();
            if (mouse.button !== Qt.RightButton) return;
            root.dragging = true
            root.lastPos = Qt.point(mouse.x, mouse.y)
            root.dragSamples=[{t:Date.now(),x:mouse.x,y:mouse.y}];
            root.velX = 0; root.velY = 0
        }
        onPositionChanged: mouse => {
            if (Math.abs(mouse.x-root.hoverPosition.x)+Math.abs(mouse.y-root.hoverPosition.y)>0.75) root.noteInteraction();
            root.keyboardMode = false;
            root.pointerInside = true;
            root.hoverPosition = Qt.point(mouse.x,mouse.y);
            if (!root.dragging) { root.updateHover(); return; }
            root.hoveredTask = -1
            const dx = mouse.x - root.lastPos.x
            const dy = mouse.y - root.lastPos.y
            if (Math.abs(dx)+Math.abs(dy)<0.01) return;
            root.sampleDrag(mouse.x,mouse.y);
            root.applyScreenRotation(dy*0.29*root.prefs.rotationSpeed,dx*0.29*root.prefs.rotationSpeed)
            root.lastPos = Qt.point(mouse.x, mouse.y)
        }
        onReleased: mouse => {
            root.noteInteraction();
            if (mouse.button === Qt.RightButton) {
                root.dragging = false;
                if (root.dragSamples.length && Date.now()-root.dragSamples[root.dragSamples.length-1].t>120) { root.velX=0; root.velY=0; }
                if (!root.prefs.inertiaEnabled) { root.velX=0; root.velY=0; }
                root.updateHover();
            }
        }
        onClicked: mouse => {
            if (mouse.button !== Qt.LeftButton || root.dragging) return;
            const index = root.hitFace(mouse.x, mouse.y);
            if (index >= 0) root.effectBridge.activateWindowId(root.nativeWindowIds[index]);
        }
        onCanceled: { root.dragging = false; root.velX = 0; root.velY = 0 }
    }

    Rectangle {
        objectName: "switcherHeader"
        visible: root.prefs.showHeader
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 55
        color: "#cc10141d"
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#8e2438" }
        Row {
            anchors.left: parent.left; anchors.leftMargin: 22
            anchors.verticalCenter: parent.verticalCenter
            spacing: 18
            Text { text: "POLYSWITCH  /  STAGE 4.14"; color: "#fff0f3"; font.bold: true; font.pixelSize: 17 }
            Text { text: root.nativeWindowCount + " WINDOWS"; color: "#ff5b73"; font.pixelSize: 14 }
            Text { text: "Wheel zoom · RMB rotate · Arrows select · Enter switch · Esc close"; color: "#9fa8b7"; font.pixelSize: 12 }
        }
    }

    C.TextField {
        id: searchField
        objectName: "taskSearchField"
        z: 9
        x: 22; y: root.prefs.showHeader ? 66 : 12
        width: Math.min(390,root.width-(root.prefs.showHeader ? 44 : 160)); height: 40
        placeholderText: "Type to find a task…"
        color: "#fff0f3"
        placeholderTextColor: "#b5a9b4"
        selectByMouse: true
        leftPadding: 12; rightPadding: 52
        background: Rectangle { color: "#ed1c1822"; radius: 7; border.color: searchField.activeFocus ? root.accent : "#79404f" }
        onTextChanged: root.searchQuery=text
        Keys.priority: Keys.BeforeItem
        Keys.onPressed: event => {
            if (event.key===Qt.Key_Up || event.key===Qt.Key_Down || event.key===Qt.Key_Tab || event.key===Qt.Key_Backtab) {
                root.stepSelection(event.key===Qt.Key_Up || event.key===Qt.Key_Backtab ? -1 : 1);
                event.accepted=true;
            } else if (event.key===Qt.Key_Return || event.key===Qt.Key_Enter) {
                root.activateSelection(); event.accepted=true;
            }
        }
        C.ToolButton {
            anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
            width: 44; height: 36
            text: "×"; visible: searchField.text.length>0
            contentItem: Text { text: "×"; color: "#fff0f3"; font.pixelSize: 22; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            onClicked: { searchField.clear(); root.forceActiveFocus(); }
        }
    }
    Text {
        z: 9; x: 26; y: searchField.y+46
        width: searchField.width
        visible: root.searchQuery.trim().length>0
        text: root.searchMatches.length ? root.searchMatches.length+" matches · ↑/↓ browse · Enter switch" : "No matching windows"
        color: root.searchMatches.length ? "#fff0f3" : "#ff778e"
        font.pixelSize: 12
    }

    C.Button {
        anchors.right: parent.right; anchors.rightMargin: 18
        anchors.top: parent.top; anchors.topMargin: 10
        z: 10
        text: "Settings"
        focusPolicy: Qt.NoFocus
        width: 104; height: 34
        contentItem: Text { text: "Settings"; color: "#fff0f3"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        background: Rectangle { color: "#30202b"; radius: 6; border.color: root.accent }
        objectName: "settingsButton"
        onClicked: {
            root.settingsPreviewTask = root.hoveredTask >= 0 ? root.hoveredTask : root.hitFace(root.width/2,root.height/2);
            root.settingsOpen = !root.settingsOpen;
            if (!root.settingsOpen) root.effectBridge.settings.flush();
        }
    }
    SettingsPanel {
        objectName: "settingsPanel"
        visible: root.settingsOpen
        z: 20
        anchors.right: parent.right; anchors.rightMargin: 18
        anchors.top: parent.top; anchors.topMargin: 66
        anchors.bottom: parent.bottom; anchors.bottomMargin: 18
        width: Math.min(370,root.width-36)
        store: root.effectBridge.settings
        onCloseRequested: { root.settingsOpen = false; root.forceActiveFocus(); }
    }
    Text {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Live windows · Hover to highlight · Settings to customize"
        color: "#b5b8c4"; font.pixelSize: 12
    }
}
