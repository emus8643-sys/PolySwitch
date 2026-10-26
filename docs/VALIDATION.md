# Validation

Development environment: KDE neon User 24.04, KWin 6.8.1, Qt 6.11.1, Wayland.

Core checks cover shape counts 0–64, upright texture bases for 2,080 faces, and settings validation/persistence in a temporary configuration directory.

Integration checks performed before this release in a separate virtual KWin session include:

- 2,816 screen-relative rotation directions, actual right-drag handlers and release momentum.
- Actual keyboard face navigation, title search ranking, match cycling/activation, no-match behavior, and selected-window closure.
- A native elapsed-time check of the earlier immediate-upright implementation with Settings open on either primary output; superseded by the configurable smooth-roll checks below.
- Actual Wayland client click-and-type delivery on the non-primary output, preservation of external drags, restoration of switcher input and safe close outside.
- Live counts 6 → 7 → 5 → 0, caption changes, minimized-window retention.
- Zoom wheel/keys/slider, limits and picking at 50/100/200%, plus Settings wheel isolation.
- Passive glow styles/color/strength/spread, hover styles, portable shader loading, gray dropdown rows, toggle colors and saved preferences.

Screenshots use synthetic test windows, not personal desktop content. This is not a claim of compatibility with all distributions, drivers, Qt versions or Plasma releases. Long-running field testing and packaging for additional distributions remain welcome.

## 0.4.14 header and backgrounds

Core tests verify that imported images are copied and downscaled, remain usable after the original is deleted, and retain header/mode/opacity preferences. An isolated Wayland UI check clicks the header toggle and embedded image picker, imports an image, deletes the source, checks rendered background pixels and visibility, and verifies settings from a fresh process. User preferences/data are isolated for all tests.

## 0.4.15 native chooser and morphs

A private D-Bus portal fixture checks import, cancellation, invalid URLs and unavailable-service behavior. A separate virtual KWin instance verifies effect deactivation/input release and return to Settings after import/cancel. Morph tests check stable task identities, mid-transition retargeting, final shape counts, planar faces and paired boundary edges through 6/7/12/20/5 faces. A Wayland visual harness renders live transitions across 3–12 tasks and a rapid second task update. The GIF uses synthetic previews.

## Local 0.4.16 theme integration

Isolated KDE config fixtures exercise light/dark detection, explicit accent colors, complementary glow generation, atomic file replacement, automatic/manual/mixed modes, numeric/color overrides and persistence. Manual settings are preserved while KDE changes. A separate `plasmarc` change updates desktop-style detection without overriding the application scheme. A Wayland UI harness switches schemes live with Settings open, checks an individual override and verifies that the single import button calls the native picker. Test settings and theme files use temporary XDG locations.

## Local 0.4.17 smooth upright and SVG artwork

Core fixtures cover idle/duration bounds and persistence, selected/fallback SVG paths, separate Plasma colors, live color-file replacement and saved color-source resolution. The isolated Wayland UI harness enables real KSvg artwork, verifies a loaded panel, tests a two-second idle threshold and an 800 ms roll at intermediate and settled orientations, and confirms interruption stops movement. Light/dark/accent/manual/mixed checks and the single native import button are repeated with SVG enabled. User settings remain isolated.

## Local 0.4.18 setup, recovery and profiles

Core tests verify portable backgrounds after the original and previous managed image are deleted; import without applying; duplicate names with distinct IDs; apply/reset/undo; preservation of navigation and guide completion; persistent lists; and rejection of invalid versions, image payloads, non-appearance settings and path traversal. A private D-Bus file chooser fixture exercises OpenFile/SaveFile, export, import, cancel and service failure.

The isolated Wayland UI harness clicks through the first-run guide, verifies saved completion from a fresh process, replays/skips it, saves/applies a profile, dispatches import/export, confirms reset/disable, undoes appearance changes and opens the recovery tools action. Light/dark/accent/SVG/smooth-upright and native image-picker regressions remain covered. Screenshots contain synthetic tasks only.

A separate native KWin fixture verifies profile export/import/cancel while releasing input capture and returning to Settings. It also confirms that disabling stops the effect and persists its disabled configuration. Only the owned virtual KWin process is stopped by the fixture.
