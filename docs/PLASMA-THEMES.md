# Plasma styles versus application color schemes

The application color scheme is read from KDE globals (Window/View/Selection colors, General color-scheme name and accent), with the current Qt palette as fallback. KConfig notifications, palette-change events and file/directory watches handle both standard changes and atomic file replacement. The watched paths are resolved through XDG, including tests.

The selected Plasma desktop style is read separately from `plasmarc` / `Theme` / `name` and shown in Settings. A Plasma style is a collection of SVG widget assets and may supply its own `colors` file. Such a style can intentionally differ from application colors. It should therefore be a separate opt-in source rather than silently replacing the selected application scheme.

Enable **Use Plasma SVG artwork** in Settings to use the selected style's panel, header, button and line-edit assets through KSvg FrameSvgItem. Panel shadows have their own toggle. Automatic/translucent/opaque/solid variants are selectable; missing assets fall back individually to Breeze and then plain controls. Artwork is optional and the KSvg QML component loads lazily. Button artwork is translucent to preserve legibility across differing style color roles.

The **Colors** selector independently chooses the application scheme or a Plasma style's `colors` file, falling back to application colors when absent. Automatic/manual/mixed modes and per-role overrides still apply. Scheme-aware SVG uses the local attached palette; fixed-color SVG retains the artist's colors, so the Plasma color source is recommended for matching text. SVG artwork covers the 2D interface. The polygon renderer continues to own live previews and 3D glow edges; blur masks are not imported.

Configuration, style asset and color-file changes are watched. No global KSvg image set or desktop theme is changed. Saved choices are resolved again on login.

## Theme checklist

- [x] Detect active KDE application color scheme, light/dark mode and accent.
- [x] Adapt frames, highlights, glow, lighting, backgrounds, transparency and text.
- [x] Update automatically after KDE theme changes.
- [x] Follow KDE Theme enabled by default.
- [x] Individual overrides, automatic/manual/mixed modes and complementary glow hues.
- [x] Detect Plasma desktop styles separately, with optional SVG artwork and separate color source.

Primary KDE references:

- https://develop.kde.org/docs/plasma/theme/theme-details/
- https://develop.kde.org/docs/plasma/theme/theme-colors/
- https://develop.kde.org/docs/plasma/theme/background-svg/
