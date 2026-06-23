#pragma once
#include "../ue3/ue3_types.h"

namespace BallHooks {
    using ProcessEventFn = void(__thiscall*)(UObject*, UFunction*, void*);

    extern ProcessEventFn OriginalBallFadeIn;
    extern ProcessEventFn OriginalSetTextureParam;
    extern ProcessEventFn OriginalPreLoadMap;

    void __fastcall OnBallFadeIn(UObject* thisObj, UFunction* func, void* params);
    void __fastcall OnSetTextureParam(UObject* thisObj, UFunction* func, void* params);
    void __fastcall OnPreLoadMap(UObject* thisObj, UFunction* func, void* params);

    // Called from background thread - caches all pointers
    void ResetApplied();
    void PrepareOnBackgroundThread(const char* pngPath);
    void AutoLoadFreeplay();
    void RequestAutoLoadFreeplay();  // Signal hook to auto-load freeplay
}
