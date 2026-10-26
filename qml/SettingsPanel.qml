// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
import QtQuick.Controls as C
import QtQuick.Layouts

C.Pane {
    id: panel
    property var effectBridge: null
    required property var store
    signal closeRequested()
    signal walkthroughRequested()
    readonly property var prefs: store.values
    padding: 18
    palette.window: prefs.uiPanelColor
    palette.text: prefs.uiTextColor
    palette.windowText: prefs.uiTextColor
    palette.buttonText: prefs.uiTextColor
    palette.button: prefs.uiButtonColor
    palette.mid: prefs.uiBorderColor
    palette.placeholderText: prefs.uiMutedColor
    palette.base: prefs.uiBaseColor
    palette.highlight: prefs.borderColor
    palette.highlightedText: prefs.uiHighlightTextColor
    background: Item {
        Rectangle { anchors.fill:parent;color:Qt.alpha(panel.prefs.uiPanelColor,panel.prefs.panelOpacity/100);radius:12;border.color:panel.prefs.uiBorderColor }
        PlasmaFrame { anchors.fill:parent;anchors.margins:-12;prefs:panel.prefs;assetPath:panel.store.themeInfo.artwork.panel || "";framePrefix:"shadow";visible:panel.prefs.plasmaArtworkShadows;revision:panel.store.themeInfo.artworkRevision }
        PlasmaFrame { objectName:"plasmaSettingsFrame";anchors.fill:parent;prefs:panel.prefs;assetPath:panel.store.themeInfo.artwork.panel || "";revision:panel.store.themeInfo.artworkRevision;opacity:panel.prefs.panelOpacity/100 }
    }
    // Swallow unused panel clicks, keeping the preview below from activating.
    MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons; onWheel: wheel => wheel.accepted=true }
    ColumnLayout {
        anchors.fill: parent
        RowLayout {
            C.Label { color: panel.prefs.uiTextColor; text: "PolySwitch settings"; font.pixelSize: 20; font.bold: true; Layout.fillWidth: true }
            C.ToolButton { text: "Close"; Layout.preferredWidth: 64; Layout.preferredHeight: 30
                background: Rectangle { color: panel.prefs.uiButtonColor; radius: 5; border.color: panel.prefs.uiBorderColor }
                contentItem: C.Label { text: "Close"; color: panel.prefs.uiTextColor; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter } onClicked: { panel.store.flush(); panel.closeRequested(); } }
        }
        C.Label { text: "Preview changes live · Saved automatically"; color: panel.prefs.uiMutedColor; font.pixelSize: 12 }
        C.ScrollView {
            palette:panel.palette
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
                ProfileSettings { Layout.fillWidth:true;store:panel.store;effectBridge:panel.effectBridge;onWalkthroughRequested:panel.walkthroughRequested() }
                C.Label { text:"KDE THEME";color:panel.prefs.borderColor;font.bold:true }
                SettingsSwitch { text:"Follow KDE Theme";keyName:"followKdeTheme";store:panel.store;objectName:"followKdeThemeSwitch" }
                SettingsComboBox {
                    objectName:"themeModeCombo";Layout.fillWidth:true
                    model:[{text:"Automatic · follow all KDE colors",value:"automatic"},{text:"Mixed · KDE with individual overrides",value:"mixed"},{text:"Manual · use saved custom colors",value:"manual"}]
                    textRole:"text";currentIndex:model.findIndex(option=>option.value===panel.prefs.themeMode)
                    onActivated:index=>panel.store.setValue("themeMode",model[index].value)
                }
                C.Label { Layout.fillWidth:true;wrapMode:Text.WordWrap;color:panel.prefs.uiMutedColor;text:panel.store.themeInfo.schemeName+" · "+(panel.store.themeInfo.dark?"Dark":"Light")+" · Accent "+panel.store.themeInfo.accent+"\nPlasma style: "+panel.store.themeInfo.plasmaTheme+" (detected separately)" }
                SettingsComboBox {
                    objectName:"themeGlowPaletteCombo";Layout.fillWidth:true;enabled:panel.prefs.followKdeTheme
                    model:[{text:"Glow palette: accent",value:"accent"},{text:"Glow palette: complementary",value:"complementary"},{text:"Glow palette: analogous",value:"analogous"}]
                    textRole:"text";currentIndex:model.findIndex(option=>option.value===panel.prefs.themeGlowPalette)
                    onActivated:index=>panel.store.setValue("themeGlowPalette",model[index].value)
                }
                SettingsSwitch { text:"Use Plasma SVG artwork";keyName:"plasmaArtwork";store:panel.store;objectName:"plasmaArtworkSwitch" }
                SettingsSwitch { text:"Plasma SVG panel shadows";keyName:"plasmaArtworkShadows";store:panel.store }
                SettingsComboBox {
                    Layout.fillWidth:true;objectName:"themeColorSourceCombo"
                    model:[{text:"Colors: KDE application scheme",value:"application"},{text:"Colors: Plasma desktop style",value:"plasma"}]
                    textRole:"text";currentIndex:model.findIndex(option=>option.value===panel.prefs.themeColorSource)
                    onActivated:index=>panel.store.setValue("themeColorSource",model[index].value)
                }
                SettingsComboBox {
                    Layout.fillWidth:true;objectName:"plasmaArtworkVariantCombo"
                    model:[{text:"Artwork: automatic",value:"automatic"},{text:"Artwork: translucent",value:"translucent"},{text:"Artwork: opaque",value:"opaque"},{text:"Artwork: solid",value:"solid"}]
                    textRole:"text";currentIndex:model.findIndex(option=>option.value===panel.prefs.plasmaArtworkVariant)
                    onActivated:index=>panel.store.setValue("plasmaArtworkVariant",model[index].value)
                }
                C.Label { Layout.fillWidth:true;wrapMode:Text.WordWrap;color:panel.prefs.uiMutedColor;text:"SVG panels, headers, inputs and buttons use the selected Plasma style, with Breeze/plain fallback. For fixed-color artwork, choose Plasma desktop style colors to match its text. Polygon faces keep their live previews and glow." }
                C.Label { Layout.fillWidth:true;wrapMode:Text.WordWrap;color:panel.prefs.uiMutedColor;text:"Editing a theme color adds an override and switches to Mixed. Your saved manual colors remain available in Manual mode." }
                PlasmaButton { prefs:panel.prefs;artwork:panel.store.themeInfo.artwork;palette:panel.palette; text:"Clear theme overrides";enabled:panel.prefs.themeOverrides.length>0;onClicked:panel.store.clearThemeOverrides() }
                SliderSetting { Layout.fillWidth:true;label:"Header opacity";keyName:"headerOpacity";store:panel.store;suffix:"%" }
                SliderSetting { Layout.fillWidth:true;label:"Settings opacity";keyName:"panelOpacity";store:panel.store;from:70;to:100;suffix:"%" }
                ColorSetting { Layout.fillWidth:true;label:"Lighting tint";keyName:"lightingColor";store:panel.store }
                ColorSetting { Layout.fillWidth:true;label:"Light fill color";keyName:"fillLightColor";store:panel.store }
                SliderSetting { Layout.fillWidth:true;label:"Light brightness";keyName:"lightBrightness";store:panel.store;from:0.1;to:2;step:0.05 }
                C.Label { text: "HEADER AND BACKGROUND"; color: panel.prefs.borderColor; font.bold: true }
                SettingsSwitch { text: "Show header"; keyName: "showHeader"; store: panel.store; objectName:"showHeaderSwitch" }
                PlasmaButton { prefs:panel.prefs;artwork:panel.store.themeInfo.artwork;palette:panel.palette; text: "Import background image…"; objectName:"importBackgroundButton"; Layout.fillWidth:true; onClicked: { if(panel.effectBridge && typeof panel.effectBridge.chooseBackground==="function") panel.effectBridge.chooseBackground();  } }
                C.Label { Layout.fillWidth:true;wrapMode:Text.WordWrap;color:panel.prefs.uiErrorColor;text:panel.effectBridge ? panel.effectBridge.pickerError || "" : "";visible:text.length>0 }
                SettingsSwitch { text: "Smooth shape changes";keyName:"shapeMorphEnabled";store:panel.store }
                SliderSetting { Layout.fillWidth:true;label:"Shape transition duration";keyName:"shapeMorphDuration";store:panel.store;from:180;to:1200;step:10;suffix:" ms" }
                SettingsSwitch { text: "Show imported background"; keyName:"backgroundEnabled";store:panel.store }
                SettingsComboBox {
                    objectName:"backgroundModeCombo";Layout.fillWidth:true
                    model:[{text:"Background: fill screen",value:"fill"},{text:"Background: fit entire image",value:"fit"},{text:"Background: stretch",value:"stretch"}]
                    textRole:"text"
                    currentIndex:model.findIndex(option=>option.value===panel.store.values.backgroundMode)
                    onActivated:index=>panel.store.setValue("backgroundMode",model[index].value)
                }
                SliderSetting { Layout.fillWidth:true;label:"Background opacity";keyName:"backgroundOpacity";store:panel.store;from:0;to:100;suffix:"%" }
                PlasmaButton { prefs:panel.prefs;artwork:panel.store.themeInfo.artwork;palette:panel.palette; text:"Remove background";enabled:panel.store.values.backgroundImage.length>0;onClicked:panel.store.setValue("backgroundImage","") }
                C.Label { Layout.fillWidth:true;wrapMode:Text.WordWrap;color:panel.prefs.uiMutedColor;text:"Imported images are copied into PolySwitch’s local storage and persist after restarting. Settings remains accessible with the header hidden." }
                C.Label { text: "FACE CONTENT"; color: panel.prefs.borderColor; font.bold: true }
                C.Label { color: panel.prefs.uiTextColor; text: "Window display mode" }
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
                PlasmaButton { prefs:panel.prefs;artwork:panel.store.themeInfo.artwork;palette:panel.palette; text: "Reset zoom (100%)"; onClicked: panel.store.setValue("zoomPercent",100) }
                SettingsSwitch { text: "Auto-upright when idle"; keyName: "autoUpright"; store: panel.store }
                SliderSetting { Layout.fillWidth:true;label:"Auto-upright idle delay";keyName:"autoUprightDelay";store:panel.store;from:0.2;to:120;step:0.1;suffix:" s" }
                SliderSetting { Layout.fillWidth:true;label:"Auto-upright roll duration";keyName:"autoUprightDuration";store:panel.store;from:150;to:3000;step:50;suffix:" ms" }
                SettingsSwitch { text: "Keep spinning after drag"; keyName: "inertiaEnabled"; store: panel.store }
                C.Label { text: "HOVER HIGHLIGHT"; color: panel.prefs.borderColor; font.bold: true }
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
                C.Label { text: "APPEARANCE"; color: panel.prefs.borderColor; font.bold: true }
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
                PlasmaButton { prefs:panel.prefs;artwork:panel.store.themeInfo.artwork;palette:panel.palette;
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
                C.Label { color: panel.prefs.uiTextColor; text: "Preview quality" }
                SettingsComboBox {
                    Layout.fillWidth: true
                    model: [{text:"Low",value:256},{text:"Balanced",value:512},{text:"High",value:640},{text:"Very high",value:1024}]
                    textRole: "text"
                    currentIndex: model.findIndex(option => option.value === panel.store.values.previewSize)
                    onActivated: index => panel.store.setValue("previewSize", model[index].value)
                }
                SettingsSwitch { text: "Window title on hover"; keyName: "hoverTitles"; store: panel.store }
                C.Label { text:"INTERFACE COLOR OVERRIDES";color:panel.prefs.borderColor;font.bold:true }
                ColorSetting { Layout.fillWidth:true;label:"Panel color";keyName:"uiPanelColor";store:panel.store }
                ColorSetting { Layout.fillWidth:true;label:"Input background";keyName:"uiBaseColor";store:panel.store }
                ColorSetting { Layout.fillWidth:true;label:"Interface text";keyName:"uiTextColor";store:panel.store }
                ColorSetting { Layout.fillWidth:true;label:"Secondary text";keyName:"uiMutedColor";store:panel.store }
                ColorSetting { Layout.fillWidth:true;label:"Button background";keyName:"uiButtonColor";store:panel.store }
                ColorSetting { Layout.fillWidth:true;label:"Interface borders";keyName:"uiBorderColor";store:panel.store }
                ColorSetting { Layout.fillWidth:true;label:"Error text";keyName:"uiErrorColor";store:panel.store }
                ColorSetting { Layout.fillWidth:true;label:"Selected text";keyName:"uiHighlightTextColor";store:panel.store }
                C.Label { text: "ROTATION"; color: panel.prefs.borderColor; font.bold: true }
                SliderSetting { Layout.fillWidth: true; label: "Rotation speed"; keyName: "rotationSpeed"; store: panel.store; from: 0.25; to: 2.5; step: 0.05; suffix: "×" }


                C.Label { text: panel.store.error; visible: text.length > 0; color: panel.prefs.borderColor; wrapMode: Text.Wrap; Layout.fillWidth: true }
                Item { Layout.preferredHeight: 12 }
            }
        }
    }
}
