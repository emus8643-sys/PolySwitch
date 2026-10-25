#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
cd "$(dirname "$(readlink -f "$0")")"
state_dir="${XDG_DATA_HOME:-$HOME/.local/share}/polyswitch"
[[ -s "$state_dir/latest-backup" ]] || { echo 'No saved previous plugin. Use ./uninstall.sh to remove a first installation.' >&2; exit 1; }
backup="$(readlink -f "$(cat "$state_dir/latest-backup")")"
backup_dir="$(readlink -f "$state_dir/backups")"
[[ "$backup" == "$backup_dir/"* && -s "$backup" ]] || { echo 'Invalid or missing rollback backup.' >&2; exit 1; }
./disable.sh
target="$(qtpaths6 --plugin-dir)/kwin/effects/plugins/polyswitch_native4.so"
if sudo -n true 2>/dev/null; then sudo install -Dm755 "$backup" "$target"; else pkexec /usr/bin/install -Dm755 "$backup" "$target"; fi
echo 'Previous plugin restored, still disabled. Save work and sign out/in normally before enabling it.'
