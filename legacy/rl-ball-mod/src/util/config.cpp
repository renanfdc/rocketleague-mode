#include "config.h"
#include "logger.h"
#include <fstream>
#include <filesystem>

// Use nlohmann/json for parsing
#include "../../lib/json.hpp"
using json = nlohmann::json;

namespace Config {

static std::string g_filepath;
static bool g_enabled = true;
static std::string g_textureFile;
static bool g_applyToDissolve = true;
static std::string g_logLevel = "info";
static bool g_autoTest = false;
static bool g_nightMode = false;
static float g_nightIntensity = 0.3f;
static std::filesystem::file_time_type g_lastModified;

bool Load(const std::string& filepath) {
    g_filepath = filepath;

    if (!std::filesystem::exists(filepath)) {
        LOG_WARN("Config file not found: %s", filepath.c_str());
        LOG_WARN("Using default settings");
        return false;
    }

    try {
        g_lastModified = std::filesystem::last_write_time(filepath);

        std::ifstream file(filepath);
        json cfg = json::parse(file);

        g_enabled = cfg.value("enabled", true);
        g_textureFile = cfg.value("texture_file", "");
        g_applyToDissolve = cfg.value("apply_to_dissolve", true);
        g_logLevel = cfg.value("log_level", "info");
        g_autoTest = cfg.value("auto_test", false);
        g_nightMode = cfg.value("night_mode", false);
        g_nightIntensity = cfg.value("night_intensity", 0.3f);

        LOG_INFO("Config loaded from: %s", filepath.c_str());
        LOG_INFO("  enabled: %s", g_enabled ? "true" : "false");
        LOG_INFO("  texture_file: %s", g_textureFile.c_str());
        LOG_INFO("  apply_to_dissolve: %s", g_applyToDissolve ? "true" : "false");
        LOG_INFO("  log_level: %s", g_logLevel.c_str());

        return true;
    } catch (const std::exception& e) {
        LOG_ERR("Failed to parse config: %s", e.what());
        return false;
    }
}

bool IsEnabled() { return g_enabled; }
std::string GetTextureFile() { return g_textureFile; }
bool ApplyToDissolve() { return g_applyToDissolve; }
std::string GetLogLevel() { return g_logLevel; }
bool GetAutoTest() { return g_autoTest; }
bool GetNightMode() { return g_nightMode; }
float GetNightIntensity() { return g_nightIntensity; }

bool WasModified() {
    if (g_filepath.empty()) return false;

    try {
        auto currentTime = std::filesystem::last_write_time(g_filepath);
        if (currentTime != g_lastModified) {
            g_lastModified = currentTime; // Update so we don't keep re-triggering
            return true;
        }
        return false;
    } catch (...) {
        return false;
    }
}

void ReloadIfNeeded() {
    if (WasModified()) {
        LOG_INFO("Config file changed, reloading...");
        Load(g_filepath);
    }
}

} // namespace Config
