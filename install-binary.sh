#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -Eeuo pipefail
cd "$(dirname "$(readlink -f "$0")")"
for tool in kwin_wayland qtpaths6 sha256sum install uname ldd; do command -v "$tool" >/dev/null || { echo "Missing: $tool" >&2; exit 1; }; done
[[ "$(uname -m)" == x86_64 ]] || { echo 'This build requires x86_64.' >&2; exit 1; }
[[ "$(kwin_wayland --version)" == 'kwin 6.8.1' && "$(qtpaths6 --qt-version)" == '6.11.1' ]] || { echo 'This binary requires KWin 6.8.1 and Qt 6.11.1. Build from source for other versions.' >&2; exit 1; }
[[ -r /etc/os-release ]] || exit 1
. /etc/os-release
[[ "${ID:-}" == neon && "${VERSION_ID:-}" == '24.04' ]] || { echo 'This binary is tested only on KDE neon 24.04. Build from source for your distribution.' >&2; exit 1; }
sha256sum --check SHA256SUMS
if ldd polyswitch_native4.so | grep -q 'not found'; then echo 'Required runtime libraries are missing.' >&2; exit 1; fi
[[ "${1:-}" != --check-only ]] || { echo 'Compatibility and integrity checks passed; nothing installed.'; exit 0; }
[[ -z "${1:-}" ]] || { echo 'Usage: ./install-binary.sh [--check-only]' >&2; exit 2; }
plugin="$(readlink -f polyswitch_native4.so)"
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
printf '%s\n' '0.4.13' > "$state_dir/installed-version"
echo 'Installed. Save work, sign out/in normally, then enable PolySwitch in Desktop Effects. Do not restart KWin manually.'
