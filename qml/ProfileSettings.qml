// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts
ColumnLayout {
    id:profiles
    required property var store
    required property var effectBridge
    readonly property var prefs:store.values
    readonly property var entries:store.presets
    property string selectedId:""
    property string confirmAction:""
    property string status:""
    signal walkthroughRequested()
    spacing:12
    onEntriesChanged: { if(!entries.some(p=>p.id===selectedId))selectedId=entries.length?entries[0].id:""; }
    Text { text:"APPEARANCE PROFILES";color:profiles.prefs.borderColor;font.bold:true }
    Text { text:"Save your look, including theme overrides and the background image. Navigation settings stay unchanged.";color:profiles.prefs.uiMutedColor;wrapMode:Text.Wrap;Layout.fillWidth:true }
    C.TextField {
        id:nameInput;objectName:"profileNameInput";Layout.fillWidth:true;placeholderText:"Name this appearance";maximumLength:80
        color:profiles.prefs.uiTextColor;placeholderTextColor:profiles.prefs.uiMutedColor;selectedTextColor:profiles.prefs.uiHighlightTextColor;selectionColor:profiles.prefs.borderColor
        background:Rectangle { color:profiles.prefs.uiBaseColor;radius:5;border.color:profiles.prefs.uiBorderColor }
    }
    PlasmaButton {
        objectName:"saveProfileButton";Layout.fillWidth:true;prefs:profiles.prefs;artwork:profiles.store.themeInfo.artwork;text:"Save as new profile";enabled:nameInput.text.trim().length>0
        onClicked:{const id=profiles.store.savePreset(nameInput.text);if(id){profiles.selectedId=id;profiles.status="Profile saved.";nameInput.clear();}}
    }
    SettingsComboBox {
        objectName:"profilesCombo";Layout.fillWidth:true;model:profiles.entries;textRole:"name";enabled:profiles.entries.length>0
        currentIndex:profiles.entries.findIndex(p=>p.id===profiles.selectedId)
        onActivated:index=>{profiles.selectedId=profiles.entries[index].id;profiles.confirmAction="";profiles.status="";}
    }
    RowLayout {
        PlasmaButton { objectName:"applyProfileButton";Layout.fillWidth:true;prefs:profiles.prefs;artwork:profiles.store.themeInfo.artwork;text:"Apply";enabled:profiles.selectedId!=="";onClicked:if(profiles.store.applyPreset(profiles.selectedId))profiles.status="Profile applied. Undo is available below." }
        PlasmaButton { objectName:"exportProfileButton";Layout.fillWidth:true;prefs:profiles.prefs;artwork:profiles.store.themeInfo.artwork;text:"Export…";enabled:profiles.selectedId!=="";onClicked:profiles.effectBridge.exportProfile(profiles.selectedId) }
        PlasmaButton { Layout.fillWidth:true;prefs:profiles.prefs;artwork:profiles.store.themeInfo.artwork;text:"Delete…";enabled:profiles.selectedId!=="";onClicked:profiles.confirmAction="delete" }
    }
    PlasmaButton { objectName:"importProfileButton";Layout.fillWidth:true;prefs:profiles.prefs;artwork:profiles.store.themeInfo.artwork;text:"Import profile…";onClicked:profiles.effectBridge.importProfile() }
    Text { text:profiles.status;visible:text.length>0;color:profiles.prefs.uiMutedColor;wrapMode:Text.Wrap;Layout.fillWidth:true }
    Text { text:profiles.store.presetError;visible:text.length>0;color:profiles.prefs.uiErrorColor;wrapMode:Text.Wrap;Layout.fillWidth:true }
    Text { text:"RECOVERY & HELP";color:profiles.prefs.borderColor;font.bold:true }
    PlasmaButton { objectName:"replaySetupButton";Layout.fillWidth:true;prefs:profiles.prefs;artwork:profiles.store.themeInfo.artwork;text:"Show getting-started guide";onClicked:profiles.walkthroughRequested() }
    PlasmaButton { objectName:"resetAppearanceButton";Layout.fillWidth:true;prefs:profiles.prefs;artwork:profiles.store.themeInfo.artwork;text:"Reset appearance…";onClicked:profiles.confirmAction="reset" }
    PlasmaButton { objectName:"undoAppearanceButton";Layout.fillWidth:true;prefs:profiles.prefs;artwork:profiles.store.themeInfo.artwork;text:"Undo last appearance change";enabled:profiles.store.canUndoAppearance;onClicked:if(profiles.store.undoAppearance())profiles.status="Previous appearance restored." }
    PlasmaButton { objectName:"disablePolySwitchButton";Layout.fillWidth:true;prefs:profiles.prefs;artwork:profiles.store.themeInfo.artwork;text:"Disable PolySwitch…";onClicked:profiles.confirmAction="disable" }
    PlasmaButton { objectName:"recoveryToolsButton";Layout.fillWidth:true;prefs:profiles.prefs;artwork:profiles.store.themeInfo.artwork;text:"Open rollback tools & instructions";onClicked:profiles.effectBridge.openRecoveryTools() }
    Text { text:"Rollback tools restore the previous installed plugin. Save your work and sign out/in normally afterward. Never restart KWin manually.";color:profiles.prefs.uiMutedColor;wrapMode:Text.Wrap;Layout.fillWidth:true }
    Rectangle {
        visible:profiles.confirmAction!=="";Layout.fillWidth:true;implicitHeight:confirmation.implicitHeight+24;color:profiles.prefs.uiBaseColor;radius:6;border.color:profiles.prefs.uiBorderColor
        ColumnLayout {
            id:confirmation;anchors.fill:parent;anchors.margins:12
            Text { Layout.fillWidth:true;wrapMode:Text.Wrap;color:profiles.prefs.uiTextColor;text:profiles.confirmAction==="delete"?"Delete the selected profile? Your current look stays unchanged.":profiles.confirmAction==="reset"?"Reset appearance to defaults? A recovery copy will be saved. Your profiles, guide completion and navigation settings stay unchanged.":"Disable PolySwitch now and at login? To use it again, sign out/in normally and enable it in Desktop Effects." }
            RowLayout {
                PlasmaButton { objectName:"confirmRecoveryButton";prefs:profiles.prefs;artwork:profiles.store.themeInfo.artwork;text:"Confirm";onClicked:{if(profiles.confirmAction==="delete")profiles.store.deletePreset(profiles.selectedId);else if(profiles.confirmAction==="reset")profiles.store.resetAppearance();else profiles.effectBridge.disablePolySwitch();profiles.confirmAction="";} }
                PlasmaButton { prefs:profiles.prefs;artwork:profiles.store.themeInfo.artwork;text:"Cancel";onClicked:profiles.confirmAction="" }
            }
        }
    }
}
