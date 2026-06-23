#include "proxy.h"
#include "util/logger.h"
#include "util/config.h"
#include "ue3/ue3_globals.h"
#include "hooks/hook_manager.h"
#include "hooks/ball_hooks.h"
#include "d3d11/tex_hook.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static HMODULE g_hModule = nullptr;

static std::string GetModDirectory() {
    char p[MAX_PATH];
    GetModuleFileNameA(g_hModule, p, MAX_PATH);
    return (std::filesystem::path(p).parent_path() / "BallMod").string();
}

// Send Enter key via Windows SendInput
static void PressEnter() {
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_RETURN;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_RETURN;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));
}

// Navigate to freeplay by pressing Enter repeatedly
static void NavigateToFreeplay() {
    LOG_INFO("[AUTO] Starting menu navigation...");

    // Phase 1: Pass title screen (Press Any Button)
    for (int i = 0; i < 3; i++) {
        PressEnter();
        LOG_INFO("[AUTO] Enter sent (%d/3) - title screen", i + 1);
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    // Phase 2: Close popups (server error etc)
    std::this_thread::sleep_for(std::chrono::seconds(5));
    for (int i = 0; i < 3; i++) {
        PressEnter();
        LOG_INFO("[AUTO] Enter sent (%d/3) - popups", i + 1);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // Phase 3: Navigate to Freeplay
    // Menu: PLAY is at top. Up arrow ensures we select it.
    INPUT upKey[2] = {};
    upKey[0].type = INPUT_KEYBOARD;
    upKey[0].ki.wVk = VK_UP;
    upKey[1].type = INPUT_KEYBOARD;
    upKey[1].ki.wVk = VK_UP;
    upKey[1].ki.dwFlags = KEYEVENTF_KEYUP;

    // Go UP to ensure PLAY is selected (top item)
    for (int u = 0; u < 8; u++) {
        SendInput(2, upKey, sizeof(INPUT));
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    LOG_INFO("[AUTO] Up x8 -> PLAY selected");

    PressEnter(); // Click PLAY
    LOG_INFO("[AUTO] Enter -> PLAY");
    std::this_thread::sleep_for(std::chrono::seconds(4));

    // In Play submenu, Freeplay/Custom Games
    // Navigate with Down arrows
    INPUT downKey[2] = {};
    downKey[0].type = INPUT_KEYBOARD;
    downKey[0].ki.wVk = VK_DOWN;
    downKey[1].type = INPUT_KEYBOARD;
    downKey[1].ki.wVk = VK_DOWN;
    downKey[1].ki.dwFlags = KEYEVENTF_KEYUP;

    // Play menu opened. Go down to Training
    SendInput(2, downKey, sizeof(INPUT));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    SendInput(2, downKey, sizeof(INPUT));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    LOG_INFO("[AUTO] Down x2 -> Training");

    PressEnter(); // Open Training
    LOG_INFO("[AUTO] Enter -> Training");
    std::this_thread::sleep_for(std::chrono::seconds(4));

    // Training screen: FREE PLAY is top-left grid item
    // Click on it with mouse (Enter doesn't work on grid menus)
    HWND rlWnd = FindWindowA(nullptr, "Rocket League (64-bit, DX11, Cooked)");
    if (rlWnd) {
        RECT wr;
        GetWindowRect(rlWnd, &wr);
        int winW = wr.right - wr.left;
        int winH = wr.bottom - wr.top;

        // FREE PLAY button: center of the first grid tile
        // First tile is approx at 7-15% width, 20-35% height
        // Use client area coordinates + window offset
        POINT clientOrigin = {0, 0};
        ClientToScreen(rlWnd, &clientOrigin);
        RECT clientRect;
        GetClientRect(rlWnd, &clientRect);
        int cw = clientRect.right;
        int ch = clientRect.bottom;

        // FREE PLAY tile center: ~12% from left, ~28% from top of CLIENT area
        int clickX = clientOrigin.x + (int)(cw * 0.12);
        int clickY = clientOrigin.y + (int)(ch * 0.28);

        // Move mouse and click
        SetCursorPos(clickX, clickY);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        INPUT click[2] = {};
        click[0].type = INPUT_MOUSE;
        click[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        click[1].type = INPUT_MOUSE;
        click[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(2, click, sizeof(INPUT));
        LOG_INFO("[AUTO] Mouse click on FREE PLAY at (%d, %d)", clickX, clickY);
    } else {
        LOG_WARN("[AUTO] RL window not found for click");
        PressEnter(); // Fallback
    }

    std::this_thread::sleep_for(std::chrono::seconds(4));

    // Now we're in the FREE PLAY submenu (FREE PLAY / CREATE ONLINE / JOIN ONLINE)
    // Click on FREE PLAY again (first option, same position)
    if (rlWnd) {
        POINT co2 = {0,0};
        ClientToScreen(rlWnd, &co2);
        RECT cr2;
        GetClientRect(rlWnd, &cr2);
        int cx = co2.x + (int)(cr2.right * 0.12);
        int cy = co2.y + (int)(cr2.bottom * 0.30);
        SetCursorPos(cx, cy);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        INPUT cl[2] = {};
        cl[0].type = INPUT_MOUSE; cl[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        cl[1].type = INPUT_MOUSE; cl[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(2, cl, sizeof(INPUT));
        LOG_INFO("[AUTO] Click FREE PLAY option at (%d, %d)", cx, cy);
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    PressEnter(); // Any additional confirm
    LOG_INFO("[AUTO] Enter -> final confirm");

    LOG_INFO("[AUTO] Navigation complete - waiting for map load...");

    // Wait for map to load and auto-join team
    std::this_thread::sleep_for(std::chrono::seconds(15));
    for (int j = 0; j < 5; j++) {
        PressEnter(); // Join Blue team / dismiss popups
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    LOG_INFO("[AUTO] Auto-join team sent");
}

// Take screenshot and save to file
static void TakeScreenshot(const std::string& path) {
    HWND hwnd = FindWindowA(nullptr, "Rocket League (64-bit, DX11, Cooked)");
    if (!hwnd) { LOG_WARN("[AUTO] Game window not found for screenshot"); return; }

    RECT rect;
    GetWindowRect(hwnd, &rect);
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    if (w <= 0 || h <= 0) return;

    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBmp = CreateCompatibleBitmap(hdcScreen, w, h);
    SelectObject(hdcMem, hBmp);
    BitBlt(hdcMem, 0, 0, w, h, hdcScreen, rect.left, rect.top, SRCCOPY);

    // Save as BMP
    BITMAPINFOHEADER bi = {};
    bi.biSize = sizeof(bi);
    bi.biWidth = w;
    bi.biHeight = -h; // top-down
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;

    int rowSize = ((w * 3 + 3) & ~3);
    std::vector<uint8_t> pixels(rowSize * h);
    GetDIBits(hdcMem, hBmp, 0, h, pixels.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    // Write BMP file
    BITMAPFILEHEADER bf = {};
    bf.bfType = 0x4D42;
    bf.bfSize = sizeof(bf) + sizeof(bi) + pixels.size();
    bf.bfOffBits = sizeof(bf) + sizeof(bi);

    std::ofstream f(path, std::ios::binary);
    f.write((char*)&bf, sizeof(bf));
    f.write((char*)&bi, sizeof(bi));
    f.write((char*)pixels.data(), pixels.size());

    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    LOG_INFO("[AUTO] Screenshot saved: %s", path.c_str());
}

static void ModThread() {
    std::this_thread::sleep_for(std::chrono::seconds(15));

    std::string modDir = GetModDirectory();
    Logger::Get().Init(modDir + "\\log.txt");
    LOG_INFO("========================================");
    LOG_INFO("  RL Ball Mod v8 - Full Autonomous");
    LOG_INFO("  Build: " __DATE__ " " __TIME__);
    LOG_INFO("========================================");

    Config::Load(modDir + "\\config.json");
    if (!Config::IsEnabled()) { LOG_INFO("Disabled."); return; }

    bool autoTest = Config::GetAutoTest();

    // UE3 init
    bool ok = false;
    for (int i = 1; i <= 3; i++) {
        LOG_INFO("UE3 init %d/3...", i);
        if (UE3::Initialize()) { ok = true; break; }
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    if (!ok) { LOG_ERR("UE3 init failed"); return; }

    Hooks::Initialize();
    Hooks::InstallBallHooks();

    std::string texFile = Config::GetTextureFile();
    std::string texPath = modDir + "\\textures\\" + texFile;
    BallHooks::PrepareOnBackgroundThread(texPath.c_str());

    // Night mode via D3D11
    bool nightMode = Config::GetNightMode();
    if (nightMode) {
        LOG_INFO("[Night] Night mode enabled - initializing D3D11 hooks...");
        std::string logPath = modDir + "\\texhook_log.txt";
        TexHook::Init("", logPath.c_str());
        // NightMode::Init is called inside TexHook when device is obtained
        // NightMode::SetEnabled and SetIntensity are called from Present hook
        // We need to signal NightMode to activate
        // This happens in tex_hook.cpp Hook_Present after device init
    }

    if (autoTest) {
        LOG_INFO("[AUTO] === AUTONOMOUS MODE ===");

        // Signal hook to auto-load freeplay via ConsoleCommand
        BallHooks::RequestAutoLoadFreeplay();

        // Wait for game to reach title screen
        std::this_thread::sleep_for(std::chrono::seconds(20));

        // Navigate to freeplay (SendInput from background thread)
        NavigateToFreeplay();

        // Wait for map to load
        std::this_thread::sleep_for(std::chrono::seconds(15));

        // Wait for texture to be applied (check log)
        LOG_INFO("[AUTO] Waiting for ball texture application...");
        for (int i = 0; i < 30; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            // Check if hook applied texture (g_applied flag)
            // We can check by reading the log file we just wrote
        }

        // Take screenshot
        std::this_thread::sleep_for(std::chrono::seconds(5));
        TakeScreenshot(modDir + "\\screenshot.bmp");
        LOG_INFO("[AUTO] === TEST COMPLETE ===");
    } else {
        LOG_INFO("Ready! Enter Freeplay manually.");
    }

    // Hot-reload loop: detect config changes and reset texture
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        if (Config::WasModified()) {
            Config::Load(modDir + "\\config.json");
            BallHooks::ResetApplied();
            LOG_INFO("Config changed - texture will re-apply");

            // Re-prepare with new texture
            std::string newTex = Config::GetTextureFile();
            std::string newPath = modDir + "\\textures\\" + newTex;
            BallHooks::PrepareOnBackgroundThread(newPath.c_str());
        }
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        if (!Proxy::Initialize()) return FALSE;
        std::thread(ModThread).detach();
    } else if (reason == DLL_PROCESS_DETACH) {
        Hooks::Shutdown();
        Proxy::Shutdown();
        Logger::Get().Close();
    }
    return TRUE;
}
