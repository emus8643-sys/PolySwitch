#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -Eeuo pipefail
cd "$(dirname "$(readlink -f "$0")")"
case "${1:-}" in ''|--build-only) ;; *) echo 'Usage: ./update.sh [--build-only]' >&2; exit 2;; esac
for tool in cmake make qtpaths6 install; do command -v "$tool" >/dev/null || { echo "Missing tool: $tool" >&2; exit 1; }; done
build_dir="${POLYSWITCH_BUILD_DIR:-build}"
cmake -S . -B "$build_dir" -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build "$build_dir" -j"${POLYSWITCH_BUILD_JOBS:-4}"
plugin="$(find "$build_dir/lib" -type f -name polyswitch_native4.so -print -quit)"
[[ -n "$plugin" && -s "$plugin" ]] || { echo 'Compiled plugin not found.' >&2; exit 1; }
plugin="$(readlink -f "$plugin")"
echo "Compiled: $plugin"
[[ "${1:-}" != --build-only ]] || exit 0
./disable.sh
plugin_dir="$(qtpaths6 --plugin-dir)"
[[ "$plugin_dir" == /* ]] || { echo 'Qt plugin directory must be absolute.' >&2; exit 1; }
target="$plugin_dir/kwin/effects/plugins/polyswitch_native4.so"
state_dir="${XDG_DATA_HOME:-$HOME/.local/share}/polyswitch"
mkdir -p "$state_dir/backups"
chmod 700 "$state_dir" "$state_dir/backups"
if [[ -f "$target" ]]; then
    backup="$state_dir/backups/polyswitch-$(date -u +%Y%m%dT%H%M%SZ)-$$.so"
    cp -- "$target" "$backup"
    printf '%s\n' "$backup" > "$state_dir/latest-backup"
fi
if sudo -n true 2>/dev/null; then sudo install -Dm755 "$plugin" "$target"; else pkexec /usr/bin/install -Dm755 "$plugin" "$target"; fi
printf '%s\n' '0.4.14' > "$state_dir/installed-version"
echo 'Installed. Save work, sign out/in normally, then enable PolySwitch in Desktop Effects. Do not restart KWin manually.'
