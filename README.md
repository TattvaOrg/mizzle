# mizzle

Wayland-native C++20 + GLSL Liquid Glass UI overlay (`gtk4`, `gtk4-layer-shell-0`, OpenGL/GLES3, Dual-Kawase blur, and Cairo/Pango vector rendering).

## Features

- **GLSL Liquid Glass Material** (`shaders/liquid_glass.frag`):
  - Rounded-box SDF geometry with slim `6–12px` perimeter meniscus refraction (`cornerBoost` around rounded corners)
  - Smooth low-frequency fluid interior texture (`liquidHeight` / `liquidNormal`)
  - 5-tap spectral dispersion along curved edges and anisotropic diagonal specular rim highlights
  - 5-level Dual-Kawase blur pyramid (`shaders/kawase_down.frag`, `shaders/kawase_up.frag`)
  - macOS-style click-only animations (press squeeze + damped Dock bounce + `0.35s` glass click ripple; zero hover glow)
- **Desktop Components** (`src/ui_widgets.cpp`):
  - Edge-to-edge transparent Menu Bar
  - Slim Control Center (Wi-Fi, Bluetooth, AirDrop, Focus, Media card, Sound & Display sliders, 4×2 circular quick actions)
  - Top-left Widgets (Calendar, Architectural preview card, translucent World Clocks)
  - Interactive Spotlight Search pill, Applications frosted glass window (`F`), Alert dialog (`A`), and live Shader Lab (`T`)
  - Bottom Dock with 12 vector icons and 4 cycleable icon styles (`Default`, `Dark`, `Clear Glass`, `Tinted Glass`)
- **Three Backdrop Modes** (`B` key or top-right toggle):
  - **Live BG**: captures your live Wayland desktop via `grim` before mapping the layer-shell overlay
  - **Lake Tahoe** (`--silk-bg`): animated twilight lake and mountain reflection shader
  - **Optics Lab** (`--demo-bg`): diagonal architectural stripes, drifting glowing orbs, and spectrum bar

## Prerequisites (Linux / Wayland)

- C++20 compiler (`g++` or `clang++`), `cmake` (>= 3.20), `ninja`
- `gtk4`, `gtk4-layer-shell-0`, `epoxy`, `pangocairo`, `egl`
- `grim` (for live Wayland desktop backdrop capture on `wlroots`/`niri` compositors)

## Quick Start

```bash
./view.sh            # Build and launch full-screen Wayland overlay
./view.sh --window   # Launch in a normal resizable window
./view.sh --silk-bg  # Launch with the animated Lake Tahoe backdrop
./view.sh --demo-bg  # Launch with the Optics Lab refraction backdrop
```

## Hotkeys

- `1` / `2` / `3` / `4`: Presets (**Clear**, **Frosted**, **Smoked**, **Aqua**)
- `I`: Cycle Dock icon styles (**Default**, **Dark**, **Clear Glass**, **Tinted Glass**)
- `B`: Cycle backdrop (**Live BG**, **Lake Tahoe**, **Optics Lab**)
- `R`: Re-capture live desktop behind the glass
- `F` / `A` / `W`: Toggle Applications window / Alert modal / Widgets
- `T` / `C` / `S`: Toggle Shader Lab / Control Center / Spotlight
- `Esc` / `Q`: Exit
