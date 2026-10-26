// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts

C.Pane {
    id: panel
    required property var store
    signal closeRequested()
    property var backgroundListing: ({entries:[]})
    function browseBackground(folder) {
        backgroundListing=store.backgroundFolder(folder || "");
        backgroundPath.text=backgroundListing.path;
    }
    padding: 18
    palette.window: "#14131c"
    palette.text: "#fff0f3"
    palette.windowText: "#fff0f3"
    palette.buttonText: "#fff0f3"
    palette.button: "#30242d"
    palette.base: "#211c26"
    palette.highlight: "#ef334d"
    background: Rectangle { color: "#f514131c"; radius: 12; border.color: "#79404f" }
    // Swallow unused panel clicks, keeping the preview below from activating.
    MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons; onWheel: wheel => wheel.accepted=true }
    ColumnLayout {
        anchors.fill: parent
        RowLayout {
            C.Label { color: "#fff0f3"; text: "PolySwitch settings"; font.pixelSize: 20; font.bold: true; Layout.fillWidth: true }
            C.ToolButton { text: "Close"; Layout.preferredWidth: 64; Layout.preferredHeight: 30
                background: Rectangle { color: "#30202b"; radius: 5; border.color: "#79404f" }
                contentItem: C.Label { text: "Close"; color: "#fff0f3"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter } onClicked: { panel.store.flush(); panel.closeRequested(); } }
        }
        C.Label { text: "Preview changes live · Saved automatically"; color: "#bfb3bd"; font.pixelSize: 12 }
        C.ScrollView {
            id: settingsScroll
            C.ScrollBar.vertical.policy: C.ScrollBar.AlwaysOn
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            contentHeight: settingsColumn.implicitHeight
            ColumnLayout {
                id: settingsColumn
                width: settingsScroll.availableWidth
                spacing: 16
                C.Label { text: "HEADER AND BACKGROUND"; color: "#ff778e"; font.bold: true }
                SettingsSwitch { text: "Show header"; keyName: "showHeader"; store: panel.store; objectName:"showHeaderSwitch" }
                C.Button { text: "Import background image…"; objectName:"importBackgroundButton"; Layout.fillWidth:true; onClicked: { panel.browseBackground(""); backgroundPicker.open(); } }
                SettingsSwitch { text: "Show imported background"; keyName:"backgroundEnabled";store:panel.store }
                SettingsComboBox {
                    objectName:"backgroundModeCombo";Layout.fillWidth:true
                    model:[{text:"Background: fill screen",value:"fill"},{text:"Background: fit entire image",value:"fit"},{text:"Background: stretch",value:"stretch"}]
                    textRole:"text"
                    currentIndex:model.findIndex(option=>option.value===panel.store.values.backgroundMode)
                    onActivated:index=>panel.store.setValue("backgroundMode",model[index].value)
                }
                SliderSetting { Layout.fillWidth:true;label:"Background opacity";keyName:"backgroundOpacity";store:panel.store;from:0;to:100;suffix:"%" }
                C.Button { text:"Remove background";enabled:panel.store.values.backgroundImage.length>0;onClicked:panel.store.setValue("backgroundImage","") }
                C.Label { Layout.fillWidth:true;wrapMode:Text.WordWrap;color:"#bfb3bd";text:"Imported images are copied into PolySwitch’s local storage and persist after restarting. Settings remains accessible with the header hidden." }
                C.Label { text: "FACE CONTENT"; color: "#ff778e"; font.bold: true }
                C.Label { color: "#fff0f3"; text: "Window display mode" }
                SettingsComboBox {
                    id: displayMode
                    implicitHeight: 38
                    objectName: "previewModeCombo"
                    Layout.fillWidth: true
                    model: [{text:"Fit entire window inside face",value:"fit"},{text:"Fill face (crop edges)",value:"fill"},{text:"Title only",value:"title"}]
                    textRole: "text"
                    currentIndex: model.findIndex(option => option.value === panel.store.values.previewMode)
                    onActivated: index => panel.store.setValue("previewMode",model[index].value)
                }
                SliderSetting { Layout.fillWidth: true; label: "Zoom"; keyName: "zoomPercent"; store: panel.store; from: 50; to: 200; step: 1; suffix: "%" }
                C.Button { text: "Reset zoom (100%)"; onClicked: panel.store.setValue("zoomPercent",100) }
                SettingsSwitch { text: "Auto-upright after 1 second"; keyName: "autoUpright"; store: panel.store }
                SettingsSwitch { text: "Keep spinning after drag"; keyName: "inertiaEnabled"; store: panel.store }
                C.Label { text: "HOVER HIGHLIGHT"; color: "#ff778e"; font.bold: true }
                SettingsComboBox {
                    objectName: "glowStyleCombo"
                    Layout.fillWidth: true
                    model: [{text:"Neon (custom color)",value:"neon"},{text:"Sunlight · white and gold",value:"sunlight"},{text:"Ice · white and blue",value:"ice"},{text:"Breathing neon",value:"pulse"}]
                    textRole: "text"
                    currentIndex: model.findIndex(option => option.value === panel.store.values.glowStyle)
                    onActivated: index => panel.store.setValue("glowStyle",model[index].value)
                }
                SettingsSwitch { text: "Bright glow on hover"; keyName: "glowEnabled"; store: panel.store }
                ColorSetting { Layout.fillWidth: true; label: "Custom neon color"; keyName: "hoverColor"; store: panel.store }
                SliderSetting { Layout.fillWidth: true; label: "Glow strength"; keyName: "glowStrength"; store: panel.store; from: 0; to: 100; suffix: "%" }
                SliderSetting { Layout.fillWidth: true; label: "Glow spread"; keyName: "glowRadius"; store: panel.store; from: 6; to: 48; suffix: " px" }
                C.Label { text: "APPEARANCE"; color: "#ff778e"; font.bold: true }
                SettingsSwitch { text: "Glass frames and ambient edges"; keyName: "glassFrames"; store: panel.store }
                ColorSetting { Layout.fillWidth: true; label: "Passive glow color"; keyName: "frameGlowColor"; store: panel.store }
                SettingsComboBox {
                    objectName: "frameGlowStyleCombo"
                    Layout.fillWidth: true
                    model: [{text:"Passive: sharp neon",value:"neon"},{text:"Passive: soft bloom",value:"soft"},{text:"Passive: breathing neon",value:"pulse"},{text:"Passive: radiant edges",value:"radiant"}]
                    textRole: "text"
                    currentIndex: model.findIndex(option=>option.value===panel.store.values.frameGlowStyle)
                    onActivated: index=>panel.store.setValue("frameGlowStyle",model[index].value)
                }
                SliderSetting { Layout.fillWidth: true; label: "Passive glow spread"; keyName: "frameGlowSpread"; store: panel.store; from:6;to:64;suffix:" px" }
                SliderSetting { Layout.fillWidth: true; label: "Ambient frame glow"; keyName: "frameGlowStrength"; store: panel.store; from: 0; to: 100; suffix: "%" }
                SliderSetting { Layout.fillWidth: true; label: "Glass reflection"; keyName: "glassSheen"; store: panel.store; from: 0; to: 40; suffix: "%" }
                C.Button {
                    text: "Apply red glass reference look"
                    onClicked: {
                        const values={glassFrames:true,frameGlowStrength:55,frameGlowColor:"#ff183d",frameGlowStyle:"neon",frameGlowSpread:18,glassSheen:16,borderColor:"#ff183d",rimColor:"#0b0a10",faceColor:"#080a10",rimPercent:2,edgePercent:0.5,previewMode:"fill",previewPadding:0,showFaceTitle:false};
                        for(const key in values)panel.store.setValue(key,values[key]);
                    }
                }
                ColorSetting { Layout.fillWidth: true; label: "Border color"; keyName: "borderColor"; store: panel.store }
                ColorSetting { Layout.fillWidth: true; label: "Dark rim color"; keyName: "rimColor"; store: panel.store }
                ColorSetting { Layout.fillWidth: true; label: "Background color"; keyName: "backgroundColor"; store: panel.store }
                SliderSetting { Layout.fillWidth: true; label: "Dark rim width"; keyName: "rimPercent"; store: panel.store; from: 0.5; to: 12; step: 0.1; suffix: "%" }
                SliderSetting { Layout.fillWidth: true; label: "Edge width"; keyName: "edgePercent"; store: panel.store; from: 0.2; to: 3; step: 0.1; suffix: "%" }
                ColorSetting { Layout.fillWidth: true; label: "Face background"; keyName: "faceColor"; store: panel.store }
                SliderSetting { Layout.fillWidth: true; label: "Window opacity"; keyName: "previewOpacity"; store: panel.store; from: 0; to: 100; suffix: "%" }
                SliderSetting { Layout.fillWidth: true; label: "Content padding"; keyName: "previewPadding"; store: panel.store; from: 0; to: 20; step: 0.5; suffix: "%" }
                SettingsSwitch { text: "Title over live preview"; keyName: "showFaceTitle"; store: panel.store }
                SettingsSwitch { text: "Bold title"; keyName: "titleBold"; store: panel.store }
                ColorSetting { Layout.fillWidth: true; label: "Title color"; keyName: "titleColor"; store: panel.store }
                ColorSetting { Layout.fillWidth: true; label: "Title background"; keyName: "titleBackgroundColor"; store: panel.store }
                SliderSetting { Layout.fillWidth: true; label: "Title size"; keyName: "titleSize"; store: panel.store; from: 12; to: 64; suffix: " px" }
                SliderSetting { Layout.fillWidth: true; label: "Title background opacity"; keyName: "titleBackgroundOpacity"; store: panel.store; from: 0; to: 100; suffix: "%" }
                SettingsComboBox {
                    Layout.fillWidth: true
                    model: [{text:"Title at top",value:"top"},{text:"Title at center",value:"center"},{text:"Title at bottom",value:"bottom"}]
                    textRole: "text"
                    currentIndex: model.findIndex(option => option.value === panel.store.values.titlePosition)
                    onActivated: index => panel.store.setValue("titlePosition",model[index].value)
                }
                SettingsSwitch { text: "Live window previews"; keyName: "livePreviews"; store: panel.store }
                C.Label { color: "#fff0f3"; text: "Preview quality" }
                SettingsComboBox {
                    Layout.fillWidth: true
                    model: [{text:"Low",value:256},{text:"Balanced",value:512},{text:"High",value:640},{text:"Very high",value:1024}]
                    textRole: "text"
                    currentIndex: model.findIndex(option => option.value === panel.store.values.previewSize)
                    onActivated: index => panel.store.setValue("previewSize", model[index].value)
                }
                SettingsSwitch { text: "Window title on hover"; keyName: "hoverTitles"; store: panel.store }
                C.Label { text: "ROTATION"; color: "#ff778e"; font.bold: true }
                SliderSetting { Layout.fillWidth: true; label: "Rotation speed"; keyName: "rotationSpeed"; store: panel.store; from: 0.25; to: 2.5; step: 0.05; suffix: "×" }

                C.Button { text: "Reset defaults"; onClicked: panel.store.resetDefaults() }
                C.Label { text: panel.store.error; visible: text.length > 0; color: "#ff778e"; wrapMode: Text.Wrap; Layout.fillWidth: true }
                Item { Layout.preferredHeight: 12 }
            }
        }
    }
    C.Popup {
        id: backgroundPicker
        objectName:"backgroundPicker"
        parent: C.Overlay.overlay
        x:(parent.width-width)/2;y:(parent.height-height)/2
        width:Math.min(660,parent.width-36);height:Math.min(580,parent.height-36)
        padding:18;modal:true;focus:true
        closePolicy:C.Popup.CloseOnEscape
        background:Rectangle { color:"#18171f";border.color:"#79404f";radius:10 }
        contentItem: ColumnLayout {
            spacing:10
            C.Label { text:"Import background image";font.pixelSize:20;color:"#fff0f3" }
            RowLayout {
                Layout.fillWidth:true
                C.Button { text:"Up";onClicked:panel.browseBackground(panel.backgroundListing.parent) }
                C.TextField { id:backgroundPath;objectName:"backgroundFolderPath";Layout.fillWidth:true;placeholderText:"Folder path";onAccepted:panel.browseBackground(text) }
                C.Button { text:"Go";onClicked:panel.browseBackground(backgroundPath.text) }
            }
            C.Label { text:"Choose an image to copy into PolySwitch";color:"#bfb3bd" }
            ListView {
                id: backgroundFiles
                objectName:"backgroundFileList"
                Layout.fillWidth:true;Layout.fillHeight:true;clip:true
                model:panel.backgroundListing.entries
                C.ScrollBar.vertical:C.ScrollBar {}
                delegate:C.ItemDelegate {
                    required property var modelData
                    width:backgroundFiles.width;height:38
                    text:(modelData.directory ? "▸  " : "▧  ")+modelData.name
                    contentItem:C.Label { text:parent.text;color:"#fff0f3";elide:Text.ElideMiddle;verticalAlignment:Text.AlignVCenter }
                    background:Rectangle { color:parent.hovered?"#3c2933":"#211f29";radius:4 }
                    onClicked: {
                        if(modelData.directory)panel.browseBackground(modelData.url);
                        else if(panel.store.importBackground(modelData.url))backgroundPicker.close();
                    }
                }
            }
            C.Label { Layout.fillWidth:true;wrapMode:Text.WordWrap;color:"#ff9dab";text:panel.store.backgroundError || "";visible:text.length>0 }
            C.Button { text:"Cancel";Layout.alignment:Qt.AlignRight;onClicked:backgroundPicker.close() }
        }
    }
}
