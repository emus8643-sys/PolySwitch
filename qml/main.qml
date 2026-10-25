// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick

Item {
    id: host
    required property var effectBridge
    required property string outputName
    readonly property bool primary: outputName === effectBridge.primaryOutputName
    focus: primary
    visible: primary
    enabled: primary
    Loader {
        focus: true
        anchors.fill: parent
        active: host.primary
        onLoaded: Qt.callLater(function() { if (item) item.forceActiveFocus(); })
        sourceComponent: Component {
            Switcher { effectBridge: host.effectBridge }
        }
    }
}
