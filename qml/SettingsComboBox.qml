// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
import QtQuick.Controls as C

C.ComboBox {
    id: control
    implicitHeight: 38
    palette.highlight: "#555862"
    palette.highlightedText: "#ffffff"
    contentItem: TextInput {
        readOnly: true; enabled: false
        text: control.displayText
        color: "#f2f2f4"
        verticalAlignment: Text.AlignVCenter
        leftPadding: 10; rightPadding: 30; clip: true
    }
    background: Rectangle { color: "#303137"; radius: 5; border.color: control.activeFocus ? "#b5b7c1" : "#656770" }
    indicator: Text { x: control.width-width-10; y: (control.height-height)/2; text: "▾"; color: "#dedee5"; font.pixelSize: 16 }
    popup: C.Popup {
        popupType: C.Popup.Item
        y: control.height+3
        width: control.width
        height: control.count*38+2
        padding: 1
        closePolicy: C.Popup.CloseOnEscape | C.Popup.CloseOnPressOutsideParent
        background: Rectangle { color: "#292a30"; border.color: "#737580"; radius: 4 }
        contentItem: Column {
            Repeater {
                model: control.model
                Rectangle {
                    id: choice
                    required property var modelData
                    required property int index
                    objectName: "popupChoice"+index
                    readonly property string text: String(modelData[control.textRole] || modelData)
                    width: control.width-2; height: 38
                    color: rowMouse.containsMouse || control.highlightedIndex===index ? "#555862" : "#292a30"
                    Text { anchors.fill: parent; anchors.margins: 8; text: choice.text; color: "#f2f2f4"; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent; hoverEnabled: true
                        onClicked: { control.currentIndex=choice.index; control.activated(choice.index); control.popup.close(); }
                    }
                }
            }
        }
    }
}
