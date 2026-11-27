#include "system_info.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_set>

namespace fs = std::filesystem;

namespace lg {

namespace {

std::string trim(const std::string& s) {
    size_t a = 0;
    while (a < s.size() && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) ++a;
    size_t b = s.size();
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '\n')) --b;
    return s.substr(a, b - a);
}

std::string toLowerStr(std::string s) {
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return s;
}

bool readFirstLine(const fs::path& p, std::string& out) {
    std::ifstream in(p);
    if (!in) return false;
    std::getline(in, out);
    out = trim(out);
    return !out.empty();
}

std::string runCommandCapture(const char* cmd) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return {};
    std::string out;
    char buf[512];
    while (std::fgets(buf, sizeof(buf), pipe) != nullptr) {
        out += buf;
        if (out.size() > 32768) break;
    }
    pclose(pipe);
    return out;
}

std::string stripExecFieldCodes(const std::string& execLine) {
    std::string out;
    out.reserve(execLine.size());
    for (size_t i = 0; i < execLine.size(); ++i) {
        if (execLine[i] == '%' && i + 1 < execLine.size()) {
            ++i;
            continue;
        }
        out.push_back(execLine[i]);
    }
    return trim(out);
}

bool commandExists(const std::string& name) {
    if (name.empty()) return false;
    if (name.find('/') != std::string::npos) {
        return access(name.c_str(), X_OK) == 0;
    }
    const char* pathEnv = std::getenv("PATH");
    if (!pathEnv) return false;
    std::stringstream ss(pathEnv);
    std::string dir;
    while (std::getline(ss, dir, ':')) {
        if (dir.empty()) continue;
        fs::path full = fs::path(dir) / name;
        if (access(full.c_str(), X_OK) == 0) return true;
    }
    return false;
}

std::string extractJsonStringField(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return {};
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return {};
    size_t q1 = json.find('"', pos + 1);
    if (q1 == std::string::npos) return {};
    size_t q2 = q1 + 1;
    while (q2 < json.size()) {
        if (json[q2] == '"' && json[q2 - 1] != '\\') break;
        ++q2;
    }
    if (q2 >= json.size()) return {};
    return json.substr(q1 + 1, q2 - q1 - 1);
}

std::string prettifyAppId(const std::string& raw) {
    if (raw.empty()) return "Finder";
    std::string base = raw;
    size_t dot = base.rfind('.');
    if (dot != std::string::npos && dot + 1 < base.size()) {
        // Reverse-DNS like org.mozilla.firefox -> firefox
        std::string tail = base.substr(dot + 1);
        if (tail.size() > 1) base = tail;
    }
    if (!base.empty() && base[0] >= 'a' && base[0] <= 'z') {
        base[0] = static_cast<char>(base[0] - 'a' + 'A');
    }
    return base;
}

} // namespace

void spawnDetached(const std::string& shellCommand) {
    if (shellCommand.empty()) return;
    pid_t pid = fork();
    if (pid < 0) return;
    if (pid == 0) {
        setsid();
        pid_t pid2 = fork();
        if (pid2 < 0) _exit(1);
        if (pid2 > 0) _exit(0);
        execl("/bin/sh", "sh", "-c", shellCommand.c_str(), nullptr);
        _exit(127);
    }
    int status = 0;
    waitpid(pid, &status, 0);
}

std::string detectFirstExecutable(const std::vector<std::string>& candidates) {
    for (const auto& c : candidates) {
        if (commandExists(c)) return c;
    }
    return {};
}

void launchDesktopApp(const DesktopApp& app) {
    std::string cmd = stripExecFieldCodes(app.exec);
    if (cmd.empty()) return;
    if (app.terminal) {
        std::string term = detectFirstExecutable({"kitty", "alacritty", "foot", "ghostty", "wezterm", "gnome-terminal", "konsole", "xfce4-terminal"});
        if (!term.empty()) {
            if (term == "wezterm") cmd = "wezterm start -- " + cmd;
            else if (term == "gnome-terminal") cmd = "gnome-terminal -- " + cmd;
            else cmd = term + " -e " + cmd;
        }
    }
    spawnDetached(cmd + " >/dev/null 2>&1");
}

void launchDockApp(int dockIndex) {
    switch (dockIndex) {
        case 0: { // Finder / Files
            std::string fm = detectFirstExecutable({"nautilus", "thunar", "dolphin", "nemo", "pcmanfm", "xdg-open"});
            if (fm == "xdg-open") spawnDetached("xdg-open \"$HOME\" >/dev/null 2>&1");
            else if (!fm.empty()) spawnDetached(fm + " \"$HOME\" >/dev/null 2>&1");
            break;
        }
        case 2: { // Browser
            std::string br = detectFirstExecutable({"firefox", "zen-browser", "chromium", "google-chrome", "brave", "vivaldi", "xdg-open"});
            if (br == "xdg-open") spawnDetached("xdg-open https:// >/dev/null 2>&1");
            else if (!br.empty()) spawnDetached(br + " >/dev/null 2>&1");
            break;
        }
        case 4: { // Mail
            std::string mail = detectFirstExecutable({"thunderbird", "geary", "betterbird"});
            if (!mail.empty()) spawnDetached(mail + " >/dev/null 2>&1");
            else spawnDetached("xdg-open mailto: >/dev/null 2>&1");
            break;
        }
        case 5: { // Photos
            std::string ph = detectFirstExecutable({"loupe", "eog", "gwenview", "ristretto", "xdg-open"});
            if (ph == "xdg-open") spawnDetached("xdg-open \"$HOME/Pictures\" >/dev/null 2>&1");
            else if (!ph.empty()) spawnDetached(ph + " \"$HOME/Pictures\" >/dev/null 2>&1");
            break;
        }
        case 6: { // Calendar
            std::string cal = detectFirstExecutable({"gnome-calendar", "korganizer", "merkuro-calendar"});
            if (!cal.empty()) spawnDetached(cal + " >/dev/null 2>&1");
            break;
        }
        case 7: { // Notes / Editor
            std::string ed = detectFirstExecutable({"obsidian", "gnome-text-editor", "kate", "mousepad", "gedit", "code"});
            if (!ed.empty()) spawnDetached(ed + " >/dev/null 2>&1");
            break;
        }
        case 8: { // Music
            if (commandExists("playerctl")) {
                std::string st = trim(runCommandCapture("playerctl status 2>/dev/null"));
                if (!st.empty()) {
                    sendMediaCommand("play-pause");
                    return;
                }
            }
            std::string mus = detectFirstExecutable({"spotify", "amberol", "rhythmbox", "elisa", "vlc"});
            if (!mus.empty()) spawnDetached(mus + " >/dev/null 2>&1");
            break;
        }
        case 9: { // Terminal
            std::string term = detectFirstExecutable({"kitty", "alacritty", "foot", "ghostty", "wezterm", "gnome-terminal", "konsole", "xfce4-terminal", "xterm"});
            if (!term.empty()) spawnDetached(term + " >/dev/null 2>&1");
            break;
        }
        case 11: { // Trash
            spawnDetached("gio open trash:/// >/dev/null 2>&1 || xdg-open \"$HOME/.local/share/Trash/files\" >/dev/null 2>&1");
            break;
        }
        default:
            break;
    }
}

std::vector<DesktopApp> loadDesktopApps() {
    std::vector<DesktopApp> apps;
    std::unordered_set<std::string> seenIds;

    std::vector<fs::path> dirs = {
        fs::path("/usr/share/applications"),
        fs::path("/usr/local/share/applications"),
    };
    if (const char* home = std::getenv("HOME")) {
        dirs.insert(dirs.begin(), fs::path(home) / ".local" / "share" / "applications");
    }

    int colorIdx = 0;
    for (const auto& dir : dirs) {
        std::error_code ec;
        if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) continue;
        for (const auto& entry : fs::directory_iterator(dir, ec)) {
            if (!entry.is_regular_file(ec)) continue;
            if (entry.path().extension() != ".desktop") continue;

            std::string id = entry.path().filename().string();
            if (seenIds.count(id)) continue;

            std::ifstream in(entry.path());
            if (!in) continue;

            bool inDesktopEntry = false;
            bool noDisplay = false;
            bool hidden = false;
            bool terminal = false;
            std::string type;
            std::string name;
            std::string comment;
            std::string exec;
            std::string categories;

            std::string line;
            while (std::getline(in, line)) {
                line = trim(line);
                if (line.empty() || line[0] == '#') continue;
                if (line[0] == '[') {
                    inDesktopEntry = (line == "[Desktop Entry]");
                    continue;
                }
                if (!inDesktopEntry) continue;
                size_t eq = line.find('=');
                if (eq == std::string::npos) continue;
                std::string k = line.substr(0, eq);
                std::string v = line.substr(eq + 1);
                if (k == "Type") type = v;
                else if (k == "Name" && name.empty()) name = v;
                else if (k == "Comment" && comment.empty()) comment = v;
                else if (k == "Exec" && exec.empty()) exec = v;
                else if (k == "Categories") categories = v;
                else if (k == "NoDisplay" && toLowerStr(v) == "true") noDisplay = true;
                else if (k == "Hidden" && toLowerStr(v) == "true") hidden = true;
                else if (k == "Terminal" && toLowerStr(v) == "true") terminal = true;
            }

            if (type != "Application" || noDisplay || hidden || name.empty() || exec.empty()) continue;
            seenIds.insert(id);

            DesktopApp app;
            app.id = id;
            app.name = name;
            app.comment = comment.empty() ? "Application" : comment;
            app.exec = exec;
            app.categories = categories;
            app.terminal = terminal;
            app.colorIndex = colorIdx++;
            apps.push_back(std::move(app));
        }
    }

    std::sort(apps.begin(), apps.end(), [](const DesktopApp& a, const DesktopApp& b) {
        return toLowerStr(a.name) < toLowerStr(b.name);
    });
    return apps;
}

void setSystemVolume(float normalized) {
    int pct = static_cast<int>(std::clamp(normalized, 0.0f, 1.0f) * 100.0f + 0.5f);
    if (commandExists("wpctl")) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "wpctl set-volume @DEFAULT_AUDIO_SINK@ %.2f >/dev/null 2>&1", std::clamp(normalized, 0.0f, 1.0f));
        spawnDetached(buf);
    } else if (commandExists("pactl")) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "pactl set-sink-volume @DEFAULT_SINK@ %d%% >/dev/null 2>&1", pct);
        spawnDetached(buf);
    }
}

void setSystemBrightness(float normalized) {
    int pct = static_cast<int>(std::clamp(normalized, 0.05f, 1.0f) * 100.0f + 0.5f);
    if (commandExists("brightnessctl")) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "brightnessctl -q set %d%% >/dev/null 2>&1", pct);
        spawnDetached(buf);
    }
}

void toggleSystemWifi(bool enable) {
    if (commandExists("nmcli")) {
        spawnDetached(std::string("nmcli radio wifi ") + (enable ? "on" : "off") + " >/dev/null 2>&1");
    }
}

void toggleSystemBluetooth(bool enable) {
    if (commandExists("bluetoothctl")) {
        spawnDetached(std::string("bluetoothctl power ") + (enable ? "on" : "off") + " >/dev/null 2>&1");
    }
}

void sendMediaCommand(const std::string& action) {
    if (commandExists("playerctl")) {
        spawnDetached("playerctl " + action + " >/dev/null 2>&1");
    }
}

void lockScreenOrSleep() {
    std::string locker = detectFirstExecutable({"hyprlock", "swaylock", "gtklock"});
    if (!locker.empty()) {
        spawnDetached(locker + " >/dev/null 2>&1");
    } else {
        spawnDetached("loginctl lock-session >/dev/null 2>&1");
    }
}

SystemMonitor::SystemMonitor() {
    refreshFastNow();
}

SystemMonitor::~SystemMonitor() {
    stop();
}

void SystemMonitor::start() {
    if (running_.exchange(true)) return;
    worker_ = std::thread([this]() {
        int tick = 0;
        while (running_.load()) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                readFastSysfsLocked();
            }
            if (tick % 2 == 0) {
                pollSlowTools();
            }
            ++tick;
            for (int i = 0; i < 10 && running_.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    });
}

void SystemMonitor::stop() {
    if (!running_.exchange(false)) return;
    if (worker_.joinable()) worker_.join();
}

SystemTelemetry SystemMonitor::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return telemetry_;
}

void SystemMonitor::refreshFastNow() {
    std::lock_guard<std::mutex> lock(mutex_);
    readFastSysfsLocked();
}

void SystemMonitor::readFastSysfsLocked() {
    // 1. CPU usage from /proc/stat
    {
        std::ifstream in("/proc/stat");
        std::string label;
        unsigned long long user = 0, nice = 0, sys = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0;
        if (in >> label >> user >> nice >> sys >> idle >> iowait >> irq >> softirq >> steal && label == "cpu") {
            unsigned long long idleAll = idle + iowait;
            unsigned long long total = user + nice + sys + idleAll + irq + softirq + steal;
            if (prevCpuTotal_ > 0 && total > prevCpuTotal_) {
                unsigned long long dTotal = total - prevCpuTotal_;
                unsigned long long dIdle = idleAll - prevCpuIdle_;
                telemetry_.cpuPercent = std::clamp(100.0f * static_cast<float>(dTotal - dIdle) / static_cast<float>(dTotal), 0.0f, 100.0f);
            }
            prevCpuIdle_ = idleAll;
            prevCpuTotal_ = total;
        }
    }

    // 2. RAM usage from /proc/meminfo
    {
        std::ifstream in("/proc/meminfo");
        std::string k;
        unsigned long long v = 0;
        std::string unit;
        unsigned long long memTotalKb = 0, memAvailKb = 0;
        while (in >> k >> v >> unit) {
            if (k == "MemTotal:") memTotalKb = v;
            else if (k == "MemAvailable:") memAvailKb = v;
            if (memTotalKb > 0 && memAvailKb > 0) break;
        }
        if (memTotalKb > 0) {
            unsigned long long usedKb = memTotalKb > memAvailKb ? (memTotalKb - memAvailKb) : 0;
            telemetry_.ramTotalGb = static_cast<float>(memTotalKb) / (1024.0f * 1024.0f);
            telemetry_.ramUsedGb = static_cast<float>(usedKb) / (1024.0f * 1024.0f);
            telemetry_.ramPercent = std::clamp(100.0f * static_cast<float>(usedKb) / static_cast<float>(memTotalKb), 0.0f, 100.0f);
        }
    }

    // 3. Battery from /sys/class/power_supply
    {
        std::error_code ec;
        bool foundBat = false;
        for (const auto& entry : fs::directory_iterator("/sys/class/power_supply", ec)) {
            std::string name = entry.path().filename().string();
            if (name.rfind("BAT", 0) != 0) continue;
            std::string capStr, statusStr;
            if (readFirstLine(entry.path() / "capacity", capStr)) {
                telemetry_.hasBattery = true;
                telemetry_.batteryPercent = std::clamp(std::atoi(capStr.c_str()), 0, 100);
                readFirstLine(entry.path() / "status", statusStr);
                telemetry_.batteryCharging = (statusStr == "Charging" || statusStr == "Full");
                foundBat = true;
                break;
            }
        }
        if (!foundBat) telemetry_.hasBattery = false;
    }

    // 4. Backlight from /sys/class/backlight
    {
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator("/sys/class/backlight", ec)) {
            std::string curStr, maxStr;
            if (readFirstLine(entry.path() / "brightness", curStr) &&
                readFirstLine(entry.path() / "max_brightness", maxStr)) {
                int cur = std::atoi(curStr.c_str());
                int mx = std::atoi(maxStr.c_str());
                if (mx > 0) {
                    telemetry_.hasBacklight = true;
                    telemetry_.brightness = std::clamp(static_cast<float>(cur) / static_cast<float>(mx), 0.0f, 1.0f);
                    break;
                }
            }
        }
    }
}

void SystemMonitor::pollSlowTools() {
    SystemTelemetry next;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        next = telemetry_;
    }

    if (commandExists("wpctl")) {
        std::string out = runCommandCapture("wpctl get-volume @DEFAULT_AUDIO_SINK@ 2>/dev/null");
        size_t pos = out.find("Volume:");
        if (pos != std::string::npos) {
            float v = std::strtof(out.c_str() + pos + 7, nullptr);
            next.volume = std::clamp(v, 0.0f, 1.0f);
        }
    }

    if (commandExists("nmcli")) {
        std::string radio = trim(runCommandCapture("nmcli radio wifi 2>/dev/null"));
        if (!radio.empty()) {
            next.wifiAvailable = true;
            next.wifiOn = (radio == "enabled");
            if (next.wifiOn) {
                std::string active = trim(runCommandCapture("nmcli -t -f ACTIVE,SSID dev wifi 2>/dev/null | grep '^yes:' | head -n1 | cut -d: -f2-"));
                next.wifiSsid = active.empty() ? "Not Connected" : active;
            } else {
                next.wifiSsid = "Off";
            }
        }
    }

    if (commandExists("bluetoothctl")) {
        std::string out = runCommandCapture("bluetoothctl show 2>/dev/null | grep -m1 'Powered:'");
        if (!out.empty()) {
            next.btOn = (out.find("yes") != std::string::npos);
        }
    }

    if (commandExists("playerctl")) {
        std::string st = trim(runCommandCapture("playerctl status 2>/dev/null"));
        next.musicPlaying = (st == "Playing");
        if (st == "Playing" || st == "Paused") {
            next.mediaTitle = trim(runCommandCapture("playerctl metadata title 2>/dev/null"));
            next.mediaArtist = trim(runCommandCapture("playerctl metadata artist 2>/dev/null"));
        } else {
            next.mediaTitle.clear();
            next.mediaArtist.clear();
        }
    }

    if (commandExists("niri")) {
        std::string fw = runCommandCapture("niri msg -j focused-window 2>/dev/null");
        std::string appId = extractJsonStringField(fw, "app_id");
        if (!appId.empty() && toLowerStr(appId).find("mizzle") == std::string::npos &&
            toLowerStr(appId).find("liquid-glass-ui") == std::string::npos) {
            next.focusedAppName = prettifyAppId(appId);
        } else if (fw.find("null") != std::string::npos || fw.empty()) {
            next.focusedAppName = "Finder";
        }

        std::string wins = toLowerStr(runCommandCapture("niri msg -j windows 2>/dev/null"));
        next.terminalRunning = (wins.find("kitty") != std::string::npos || wins.find("alacritty") != std::string::npos ||
                                wins.find("foot") != std::string::npos || wins.find("ghostty") != std::string::npos ||
                                wins.find("wezterm") != std::string::npos);
        next.browserRunning = (wins.find("firefox") != std::string::npos || wins.find("chrom") != std::string::npos ||
                               wins.find("brave") != std::string::npos || wins.find("zen") != std::string::npos);
        next.filesRunning = (wins.find("nautilus") != std::string::npos || wins.find("thunar") != std::string::npos ||
                             wins.find("dolphin") != std::string::npos || wins.find("nemo") != std::string::npos);
        next.musicRunning = next.musicPlaying || (wins.find("spotify") != std::string::npos || wins.find("amberol") != std::string::npos);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    // Preserve fresh fast sysfs metrics
    next.cpuPercent = telemetry_.cpuPercent;
    next.ramPercent = telemetry_.ramPercent;
    next.ramUsedGb = telemetry_.ramUsedGb;
    next.ramTotalGb = telemetry_.ramTotalGb;
    next.hasBattery = telemetry_.hasBattery;
    next.batteryPercent = telemetry_.batteryPercent;
    next.batteryCharging = telemetry_.batteryCharging;
    next.hasBacklight = telemetry_.hasBacklight;
    if (telemetry_.hasBacklight) next.brightness = telemetry_.brightness;
    telemetry_ = std::move(next);
}

} // namespace lg
