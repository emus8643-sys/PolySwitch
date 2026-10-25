// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
ColumnLayout {
    id: field
    required property string label
    required property string keyName
    required property var store
    property real from: 0
    property real to: 100
    property real step: 1
    property string suffix: ""
    RowLayout {
        C.Label { color: "#fff0f3"; text: field.label; Layout.fillWidth: true }
        C.Label { text: Number(field.store.values[field.keyName]).toFixed(field.step < 0.1 ? 2 : field.step < 1 ? 1 : 0) + field.suffix; color: "#ff778e" }
    }
    C.Slider {
        objectName: field.keyName + "Slider"
        Layout.fillWidth: true
        from: field.from; to: field.to; stepSize: 0
        value: Number(field.store.values[field.keyName])
        onMoved: field.store.setValue(field.keyName, Math.round(value / field.step) * field.step)
    }
}
