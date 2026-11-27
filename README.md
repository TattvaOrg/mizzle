# mizzle

Wayland-native **macOS Tahoe Liquid Glass Desktop Environment UI** (`gtk4`, `gtk4-layer-shell-0`, OpenGL/GLES3, Dual-Kawase blur, and Cairo/Pango vector rendering).

Mizzle runs as a non-blocking desktop UI layer alongside your existing Wayland compositor (such as `niri`, `hyprland`, or `sway`) using independent layer-shell surfaces—without acting as a window manager or blocking your windows.

## Features

- **GLSL Liquid Glass Material** (`shaders/liquid_glass.frag`):
  - Rounded-box SDF geometry with perimeter meniscus refraction and corner droplet boost
  - 5-tap spectral dispersion and anisotropic diagonal specular rim highlights
  - 5-level Dual-Kawase blur pyramid (`shaders/kawase_down.frag`, `shaders/kawase_up.frag`)
  - Translucent live-compositor glass blending in desktop environment mode (`--env`)
- **macOS Tahoe Menu Bar** (`mizzle-bar`):
  - Exclusive top `28px` bar with Mizzle/Apple menu, active focused app title, interactive `File / Edit / View / Window / Help` dropdowns, live Battery, Wi-Fi, Spotlight, Control Center, and Date & Time
- **macOS Tahoe Dock** (`mizzle-dock`):
  - Smooth spring hover magnification, floating glass tooltip pill, click bounce, running app dots, optional Auto-Hide, and real Linux app launching
- **Resizable & Movable Desktop Widgets** (`mizzle-widgets`):
  - Bottom-layer desktop widgets (always below normal windows, click-through outside widget bounds)
  - Real current-month **Calendar**, live CPU/RAM/Battery **System & Gallery**, and UTC **World Clocks**
  - Switchable **`S` / `M` / `L`** size tiers AND direct bottom-right corner drag resizing, persisted in `~/.config/mizzle/config.ini`
- **Spotlight, Applications Grid, Control Center & Shader Lab** (`mizzle-overlay`):
  - Real `.desktop` app search & category grid, live `wpctl`/`brightnessctl`/`nmcli`/`bluetoothctl`/`playerctl` controls, and live shader + widget sizing controls

## Install / Update / Uninstall

```bash
./install.sh     # Interactive installer & updater (checks existing install, chooses Release vs Main branch)
./update.sh      # Update existing installation
./uninstall.sh   # Cleanly uninstall binaries, autostart, and compositor hooks
```

Non-interactive flags:
- `./install.sh --release -y` — install/update latest stable release/update
- `./install.sh --main -y` — install/update bleeding-edge `main` branch
- `./install.sh --local -y` — install from the current local checkout

## CLI Usage

```bash
mizzle start         # Start Mizzle Desktop Environment UI
mizzle stop          # Stop Mizzle
mizzle restart       # Restart Mizzle
mizzle spotlight     # Toggle Spotlight Search (great for compositor keybinds)
mizzle settings      # Open Shader & Widget Lab
mizzle cc            # Toggle Control Center
mizzle edit-widgets  # Toggle Widget Resize / Edit mode
```

For quick foreground previews:
```bash
./view.sh --window   # Resizable preview window
./view.sh --silk-bg  # Lake Tahoe animated backdrop
./view.sh --demo-bg  # Optics Lab refraction backdrop
```

## Hotkeys

- `1` / `2` / `3` / `4`: Presets (**Clear**, **Frosted**, **Smoked**, **Aqua**)
- `E`: Toggle Widget Edit / Resize mode
- `I`: Cycle Dock icon styles (**Default**, **Dark**, **Clear Glass**, **Tinted Glass**)
- `B`: Cycle backdrop (**Desktop/Live BG**, **Lake Tahoe**, **Optics Lab**)
- `R`: Re-capture live desktop behind the glass
- `F` / `A` / `W`: Toggle Applications grid / Alert modal / Widgets
- `T` / `C` / `S`: Toggle Shader Lab / Control Center / Spotlight
- `Esc` / `Q`: Dismiss popup (or exit in preview mode)
