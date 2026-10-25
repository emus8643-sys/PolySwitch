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
    spacing: 6
    C.Label { color: "#fff0f3"; text: field.label; font.bold: true }
    RowLayout {
        Rectangle { width: 24; height: 24; radius: 5; color: field.store.values[field.keyName]; border.color: "#81747d" }
        C.TextField {
            objectName: field.keyName + "Field"
            Layout.fillWidth: true
            text: String(field.store.values[field.keyName])
            placeholderText: "#RRGGBB"
            validator: RegularExpressionValidator { regularExpression: /#[0-9a-fA-F]{6}/ }
            onEditingFinished: {
                if (acceptableInput) field.store.setValue(field.keyName, text);
                text = Qt.binding(() => String(field.store.values[field.keyName]));
            }
        }
    }
    RowLayout {
        spacing: 12
        Repeater {
            model: ["#ff163b", "#ef334d", "#a95cff", "#419aff", "#21d6a1", "#17131b"]
            delegate: Rectangle {
                required property string modelData
                width: 26; height: 26; radius: 13
                color: modelData
                border.width: field.store.values[field.keyName] === modelData ? 3 : 1
                border.color: field.store.values[field.keyName] === modelData ? "white" : "#807780"
                MouseArea { anchors.fill: parent; onClicked: field.store.setValue(field.keyName, parent.modelData) }
            }
        }
    }
}
