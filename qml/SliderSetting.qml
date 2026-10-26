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
        C.Label { color: field.store.values.uiTextColor; text: field.label; Layout.fillWidth: true }
        C.Label { text: Number(field.store.values[field.keyName]).toFixed(field.step < 0.1 ? 2 : field.step < 1 ? 1 : 0) + field.suffix; color: field.store.values.borderColor }
    }
    C.Slider {
        id:slider
        background: Rectangle {
            x:slider.leftPadding;y:slider.topPadding+slider.availableHeight/2-height/2
            implicitWidth:200;implicitHeight:4;width:slider.availableWidth;height:4;radius:2;color:field.store.values.uiBorderColor
            Rectangle { width:slider.visualPosition*parent.width;height:parent.height;radius:2;color:field.store.values.borderColor }
        }
        handle: Rectangle {
            x:slider.leftPadding+slider.visualPosition*(slider.availableWidth-width);y:slider.topPadding+(slider.availableHeight-height)/2
            width:18;height:18;radius:9;color:field.store.values.uiTextColor;border.color:field.store.values.uiBorderColor
        }

        palette.highlight:field.store.values.borderColor
        palette.button:field.store.values.uiButtonColor
        palette.base:field.store.values.uiBaseColor
        palette.window:field.store.values.uiPanelColor
        palette.mid:field.store.values.uiBorderColor
        palette.dark:field.store.values.uiBorderColor

        objectName: field.keyName + "Slider"
        Layout.fillWidth: true
        from: field.from; to: field.to; stepSize: 0
        value: Number(field.store.values[field.keyName])
        onMoved: field.store.setValue(field.keyName, Math.round(value / field.step) * field.step)
    }
}
