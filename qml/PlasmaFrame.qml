// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
Loader {
    id:frame
    required property var prefs
    property string assetPath:""
    property string framePrefix:""
    property color surfaceColor:prefs.uiPanelColor
    property string revision:""
    active:prefs.plasmaArtwork && assetPath.length>0
    source:active?"PlasmaSvgFrame.qml":""
    onRevisionChanged: {
        if(!active)return;
        active=false;
        Qt.callLater(function(){frame.active=Qt.binding(()=>frame.prefs.plasmaArtwork && frame.assetPath.length>0);});
    }
}
