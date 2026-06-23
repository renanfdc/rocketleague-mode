#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Proxy {
    bool Initialize();
    void Shutdown();
}
