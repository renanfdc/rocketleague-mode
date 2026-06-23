#pragma once
#include "../ue3/ue3_types.h"
#include <string>

namespace TextureManager {
    // Initialize texture system
    bool Initialize(const std::string& modDir);

    // Load a texture from file and create UTexture2DDynamic
    bool LoadTexture(const std::string& filename);

    // Get the currently loaded custom texture
    UObject* GetCurrentTexture();

    // Called when the map changes (clear transient references)
    void OnMapChange();

    // Check for config changes and reload texture if needed
    void CheckForReload();

    // Cleanup
    void Shutdown();
}
