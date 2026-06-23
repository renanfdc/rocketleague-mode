#include "texture_manager.h"
#include "texture_loader.h"
#include "../ue3/ue3_reflection.h"
#include "../util/logger.h"
#include "../util/config.h"
#include <filesystem>

namespace TextureManager {

static UObject* g_currentTexture = nullptr;
static std::string g_currentFile;
static std::string g_modDir;
static bool g_initialized = false;

bool Initialize(const std::string& modDir) {
    g_modDir = modDir;
    g_initialized = true;

    std::string textureDir = modDir + "\\textures";
    if (!std::filesystem::exists(textureDir)) {
        std::filesystem::create_directories(textureDir);
        LOG_INFO("Created textures directory: %s", textureDir.c_str());
    }

    // Load texture from config
    std::string texFile = Config::GetTextureFile();
    if (!texFile.empty()) {
        return LoadTexture(texFile);
    }

    LOG_INFO("No texture configured in config.json");
    return true;
}

bool LoadTexture(const std::string& filename) {
    if (!g_initialized) return false;

    std::string fullPath;
    if (std::filesystem::path(filename).is_absolute()) {
        fullPath = filename;
    } else {
        fullPath = g_modDir + "\\textures\\" + filename;
    }

    if (!std::filesystem::exists(fullPath)) {
        LOG_ERR("Texture file does not exist: %s", fullPath.c_str());
        return false;
    }

    LOG_INFO("Loading texture: %s", fullPath.c_str());

    UObject* texture = TextureLoader::CreateTextureFromFile(fullPath);
    if (!texture) {
        LOG_ERR("Failed to create texture from: %s", fullPath.c_str());
        return false;
    }

    g_currentTexture = texture;
    g_currentFile = filename;
    LOG_INFO("Texture loaded successfully: %s", filename.c_str());
    return true;
}

UObject* GetCurrentTexture() {
    return g_currentTexture;
}

void OnMapChange() {
    // The UTexture2DDynamic may become invalid after map change
    // We'll need to recreate it when the ball spawns
    LOG_DEBUG("Map change - texture reference may need refresh");
    // Don't null out g_currentTexture - it might still be valid
    // The hook will recreate if needed
}

void CheckForReload() {
    if (!g_initialized) return;

    std::string newTexFile = Config::GetTextureFile();
    if (newTexFile != g_currentFile && !newTexFile.empty()) {
        LOG_INFO("Config changed - reloading texture: %s", newTexFile.c_str());
        LoadTexture(newTexFile);
    }
}

void Shutdown() {
    g_currentTexture = nullptr;
    g_initialized = false;
}

} // namespace TextureManager
