// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
import QtQuick.Controls as C
C.Button {
    id:button
    required property var prefs
    required property var artwork
    implicitHeight:38
    contentItem:Text { text:button.text;font:button.font;color:button.enabled?button.prefs.uiTextColor:button.prefs.uiMutedColor;horizontalAlignment:Text.AlignHCenter;verticalAlignment:Text.AlignVCenter;elide:Text.ElideRight }
    background:Item {
        implicitWidth:120;implicitHeight:38
        Rectangle { anchors.fill:parent;color:button.down?Qt.darker(button.prefs.uiButtonColor,1.2):button.prefs.uiButtonColor;border.color:button.prefs.uiBorderColor;radius:5 }
        PlasmaFrame { anchors.fill:parent;prefs:button.prefs;assetPath:button.artwork.button || "";framePrefix:button.down?"pressed":button.hovered?"hover":"normal";surfaceColor:button.prefs.uiButtonColor;opacity:0.22;revision:button.artwork._revision || "" }
    }
}
