#pragma once

#include <string>

namespace lg {

// Renders a full 1440x900 frame offscreen via surfaceless EGL using the real
// GLSL shader pipeline and Cairo UI renderer, writing a PNG for inspection.
bool renderPreviewPng(const std::string& outPath, int width, int height, std::string& outError);

} // namespace lg
