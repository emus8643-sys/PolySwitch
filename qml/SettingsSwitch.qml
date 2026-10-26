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
        color: control.checked ? Qt.darker(control.store.values.borderColor,1.6) : control.store.values.uiButtonColor
        border.color: control.activeFocus ? control.store.values.uiTextColor : control.store.values.uiBorderColor
        Rectangle {
            width: 18; height: 18; radius: 9
            x: control.checked ? parent.width-width-3 : 3
            y: 3
            color: control.checked ? "#ffffff" : control.store.values.uiTextColor
        }
    }
    contentItem: C.Label {
        text: control.text
        color: control.store.values.uiTextColor
        leftPadding: control.indicator.width + control.spacing
        verticalAlignment: Text.AlignVCenter
    }
}
