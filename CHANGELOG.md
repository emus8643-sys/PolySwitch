# 0.4.18 — setup, recovery and portable profiles

- Add a three-page first-run guide with persisted completion and replay from Settings.
- Save named appearance profiles, apply/delete them, and import/export portable JSON files through native desktop pickers. Background PNGs are embedded; import does not alter the current look.
- Validate preset contents and image bounds, preserve navigation settings, and give every imported/saved profile a new identity.
- Save a recovery copy before applying a profile or resetting appearance; add persistent undo, confirmation before reset/delete/disable, and a disable control.
- Install standalone rollback/disable helpers and recovery instructions, accessible from Settings. KWin is never restarted manually.
- Included in the public 0.4.18 release.

# 0.4.17 — local smooth upright and Plasma artwork

- Add separate auto-upright idle-delay and roll-duration controls. Use eased spherical quaternion interpolation; input interrupts the roll.
- Integrate the selected Plasma style's SVG panels, header, search field and subtle button artwork, with optional panel shadows and variant selection.
- Offer Plasma style colors independently from KDE application colors. Follow style, asset and color changes live; fall back to Breeze/plain controls when assets are unavailable.
- Preserve saved color-source/variant choices on reload and restore application colors on reset.
- Included in the public 0.4.18 release.

# 0.4.16 — local KDE theme integration

- Remove the embedded image browser and its old button; retain the native image picker.
- Follow KDE application colors/accent by default, including live light/dark updates.
- Add automatic/manual/mixed modes, persistent individual color overrides and complementary/analogous glow palettes.
- Theme UI, backgrounds, translucency, frames, highlights and lighting. Preserve saved custom appearance.
- Detect the Plasma desktop style separately; document SVG-style integration findings.
- Included in the public 0.4.18 release.

# 0.4.15 — native image picker and live shape morphs

- Open the desktop portal image picker while safely releasing switcher input; return to Settings after selection/cancel.
- Keep the embedded browser as a fallback. Imported backgrounds still persist.
- Smoothly morph task shapes with stable window identities, nearby face assignments and interruptible transitions.
- Keep closed faces flat and joined; add transition on/off and duration controls.

# 0.4.14 — personal backgrounds and optional header

- Persistently hide the header while keeping Settings/search accessible.
- Import local background images into managed storage so they survive moving/deleting the original.
- Add fill/fit/stretch, opacity and background visibility controls.

# 0.4.13 — first public experimental release

- Fix idle upright alignment with Settings open; use elapsed native input time rather than accumulated animation steps.
- Add independent passive glow color, strength, spread, and neon/soft/breathing/radiant styles.
- Include live window updates, minimized-window activation, upright keyboard navigation, type-to-search, zoom, glass frames, hover presets and configurable face content.
- Restrict the switcher to the primary output while allowing ordinary interaction on other monitors.
- Provide self-contained installation, backup, rollback and uninstall scripts, plus isolated tests.
