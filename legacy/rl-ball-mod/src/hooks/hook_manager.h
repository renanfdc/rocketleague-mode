#pragma once
#include "../ue3/ue3_types.h"
#include <functional>
#include <string>

namespace Hooks {
    // Initialize MinHook
    bool Initialize();

    // Shutdown MinHook and remove all hooks
    void Shutdown();

    // Hook a UFunction's native implementation
    // Returns true if hook was installed successfully
    bool HookUFunction(const std::string& fullFuncName,
                       void* detour, void** trampoline);

    // Install all ball-related hooks
    bool InstallBallHooks();
}
