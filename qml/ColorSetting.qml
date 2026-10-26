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
    readonly property var prefs: store.values
    spacing: 6
    C.Label { color: field.store.values.uiTextColor; text: field.label; font.bold: true }
    RowLayout {
        Rectangle { width: 24; height: 24; radius: 5; color: field.store.values[field.keyName]; border.color: "#81747d" }
        C.TextField {
            color:field.prefs.uiTextColor
            selectionColor:field.prefs.borderColor
            selectedTextColor:field.prefs.uiHighlightTextColor
            background:Rectangle { color:field.prefs.uiBaseColor;radius:4;border.color:field.prefs.uiBorderColor }

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
    C.CheckBox {
        id: overrideControl
        indicator: Rectangle {
            width:20;height:20;radius:4;x:overrideControl.leftPadding;y:(overrideControl.height-height)/2
            color:field.prefs.uiBaseColor;border.color:overrideControl.checked?field.prefs.borderColor:field.prefs.uiBorderColor
            Text { anchors.centerIn:parent;text:overrideControl.checked?"✓":"";color:field.prefs.borderColor;font.pixelSize:16 }
        }
        contentItem: Text { text:overrideControl.text;color:field.prefs.uiTextColor;leftPadding:overrideControl.indicator.width+overrideControl.spacing;verticalAlignment:Text.AlignVCenter;font:overrideControl.font }

        palette.windowText:field.prefs.uiTextColor
        palette.text:field.prefs.uiTextColor
        palette.base:field.prefs.uiBaseColor
        palette.button:field.prefs.uiButtonColor
        palette.highlight:field.prefs.borderColor
        palette.highlightedText:field.prefs.uiHighlightTextColor

        visible:field.store.values.followKdeTheme
        text:"Override KDE color"
        checked:field.store.values.themeMode==="mixed" && field.store.values.themeOverrides.indexOf(field.keyName)>=0
        onToggled:field.store.setThemeOverride(field.keyName,checked)
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
                border.color: field.store.values[field.keyName] === modelData ? field.prefs.uiTextColor : field.prefs.uiBorderColor
                MouseArea { anchors.fill: parent; onClicked: field.store.setValue(field.keyName, parent.modelData) }
            }
        }
    }
}
