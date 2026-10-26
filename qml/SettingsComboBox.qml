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
        color: control.palette.text
        verticalAlignment: Text.AlignVCenter
        leftPadding: 10; rightPadding: 30; clip: true
    }
    background: Rectangle { color: control.palette.button; radius: 5; border.color: control.activeFocus ? control.palette.text : control.palette.mid }
    indicator: Text { x: control.width-width-10; y: (control.height-height)/2; text: "▾"; color: control.palette.text; font.pixelSize: 16 }
    popup: C.Popup {
        popupType: C.Popup.Item
        y: control.height+3
        width: control.width
        height: Math.min(control.count*38+2,320)
        padding: 1
        closePolicy: C.Popup.CloseOnEscape | C.Popup.CloseOnPressOutsideParent
        background: Rectangle { color: control.palette.base; border.color: control.palette.mid; radius: 4 }
        contentItem: C.ScrollView {
            clip:true
            contentWidth:availableWidth
            Column {
            Repeater {
                model: control.model
                Rectangle {
                    id: choice
                    required property var modelData
                    required property int index
                    objectName: "popupChoice"+index
                    readonly property string text: String(modelData[control.textRole] || modelData)
                    width: control.width-2; height: 38
                    color: rowMouse.containsMouse || control.highlightedIndex===index ? Qt.darker(control.palette.button,1.2) : control.palette.base
                    Text { anchors.fill: parent; anchors.margins: 8; text: choice.text; color: control.palette.text; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent; hoverEnabled: true
                        onClicked: { control.activated(choice.index); control.popup.close(); }
                    }
                }
            }
            }
        }
    }
}
