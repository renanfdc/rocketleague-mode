#include "tex_hook.h"
#include "../util/logger.h"
#include "../util/config.h"

// Forward declare NightMode
namespace NightMode {
    void Init(ID3D11Device* device, ID3D11DeviceContext* context);
    void Render(IDXGISwapChain* swapChain);
    void SetEnabled(bool enabled);
    void SetIntensity(float intensity);
    void Shutdown();
}
#include <d3d11.h>
#include <dxgi.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <atomic>
#include <unordered_map>

// MinHook
#include "../../lib/MinHook/MinHook.h"

// stb_image (implementation in stb_impl.cpp)
#include "../../lib/stb_image.h"

// GUID definitions for MinGW (no __uuidof support)
static const GUID IID_ID3D11Texture2D_local =
    {0x6f15aaf2, 0xd208, 0x4e89, {0x9a, 0xb4, 0x48, 0x95, 0x35, 0xd3, 0x4f, 0x9c}};
static const GUID IID_ID3D11Device_local =
    {0xdb6f6ddb, 0xac77, 0x4e88, {0x82, 0x53, 0x81, 0x9d, 0xf9, 0xbb, 0xf1, 0x40}};

// ========== Configuration ==========
// Set to true for first run to discover ball texture dimensions
static std::atomic<bool> g_logMode{true};

// Sky texture targets - DISABLED for logging phase
// Will be enabled once we identify cubemap textures
static bool g_skyReplaceEnabled = false;
static ID3D11ShaderResourceView* g_nightSkySRV_BC1 = nullptr;

static struct TargetTex {
    UINT width, height;
    DXGI_FORMAT format;
    UINT slot;
    UINT arraySize; // 6 = cubemap
} g_targets[] = {
    // BALL diffuse: 640x640 BC1_UNORM_SRGB (DXGI_FORMAT 72), ArraySize=1
    // Identified via discovery log - this is THE ball texture
    {640, 640, (DXGI_FORMAT)72, 0, 1},  // Ball BC1_SRGB
    {640, 640, (DXGI_FORMAT)71, 0, 1},  // Ball BC1_UNORM (non-SRGB fallback)
    // CUBEMAP skybox: 256x256 R8G8B8A8 ArraySize=6
    {256, 256, (DXGI_FORMAT)29, 6, 6},  // Slot 6 (most frequent)
    {256, 256, (DXGI_FORMAT)29, 2, 6},  // Slot 2
    {256, 256, (DXGI_FORMAT)29, 1, 6},  // Slot 1
    {256, 256, (DXGI_FORMAT)29, 4, 6},  // Slot 4
    {256, 256, (DXGI_FORMAT)72, 2, 6},  // BC1_SRGB variant
    // Sentinel
    {0, 0, (DXGI_FORMAT)0, 0, 0}
};

// ========== Globals ==========
static ID3D11Device* g_device = nullptr;
static ID3D11DeviceContext* g_context = nullptr;
static ID3D11Texture2D* g_customTex = nullptr;
static ID3D11ShaderResourceView* g_customSRV = nullptr;
static std::atomic<bool> g_initialized{false};
static char g_pngPath[MAX_PATH] = {};
static char g_logPath[MAX_PATH] = {};
static FILE* g_texLog = nullptr;
static UINT g_frameCount = 0;
static std::unordered_map<void*, bool> g_srvCache;

// ========== Function types ==========
typedef HRESULT(__stdcall* PresentFn)(IDXGISwapChain*, UINT, UINT);
typedef void(__stdcall* PSSetSRFn)(ID3D11DeviceContext*, UINT, UINT,
                                    ID3D11ShaderResourceView* const*);

static PresentFn g_origPresent = nullptr;
static PSSetSRFn g_origPSSetSR = nullptr;

// ========== Simple BC1 (DXT1) compressor ==========
// Compresses RGBA pixels to BC1 format (4x4 blocks -> 8 bytes each)
static void CompressBC1(const unsigned char* rgba, int w, int h, unsigned char* out) {
    int bw = (w + 3) / 4;
    int bh = (h + 3) / 4;

    for (int by = 0; by < bh; by++) {
        for (int bx = 0; bx < bw; bx++) {
            // Sample 4x4 block, find min/max colors
            unsigned char minR=255, minG=255, minB=255;
            unsigned char maxR=0, maxG=0, maxB=0;

            for (int y = 0; y < 4; y++) {
                for (int x = 0; x < 4; x++) {
                    int px = bx * 4 + x, py = by * 4 + y;
                    if (px >= w) px = w - 1;
                    if (py >= h) py = h - 1;
                    int idx = (py * w + px) * 4;
                    if (rgba[idx] < minR) minR = rgba[idx];
                    if (rgba[idx] > maxR) maxR = rgba[idx];
                    if (rgba[idx+1] < minG) minG = rgba[idx+1];
                    if (rgba[idx+1] > maxG) maxG = rgba[idx+1];
                    if (rgba[idx+2] < minB) minB = rgba[idx+2];
                    if (rgba[idx+2] > maxB) maxB = rgba[idx+2];
                }
            }

            // Pack colors as RGB565
            uint16_t c0 = ((maxR >> 3) << 11) | ((maxG >> 2) << 5) | (maxB >> 3);
            uint16_t c1 = ((minR >> 3) << 11) | ((minG >> 2) << 5) | (minB >> 3);
            if (c0 < c1) { uint16_t t = c0; c0 = c1; c1 = t; }

            int blockOff = (by * bw + bx) * 8;
            out[blockOff + 0] = c0 & 0xFF;
            out[blockOff + 1] = c0 >> 8;
            out[blockOff + 2] = c1 & 0xFF;
            out[blockOff + 3] = c1 >> 8;

            // Generate 2-bit indices for each pixel
            uint32_t indices = 0;
            for (int y = 0; y < 4; y++) {
                for (int x = 0; x < 4; x++) {
                    int px = bx * 4 + x, py = by * 4 + y;
                    if (px >= w) px = w - 1;
                    if (py >= h) py = h - 1;
                    int idx = (py * w + px) * 4;

                    // Simple nearest-match to c0 or c1
                    int dr0 = rgba[idx] - maxR, dg0 = rgba[idx+1] - maxG, db0 = rgba[idx+2] - maxB;
                    int dr1 = rgba[idx] - minR, dg1 = rgba[idx+1] - minG, db1 = rgba[idx+2] - minB;
                    int d0 = dr0*dr0 + dg0*dg0 + db0*db0;
                    int d1 = dr1*dr1 + dg1*dg1 + db1*db1;

                    uint32_t sel;
                    if (d0 <= d1) sel = 0;       // closest to c0
                    else if (d1 < d0/3) sel = 1; // closest to c1
                    else if (d0 < d1) sel = 2;   // 2/3 c0 + 1/3 c1
                    else sel = 3;                 // 1/3 c0 + 2/3 c1

                    indices |= sel << ((y * 4 + x) * 2);
                }
            }
            out[blockOff + 4] = indices & 0xFF;
            out[blockOff + 5] = (indices >> 8) & 0xFF;
            out[blockOff + 6] = (indices >> 16) & 0xFF;
            out[blockOff + 7] = (indices >> 24) & 0xFF;
        }
    }
}

// ========== Create textures in multiple formats ==========
static ID3D11ShaderResourceView* g_srvRGBA = nullptr;
static ID3D11ShaderResourceView* g_srvBC1 = nullptr;
static ID3D11ShaderResourceView* g_srvBC1_SRGB = nullptr;

static bool CreateTexturesFromPNG(ID3D11Device* device) {
    if (!g_pngPath[0]) return false;

    int w, h, ch;
    unsigned char* pixels = stbi_load(g_pngPath, &w, &h, &ch, 4);
    if (!pixels) {
        LOG_ERR("[D3D11] Failed to load PNG: %s", g_pngPath);
        return false;
    }
    LOG_INFO("[D3D11] PNG loaded: %dx%d", w, h);

    // Create dark night sky texture (2048x2048, dark blue)
    if (Config::GetNightMode()) {
        int skyW = 2048, skyH = 2048;
        // Dark blue-black sky pixels
        unsigned char* skyPixels = new unsigned char[skyW * skyH * 4];
        for (int y = 0; y < skyH; y++) {
            for (int x = 0; x < skyW; x++) {
                int idx = (y * skyW + x) * 4;
                // Gradient: darker at top, slightly lighter at horizon
                float t = (float)y / skyH; // 0=top, 1=bottom
                skyPixels[idx + 0] = (unsigned char)(5 + t * 15);   // R: very dark
                skyPixels[idx + 1] = (unsigned char)(8 + t * 20);   // G: slightly more
                skyPixels[idx + 2] = (unsigned char)(20 + t * 40);  // B: blueish
                skyPixels[idx + 3] = 255;
            }
        }

        // Add some "stars" (random bright pixels in upper half)
        srand(42);
        for (int s = 0; s < 500; s++) {
            int sx = rand() % skyW;
            int sy = rand() % (skyH / 2);
            int idx = (sy * skyW + sx) * 4;
            int brightness = 150 + rand() % 105;
            skyPixels[idx + 0] = brightness;
            skyPixels[idx + 1] = brightness;
            skyPixels[idx + 2] = brightness;
        }

        // Create RGBA SRV for sky
        {
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = skyW; desc.Height = skyH; desc.MipLevels = 1; desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA init = {};
            init.pSysMem = skyPixels; init.SysMemPitch = skyW * 4;

            ID3D11Texture2D* tex = nullptr;
            if (SUCCEEDED(device->CreateTexture2D(&desc, &init, &tex))) {
                D3D11_SHADER_RESOURCE_VIEW_DESC srv = {};
                srv.Format = desc.Format; srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srv.Texture2D.MipLevels = 1;
                // We'll use this for R8G8B8A8 format
                ID3D11ShaderResourceView* nightSRV = nullptr;
                device->CreateShaderResourceView(tex, &srv, &nightSRV);
                tex->Release();
                if (nightSRV) LOG_INFO("[D3D11] Night sky RGBA SRV created");
                // Store as "sky" SRV - but we need BC1/BC3 format versions too
            }
        }

        // Create BC1 compressed night sky
        {
            int bc1Size = ((skyW + 3) / 4) * ((skyH + 3) / 4) * 8;
            unsigned char* bc1Data = new unsigned char[bc1Size];
            CompressBC1(skyPixels, skyW, skyH, bc1Data);

            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = skyW; desc.Height = skyH; desc.MipLevels = 1; desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_BC1_UNORM;
            desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA init = {};
            init.pSysMem = bc1Data; init.SysMemPitch = ((skyW + 3) / 4) * 8;

            ID3D11Texture2D* tex = nullptr;
            if (SUCCEEDED(device->CreateTexture2D(&desc, &init, &tex))) {
                D3D11_SHADER_RESOURCE_VIEW_DESC srv = {};
                srv.Format = DXGI_FORMAT_BC1_UNORM;
                srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srv.Texture2D.MipLevels = 1;
                device->CreateShaderResourceView(tex, &srv, &g_nightSkySRV_BC1);
                tex->Release();

                // Also create BC1_SRGB version
                desc.Format = DXGI_FORMAT_BC1_UNORM_SRGB;
                init.pSysMem = bc1Data;
                if (SUCCEEDED(device->CreateTexture2D(&desc, &init, &tex))) {
                    srv.Format = DXGI_FORMAT_BC1_UNORM_SRGB;
                    ID3D11ShaderResourceView* srgbSRV = nullptr;
                    device->CreateShaderResourceView(tex, &srv, &srgbSRV);
                    tex->Release();
                    // Use srgbSRV as needed
                }
                LOG_INFO("[D3D11] Night sky BC1 SRV created");
            }
            delete[] bc1Data;
        }

        delete[] skyPixels;
        // Also create 1024x1024 version for smaller sky textures
        {
            int sw = 1024, sh = 1024;
            unsigned char* sp = new unsigned char[sw * sh * 4];
            for (int y = 0; y < sh; y++) {
                for (int x = 0; x < sw; x++) {
                    int idx = (y * sw + x) * 4;
                    float t = (float)y / sh;
                    sp[idx+0] = (unsigned char)(5 + t * 15);
                    sp[idx+1] = (unsigned char)(8 + t * 20);
                    sp[idx+2] = (unsigned char)(20 + t * 40);
                    sp[idx+3] = 255;
                }
            }
            // Stars
            for (int s = 0; s < 200; s++) {
                int sx = rand() % sw, sy = rand() % (sh/2);
                int idx = (sy * sw + sx) * 4;
                sp[idx+0] = sp[idx+1] = sp[idx+2] = 150 + rand() % 105;
            }

            int bc1Size = ((sw+3)/4) * ((sh+3)/4) * 8;
            unsigned char* bc1 = new unsigned char[bc1Size];
            CompressBC1(sp, sw, sh, bc1);

            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width=sw; desc.Height=sh; desc.MipLevels=1; desc.ArraySize=1;
            desc.Format = DXGI_FORMAT_BC1_UNORM;
            desc.SampleDesc.Count=1; desc.Usage=D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            D3D11_SUBRESOURCE_DATA init = {};
            init.pSysMem = bc1; init.SysMemPitch = ((sw+3)/4)*8;

            ID3D11Texture2D* tex = nullptr;
            if (SUCCEEDED(device->CreateTexture2D(&desc, &init, &tex))) {
                // Create both UNORM and SRGB SRVs
                D3D11_SHADER_RESOURCE_VIEW_DESC srv = {};
                srv.Format = DXGI_FORMAT_BC1_UNORM;
                srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srv.Texture2D.MipLevels = 1;
                // Store in g_nightSkySRV_BC1 (overwrite the 2048 one - this is for matching)
                ID3D11ShaderResourceView* s1024 = nullptr;
                device->CreateShaderResourceView(tex, &srv, &s1024);
                tex->Release();
                if (s1024) {
                    g_nightSkySRV_BC1 = s1024; // Use 1024 version (matches most targets)
                    LOG_INFO("[D3D11] Night sky 2048x1024 BC1 SRV created");
                }
            }
            delete[] bc1;
            delete[] sp;
        }

        g_skyReplaceEnabled = true;
        LOG_INFO("[D3D11] Night sky replacement ENABLED");
    }

    // Create RGBA texture (for format 29 = R8G8B8A8_UNORM)
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = w; desc.Height = h; desc.MipLevels = 1; desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem = pixels; init.SysMemPitch = w * 4;

        ID3D11Texture2D* tex = nullptr;
        if (SUCCEEDED(device->CreateTexture2D(&desc, &init, &tex))) {
            D3D11_SHADER_RESOURCE_VIEW_DESC srv = {};
            srv.Format = desc.Format; srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srv.Texture2D.MipLevels = 1;
            device->CreateShaderResourceView(tex, &srv, &g_srvRGBA);
            tex->Release();
            LOG_INFO("[D3D11] Created RGBA SRV");
        }
    }

    // Create BC1 compressed texture (for format 71 = BC1_UNORM)
    {
        int bc1Size = ((w + 3) / 4) * ((h + 3) / 4) * 8;
        unsigned char* bc1Data = new unsigned char[bc1Size];
        CompressBC1(pixels, w, h, bc1Data);

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = w; desc.Height = h; desc.MipLevels = 1; desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_BC1_UNORM;
        desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem = bc1Data; init.SysMemPitch = ((w + 3) / 4) * 8;

        ID3D11Texture2D* tex = nullptr;
        HRESULT hr = device->CreateTexture2D(&desc, &init, &tex);
        if (SUCCEEDED(hr)) {
            D3D11_SHADER_RESOURCE_VIEW_DESC srv = {};
            srv.Format = DXGI_FORMAT_BC1_UNORM;
            srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srv.Texture2D.MipLevels = 1;
            device->CreateShaderResourceView(tex, &srv, &g_srvBC1);
            tex->Release();
            LOG_INFO("[D3D11] Created BC1_UNORM SRV");

            // Also BC1_SRGB
            desc.Format = DXGI_FORMAT_BC1_UNORM_SRGB;
            init.pSysMem = bc1Data;
            if (SUCCEEDED(device->CreateTexture2D(&desc, &init, &tex))) {
                srv.Format = DXGI_FORMAT_BC1_UNORM_SRGB;
                device->CreateShaderResourceView(tex, &srv, &g_srvBC1_SRGB);
                tex->Release();
                LOG_INFO("[D3D11] Created BC1_SRGB SRV");
            }
        } else {
            LOG_ERR("[D3D11] BC1 CreateTexture2D failed: 0x%08lX", hr);
        }
        delete[] bc1Data;
    }

    stbi_image_free(pixels);
    g_customSRV = g_srvRGBA; // default
    return (g_srvRGBA || g_srvBC1);
}

// BC1_SRGB SRV (declared in createNightSky lambda, need global access)
static ID3D11ShaderResourceView* g_nightSRV_BC1_SRGB_global = nullptr;

static ID3D11ShaderResourceView* GetSRVForFormat(DXGI_FORMAT fmt) {
    if (g_skyReplaceEnabled) {
        switch(fmt) {
            case DXGI_FORMAT_BC1_UNORM_SRGB: // 72
                return g_nightSRV_BC1_SRGB_global ? g_nightSRV_BC1_SRGB_global : g_nightSkySRV_BC1;
            case DXGI_FORMAT_BC1_UNORM: // 71
                return g_nightSkySRV_BC1;
            default:
                return g_nightSkySRV_BC1; // fallback
        }
    }
    // Fallback for ball textures
    switch (fmt) {
        case DXGI_FORMAT_BC1_UNORM:      return g_srvBC1;
        case DXGI_FORMAT_BC1_UNORM_SRGB: return g_srvBC1_SRGB;
        case DXGI_FORMAT_BC3_UNORM:      return g_srvBC1;
        case DXGI_FORMAT_BC3_UNORM_SRGB: return g_srvBC1_SRGB;
        case DXGI_FORMAT_R8G8B8A8_UNORM: return g_srvRGBA;
        default: return g_srvBC1;
    }
}

// ========== Check if texture matches any target ==========
static DXGI_FORMAT g_lastMatchedFormat = DXGI_FORMAT_UNKNOWN;

static bool IsTargetTexture(ID3D11ShaderResourceView* srv, UINT slot) {
    if (!srv || g_targets[0].width == 0) return false;

    // Fast cache lookup (pointer-based, no COM calls)
    auto it = g_srvCache.find((void*)srv);
    if (it != g_srvCache.end()) return it->second;

    // Slow path: first time seeing this SRV
    ID3D11Resource* resource = nullptr;
    srv->GetResource(&resource);
    if (!resource) { g_srvCache[(void*)srv] = false; return false; }

    ID3D11Texture2D* tex2d = nullptr;
    HRESULT hr = resource->QueryInterface(IID_ID3D11Texture2D_local, (void**)&tex2d);
    resource->Release();
    if (FAILED(hr) || !tex2d) { g_srvCache[(void*)srv] = false; return false; }

    D3D11_TEXTURE2D_DESC desc;
    tex2d->GetDesc(&desc);
    tex2d->Release();

    bool match = false;
    for (int i = 0; g_targets[i].width > 0; i++) {
        if (desc.Width == g_targets[i].width &&
            desc.Height == g_targets[i].height &&
            desc.Format == g_targets[i].format &&
            desc.ArraySize == g_targets[i].arraySize) {
            g_lastMatchedFormat = desc.Format;
            match = true;
            break;
        }
    }
    g_srvCache[(void*)srv] = match;
    return match;
}

// ========== Log texture info ==========
static void LogTexture(ID3D11ShaderResourceView* srv, UINT slot) {
    if (!g_texLog || !srv) return;

    ID3D11Resource* resource = nullptr;
    srv->GetResource(&resource);
    if (!resource) return;

    ID3D11Texture2D* tex2d = nullptr;
    HRESULT hr = resource->QueryInterface(IID_ID3D11Texture2D_local, (void**)&tex2d);
    resource->Release();
    if (FAILED(hr) || !tex2d) return;

    D3D11_TEXTURE2D_DESC desc;
    tex2d->GetDesc(&desc);
    tex2d->Release();

    // Log textures >= 64x64 (skip tiny utility textures)
    if (desc.Width >= 64 && desc.Height >= 64) {
        fprintf(g_texLog, "Frame=%u Slot=%u Size=%ux%u Fmt=%u Mips=%u Array=%u Bind=0x%X\n",
                g_frameCount, slot, desc.Width, desc.Height,
                (unsigned)desc.Format, desc.MipLevels, desc.ArraySize, desc.BindFlags);
    }
}

// ========== Hook: PSSetShaderResources ==========
static void __stdcall Hook_PSSetSR(
    ID3D11DeviceContext* ctx, UINT startSlot, UINT numViews,
    ID3D11ShaderResourceView* const* ppSRVs)
{
    // Logging mode: discover textures
    if (g_logMode.load() && g_texLog && ppSRVs) {
        for (UINT i = 0; i < numViews; i++) {
            if (ppSRVs[i]) LogTexture(ppSRVs[i], startSlot + i);
        }
    }

    // Replacement mode: swap matching textures with format-compatible SRV
    if ((g_srvRGBA || g_srvBC1 || g_nightSkySRV_BC1 || g_nightSRV_BC1_SRGB_global) && ppSRVs) {
        ID3D11ShaderResourceView* modified[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
        bool swapped = false;
        UINT count = (numViews > 128) ? 128 : numViews;

        for (UINT i = 0; i < count; i++) {
            modified[i] = ppSRVs[i];
            if (ppSRVs[i] && IsTargetTexture(ppSRVs[i], startSlot + i)) {
                // Use SRV with matching format
                ID3D11ShaderResourceView* replacement = GetSRVForFormat(g_lastMatchedFormat);
                if (replacement) {
                    modified[i] = replacement;
                    swapped = true;
                }
            }
        }

        if (swapped) {
            static int swapLog = 0;
            if (swapLog < 5) {
                swapLog++;
                LOG_INFO("[D3D11] Swapped texture! slot=%u fmt=%u", startSlot, (unsigned)g_lastMatchedFormat);
            }
            g_origPSSetSR(ctx, startSlot, count, modified);
            return;
        }
    }

    g_origPSSetSR(ctx, startSlot, numViews, ppSRVs);
}

// ========== Hook: Present ==========
static HRESULT __stdcall Hook_Present(IDXGISwapChain* swapChain, UINT sync, UINT flags) {
    if (!g_initialized.load()) {
        HRESULT hr = swapChain->GetDevice(IID_ID3D11Device_local, (void**)&g_device);
        if (SUCCEEDED(hr) && g_device) {
            g_device->GetImmediateContext(&g_context);
            LOG_INFO("[D3D11] Got game device and context");

            // Create custom textures in multiple formats
            if (CreateTexturesFromPNG(g_device)) {
                LOG_INFO("[D3D11] Ball textures ready (RGBA + BC1)!");
            }

            // Initialize night mode if enabled
            NightMode::Init(g_device, g_context);
            if (Config::GetNightMode()) {
                NightMode::SetEnabled(true);
                NightMode::SetIntensity(Config::GetNightIntensity());
                LOG_INFO("[D3D11] Night mode ACTIVE");

                // Create night sky textures HERE (not in CreateTexturesFromPNG)
                auto createNightSky = [&](int sw, int sh) {
                    unsigned char* sp = new unsigned char[sw * sh * 4];
                    srand(42);
                    for (int y = 0; y < sh; y++) {
                        for (int x = 0; x < sw; x++) {
                            int idx = (y * sw + x) * 4;
                            float t = (float)y / sh;
                            sp[idx+0] = (unsigned char)(5 + t * 15);
                            sp[idx+1] = (unsigned char)(8 + t * 20);
                            sp[idx+2] = (unsigned char)(20 + t * 40);
                            sp[idx+3] = 255;
                        }
                    }
                    for (int s = 0; s < sw/5; s++) {
                        int sx = rand()%sw, sy = rand()%(sh/2);
                        int idx = (sy*sw+sx)*4;
                        sp[idx+0]=sp[idx+1]=sp[idx+2]=150+rand()%105;
                    }

                    int bc1Sz = ((sw+3)/4)*((sh+3)/4)*8;
                    unsigned char* bc1 = new unsigned char[bc1Sz];
                    CompressBC1(sp, sw, sh, bc1);

                    D3D11_TEXTURE2D_DESC desc = {};
                    desc.Width=sw; desc.Height=sh; desc.MipLevels=1; desc.ArraySize=1;
                    desc.Format=DXGI_FORMAT_BC1_UNORM; desc.SampleDesc.Count=1;
                    desc.Usage=D3D11_USAGE_DEFAULT; desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
                    D3D11_SUBRESOURCE_DATA init = {};
                    init.pSysMem=bc1; init.SysMemPitch=((sw+3)/4)*8;

                    ID3D11Texture2D* tex = nullptr;
                    if (SUCCEEDED(g_device->CreateTexture2D(&desc, &init, &tex))) {
                        D3D11_SHADER_RESOURCE_VIEW_DESC srv = {};
                        srv.Format=DXGI_FORMAT_BC1_UNORM;
                        srv.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;
                        srv.Texture2D.MipLevels=1;
                        g_device->CreateShaderResourceView(tex, &srv, &g_nightSkySRV_BC1);
                        tex->Release();
                        LOG_INFO("[D3D11] Night sky %dx%d BC1 SRV created!", sw, sh);
                    }
                    delete[] bc1;
                    delete[] sp;
                };

                // Create 1024x1024 night sky with TYPELESS base + multiple SRV formats
                {
                    int sw=2048, sh=2048;
                    unsigned char* sp = new unsigned char[sw*sh*4];
                    srand(42);
                    for(int y=0;y<sh;y++) for(int x=0;x<sw;x++) {
                        int i=(y*sw+x)*4; float t=(float)y/sh;
                        sp[i]=5+(int)(t*15); sp[i+1]=8+(int)(t*20);
                        sp[i+2]=20+(int)(t*40); sp[i+3]=255;
                    }
                    for(int s=0;s<200;s++){int sx=rand()%sw,sy=rand()%(sh/2);
                        int i=(sy*sw+sx)*4;sp[i]=sp[i+1]=sp[i+2]=150+rand()%105;}

                    int bc1Sz=((sw+3)/4)*((sh+3)/4)*8;
                    unsigned char* bc1=new unsigned char[bc1Sz];
                    CompressBC1(sp, sw, sh, bc1);

                    // Create as BC1_TYPELESS so we can make SRVs of any BC1 format
                    D3D11_TEXTURE2D_DESC desc={};
                    desc.Width=sw; desc.Height=sh; desc.MipLevels=1; desc.ArraySize=1;
                    desc.Format=DXGI_FORMAT_BC1_TYPELESS; // TYPELESS!
                    desc.SampleDesc.Count=1; desc.Usage=D3D11_USAGE_DEFAULT;
                    desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
                    D3D11_SUBRESOURCE_DATA init={}; init.pSysMem=bc1; init.SysMemPitch=((sw+3)/4)*8;

                    ID3D11Texture2D* tex=nullptr;
                    if(SUCCEEDED(g_device->CreateTexture2D(&desc,&init,&tex))) {
                        // BC1_UNORM SRV
                        D3D11_SHADER_RESOURCE_VIEW_DESC srv={};
                        srv.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;
                        srv.Texture2D.MipLevels=1;

                        srv.Format=DXGI_FORMAT_BC1_UNORM;
                        g_device->CreateShaderResourceView(tex,&srv,&g_nightSkySRV_BC1);

                        // BC1_SRGB SRV (same texture, different view!)
                        srv.Format=DXGI_FORMAT_BC1_UNORM_SRGB;
                        g_device->CreateShaderResourceView(tex,&srv,&g_nightSRV_BC1_SRGB_global);

                        tex->Release();
                        LOG_INFO("[D3D11] Night sky 2048 TYPELESS + BC1_UNORM + BC1_SRGB SRVs created!");
                    }
                    delete[] bc1; delete[] sp;
                }

                // Create night sky CUBEMAP (6 faces, 256x256, R8G8B8A8)
                {
                    int cw=256, ch=256, fb=cw*ch*4;
                    unsigned char* faces[6];
                    srand(42);
                    for(int f=0;f<6;f++){
                        faces[f]=new unsigned char[fb];
                        float dk=(f==2)?0.5f:(f==3)?1.5f:1.0f;
                        for(int y=0;y<ch;y++) for(int x=0;x<cw;x++){
                            int i=(y*cw+x)*4; float t=(float)y/ch;
                            faces[f][i]=(unsigned char)((3+t*8)*dk);
                            faces[f][i+1]=(unsigned char)((5+t*12)*dk);
                            faces[f][i+2]=(unsigned char)((15+t*30)*dk);
                            faces[f][i+3]=255;
                        }
                        if(f!=3) for(int s=0;s<80;s++){
                            int sx=rand()%cw,sy=rand()%ch,i=(sy*cw+sx)*4,b=140+rand()%115;
                            faces[f][i]=b;faces[f][i+1]=b;faces[f][i+2]=b;
                        }
                    }
                    D3D11_TEXTURE2D_DESC cd={};
                    cd.Width=cw;cd.Height=ch;cd.MipLevels=1;cd.ArraySize=6;
                    cd.Format=DXGI_FORMAT_R8G8B8A8_UNORM;cd.SampleDesc.Count=1;
                    cd.Usage=D3D11_USAGE_DEFAULT;cd.BindFlags=D3D11_BIND_SHADER_RESOURCE;
                    cd.MiscFlags=D3D11_RESOURCE_MISC_TEXTURECUBE;
                    D3D11_SUBRESOURCE_DATA id[6]={};
                    for(int f=0;f<6;f++){id[f].pSysMem=faces[f];id[f].SysMemPitch=cw*4;}
                    ID3D11Texture2D* ct=nullptr;
                    if(SUCCEEDED(g_device->CreateTexture2D(&cd,id,&ct))){
                        D3D11_SHADER_RESOURCE_VIEW_DESC sv={};
                        sv.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
                        sv.ViewDimension=D3D11_SRV_DIMENSION_TEXTURECUBE;
                        sv.TextureCube.MipLevels=1;
                        ID3D11ShaderResourceView* cs=nullptr;
                        g_device->CreateShaderResourceView(ct,&sv,&cs);
                        ct->Release();
                        if(cs){g_nightSkySRV_BC1=cs;LOG_INFO("[D3D11] Night CUBEMAP created!");}
                    }
                    for(int f=0;f<6;f++) delete[] faces[f];
                }

                g_skyReplaceEnabled = true;
                LOG_INFO("[D3D11] Sky cubemap replacement ENABLED");
            }

            // Open texture log
            if (g_logMode.load() && g_logPath[0]) {
                g_texLog = fopen(g_logPath, "w");
                if (g_texLog) {
                    fprintf(g_texLog, "=== Texture Discovery Log ===\n");
                    fprintf(g_texLog, "Play in Freeplay and look at the ball.\n");
                    fprintf(g_texLog, "Find textures that appear consistently.\n\n");
                    LOG_INFO("[D3D11] Logging textures to %s", g_logPath);
                }
            }

            g_initialized.store(true);
        }
    }

    g_frameCount++;
    // Clear SRV cache every 60 seconds
    if (g_frameCount % 3600 == 0) g_srvCache.clear();

    // Stop logging after 300 frames (~5 seconds)
    if (g_logMode.load() && g_texLog && g_frameCount > 300) {
        fprintf(g_texLog, "\n=== Logging stopped after 500 frames ===\n");
        fclose(g_texLog);
        g_texLog = nullptr;
        g_logMode.store(false);
        LOG_INFO("[D3D11] Texture logging complete - check texhook_log.txt");
    }

    // Night mode overlay DISABLED - using sky texture replacement instead
    NightMode::Render(swapChain);

    return g_origPresent(swapChain, sync, flags);
}

// ========== Init ==========
namespace TexHook {

void Init(const char* pngPath, const char* logPath) {
    if (pngPath) strncpy(g_pngPath, pngPath, MAX_PATH - 1);
    if (logPath) strncpy(g_logPath, logPath, MAX_PATH - 1);

    LOG_INFO("[D3D11] Initializing texture hook...");

    // Wait for DXGI to be loaded
    HMODULE hDXGI = nullptr;
    for (int i = 0; i < 30; i++) {
        hDXGI = GetModuleHandleA("dxgi.dll");
        if (hDXGI) break;
        Sleep(1000);
    }
    if (!hDXGI) {
        LOG_ERR("[D3D11] dxgi.dll not loaded after 30s");
        return;
    }
    LOG_INFO("[D3D11] dxgi.dll found");

    // Create dummy window
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "TexHookDummy";
    RegisterClassExA(&wc);
    HWND hWnd = CreateWindowA("TexHookDummy", NULL, WS_OVERLAPPEDWINDOW,
                               0, 0, 2, 2, NULL, NULL, wc.hInstance, NULL);

    // Create dummy D3D11 device + swapchain
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Width = 2;
    scd.BufferDesc.Height = 2;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate = {60, 1};
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};
    D3D_FEATURE_LEVEL obtained;
    IDXGISwapChain* dummySC = nullptr;
    ID3D11Device* dummyDev = nullptr;
    ID3D11DeviceContext* dummyCtx = nullptr;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        featureLevels, 1, D3D11_SDK_VERSION, &scd,
        &dummySC, &dummyDev, &obtained, &dummyCtx);

    if (FAILED(hr)) {
        LOG_ERR("[D3D11] CreateDeviceAndSwapChain failed: 0x%08lX", hr);
        DestroyWindow(hWnd);
        return;
    }

    // Extract vtable addresses
    void** scVtable = *(void***)dummySC;
    void** ctxVtable = *(void***)dummyCtx;
    void* pPresent = scVtable[8];   // IDXGISwapChain::Present
    void* pPSSetSR = ctxVtable[8];  // PSSetShaderResources

    LOG_INFO("[D3D11] Present vtable[8] = 0x%llX", (uintptr_t)pPresent);
    LOG_INFO("[D3D11] PSSetSR vtable[8] = 0x%llX", (uintptr_t)pPSSetSR);

    dummyCtx->Release();
    dummyDev->Release();
    dummySC->Release();
    DestroyWindow(hWnd);
    UnregisterClassA("TexHookDummy", wc.hInstance);

    // Install hooks
    MH_STATUS status = MH_CreateHook(pPresent, (void*)Hook_Present, (void**)&g_origPresent);
    if (status != MH_OK) {
        LOG_ERR("[D3D11] Hook Present failed: %d", status);
        return;
    }

    status = MH_CreateHook(pPSSetSR, (void*)Hook_PSSetSR, (void**)&g_origPSSetSR);
    if (status != MH_OK) {
        LOG_ERR("[D3D11] Hook PSSetSR failed: %d", status);
        return;
    }

    MH_EnableHook(MH_ALL_HOOKS);
    LOG_INFO("[D3D11] All hooks installed! Logging mode: ON");
    LOG_INFO("[D3D11] Enter Freeplay to discover ball texture dimensions");
}

void Shutdown() {
    MH_DisableHook(MH_ALL_HOOKS);
    if (g_srvRGBA) { g_srvRGBA->Release(); g_srvRGBA = nullptr; }
    if (g_srvBC1) { g_srvBC1->Release(); g_srvBC1 = nullptr; }
    if (g_srvBC1_SRGB) { g_srvBC1_SRGB->Release(); g_srvBC1_SRGB = nullptr; }
    if (g_customTex) { g_customTex->Release(); g_customTex = nullptr; }
    if (g_context) { g_context->Release(); g_context = nullptr; }
    if (g_device) { g_device->Release(); g_device = nullptr; }
    if (g_texLog) { fclose(g_texLog); g_texLog = nullptr; }
}

} // namespace TexHook
