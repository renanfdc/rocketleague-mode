#include "hook_manager.h"
#include "ball_hooks.h"
#include "../ue3/ue3_reflection.h"
#include "../util/logger.h"
#include "../util/pattern_scan.h"

// MinHook
#include "../../lib/MinHook/MinHook.h"

namespace Hooks {

static bool g_initialized = false;

bool Initialize() {
    if (g_initialized) return true;

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK) {
        LOG_ERR("MinHook init failed: %d", status);
        return false;
    }

    g_initialized = true;
    LOG_INFO("MinHook initialized");
    return true;
}

void Shutdown() {
    if (!g_initialized) return;

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    g_initialized = false;
    LOG_INFO("MinHook shutdown");
}

bool HookUFunction(const std::string& fullFuncName,
                   void* detour, void** trampoline) {
    if (!g_initialized) {
        LOG_ERR("MinHook not initialized");
        return false;
    }

    // Find the UFunction
    UFunction* func = UE3::FindFunctionByFullName(fullFuncName);
    if (!func) {
        LOG_ERR("UFunction not found: %s", fullFuncName.c_str());
        return false;
    }

    // Get the native function pointer (at offset 0x158 in UFunction)
    void* nativeFunc = func->Func;

    if (!nativeFunc) {
        LOG_ERR("UFunction '%s' has null Func pointer at offset 0x158", fullFuncName.c_str());
        return false;
    }

    LOG_INFO("Hooking '%s' at 0x%llX", fullFuncName.c_str(),
             reinterpret_cast<uintptr_t>(nativeFunc));

    MH_STATUS status = MH_CreateHook(nativeFunc, detour, trampoline);
    if (status != MH_OK) {
        LOG_ERR("MH_CreateHook failed for '%s': %d", fullFuncName.c_str(), status);
        return false;
    }

    status = MH_EnableHook(nativeFunc);
    if (status != MH_OK) {
        LOG_ERR("MH_EnableHook failed for '%s': %d", fullFuncName.c_str(), status);
        return false;
    }

    LOG_INFO("Successfully hooked: %s", fullFuncName.c_str());
    return true;
}

bool InstallBallHooks() {
    LOG_INFO("=== Installing Ball Hooks ===");
    bool ok = true;

    // Hook 1: Ball fade-in (texture reapplication on ball spawn)
    ok &= HookUFunction(
        "TAGame.FXActor_Ball_TA.StartBallFadeIn",
        reinterpret_cast<void*>(BallHooks::OnBallFadeIn),
        reinterpret_cast<void**>(&BallHooks::OriginalBallFadeIn));

    // Hook 2: SetTextureParameterValue (intercept texture changes)
    ok &= HookUFunction(
        "Engine.MaterialInstanceConstant.SetTextureParameterValue",
        reinterpret_cast<void*>(BallHooks::OnSetTextureParam),
        reinterpret_cast<void**>(&BallHooks::OriginalSetTextureParam));

    // Hook 3: PreLoadMap (cleanup on map change)
    ok &= HookUFunction(
        "ProjectX.EngineShare_X.EventPreLoadMap",
        reinterpret_cast<void*>(BallHooks::OnPreLoadMap),
        reinterpret_cast<void**>(&BallHooks::OriginalPreLoadMap));

    if (ok) {
        LOG_INFO("=== All Ball Hooks Installed ===");
    } else {
        LOG_WARN("=== Some hooks failed (mod may still partially work) ===");
    }

    return ok;
}

} // namespace Hooks
