#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
cd "$(dirname "$(readlink -f "$0")")"
./disable.sh
target="$(qtpaths6 --plugin-dir)/kwin/effects/plugins/polyswitch_native4.so"
if [[ -f "$target" ]]; then
    if sudo -n true 2>/dev/null; then sudo /usr/bin/unlink "$target"; else pkexec /usr/bin/unlink "$target"; fi
fi
echo 'Plugin removed. Settings and backups are preserved. Save work and sign out/in normally.'
