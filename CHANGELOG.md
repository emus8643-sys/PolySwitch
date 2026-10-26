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
