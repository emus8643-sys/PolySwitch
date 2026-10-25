#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
kwriteconfig6 --file kwinrc --group Plugins --key polyswitch_native4Enabled false
if command -v qdbus6 >/dev/null && qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.activeEffects 2>/dev/null | grep -qx polyswitch_native4; then
    echo 'PolySwitch is open. Close it with Escape or Meta+Shift+P, then rerun this command.' >&2
    exit 2
fi
if command -v qdbus6 >/dev/null; then qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.unloadEffect polyswitch_native4 >/dev/null 2>&1 || true; fi
echo 'PolySwitch disabled. KWin was not restarted.'
