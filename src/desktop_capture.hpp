#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace lg {

struct CapturedFrame {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> rgb; // 3 bytes per pixel, top-to-bottom

    bool valid() const {
        return width > 0 && height > 0 &&
               rgb.size() == static_cast<size_t>(width) * static_cast<size_t>(height) * 3;
    }
};

// Captures the live Wayland desktop via `grim -t ppm -` (uses wlr-screencopy).
// Returns false if grim is unavailable or failed (e.g. headless environment).
bool captureLiveDesktop(CapturedFrame& outFrame, std::string& outError);

} // namespace lg
