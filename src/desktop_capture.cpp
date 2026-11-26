#include "desktop_capture.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace lg {

namespace {

bool parseNextToken(const std::vector<uint8_t>& buf, size_t& pos, std::string& token) {
    token.clear();
    while (pos < buf.size()) {
        uint8_t c = buf[pos];
        if (std::isspace(c)) {
            ++pos;
            continue;
        }
        if (c == '#') {
            while (pos < buf.size() && buf[pos] != '\n') ++pos;
            continue;
        }
        break;
    }
    while (pos < buf.size() && !std::isspace(buf[pos])) {
        if (buf[pos] == '#') break;
        token.push_back(static_cast<char>(buf[pos]));
        ++pos;
    }
    return !token.empty();
}

} // namespace

bool captureLiveDesktop(CapturedFrame& outFrame, std::string& outError) {
    FILE* pipe = popen("grim -t ppm - 2>/dev/null", "r");
    if (!pipe) {
        outError = "Failed to spawn grim";
        return false;
    }

    std::vector<uint8_t> raw;
    uint8_t chunk[65536];
    while (true) {
        size_t n = std::fread(chunk, 1, sizeof(chunk), pipe);
        if (n > 0) {
            raw.insert(raw.end(), chunk, chunk + n);
        }
        if (n < sizeof(chunk)) break;
    }
    int status = pclose(pipe);
    if (status != 0 || raw.empty()) {
        outError = "grim exited non-zero or produced no image (Wayland screencopy unavailable?)";
        return false;
    }

    size_t pos = 0;
    std::string magic, wStr, hStr, maxStr;
    if (!parseNextToken(raw, pos, magic) || magic != "P6" ||
        !parseNextToken(raw, pos, wStr) ||
        !parseNextToken(raw, pos, hStr) ||
        !parseNextToken(raw, pos, maxStr)) {
        outError = "Invalid PPM header from grim";
        return false;
    }

    // Consume the single whitespace separator before binary pixel bytes
    if (pos >= raw.size() || !std::isspace(raw[pos])) {
        outError = "Malformed PPM binary boundary";
        return false;
    }
    ++pos;

    int w = std::atoi(wStr.c_str());
    int h = std::atoi(hStr.c_str());
    int maxVal = std::atoi(maxStr.c_str());
    if (w <= 0 || h <= 0 || maxVal != 255) {
        outError = "Unsupported PPM dimensions or bit depth";
        return false;
    }

    size_t expectedBytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3;
    if (raw.size() - pos < expectedBytes) {
        outError = "Truncated PPM payload from grim";
        return false;
    }

    outFrame.width = w;
    outFrame.height = h;
    outFrame.rgb.assign(raw.begin() + static_cast<std::ptrdiff_t>(pos),
                        raw.begin() + static_cast<std::ptrdiff_t>(pos + expectedBytes));
    return true;
}

} // namespace lg
