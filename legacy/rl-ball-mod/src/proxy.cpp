#include "proxy.h"
#include "util/logger.h"

static HMODULE g_realWinmm = nullptr;

// Array of function pointers for all 180 exports
static FARPROC g_origFuncs[200] = {};

static const char* g_funcNames[] = {
    "CloseDriver","DefDriverProc","DriverCallback","DrvGetModuleHandle",
    "GetDriverModuleHandle","OpenDriver","PlaySound","PlaySoundA","PlaySoundW",
    "SendDriverMessage","WOWAppExit","auxGetDevCapsA","auxGetDevCapsW",
    "auxGetNumDevs","auxGetVolume","auxOutMessage","auxSetVolume",
    "joyConfigChanged","joyGetDevCapsA","joyGetDevCapsW","joyGetNumDevs",
    "joyGetPos","joyGetPosEx","joyGetThreshold","joyReleaseCapture",
    "joySetCapture","joySetThreshold","mciDriverNotify","mciDriverYield",
    "mciExecute","mciFreeCommandResource","mciGetCreatorTask","mciGetDeviceIDA",
    "mciGetDeviceIDFromElementIDA","mciGetDeviceIDFromElementIDW","mciGetDeviceIDW",
    "mciGetDriverData","mciGetErrorStringA","mciGetErrorStringW","mciGetYieldProc",
    "mciLoadCommandResource","mciSendCommandA","mciSendCommandW","mciSendStringA",
    "mciSendStringW","mciSetDriverData","mciSetYieldProc","midiConnect",
    "midiDisconnect","midiInAddBuffer","midiInClose","midiInGetDevCapsA",
    "midiInGetDevCapsW","midiInGetErrorTextA","midiInGetErrorTextW","midiInGetID",
    "midiInGetNumDevs","midiInMessage","midiInOpen","midiInPrepareHeader",
    "midiInReset","midiInStart","midiInStop","midiInUnprepareHeader",
    "midiOutCacheDrumPatches","midiOutCachePatches","midiOutClose",
    "midiOutGetDevCapsA","midiOutGetDevCapsW","midiOutGetErrorTextA",
    "midiOutGetErrorTextW","midiOutGetID","midiOutGetNumDevs","midiOutGetVolume",
    "midiOutLongMsg","midiOutMessage","midiOutOpen","midiOutPrepareHeader",
    "midiOutReset","midiOutSetVolume","midiOutShortMsg","midiOutUnprepareHeader",
    "midiStreamClose","midiStreamOpen","midiStreamOut","midiStreamPause",
    "midiStreamPosition","midiStreamProperty","midiStreamRestart","midiStreamStop",
    "mixerClose","mixerGetControlDetailsA","mixerGetControlDetailsW",
    "mixerGetDevCapsA","mixerGetDevCapsW","mixerGetID","mixerGetLineControlsA",
    "mixerGetLineControlsW","mixerGetLineInfoA","mixerGetLineInfoW",
    "mixerGetNumDevs","mixerMessage","mixerOpen","mixerSetControlDetails",
    "mmDrvInstall","mmGetCurrentTask","mmTaskBlock","mmTaskCreate",
    "mmTaskSignal","mmTaskYield","mmioAdvance","mmioAscend","mmioClose",
    "mmioCreateChunk","mmioDescend","mmioFlush","mmioGetInfo",
    "mmioInstallIOProcA","mmioInstallIOProcW","mmioOpenA","mmioOpenW",
    "mmioRead","mmioRenameA","mmioRenameW","mmioSeek","mmioSendMessage",
    "mmioSetBuffer","mmioSetInfo","mmioStringToFOURCCA","mmioStringToFOURCCW",
    "mmioWrite","mmsystemGetVersion","sndPlaySoundA","sndPlaySoundW",
    "timeBeginPeriod","timeEndPeriod","timeGetDevCaps","timeGetSystemTime",
    "timeGetTime","timeKillEvent","timeSetEvent","waveInAddBuffer",
    "waveInClose","waveInGetDevCapsA","waveInGetDevCapsW","waveInGetErrorTextA",
    "waveInGetErrorTextW","waveInGetID","waveInGetNumDevs","waveInGetPosition",
    "waveInMessage","waveInOpen","waveInPrepareHeader","waveInReset",
    "waveInStart","waveInStop","waveInUnprepareHeader","waveOutBreakLoop",
    "waveOutClose","waveOutGetDevCapsA","waveOutGetDevCapsW",
    "waveOutGetErrorTextA","waveOutGetErrorTextW","waveOutGetID",
    "waveOutGetNumDevs","waveOutGetPitch","waveOutGetPlaybackRate",
    "waveOutGetPosition","waveOutGetVolume","waveOutMessage","waveOutOpen",
    "waveOutPause","waveOutPrepareHeader","waveOutReset","waveOutRestart",
    "waveOutSetPitch","waveOutSetPlaybackRate","waveOutSetVolume",
    "waveOutUnprepareHeader","waveOutWrite",
    nullptr
};

bool Proxy::Initialize() {
    char sysDir[MAX_PATH];
    GetSystemDirectoryA(sysDir, MAX_PATH);
    std::string realPath = std::string(sysDir) + "\\winmm.dll";

    g_realWinmm = LoadLibraryA(realPath.c_str());
    if (!g_realWinmm) return false;

    for (int i = 0; g_funcNames[i]; i++) {
        g_origFuncs[i] = GetProcAddress(g_realWinmm, g_funcNames[i]);
    }

    return true;
}

void Proxy::Shutdown() {
    if (g_realWinmm) {
        FreeLibrary(g_realWinmm);
        g_realWinmm = nullptr;
    }
}

// Generate proxy stubs for all 180 functions
// Each proxy_XXX function just jumps to the original
#define PROXY_FUNC(name, idx) \
    extern "C" __declspec(dllexport) void __stdcall proxy_##name() { \
        if (g_origFuncs[idx]) { \
            ((void(__stdcall*)())g_origFuncs[idx])(); \
        } \
    }

// Actually, we need naked forwarding via assembly for variable args
// Use a macro that creates a naked jump thunk
#ifdef __GNUC__
#define PROXY_NAKED(name, idx) \
    extern "C" __attribute__((naked)) void proxy_##name() { \
        asm("jmp *%0" : : "m"(g_origFuncs[idx])); \
    }
#else
#define PROXY_NAKED(name, idx) PROXY_FUNC(name, idx)
#endif

PROXY_NAKED(CloseDriver, 0)
PROXY_NAKED(DefDriverProc, 1)
PROXY_NAKED(DriverCallback, 2)
PROXY_NAKED(DrvGetModuleHandle, 3)
PROXY_NAKED(GetDriverModuleHandle, 4)
PROXY_NAKED(OpenDriver, 5)
PROXY_NAKED(PlaySound, 6)
PROXY_NAKED(PlaySoundA, 7)
PROXY_NAKED(PlaySoundW, 8)
PROXY_NAKED(SendDriverMessage, 9)
PROXY_NAKED(WOWAppExit, 10)
PROXY_NAKED(auxGetDevCapsA, 11)
PROXY_NAKED(auxGetDevCapsW, 12)
PROXY_NAKED(auxGetNumDevs, 13)
PROXY_NAKED(auxGetVolume, 14)
PROXY_NAKED(auxOutMessage, 15)
PROXY_NAKED(auxSetVolume, 16)
PROXY_NAKED(joyConfigChanged, 17)
PROXY_NAKED(joyGetDevCapsA, 18)
PROXY_NAKED(joyGetDevCapsW, 19)
PROXY_NAKED(joyGetNumDevs, 20)
PROXY_NAKED(joyGetPos, 21)
PROXY_NAKED(joyGetPosEx, 22)
PROXY_NAKED(joyGetThreshold, 23)
PROXY_NAKED(joyReleaseCapture, 24)
PROXY_NAKED(joySetCapture, 25)
PROXY_NAKED(joySetThreshold, 26)
PROXY_NAKED(mciDriverNotify, 27)
PROXY_NAKED(mciDriverYield, 28)
PROXY_NAKED(mciExecute, 29)
PROXY_NAKED(mciFreeCommandResource, 30)
PROXY_NAKED(mciGetCreatorTask, 31)
PROXY_NAKED(mciGetDeviceIDA, 32)
PROXY_NAKED(mciGetDeviceIDFromElementIDA, 33)
PROXY_NAKED(mciGetDeviceIDFromElementIDW, 34)
PROXY_NAKED(mciGetDeviceIDW, 35)
PROXY_NAKED(mciGetDriverData, 36)
PROXY_NAKED(mciGetErrorStringA, 37)
PROXY_NAKED(mciGetErrorStringW, 38)
PROXY_NAKED(mciGetYieldProc, 39)
PROXY_NAKED(mciLoadCommandResource, 40)
PROXY_NAKED(mciSendCommandA, 41)
PROXY_NAKED(mciSendCommandW, 42)
PROXY_NAKED(mciSendStringA, 43)
PROXY_NAKED(mciSendStringW, 44)
PROXY_NAKED(mciSetDriverData, 45)
PROXY_NAKED(mciSetYieldProc, 46)
PROXY_NAKED(midiConnect, 47)
PROXY_NAKED(midiDisconnect, 48)
PROXY_NAKED(midiInAddBuffer, 49)
PROXY_NAKED(midiInClose, 50)
PROXY_NAKED(midiInGetDevCapsA, 51)
PROXY_NAKED(midiInGetDevCapsW, 52)
PROXY_NAKED(midiInGetErrorTextA, 53)
PROXY_NAKED(midiInGetErrorTextW, 54)
PROXY_NAKED(midiInGetID, 55)
PROXY_NAKED(midiInGetNumDevs, 56)
PROXY_NAKED(midiInMessage, 57)
PROXY_NAKED(midiInOpen, 58)
PROXY_NAKED(midiInPrepareHeader, 59)
PROXY_NAKED(midiInReset, 60)
PROXY_NAKED(midiInStart, 61)
PROXY_NAKED(midiInStop, 62)
PROXY_NAKED(midiInUnprepareHeader, 63)
PROXY_NAKED(midiOutCacheDrumPatches, 64)
PROXY_NAKED(midiOutCachePatches, 65)
PROXY_NAKED(midiOutClose, 66)
PROXY_NAKED(midiOutGetDevCapsA, 67)
PROXY_NAKED(midiOutGetDevCapsW, 68)
PROXY_NAKED(midiOutGetErrorTextA, 69)
PROXY_NAKED(midiOutGetErrorTextW, 70)
PROXY_NAKED(midiOutGetID, 71)
PROXY_NAKED(midiOutGetNumDevs, 72)
PROXY_NAKED(midiOutGetVolume, 73)
PROXY_NAKED(midiOutLongMsg, 74)
PROXY_NAKED(midiOutMessage, 75)
PROXY_NAKED(midiOutOpen, 76)
PROXY_NAKED(midiOutPrepareHeader, 77)
PROXY_NAKED(midiOutReset, 78)
PROXY_NAKED(midiOutSetVolume, 79)
PROXY_NAKED(midiOutShortMsg, 80)
PROXY_NAKED(midiOutUnprepareHeader, 81)
PROXY_NAKED(midiStreamClose, 82)
PROXY_NAKED(midiStreamOpen, 83)
PROXY_NAKED(midiStreamOut, 84)
PROXY_NAKED(midiStreamPause, 85)
PROXY_NAKED(midiStreamPosition, 86)
PROXY_NAKED(midiStreamProperty, 87)
PROXY_NAKED(midiStreamRestart, 88)
PROXY_NAKED(midiStreamStop, 89)
PROXY_NAKED(mixerClose, 90)
PROXY_NAKED(mixerGetControlDetailsA, 91)
PROXY_NAKED(mixerGetControlDetailsW, 92)
PROXY_NAKED(mixerGetDevCapsA, 93)
PROXY_NAKED(mixerGetDevCapsW, 94)
PROXY_NAKED(mixerGetID, 95)
PROXY_NAKED(mixerGetLineControlsA, 96)
PROXY_NAKED(mixerGetLineControlsW, 97)
PROXY_NAKED(mixerGetLineInfoA, 98)
PROXY_NAKED(mixerGetLineInfoW, 99)
PROXY_NAKED(mixerGetNumDevs, 100)
PROXY_NAKED(mixerMessage, 101)
PROXY_NAKED(mixerOpen, 102)
PROXY_NAKED(mixerSetControlDetails, 103)
PROXY_NAKED(mmDrvInstall, 104)
PROXY_NAKED(mmGetCurrentTask, 105)
PROXY_NAKED(mmTaskBlock, 106)
PROXY_NAKED(mmTaskCreate, 107)
PROXY_NAKED(mmTaskSignal, 108)
PROXY_NAKED(mmTaskYield, 109)
PROXY_NAKED(mmioAdvance, 110)
PROXY_NAKED(mmioAscend, 111)
PROXY_NAKED(mmioClose, 112)
PROXY_NAKED(mmioCreateChunk, 113)
PROXY_NAKED(mmioDescend, 114)
PROXY_NAKED(mmioFlush, 115)
PROXY_NAKED(mmioGetInfo, 116)
PROXY_NAKED(mmioInstallIOProcA, 117)
PROXY_NAKED(mmioInstallIOProcW, 118)
PROXY_NAKED(mmioOpenA, 119)
PROXY_NAKED(mmioOpenW, 120)
PROXY_NAKED(mmioRead, 121)
PROXY_NAKED(mmioRenameA, 122)
PROXY_NAKED(mmioRenameW, 123)
PROXY_NAKED(mmioSeek, 124)
PROXY_NAKED(mmioSendMessage, 125)
PROXY_NAKED(mmioSetBuffer, 126)
PROXY_NAKED(mmioSetInfo, 127)
PROXY_NAKED(mmioStringToFOURCCA, 128)
PROXY_NAKED(mmioStringToFOURCCW, 129)
PROXY_NAKED(mmioWrite, 130)
PROXY_NAKED(mmsystemGetVersion, 131)
PROXY_NAKED(sndPlaySoundA, 132)
PROXY_NAKED(sndPlaySoundW, 133)
PROXY_NAKED(timeBeginPeriod, 134)
PROXY_NAKED(timeEndPeriod, 135)
PROXY_NAKED(timeGetDevCaps, 136)
PROXY_NAKED(timeGetSystemTime, 137)
PROXY_NAKED(timeGetTime, 138)
PROXY_NAKED(timeKillEvent, 139)
PROXY_NAKED(timeSetEvent, 140)
PROXY_NAKED(waveInAddBuffer, 141)
PROXY_NAKED(waveInClose, 142)
PROXY_NAKED(waveInGetDevCapsA, 143)
PROXY_NAKED(waveInGetDevCapsW, 144)
PROXY_NAKED(waveInGetErrorTextA, 145)
PROXY_NAKED(waveInGetErrorTextW, 146)
PROXY_NAKED(waveInGetID, 147)
PROXY_NAKED(waveInGetNumDevs, 148)
PROXY_NAKED(waveInGetPosition, 149)
PROXY_NAKED(waveInMessage, 150)
PROXY_NAKED(waveInOpen, 151)
PROXY_NAKED(waveInPrepareHeader, 152)
PROXY_NAKED(waveInReset, 153)
PROXY_NAKED(waveInStart, 154)
PROXY_NAKED(waveInStop, 155)
PROXY_NAKED(waveInUnprepareHeader, 156)
PROXY_NAKED(waveOutBreakLoop, 157)
PROXY_NAKED(waveOutClose, 158)
PROXY_NAKED(waveOutGetDevCapsA, 159)
PROXY_NAKED(waveOutGetDevCapsW, 160)
PROXY_NAKED(waveOutGetErrorTextA, 161)
PROXY_NAKED(waveOutGetErrorTextW, 162)
PROXY_NAKED(waveOutGetID, 163)
PROXY_NAKED(waveOutGetNumDevs, 164)
PROXY_NAKED(waveOutGetPitch, 165)
PROXY_NAKED(waveOutGetPlaybackRate, 166)
PROXY_NAKED(waveOutGetPosition, 167)
PROXY_NAKED(waveOutGetVolume, 168)
PROXY_NAKED(waveOutMessage, 169)
PROXY_NAKED(waveOutOpen, 170)
PROXY_NAKED(waveOutPause, 171)
PROXY_NAKED(waveOutPrepareHeader, 172)
PROXY_NAKED(waveOutReset, 173)
PROXY_NAKED(waveOutRestart, 174)
PROXY_NAKED(waveOutSetPitch, 175)
PROXY_NAKED(waveOutSetPlaybackRate, 176)
PROXY_NAKED(waveOutSetVolume, 177)
PROXY_NAKED(waveOutUnprepareHeader, 178)
PROXY_NAKED(waveOutWrite, 179)
