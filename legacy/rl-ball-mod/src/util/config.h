#pragma once
#include <string>

namespace Config {
    // Load config from JSON file
    bool Load(const std::string& filepath);

    // Getters
    bool IsEnabled();
    std::string GetTextureFile();
    bool ApplyToDissolve();
    std::string GetLogLevel();
    bool GetAutoTest();
    bool GetNightMode();
    float GetNightIntensity();

    // Check if config file was modified since last load
    bool WasModified();

    // Reload if modified
    void ReloadIfNeeded();
}
