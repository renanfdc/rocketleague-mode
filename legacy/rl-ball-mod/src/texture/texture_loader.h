#pragma once
#include "../ue3/ue3_types.h"
#include <string>
#include <vector>

namespace TextureLoader {
    // Create a UTexture2DDynamic from an image file (PNG or JPEG)
    // Uses UE3's built-in UpdateMipFromPNG/UpdateMipFromJPEG
    UObject* CreateTextureFromFile(const std::string& filepath);

    // Read file to byte buffer
    bool ReadFile(const std::string& filepath, std::vector<uint8_t>& buffer);

    // Detect image format
    enum class ImageFormat { PNG, JPEG, UNKNOWN };
    ImageFormat DetectFormat(const std::vector<uint8_t>& data);
}
