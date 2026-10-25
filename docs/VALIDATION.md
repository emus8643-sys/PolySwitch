# Validation

Development environment: KDE neon User 24.04, KWin 6.8.1, Qt 6.11.1, Wayland.

Core checks cover shape counts 0–64, upright texture bases for 2,080 faces, and settings validation/persistence in a temporary configuration directory.

Integration checks performed before this release in a separate virtual KWin session include:

- 2,816 screen-relative rotation directions, actual right-drag handlers and release momentum.
- Actual keyboard face navigation, title search ranking, match cycling/activation, no-match behavior, and selected-window closure.
- A native elapsed-time check: no alignment at 500 ms, upright alignment by 1.25 seconds, with Settings open, on either primary output.
- Actual Wayland client click-and-type delivery on the non-primary output, preservation of external drags, restoration of switcher input and safe close outside.
- Live counts 6 → 7 → 5 → 0, caption changes, minimized-window retention.
- Zoom wheel/keys/slider, limits and picking at 50/100/200%, plus Settings wheel isolation.
- Passive glow styles/color/strength/spread, hover styles, portable shader loading, gray dropdown rows, toggle colors and saved preferences.

Screenshots use synthetic test windows, not personal desktop content. This is not a claim of compatibility with all distributions, drivers, Qt versions or Plasma releases. Long-running field testing and packaging for additional distributions remain welcome.
