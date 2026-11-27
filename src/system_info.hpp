#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace lg {

struct DesktopApp {
    std::string id;
    std::string name;
    std::string comment;
    std::string exec;
    std::string categories;
    bool terminal = false;
    int colorIndex = 0;
};

struct SystemTelemetry {
    float cpuPercent = 0.0f;
    float ramPercent = 0.0f;
    float ramUsedGb = 0.0f;
    float ramTotalGb = 0.0f;

    bool hasBattery = false;
    int batteryPercent = 100;
    bool batteryCharging = false;

    bool hasBacklight = false;
    float brightness = 0.72f;

    float volume = 0.75f;
    bool wifiAvailable = true;
    bool wifiOn = true;
    std::string wifiSsid = "Wi-Fi";

    bool btOn = false;
    bool musicPlaying = false;
    std::string mediaTitle;
    std::string mediaArtist;

    std::string focusedAppName = "Finder";

    bool filesRunning = false;
    bool browserRunning = false;
    bool terminalRunning = false;
    bool musicRunning = false;
};

void spawnDetached(const std::string& shellCommand);
std::string detectFirstExecutable(const std::vector<std::string>& candidates);
void launchDesktopApp(const DesktopApp& app);
void launchDockApp(int dockIndex);
std::vector<DesktopApp> loadDesktopApps();

void setSystemVolume(float normalized);
void setSystemBrightness(float normalized);
void toggleSystemWifi(bool enable);
void toggleSystemBluetooth(bool enable);
void sendMediaCommand(const std::string& action); // "play-pause", "next", "previous"
void lockScreenOrSleep();

class SystemMonitor {
public:
    SystemMonitor();
    ~SystemMonitor();

    SystemMonitor(const SystemMonitor&) = delete;
    SystemMonitor& operator=(const SystemMonitor&) = delete;

    void start();
    void stop();

    SystemTelemetry snapshot() const;
    void refreshFastNow();

private:
    mutable std::mutex mutex_;
    SystemTelemetry telemetry_{};
    std::atomic<bool> running_{false};
    std::thread worker_;

    unsigned long long prevCpuIdle_ = 0;
    unsigned long long prevCpuTotal_ = 0;

    void readFastSysfsLocked();
    void pollSlowTools();
};

} // namespace lg
