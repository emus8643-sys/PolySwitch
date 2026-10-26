# PolySwitch recovery

Press Escape or Meta+Shift+P to close PolySwitch. In Settings, Reset appearance saves a recovery copy first. Undo last appearance change restores the state before the most recent reset or profile application. Profiles and navigation settings are preserved.

Disable PolySwitch in Settings prevents this instance from opening again and disables it at login. Sign out/in normally, then enable it in System Settings → Desktop Effects when ready. You can also run `./disable.sh` from this directory in a terminal.

To restore the previous installed build, open a terminal in this directory and run:

```sh
./rollback.sh
```

Authenticate only in KDE's administrator prompt. The script restores the saved previous plugin and leaves it disabled. Save your work and sign out/in normally before enabling PolySwitch in Desktop Effects. Do not restart KWin manually.

Backups are in `~/.local/share/polyswitch/backups` (or the corresponding XDG data location). Rollback requires an existing backup from an update. Uninstall instructions remain in the source README.

Appearance profiles are JSON files containing appearance settings and an optional embedded PNG. They contain no window list or executable code. Import adds a saved profile; Apply changes the current look. Imported backgrounds are copied into managed storage. Each save/import gets a new identity, so duplicate names do not replace existing profiles.
