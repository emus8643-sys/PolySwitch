// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
import QtQuick.Controls as C
C.Switch {
    id: control
    required property var store
    required property string keyName
    checked: store.values[keyName]
    onToggled: store.setValue(keyName, checked)
    indicator: Rectangle {
        objectName: control.keyName + "Track"
        implicitWidth: 42; implicitHeight: 24
        x: control.leftPadding
        y: (control.height-height)/2
        radius: 12
        color: control.checked ? "#8f2338" : "#55565e"
        border.color: control.activeFocus ? "#fff0f3" : (control.checked ? "#c43b53" : "#777983")
        Rectangle {
            width: 18; height: 18; radius: 9
            x: control.checked ? parent.width-width-3 : 3
            y: 3
            color: control.checked ? "#ffe5e9" : "#dedee3"
        }
    }
    contentItem: C.Label {
        text: control.text
        color: "#fff0f3"
        leftPadding: control.indicator.width + control.spacing
        verticalAlignment: Text.AlignVCenter
    }
}
