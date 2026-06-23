#include "texture_loader.h"
#include "../ue3/ue3_reflection.h"
#include "../util/logger.h"
#include <fstream>
#include "../../lib/stb_image.h"

namespace TextureLoader {

bool ReadFile(const std::string& filepath, std::vector<uint8_t>& buffer) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;
    size_t size = file.tellg();
    if (size == 0) return false;
    buffer.resize(size);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return true;
}

ImageFormat DetectFormat(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return ImageFormat::UNKNOWN;
    if (data[0] == 0x89 && data[1] == 0x50) return ImageFormat::PNG;
    if (data[0] == 0xFF && data[1] == 0xD8) return ImageFormat::JPEG;
    return ImageFormat::UNKNOWN;
}

UObject* CreateTextureFromFile(const std::string& filepath) {
    std::vector<uint8_t> fileData;
    if (!ReadFile(filepath, fileData)) return nullptr;

    ImageFormat format = DetectFormat(fileData);
    if (format == ImageFormat::UNKNOWN) {
        LOG_ERR("Unknown image format");
        return nullptr;
    }

    int width, height, channels;
    stbi_info_from_memory(fileData.data(), (int)fileData.size(), &width, &height, &channels);
    LOG_INFO("Image: %s %dx%d ch=%d (%zu bytes)",
             format == ImageFormat::PNG ? "PNG" : "JPEG", width, height, channels, fileData.size());

    // Find UE3 functions
    UFunction* initFunc = UE3::FindFunctionByFullName("Engine.Texture2DDynamic.Init");
    if (!initFunc) { LOG_ERR("Texture2DDynamic.Init not found"); return nullptr; }

    const char* updateName = (format == ImageFormat::PNG)
        ? "Engine.Texture2DDynamic.UpdateMipFromPNG"
        : "Engine.Texture2DDynamic.UpdateMipFromJPEG";
    UFunction* updateFunc = UE3::FindFunctionByFullName(updateName);
    if (!updateFunc) { LOG_ERR("%s not found", updateName); return nullptr; }

    UFunction* dupFunc = UE3::FindFunctionByFullName("Core.Object.DuplicateObject");
    if (!dupFunc) { LOG_ERR("DuplicateObject not found"); return nullptr; }

    UObject* defaultTex = UE3::FindDefaultObject("Texture2DDynamic");
    if (!defaultTex) { LOG_ERR("Default__Texture2DDynamic not found"); return nullptr; }

    LOG_INFO("Default__Texture2DDynamic at 0x%llX, Class at 0x%llX",
             (uintptr_t)defaultTex, (uintptr_t)defaultTex->Class);

    // === FIX BUG 1: Pass DestClass = defaultTex->Class (NOT nullptr) ===
    // RLSDK: defaultObject->DuplicateObject(defaultObject, defaultObject->Outer, staticClass)
    struct DupParams {
        UObject* Template;     // 0x00
        UObject* ObjOuter;     // 0x08
        UClass*  DestClass;    // 0x10 - MUST be the UClass, not nullptr!
        UObject* ReturnValue;  // 0x18
    };

    DupParams dupParams = {};
    dupParams.Template = defaultTex;
    dupParams.ObjOuter = defaultTex->Outer;
    dupParams.DestClass = defaultTex->Class;  // FIX: was nullptr
    dupParams.ReturnValue = nullptr;

    LOG_INFO("Calling DuplicateObject(Template=0x%llX, Outer=0x%llX, Class=0x%llX)",
             (uintptr_t)dupParams.Template, (uintptr_t)dupParams.ObjOuter,
             (uintptr_t)dupParams.DestClass);

    CallProcessEvent(defaultTex, dupFunc, &dupParams);

    UObject* newTexture = dupParams.ReturnValue;
    if (!newTexture) {
        LOG_ERR("DuplicateObject returned null");
        LOG_WARN("Falling back to Default object (may not render)");
        newTexture = defaultTex;
    } else {
        LOG_INFO("*** DuplicateObject SUCCESS: 0x%llX ***", (uintptr_t)newTexture);
        // Prevent garbage collection
        newTexture->ObjectFlags &= ~0x00000020ULL;  // Clear RF_TagGarbage
        newTexture->ObjectFlags |= 0x00004000ULL;   // RF_DisregardForGC
        newTexture->ObjectFlags |= 0x00000800ULL;   // RF_RootSet
    }

    // === FIX BUG 2: PF_A8R8G8B8 = 2 (not 5) ===
    // Init with dummy 50x50 - UpdateMipFromPNG resizes based on actual image
    struct InitParams {
        int32_t InSizeX;       // 0x00
        int32_t InSizeY;       // 0x04
        uint8_t InFormat;      // 0x08 - PF_A8R8G8B8 = 2
        uint8_t Pad09[3];      // 0x09 padding
        uint32_t bIsResolve;   // 0x0C - UE3 bool = uint32! false = 0
    };
    InitParams initParams = {};
    initParams.InSizeX = 50;
    initParams.InSizeY = 50;
    initParams.InFormat = 2;     // FIX: PF_A8R8G8B8 = 2
    initParams.bIsResolve = 0;   // false - CRUCIAL per CustomBallOnline

    CallProcessEvent(newTexture, initFunc, &initParams);
    LOG_INFO("Init(50, 50, PF_A8R8G8B8=2, false) called");

    // Upload image data via UpdateMipFromPNG/JPEG
    struct UpdateMipParams {
        int32_t MipIndex;      // 0x00
        uint8_t Pad04[4];      // 0x04 padding
        TArray<uint8_t> Data;  // 0x08 (ptr=0x08, count=0x10, max=0x14)
    };
    UpdateMipParams updateParams = {};
    updateParams.MipIndex = 0;
    updateParams.Data.Data = fileData.data();
    updateParams.Data.Count = (int32_t)fileData.size();
    updateParams.Data.Max = (int32_t)fileData.size();

    CallProcessEvent(newTexture, updateFunc, &updateParams);
    LOG_INFO("Texture uploaded via %s (%zu bytes)", updateName, fileData.size());

    return newTexture;
}

} // namespace TextureLoader
