// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
C.Popup {
    id:setup
    objectName:"firstRunSetup"
    required property var store
    readonly property var prefs:store.values
    signal finished()
    property int step:0
    width:Math.min(560,parent.width-32)
    height:Math.min(430,parent.height-32)
    anchors.centerIn:parent
    modal:true;focus:true;closePolicy:C.Popup.NoAutoClose
    padding:26
    onOpened:step=0
    function finishSetup(){store.setValue("onboardingCompleted",true);store.flush();close();finished();}
    background:Rectangle { color:setup.prefs.uiPanelColor;radius:14;border.color:setup.prefs.borderColor }
    C.Overlay.modal:Rectangle { color:"#b3000000" }
    contentItem:ColumnLayout {
        spacing:18
        Text { text:"Welcome to PolySwitch";font.pixelSize:24;font.bold:true;color:setup.prefs.uiTextColor;Layout.fillWidth:true;wrapMode:Text.Wrap }
        Text { text:(setup.step+1)+" / 3";color:setup.prefs.borderColor }
        Text {
            Layout.fillWidth:true;Layout.fillHeight:true
            wrapMode:Text.Wrap;font.pixelSize:16;color:setup.prefs.uiTextColor
            text:setup.step===0?"Your windows become faces on a live 3D shape.\n\nOpen or close PolySwitch with Meta+Shift+P. Right-drag to rotate, and use the wheel or + / − to zoom. Escape closes it safely.":setup.step===1?"Use the arrow keys or Tab to select a face, then Enter to switch to its window. You can also click a face.\n\nType a window title to search. Auto-upright can smoothly roll the nearest task toward you after your chosen idle delay.":"Open Settings to change previews, colors, glow and backgrounds. Save your look as a profile, or import and export profiles with their background images.\n\nRecovery offers an undoable appearance reset, a disable button and tools for restoring the previous build. You can replay this guide from Settings."
        }
        RowLayout {
            PlasmaButton { objectName:"setupSkipButton";prefs:setup.prefs;artwork:setup.store.themeInfo.artwork;text:"Skip guide";onClicked:setup.finishSetup() }
            Item { Layout.fillWidth:true }
            PlasmaButton { prefs:setup.prefs;artwork:setup.store.themeInfo.artwork;text:"Back";visible:setup.step>0;onClicked:setup.step-- }
            PlasmaButton { objectName:"setupNextButton";prefs:setup.prefs;artwork:setup.store.themeInfo.artwork;text:setup.step<2?"Next":"Get started";onClicked:setup.step<2?setup.step++:setup.finishSetup() }
        }
    }
}
