#include "config_store.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace lg {

std::string mizzleConfigPath() {
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME")) {
        if (*xdg) return (fs::path(xdg) / "mizzle" / "config.ini").string();
    }
    if (const char* home = std::getenv("HOME")) {
        return (fs::path(home) / ".config" / "mizzle" / "config.ini").string();
    }
    return "mizzle.ini";
}

bool loadMizzleConfig(MizzleConfig& outCfg) {
    std::ifstream in(mizzleConfigPath());
    if (!in) return false;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '[') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq);
        std::string v = line.substr(eq + 1);

        if (k == "preset") outCfg.preset = std::clamp(std::stoi(v), 0, 3);
        else if (k == "dock_style") outCfg.dockStyle = std::clamp(std::stoi(v), 0, 3);
        else if (k == "dock_autohide") outCfg.dockAutoHide = (std::stoi(v) != 0);
        else if (k == "backdrop_mode") outCfg.backdropMode = std::clamp(std::stoi(v), 0, 2);
        else if (k == "show_widgets") outCfg.showWidgets = (std::stoi(v) != 0);
        else if (k == "cal_size") outCfg.calSizeTier = std::clamp(std::stoi(v), 0, 2);
        else if (k == "cal_scale") outCfg.calScale = std::clamp(std::stof(v), 0.72f, 1.80f);
        else if (k == "cal_dx") outCfg.calDx = std::stof(v);
        else if (k == "cal_dy") outCfg.calDy = std::stof(v);
        else if (k == "sys_size") outCfg.sysSizeTier = std::clamp(std::stoi(v), 0, 2);
        else if (k == "sys_scale") outCfg.sysScale = std::clamp(std::stof(v), 0.72f, 1.80f);
        else if (k == "sys_dx") outCfg.sysDx = std::stof(v);
        else if (k == "sys_dy") outCfg.sysDy = std::stof(v);
        else if (k == "clk_size") outCfg.clkSizeTier = std::clamp(std::stoi(v), 0, 2);
        else if (k == "clk_scale") outCfg.clkScale = std::clamp(std::stof(v), 0.72f, 1.80f);
        else if (k == "clk_dx") outCfg.clkDx = std::stof(v);
        else if (k == "clk_dy") outCfg.clkDy = std::stof(v);
    }
    return true;
}

bool saveMizzleConfig(const MizzleConfig& cfg) {
    fs::path p = mizzleConfigPath();
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);

    std::ofstream out(p);
    if (!out) return false;

    out << "# Mizzle Desktop Environment Configuration\n"
        << "preset=" << cfg.preset << "\n"
        << "dock_style=" << cfg.dockStyle << "\n"
        << "dock_autohide=" << (cfg.dockAutoHide ? 1 : 0) << "\n"
        << "backdrop_mode=" << cfg.backdropMode << "\n"
        << "show_widgets=" << (cfg.showWidgets ? 1 : 0) << "\n"
        << "cal_size=" << cfg.calSizeTier << "\n"
        << "cal_scale=" << cfg.calScale << "\n"
        << "cal_dx=" << cfg.calDx << "\n"
        << "cal_dy=" << cfg.calDy << "\n"
        << "sys_size=" << cfg.sysSizeTier << "\n"
        << "sys_scale=" << cfg.sysScale << "\n"
        << "sys_dx=" << cfg.sysDx << "\n"
        << "sys_dy=" << cfg.sysDy << "\n"
        << "clk_size=" << cfg.clkSizeTier << "\n"
        << "clk_scale=" << cfg.clkScale << "\n"
        << "clk_dx=" << cfg.clkDx << "\n"
        << "clk_dy=" << cfg.clkDy << "\n";
    return true;
}

} // namespace lg
