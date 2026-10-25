# PolySwitch

**AI-generated software:** PolySwitch was developed with AI-generated code, directed and tested by its creator. It is an experimental project; review the source and compatibility notes before installing.

A 3D application-window switcher for KDE Plasma Wayland. Each window gets a live face on a shape that changes with the number of windows.

**Experimental native KWin plugin. Tested on KDE neon, KWin 6.8.1 and Qt 6.11.1.** Native KWin APIs and binary compatibility can change between Plasma releases. Build against the KWin installed on your machine; a compiled plugin is not a universal download.

![PolySwitch red glass appearance](docs/red-glass.png)

## Features

- Live window previews, including minimized windows, with live additions, closures and title updates.
- A single panel for one window, double-sided card for two, open triangular/square arrangements for three/four, and closed solids from five onward. Six is a cube, twelve a dodecahedron, twenty an icosahedron. Other counts use convex faceted shapes, up to 64 windows.
- Screen-relative mouse rotation and momentum, wheel zoom, arrow/Tab navigation, and upright face selection.
- Type to search window titles; the best match turns toward you. Prefix matches rank first, then alphabetical order breaks ties.
- Optional automatic upright alignment after one second of idle input, including with Settings open.
- Independent passive glow color, strength, spread and styles: neon, soft bloom, breathing neon and radiant edges.
- Separate hover effects: Neon, Sunlight, Ice and Breathing Neon.
- Fit/fill/title-only faces; customizable colors, titles, borders, reflection, opacity and preview quality.
- Primary-monitor display with normal mouse and keyboard use on other monitors.

Application windows only; individual browser tabs are not enumerated.

## Build and install

Requirements: C++20 compiler, CMake 3.24+, ECM, Qt 6 Core/Gui/Widgets/Quick/Quick3D/ShaderTools, KDE Frameworks 6 CoreAddons/GlobalAccel/Config, and matching KWin development headers. QtQuick Controls, Layouts, Shapes, Effects and Quick3D runtime modules must also be available.

On KDE neon, development package names include:

```sh
sudo apt install build-essential cmake extra-cmake-modules qt6-base-dev \
  qt6-declarative-dev qt6-quick3d-dev qt6-shadertools-dev kwin-dev \
  libkf6coreaddons-dev libkf6globalaccel-dev libkf6config-dev \
  qml6-module-qtquick-controls qml6-module-qtquick-layouts
```

Package names/runtime packaging vary by distribution. Older KWin versions may require code changes.

```sh
git clone https://github.com/emus8643-sys/PolySwitch.git
cd PolySwitch
chmod +x update.sh disable.sh rollback.sh uninstall.sh
./update.sh --build-only
./update.sh
```

Installation uses KDE authentication to replace the system plugin and saves the previous binary under `~/.local/share/polyswitch/backups/` (or `$XDG_DATA_HOME/polyswitch/backups/`). Close PolySwitch before installing. Save work and **sign out/in normally**, then enable **PolySwitch** in System Settings → Desktop Effects. Do not manually restart KWin or hot-load a replaced native plugin.

This is currently a compiled native effect, not a one-click KDE Store package.

## Controls

| Action | Control |
|---|---|
| Open/close | Meta+Shift+P |
| Rotate | Right mouse drag |
| Zoom | Wheel over the shape, or + / −; 0 resets |
| Select previous/next face upright | Left/Up or Right/Down; Tab/Shift+Tab |
| Switch window | Left click face, or Enter on selection |
| Search | Start typing, or click the search field |
| Browse search matches | Up/Down or Tab/Shift+Tab |
| Close | Escape |

When search has focus, Left/Right edit the query; Up/Down browse matches. Auto-upright is an optional Settings toggle. Settings save in `~/.config/polyswitchrc` and apply live. Editing titles/settings does not send data to any service.

## Disable, roll back or remove

Close the switcher first:

```sh
./disable.sh
./rollback.sh    # restore the previous plugin saved by update.sh
./uninstall.sh   # remove this plugin, preserve preferences and backups
```

After replacing/removing native code, sign out/in normally. If the effect cannot be opened safely, disable it in Desktop Effects; `disable.sh` also records the disabled state for the next session. Keep work saved when testing an experimental compositor plugin.

## Development and tests

```sh
cmake -S . -B build-test -DPOLYSWITCH_BUILD_TESTS=ON
cmake --build build-test -j4
ctest --test-dir build-test --output-on-failure
```

The tests create an isolated temporary configuration directory. They never write to your normal PolySwitch preferences. See [validation notes](docs/VALIDATION.md) for integration coverage and [CONTRIBUTING.md](CONTRIBUTING.md) for reporting bugs.

## License

GPL-3.0-or-later. See [LICENSE](LICENSE) and [NOTICE](NOTICE), including the retained MIT notice for the original prototype. This is an independent project, not an official KDE product.
