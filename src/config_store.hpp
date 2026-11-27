#pragma once

#include <string>

namespace lg {

struct MizzleConfig {
    int preset = 0;      // 0..3
    int dockStyle = 0;   // 0..3
    bool dockAutoHide = false;
    int backdropMode = 0; // 0=Live, 1=Lake Tahoe, 2=Optics Lab
    bool showWidgets = true;

    // Widget sizes: 0=Small, 1=Medium, 2=Large
    int calSizeTier = 1;
    float calScale = 1.0f;
    float calDx = 0.0f;
    float calDy = 0.0f;

    int sysSizeTier = 1;
    float sysScale = 1.0f;
    float sysDx = 0.0f;
    float sysDy = 0.0f;

    int clkSizeTier = 1;
    float clkScale = 1.0f;
    float clkDx = 0.0f;
    float clkDy = 0.0f;
};

std::string mizzleConfigPath();
bool loadMizzleConfig(MizzleConfig& outCfg);
bool saveMizzleConfig(const MizzleConfig& cfg);

} // namespace lg
