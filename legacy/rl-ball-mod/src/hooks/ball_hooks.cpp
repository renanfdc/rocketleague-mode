#include "ball_hooks.h"
#include "../ue3/ue3_reflection.h"
#include "../util/logger.h"
#include <cwchar>
#include <fstream>
#include <vector>
#include <atomic>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace BallHooks {

// IsA check: walks SuperField chain to support subclasses
static bool IsInstanceOf(UObject* obj, UClass* targetClass) {
    if (!obj || !targetClass) return false;
    UClass* cls = obj->Class;
    int depth = 0;
    while (cls && depth < 20) {
        if (cls == targetClass) return true;
        cls = reinterpret_cast<UClass*>(cls->SuperField);
        depth++;
    }
    return false;
}

ProcessEventFn OriginalBallFadeIn = nullptr;
ProcessEventFn OriginalSetTextureParam = nullptr;
ProcessEventFn OriginalPreLoadMap = nullptr;

static struct {
    UFunction* fn_Dup = nullptr;
    UFunction* fn_Init = nullptr;
    UFunction* fn_UpdatePNG = nullptr;
    UFunction* fn_SetTex = nullptr;
    UFunction* fn_CreateAndSetMIC = nullptr;  // NEW: CreateAndSetMaterialInstanceConstant
    UFunction* fn_SetMaterial = nullptr;       // NEW: SetMaterial (force render update)
    UClass*    cls_MIC = nullptr;
    UObject*   defTex = nullptr;
    int32_t    off_Ball = -1;
    int32_t    idx_Diffuse = -1;
    std::vector<uint8_t> pngData;
    std::atomic<bool> ready{false};
} C;

static UObject* g_tex = nullptr;
static bool g_texCreated = false;
static bool g_applied = false;
static int g_lastBallIdx = -1;
static int g_callCount = 0;

void PrepareOnBackgroundThread(const char* pngPath) {
    LOG_INFO("[BG] Caching...");

    C.fn_Dup       = UE3::FindFunctionByFullName("Core.Object.DuplicateObject");
    C.fn_Init      = UE3::FindFunctionByFullName("Engine.Texture2DDynamic.Init");
    C.fn_UpdatePNG = UE3::FindFunctionByFullName("Engine.Texture2DDynamic.UpdateMipFromPNG");
    C.fn_SetTex    = UE3::FindFunctionByFullName("Engine.MaterialInstanceConstant.SetTextureParameterValue");

    // NEW: CreateAndSetMaterialInstanceConstant - creates MIC AND applies it
    C.fn_CreateAndSetMIC = UE3::FindFunctionByFullName(
        "Engine.PrimitiveComponent.CreateAndSetMaterialInstanceConstant");
    if (!C.fn_CreateAndSetMIC) {
        // Try alternative names
        C.fn_CreateAndSetMIC = UE3::FindFunctionByFullName(
            "Engine.MeshComponent.CreateAndSetMaterialInstanceConstant");
    }
    LOG_INFO("[BG] CreateAndSetMIC: %s", C.fn_CreateAndSetMIC ? "FOUND" : "NOT FOUND");

    // NEW: SetMaterial - force render update
    C.fn_SetMaterial = UE3::FindFunctionByFullName("Engine.PrimitiveComponent.SetMaterial");
    if (!C.fn_SetMaterial) {
        C.fn_SetMaterial = UE3::FindFunctionByFullName("Engine.MeshComponent.SetMaterial");
    }
    LOG_INFO("[BG] SetMaterial: %s", C.fn_SetMaterial ? "FOUND" : "NOT FOUND");

    C.cls_MIC = UE3::FindClass("MaterialInstanceConstant");
    C.defTex  = UE3::FindDefaultObject("Texture2DDynamic");

    UClass* fxBallClass = UE3::FindClass("FXActor_Ball_TA");
    if (fxBallClass) C.off_Ball = UE3::GetPropertyOffset(fxBallClass, "Ball");
    LOG_INFO("[BG] Ball offset: 0x%X", C.off_Ball);

    auto* gn = UE3::GetGNames();
    if (gn) for (int i = 0; i < gn->Count && i < 200000; i++) {
        if (gn->IsValid(i) && gn->Data[i] && wcscmp(gn->Data[i]->Name, L"Diffuse") == 0) {
            C.idx_Diffuse = i; break;
        }
    }

    if (pngPath && pngPath[0]) {
        std::ifstream f(pngPath, std::ios::binary | std::ios::ate);
        if (f.is_open()) {
            size_t sz = f.tellg();
            C.pngData.resize(sz);
            f.seekg(0);
            f.read(reinterpret_cast<char*>(C.pngData.data()), sz);
            LOG_INFO("[BG] PNG: %zu bytes", sz);
        }
    }

    bool ok = C.fn_Dup && C.fn_Init && C.fn_UpdatePNG && C.fn_SetTex &&
              C.cls_MIC && C.defTex && C.off_Ball > 0 && C.idx_Diffuse >= 0 && !C.pngData.empty();
    if (ok) {
        C.ready.store(true);
        LOG_INFO("[BG] *** READY ***");
    } else {
        LOG_ERR("[BG] Missing components");
    }
}

static std::atomic<bool> g_autoLoadRequested{false};
static std::atomic<int> g_autoLoadPhase{0}; // 0=not started, 1=menu reached, 2=console cmd sent, 3=done

void RequestAutoLoadFreeplay() {
    g_autoLoadRequested.store(true);
    g_autoLoadPhase.store(1);
}

void __fastcall OnBallFadeIn(UObject* thisObj, UFunction* func, void* params) {
    if (OriginalBallFadeIn) OriginalBallFadeIn(thisObj, func, params);

    if (!C.ready.load()) return;

    // Throttle
    g_callCount++;
    if (g_callCount % 300 != 1) return;

    // FAST PATH: real ball replacement happens via D3D11 hook (tex_hook.cpp).
    // This UE3 polling path is legacy. Once g_applied=true, skip ALL work
    // until OnPreLoadMap / ResetApplied clears the flag (map change or config reload).
    if (g_applied) return;

    // Only apply in actual gameplay (not menu) - check for GameEvent_Soccar_TA
    static UClass* cls_GameEvent = nullptr;
    if (!cls_GameEvent) cls_GameEvent = UE3::FindClass("GameEvent_Soccar_TA");
    if (cls_GameEvent) {
        bool inGame = false;
        auto* objs = UE3::GetGObjects();
        for (int i = objs->Count - 1; i >= 0; i--) {
            UObject* o = objs->Data[i];
            if (!o) continue;
            if (IsInstanceOf(o, cls_GameEvent)) {
                // Check it's a real instance (high index, has Outer)
                if (o->Outer && o->Outer->Outer) { inGame = true; break; }
            }
        }
        if (!inGame) {
            // Only auto-load freeplay if auto_test is enabled
            if (g_autoLoadRequested.load() && g_callCount % 90 == 1) {
                g_autoLoadPhase.store(2);

                // Find PlayerController
                static UClass* cls_PC2 = nullptr;
                if (!cls_PC2) cls_PC2 = UE3::FindClass("PlayerController");
                UObject* pc = nullptr;
                if (cls_PC2) {
                    for (int i = objs->Count - 1; i >= 0; i--) {
                        UObject* o = objs->Data[i];
                        if (!o || !IsInstanceOf(o, cls_PC2)) continue;
                        pc = o;
                        break;
                    }
                }
                if (pc) {
                    UFunction* fn_CC = UE3::FindFunctionByFullName("Engine.PlayerController.ConsoleCommand");
                    if (fn_CC) {
                        const wchar_t* cmd = L"start Park_P?Playtest?game=TAGame.GameInfo_Soccar_TA";
                        int len = (int)wcslen(cmd) + 1;
                        uint8_t buf[64] = {};
                        *(wchar_t**)(buf) = const_cast<wchar_t*>(cmd);
                        *(int32_t*)(buf + 8) = len;
                        *(int32_t*)(buf + 12) = len;
                        CallProcessEvent(pc, fn_CC, buf);
                        g_autoLoadPhase.store(3);
                        LOG_INFO("[GT] ConsoleCommand('%ls') SENT!", cmd);
                    }
                } else {
                    LOG_INFO("[GT] Not in game, no PlayerController (attempt %d)", g_callCount/90);
                }
            } else if (g_callCount % 270 == 1) {
                LOG_INFO("[GT] Not in game yet");
            }
            return;
        }
    }

    // Get REAL ball - search GObjects for Ball_TA with UNIQUE Outer (not archetype)
    static UClass* cls_BallTA = nullptr;
    if (!cls_BallTA) cls_BallTA = UE3::FindClass("Ball_TA");

    UObject* ball = nullptr;
    auto* objects = UE3::GetGObjects();

    // First, find the common archetype Outer (all archetypes share same Outer)
    static UObject* archetypeOuter = nullptr;
    if (!archetypeOuter) {
        // The archetype at lowest index has the package Outer
        for (int i = 0; i < objects->Count; i++) {
            UObject* o = objects->Data[i];
            if (!o || !IsInstanceOf(o, cls_BallTA)) continue;
            archetypeOuter = o->Outer;
            break;
        }
    }

    // Find a Ball_TA whose Outer chain depth >= 3 (PersistentLevel->World->Package)
    // Real ball: Level.World.PersistentLevel.Ball_TA_X (depth >= 3)
    // Archetypes: Package.Ball_Default (depth 1-2)
    static UClass* cls_Level = nullptr;
    if (!cls_Level) cls_Level = UE3::FindClass("Level");

    for (int i = objects->Count - 1; i >= 0; i--) {
        UObject* o = objects->Data[i];
        if (!o || !IsInstanceOf(o, cls_BallTA)) continue;

        // Check Outer chain: real ball has Outer whose Class is Level/PersistentLevel
        UObject* outer = o->Outer;
        if (!outer) continue;

        // Check if Outer is a Level (PersistentLevel)
        bool isInLevel = false;
        if (cls_Level && IsInstanceOf(outer, cls_Level)) {
            isInLevel = true;
        }
        // Also check by Outer depth >= 3
        int depth = 0;
        UObject* ot = outer;
        while (ot && depth < 10) { depth++; ot = ot->Outer; }

        if (isInLevel || depth >= 3) {
            ball = o;
            LOG_INFO("[GT] REAL ball: idx=%d depth=%d inLevel=%s",
                     i, depth, isInLevel ? "YES" : "NO");
            // Removed verbose logging for performance
            // Re-apply if ball changed (new map/match)
            if (i != g_lastBallIdx) {
                g_applied = false;
                g_lastBallIdx = i;
            }
            break;
        }
    }

    // Fallback: if no different-Outer ball found, log diagnostics
    if (!ball) {
        static int s_noRealCount = 0;
        s_noRealCount++;
        if (s_noRealCount % 3 == 1) {
            LOG_INFO("[GT] No real ball found (archOuter=0x%llX). All Ball_TA count:",
                     (uintptr_t)archetypeOuter);
            int count = 0;
            for (int i = objects->Count - 1; i >= 0 && count < 5; i--) {
                UObject* o = objects->Data[i];
                if (!o || !IsInstanceOf(o, cls_BallTA)) continue;
                LOG_INFO("[GT]   idx=%d outer=0x%llX same=%s",
                         i, (uintptr_t)o->Outer,
                         o->Outer == archetypeOuter ? "YES" : "NO");
                count++;
            }
        }
        return;
    }

    // Create custom texture via DuplicateObject+Init+UpdateMipFromPNG
    // WITHOUT CompressionSettings (those may have been corrupting the texture)
    if (!g_texCreated) {
        g_texCreated = true;

        // DuplicateObject
        struct { UObject* T; UObject* O; UClass* Cl; UObject* R; } dp =
            {C.defTex, C.defTex->Outer, C.defTex->Class, nullptr};
        CallProcessEvent(C.defTex, C.fn_Dup, &dp);
        if (dp.R) {
            g_tex = dp.R;
            g_tex->ObjectFlags &= ~0x20ULL;
            g_tex->ObjectFlags |= 0x4800ULL;

            // Init with larger size matching PNG (2048x2048)
            struct { int32_t X; int32_t Y; int32_t F; int32_t R; } ip = {2048, 2048, 2, 0};
            CallProcessEvent(g_tex, C.fn_Init, &ip);

            // Upload PNG
            struct { int32_t M; uint8_t p[4]; TArray<uint8_t> D; } up = {};
            up.D.Data = C.pngData.data();
            up.D.Count = (int32_t)C.pngData.size();
            up.D.Max = up.D.Count;
            CallProcessEvent(g_tex, C.fn_UpdatePNG, &up);
            LOG_INFO("[GT] Custom texture created: 0x%llX (2048x2048, no compression settings)",
                     (uintptr_t)g_tex);
        }

        // If DuplicateObject failed, use existing texture as fallback
        if (!g_tex) {
            static UClass* cls_Tex2D = nullptr;
            if (!cls_Tex2D) cls_Tex2D = UE3::FindClass("Texture2D");
            if (cls_Tex2D) {
                for (int i = 100000; i < objects->Count; i++) {
                    UObject* o = objects->Data[i];
                    if (!o || o->Class != reinterpret_cast<UObject*>(cls_Tex2D)) continue;
                    g_tex = o;
                    LOG_INFO("[GT] Fallback texture: %s", UE3::GetFullName(o).c_str());
                    break;
                }
            }
        }
        if (!g_tex) return;
    }
    if (!g_tex) return;

    // Find mesh component and MIC via pointer-only GObjects scan
    UObject* meshComp = nullptr;
    UObject* existingMIC = nullptr;

    // Cache StaticMeshComponent class pointer
    static UClass* cls_SMC = nullptr;
    if (!cls_SMC) cls_SMC = UE3::FindClass("StaticMeshComponent");

    for (int i = objects->Count - 1; i >= 0; i--) {
        UObject* obj = objects->Data[i];
        if (!obj) continue;

        // Find StaticMeshComponent whose Outer is this ball
        if (!meshComp && cls_SMC && obj->Class == reinterpret_cast<UObject*>(cls_SMC)) {
            if (obj->Outer == ball) meshComp = obj;
        }

        // Find MIC belonging to this ball
        if (!existingMIC && obj->Class == reinterpret_cast<UObject*>(C.cls_MIC)) {
            UObject* o1 = obj->Outer;
            if (o1 && o1->Outer == ball) { existingMIC = obj; if (!meshComp) meshComp = o1; }
            else if (o1 == ball) { existingMIC = obj; }
        }

        if (meshComp) break; // mesh is enough, MIC can be created
    }

    if (!meshComp) {
        if (g_callCount % 270 == 1) {
            LOG_INFO("[GT] ball=0x%llX, no mesh found. Scanning ALL Outer==ball...", (uintptr_t)ball);
            // Debug: log ALL objects whose Outer is this ball (max 10)
            int found = 0;
            for (int i = objects->Count - 1; i >= 0 && found < 10; i--) {
                UObject* obj = objects->Data[i];
                if (!obj || obj->Outer != ball) continue;
                // Log class name (one string op, but only runs every ~9 seconds)
                std::string cls = UE3::GetName(obj->Class->Name);
                LOG_INFO("[GT]   child: class=%s addr=0x%llX", cls.c_str(), (uintptr_t)obj);
                found++;
                // If it's any kind of mesh component, use it!
                if (cls.find("Mesh") != std::string::npos || cls.find("mesh") != std::string::npos) {
                    meshComp = obj;
                }
            }
            if (found == 0) LOG_WARN("[GT]   No children found for this ball pointer!");
        }
        if (!meshComp) return;
    }

    LOG_INFO("[GT] Ball mesh found, applying texture...");

    // === FIX 1: Try CreateAndSetMaterialInstanceConstant ===
    UObject* mic = nullptr;
    if (C.fn_CreateAndSetMIC) {
        // Try with raw buffer - check multiple offsets for return value
        uint8_t buf[64] = {};
        *(int32_t*)(buf + 0) = 0; // ElementIndex = 0
        CallProcessEvent(meshComp, C.fn_CreateAndSetMIC, buf);

        // Validate return value - must be a valid user-mode pointer
        for (int off = 4; off <= 24; off += 4) {
            uintptr_t rawVal = *(uintptr_t*)(buf + off);
            if (rawVal == 0 || rawVal < 0x10000 || rawVal > 0x7FFFFFFFFFFF) continue;
            // Check if readable
            MEMORY_BASIC_INFORMATION mbi2;
            if (!VirtualQuery(reinterpret_cast<void*>(rawVal), &mbi2, sizeof(mbi2))) continue;
            if (mbi2.State != MEM_COMMIT) continue;

            mic = reinterpret_cast<UObject*>(rawVal);
            LOG_INFO("[GT] CreateAndSetMIC at +%d: 0x%llX", off, rawVal);
            break;
        }
        if (!mic) LOG_WARN("[GT] CreateAndSetMIC: no valid pointer found");
    }

    // Fallback: ConditionalCreateMIC
    if (!mic) {
        static UFunction* fn_CMIC = nullptr;
        if (!fn_CMIC) fn_CMIC = UE3::FindFunctionByFullName("Engine.MeshComponent.ConditionalCreateMIC");
        if (fn_CMIC && meshComp) {
            uint8_t buf[64] = {};
            *(int32_t*)(buf + 0) = 0;
            CallProcessEvent(meshComp, fn_CMIC, buf);
            for (int off = 4; off <= 24; off += 4) {
                uintptr_t rawVal = *(uintptr_t*)(buf + off);
                if (rawVal == 0 || rawVal < 0x10000 || rawVal > 0x7FFFFFFFFFFF) continue;
                MEMORY_BASIC_INFORMATION mbi2;
                if (!VirtualQuery(reinterpret_cast<void*>(rawVal), &mbi2, sizeof(mbi2))) continue;
                if (mbi2.State != MEM_COMMIT) continue;
                mic = reinterpret_cast<UObject*>(rawVal);
                LOG_INFO("[GT] ConditionalCreateMIC at +%d: 0x%llX", off, rawVal);
                break;
            }
        }
    }

    // Fallback: use existing MIC
    if (!mic) {
        mic = existingMIC;
        LOG_INFO("[GT] Using existing MIC: 0x%llX", (uintptr_t)mic);
    }

    if (!mic) { LOG_ERR("[GT] No MIC available"); return; }

    // === Diagnostic: try GetTextureParameterValue for known names ===
    static UFunction* fn_GetTex = nullptr;
    if (!fn_GetTex) {
        fn_GetTex = UE3::FindFunctionByFullName(
            "Engine.MaterialInstanceConstant.GetTextureParameterValue");
    }

    if (fn_GetTex && C.ready.load()) {
        auto* gn = UE3::GetGNames();
        const wchar_t* testNames[] = {
            L"Diffuse", L"DiffuseTexture", L"BaseTexture", L"Base",
            L"Normal", L"Specular", L"Emissive", L"Mask",
            L"EnvMap", L"CubeMap", L"Reflection", L"BallTexture",
            L"Color", L"Albedo", L"BaseColor", nullptr
        };

        LOG_INFO("[GT] Probing MIC texture parameters...");
        for (int n = 0; testNames[n]; n++) {
            int32_t nameIdx = -1;
            for (int k = 0; k < gn->Count && k < 200000; k++) {
                if (gn->IsValid(k) && gn->Data[k] && wcscmp(gn->Data[k]->Name, testNames[n]) == 0) {
                    nameIdx = k; break;
                }
            }
            if (nameIdx < 0) continue;

            // GetTextureParameterValue(FName, out UTexture*) -> bool
            struct { FName P; UObject* V; uint32_t Ret; } gtp = {};
            gtp.P.Index = nameIdx;
            CallProcessEvent(mic, fn_GetTex, &gtp);

            char narrow[32] = {};
            wcstombs(narrow, testNames[n], 31);

            if (gtp.V || gtp.Ret) {
                LOG_INFO("[GT]   '%s' EXISTS! value=0x%llX ret=%d", narrow, (uintptr_t)gtp.V, gtp.Ret);
            }
        }
    }

    // === Apply texture with Diffuse (and other names if Diffuse fails) ===
    struct { FName P; UObject* V; } tp;
    tp.P.Index = C.idx_Diffuse;
    tp.P.Number = 0;
    tp.V = g_tex;
    CallProcessEvent(mic, C.fn_SetTex, &tp);
    LOG_INFO("[GT] SetTextureParameterValue('Diffuse') done");

    // === FIX 2: Force render update via SetMaterial ===
    if (C.fn_SetMaterial) {
        struct { int32_t ElementIndex; uint8_t pad[4]; UObject* Material; } sm = {0, {}, mic};
        CallProcessEvent(meshComp, C.fn_SetMaterial, &sm);
        LOG_INFO("[GT] SetMaterial(0, mic) - force render update");
    }

    g_applied = true;
    LOG_INFO("[GT] *** ALL DONE - check ball texture! ***");

    // === SKYBOX DUMP: find ALL StaticMeshActor in PersistentLevel ===
    static bool s_skyDumped = false;
    if (!s_skyDumped) {
        s_skyDumped = true;
        LOG_INFO("[SKY] === Dumping level mesh actors ===");
        
        // Find StaticMeshActor class
        static UClass* cls_SMA = nullptr;
        if (!cls_SMA) cls_SMA = UE3::FindClass("StaticMeshActor");
        
        // Find all StaticMeshActor instances in a level (deep Outer chain)
        int meshCount = 0;
        for (int i = objects->Count - 1; i >= 0 && meshCount < 50; i--) {
            UObject* o = objects->Data[i];
            if (!o) continue;
            if (cls_SMA && !IsInstanceOf(o, cls_SMA)) continue;
            
            // Check if in PersistentLevel (depth >= 2)
            int depth = 0;
            UObject* ot = o->Outer;
            while (ot && depth < 5) { depth++; ot = ot->Outer; }
            if (depth < 2) continue;
            
            std::string name = UE3::GetObjectName(o);
            std::string full = UE3::GetFullName(o);
            LOG_INFO("[SKY] Mesh[%d] idx=%d %s", meshCount, i, full.c_str());
            meshCount++;
        }
        LOG_INFO("[SKY] Found %d mesh actors in level", meshCount);

        // Also dump Light and PostProcess actors
        LOG_INFO("[SKY] === Light/PostProcess actors ===");
        int lightCount = 0;
        for (int i = objects->Count - 1; i >= 0 && lightCount < 30; i--) {
            UObject* o = objects->Data[i];
            if (!o) continue;
            std::string cls = UE3::GetName(o->Class->Name);
            if (cls.find("Light") != std::string::npos ||
                cls.find("PostProcess") != std::string::npos ||
                cls.find("Fog") != std::string::npos ||
                cls.find("Atmosphere") != std::string::npos ||
                cls.find("HeightFog") != std::string::npos) {
                // Check if in a level (depth >= 2)
                int depth = 0;
                UObject* ot = o->Outer;
                while (ot && depth < 5) { depth++; ot = ot->Outer; }
                if (depth < 2) continue;
                
                std::string full = UE3::GetFullName(o);
                LOG_INFO("[SKY] Light[%d] class=%s name=%s", lightCount, cls.c_str(), full.c_str());
                lightCount++;
            }
        }
        LOG_INFO("[SKY] Found %d light/postprocess actors", lightCount);

        // Try to modify DirectionalLight brightness to simulate night
        static UClass* cls_DLC = nullptr;
        if (!cls_DLC) cls_DLC = UE3::FindClass("DirectionalLightComponent");

        if (cls_DLC) {
            // Find Brightness/Intensity property offset
            int32_t off_Brightness = UE3::GetPropertyOffset(cls_DLC, "Brightness");
            int32_t off_Intensity = UE3::GetPropertyOffset(cls_DLC, "Intensity");
            LOG_INFO("[SKY] DirectionalLightComponent: Brightness=+0x%X, Intensity=+0x%X",
                     off_Brightness, off_Intensity);

            // Find all DirectionalLightComponent instances in the level
            for (int i = objects->Count - 1; i >= 0; i--) {
                UObject* o = objects->Data[i];
                if (!o || !IsInstanceOf(o, cls_DLC)) continue;
                int depth = 0;
                UObject* ot = o->Outer;
                while (ot && depth < 5) { depth++; ot = ot->Outer; }
                if (depth < 2) continue;

                LOG_INFO("[SKY] Modifying light: %s", UE3::GetFullName(o).c_str());

                // Set Brightness to very low value (0.02 = night)
                if (off_Brightness > 0) {
                    float* pBright = reinterpret_cast<float*>(
                        reinterpret_cast<uint8_t*>(o) + off_Brightness);
                    LOG_INFO("[SKY]   Old Brightness: %.3f", *pBright);
                    *pBright = 0.02f;
                    LOG_INFO("[SKY]   New Brightness: %.3f", *pBright);
                }
                if (off_Intensity > 0) {
                    float* pInt = reinterpret_cast<float*>(
                        reinterpret_cast<uint8_t*>(o) + off_Intensity);
                    LOG_INFO("[SKY]   Old Intensity: %.3f", *pInt);
                    *pInt = 0.02f;
                    LOG_INFO("[SKY]   New Intensity: %.3f", *pInt);
                }
            }
        }
    }
}

void __fastcall OnSetTextureParam(UObject* thisObj, UFunction* func, void* params) {
    if (OriginalSetTextureParam) OriginalSetTextureParam(thisObj, func, params);
}

void __fastcall OnPreLoadMap(UObject* thisObj, UFunction* func, void* params) {
    g_applied = false;
    g_texCreated = false;
    g_tex = nullptr;
    g_callCount = 0;
    if (OriginalPreLoadMap) OriginalPreLoadMap(thisObj, func, params);
}

// Auto-load freeplay - uses multiple strategies
void AutoLoadFreeplay() {
    LOG_INFO("[GT] Auto-loading Freeplay...");

    // Strategy 1: Send keyboard input to pass title screen + navigate to freeplay
    // Use Windows SendInput API from within the game process
    INPUT inputs[2] = {};

    // Press Enter
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_RETURN;
    inputs[0].ki.dwFlags = 0; // key down

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_RETURN;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    // Send Enter multiple times with delays (pass title screen + menu navigation)
    for (int i = 0; i < 10; i++) {
        SendInput(2, inputs, sizeof(INPUT));
        Sleep(2000);
        LOG_INFO("[GT] Sent Enter key (%d/10)", i + 1);
    }

    // Strategy 2: Try ConsoleCommand if PlayerController exists now
    static UClass* cls_PC = nullptr;
    if (!cls_PC) cls_PC = UE3::FindClass("PlayerController");

    if (cls_PC) {
        auto* objects = UE3::GetGObjects();
        for (int i = objects->Count - 1; i >= 0; i--) {
            UObject* o = objects->Data[i];
            if (!o || o->Class != reinterpret_cast<UObject*>(cls_PC)) continue;
            int depth = 0;
            UObject* ot = o->Outer;
            while (ot && depth < 5) { depth++; ot = ot->Outer; }
            if (depth < 2) continue;

            LOG_INFO("[GT] Found PlayerController, trying ConsoleCommand...");

            UFunction* fn_CC = UE3::FindFunctionByFullName("Engine.PlayerController.ConsoleCommand");
            if (fn_CC) {
                const wchar_t* cmd = L"open Park_P?playtest?listen";
                int cmdLen = (int)wcslen(cmd) + 1;
                struct {
                    wchar_t* Data; int32_t Count; int32_t Max;
                    uint32_t bLog; uint8_t pad[4];
                    wchar_t* RetData; int32_t RetCount; int32_t RetMax;
                } params = {};
                params.Data = const_cast<wchar_t*>(cmd);
                params.Count = cmdLen;
                params.Max = cmdLen;
                CallProcessEvent(o, fn_CC, &params);
                LOG_INFO("[GT] ConsoleCommand('open Park_P?playtest?listen') sent!");
            }
            break;
        }
    }
}

void ResetApplied() {
    g_applied = false;
    g_texCreated = false;
    g_tex = nullptr;
    g_lastBallIdx = -1;
    C.ready.store(false);  // Force wait for new PNG data
    C.pngData.clear();     // Clear cached PNG
    LOG_INFO("[GT] Texture fully reset - waiting for new PNG");
}

} // namespace BallHooks
