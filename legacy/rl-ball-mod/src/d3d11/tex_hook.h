#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

namespace TexHook {
    void Init(const char* pngPath, const char* logPath);
    void Shutdown();
}

namespace NightMode {
    void Init(ID3D11Device* device, ID3D11DeviceContext* context);
    void Render(IDXGISwapChain* swapChain);
    void SetEnabled(bool enabled);
    void SetIntensity(float intensity);
    void Shutdown();
}
