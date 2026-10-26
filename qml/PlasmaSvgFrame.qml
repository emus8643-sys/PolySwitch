// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 PolySwitch contributors
import QtQuick
import org.kde.ksvg as KSvg
import org.kde.kirigami.platform as Platform
KSvg.FrameSvgItem {
    imagePath:parent.assetPath
    prefix:parent.framePrefix
    visible:parent.framePrefix==="" || hasElementPrefix(parent.framePrefix)
    Platform.Theme.inherit:false
    Platform.Theme.backgroundColor:parent.surfaceColor
    Platform.Theme.textColor:parent.prefs.uiTextColor
    Platform.Theme.highlightColor:parent.prefs.borderColor
    Platform.Theme.highlightedTextColor:parent.prefs.uiHighlightTextColor
}
