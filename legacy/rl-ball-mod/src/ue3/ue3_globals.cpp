#include "ue3_globals.h"
#include "../util/pattern_scan.h"
#include "../util/logger.h"
#include <vector>
#include <cstring>
#include <cwchar>
#include <string>

namespace UE3 {

static GNamesArray* g_GNames = nullptr;
static TArray<UObject*>* g_GObjects = nullptr;
static bool g_initialized = false;

// ============================================================
// Pattern scanning using CONFIRMED patterns from flaryx32/rocketleague-offsets
// and RLSDK-Generator community (2025-2026 builds)
// ============================================================

static bool IsReadable(void* addr, size_t size = 8) {
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery(addr, &mbi, sizeof(mbi))) return false;
    if (mbi.State != MEM_COMMIT) return false;
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
    return true;
}

// GNames patterns (confirmed for RL 2025-2026)
// These are MOV RCX, [RIP+disp32] instructions that load the GNames pointer
static const struct {
    const char* pattern;
    int dispOffset;  // offset within match where disp32 starts
    int instrSize;   // total instruction size for RIP calculation
} GNAMES_SIGS[] = {
    // Current (flaryx32 June 2025): mov rcx, [rip+??]; mov rcx, [rcx+rax*8]
    {"48 8B 0D ?? ?? ?? ?? 48 8B 0C C1", 3, 7},
    // Alternative longer pattern
    {"49 63 06 48 8D 55 E8 48 8B 0D ?? ?? ?? ?? 48 8B 0C C1", 10, 14},
    // GSpots sig 1
    {"48 8D 0D ?? ?? ?? ?? E8 ?? ?? FE FF 4C 8B C0 C6 05", 3, 7},
    // GSpots sig 3
    {"48 8D 0D ?? ?? ?? ?? E8 ?? ?? FF FF 48 8B D0 C6 05", 3, 7},
    // GSpots sig 4 (mov variant)
    {"48 8B 05 ?? ?? ?? ?? 48 85 C0 75 ?? B9 08 08 00", 3, 7},
    // Legacy
    {"48 8B 05 ?? ?? ?? ?? 48 8B 0C C8 48 85 C9", 3, 7},
    {nullptr, 0, 0}
};

static const struct {
    const char* pattern;
    int dispOffset;
    int instrSize;
} GOBJECTS_SIGS[] = {
    // Current (flaryx32 June 2025)
    {"48 8B C8 48 8B 05 ?? ?? ?? ?? 48 8B 0C C8", 6, 10},
    // GSpots variants
    {"4C 8B 0D ?? ?? ?? ?? 99 0F B7 D2", 3, 7},
    {"4C 8B 0D ?? ?? ?? ?? 41 3B C0 7D 17", 3, 7},
    {"4C 8B 0D ?? ?? ?? ?? 48 98 48 8D 0C 40 49", 3, 7},
    {"4C 8B 0D ?? ?? ?? ?? 8B D0 C1 EA 10", 3, 7},
    // Legacy
    {"48 89 05 ?? ?? ?? ?? 4C 8D 05 ?? ?? ?? ?? BA FA 02", 3, 7},
    {"48 8B 05 ?? ?? ?? ?? 48 8B 0C C8 48 85 C9", 3, 7},
    {nullptr, 0, 0}
};

static bool ValidateGNames(uintptr_t addr) {
    // addr should point to the TArray<FNameEntry*> struct
    // or to a pointer to it
    GNamesArray* arr = nullptr;

    // Try direct
    arr = reinterpret_cast<GNamesArray*>(addr);
    if (arr && arr->Data && arr->Count > 1000 && arr->Count < 500000 &&
        arr->Max >= arr->Count && IsReadable(arr->Data, 8)) {

        FNameEntry* first = arr->Data[0];
        if (first && IsReadable(first, 0x20)) {
            // Check for "None" as wide string at offset 0x18
            if (wcscmp(first->Name, L"None") == 0) {
                LOG_INFO("GNames validated (direct): Count=%d, [0]='None'", arr->Count);
                g_GNames = arr;
                return true;
            }
            // Log what we actually found for debugging
            char narrow[32] = {};
            wcstombs(narrow, first->Name, 31);
            LOG_INFO("  Direct: Count=%d, [0]='%s' (not None)", arr->Count, narrow);
        }
    }

    // Try as pointer-to-TArray
    uintptr_t ptr = 0;
    if (IsReadable(reinterpret_cast<void*>(addr), 8)) {
        ptr = *reinterpret_cast<uintptr_t*>(addr);
        if (ptr && IsReadable(reinterpret_cast<void*>(ptr), 16)) {
            arr = reinterpret_cast<GNamesArray*>(ptr);
            if (arr->Data && arr->Count > 1000 && arr->Count < 500000 &&
                arr->Max >= arr->Count && IsReadable(arr->Data, 8)) {

                FNameEntry* first = arr->Data[0];
                if (first && IsReadable(first, 0x20)) {
                    if (wcscmp(first->Name, L"None") == 0) {
                        LOG_INFO("GNames validated (indirect): Count=%d, [0]='None'", arr->Count);
                        g_GNames = arr;
                        return true;
                    }
                    char narrow[32] = {};
                    wcstombs(narrow, first->Name, 31);
                    LOG_INFO("  Indirect: Count=%d, [0]='%s' (not None)", arr->Count, narrow);
                }
            }
        }
    }

    return false;
}

static bool ValidateGObjects(uintptr_t addr) {
    TArray<UObject*>* arr = nullptr;

    // Try direct
    arr = reinterpret_cast<TArray<UObject*>*>(addr);
    if (arr && arr->Data && arr->Count > 5000 && arr->Count < 500000 &&
        arr->Max >= arr->Count && IsReadable(arr->Data, 8)) {

        int valid = 0, nulls = 0;
        for (int i = 0; i < 100; i++) {
            if (!arr->Data[i]) { nulls++; continue; }
            if (!IsReadable(arr->Data[i], 0x58)) continue;

            // Check Name.Index is valid
            int32_t nameIdx = arr->Data[i]->Name.Index;
            if (g_GNames && nameIdx >= 0 && nameIdx < g_GNames->Count) {
                valid++;
            }
        }

        if (valid > 30) {
            LOG_INFO("GObjects validated (direct): Count=%d, valid=%d/100", arr->Count, valid);
            g_GObjects = arr;
            return true;
        }
        LOG_INFO("  Direct: Count=%d, valid=%d, nulls=%d", arr->Count, valid, nulls);
    }

    // Try as pointer
    uintptr_t ptr = 0;
    if (IsReadable(reinterpret_cast<void*>(addr), 8)) {
        ptr = *reinterpret_cast<uintptr_t*>(addr);
        if (ptr && IsReadable(reinterpret_cast<void*>(ptr), 16)) {
            arr = reinterpret_cast<TArray<UObject*>*>(ptr);
            if (arr->Data && arr->Count > 5000 && arr->Count < 500000 &&
                arr->Max >= arr->Count && IsReadable(arr->Data, 8)) {

                int valid = 0, nulls = 0;
                for (int i = 0; i < 100; i++) {
                    if (!arr->Data[i]) { nulls++; continue; }
                    if (!IsReadable(arr->Data[i], 0x58)) continue;
                    int32_t nameIdx = arr->Data[i]->Name.Index;
                    if (g_GNames && nameIdx >= 0 && nameIdx < g_GNames->Count) valid++;
                }
                if (valid > 30) {
                    LOG_INFO("GObjects validated (indirect): Count=%d, valid=%d/100", arr->Count, valid);
                    g_GObjects = arr;
                    return true;
                }
            }
        }
    }

    return false;
}

static bool FindGNames() {
    LOG_INFO("Scanning for GNames with %d known patterns...",
             (int)(sizeof(GNAMES_SIGS)/sizeof(GNAMES_SIGS[0]) - 1));

    for (int i = 0; GNAMES_SIGS[i].pattern; i++) {
        uintptr_t match = PatternScan::FindInModule(GNAMES_SIGS[i].pattern);
        if (!match) continue;

        LOG_INFO("GNames pattern %d matched at 0x%llX", i, match);

        uintptr_t resolved = PatternScan::ResolveRIP(
            match, GNAMES_SIGS[i].dispOffset, GNAMES_SIGS[i].instrSize);
        LOG_INFO("  Resolved to 0x%llX", resolved);

        if (ValidateGNames(resolved)) {
            LOG_INFO("*** GNames found via pattern %d ***", i);
            return true;
        }
    }

    // Fallback: scan .data section for the TArray directly
    LOG_INFO("Pattern scan failed, trying .data section scan...");

    uintptr_t moduleBase;
    size_t moduleSize;
    PatternScan::GetModuleInfo(moduleBase, moduleSize);

    // Search for "None" as wide string in process memory to locate FNameEntries
    // Then find TArray in .data that points to them
    LOG_INFO("Scanning for wide L'None' string in process memory...");

    const wchar_t target[] = L"None";
    std::vector<uintptr_t> noneAddrs;

    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t addr = 0x10000;
    while (VirtualQuery(reinterpret_cast<void*>(addr), &mbi, sizeof(mbi)) && addr < 0x7FFFFFFFFFFF) {
        if (mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) &&
            mbi.RegionSize > 0 && mbi.RegionSize < 0x10000000) {

            uint8_t* base = reinterpret_cast<uint8_t*>(addr);
            // Search for L"None" = 4E 00 6F 00 6E 00 65 00 00 00
            for (size_t j = 0; j + 10 <= mbi.RegionSize; j += 2) {
                if (memcmp(base + j, target, 10) == 0) {
                    noneAddrs.push_back(addr + j);
                    if (noneAddrs.size() >= 500) break;
                }
            }
        }
        addr += mbi.RegionSize ? mbi.RegionSize : 0x1000;
        if (noneAddrs.size() >= 500) break;
    }

    LOG_INFO("Found %zu wide 'None' strings", noneAddrs.size());

    // For each None, check if (noneAddr - 0x18) could be a FNameEntry
    // and then look for pointers to that entry in .data
    // Read .data section from PE header
    uint8_t* peBase = reinterpret_cast<uint8_t*>(moduleBase);
    uint32_t peOff = *reinterpret_cast<uint32_t*>(peBase + 0x3C);
    uint16_t numSec = *reinterpret_cast<uint16_t*>(peBase + peOff + 6);
    uint16_t optSize = *reinterpret_cast<uint16_t*>(peBase + peOff + 20);
    uint8_t* secHdr = peBase + peOff + 24 + optSize;

    uintptr_t dataStart = 0;
    size_t dataSize = 0;
    for (int s = 0; s < numSec; s++) {
        char secName[9] = {};
        memcpy(secName, secHdr + s * 40, 8);
        if (strcmp(secName, ".data") == 0) {
            dataStart = moduleBase + *reinterpret_cast<uint32_t*>(secHdr + s * 40 + 12);
            dataSize = *reinterpret_cast<uint32_t*>(secHdr + s * 40 + 8);
            break;
        }
    }

    if (!dataStart) {
        LOG_ERR("Could not find .data section");
        return false;
    }

    LOG_INFO(".data at 0x%llX, size 0x%zX", dataStart, dataSize);

    for (uintptr_t noneAddr : noneAddrs) {
        uintptr_t entryAddr = noneAddr - 0x18; // FNameEntry starts 0x18 before Name

        // Scan .data for pointers to this entry (as GNames.Data[0])
        for (uintptr_t scan = dataStart; scan + 16 <= dataStart + dataSize; scan += 8) {
            uintptr_t val = 0;
            if (!IsReadable(reinterpret_cast<void*>(scan), 8)) continue;
            val = *reinterpret_cast<uintptr_t*>(scan);
            if (val == 0 || !IsReadable(reinterpret_cast<void*>(val), 8)) continue;

            // val is a pointer in .data. Check if val[0] == entryAddr (TArray.Data[0])
            uintptr_t firstEl = *reinterpret_cast<uintptr_t*>(val);
            if (firstEl != entryAddr) continue;

            // This could be GNames! The TArray struct should be at (scan - 0) if val IS Data
            // Check: scan is Data ptr, scan+8 is Count, scan+12 is Max
            int32_t count = *reinterpret_cast<int32_t*>(scan + 8);
            int32_t maxv = *reinterpret_cast<int32_t*>(scan + 12);

            if (count > 1000 && count < 500000 && maxv >= count) {
                // Validate second entry
                uintptr_t secondEl = *reinterpret_cast<uintptr_t*>(val + 8);
                if (secondEl && IsReadable(reinterpret_cast<void*>(secondEl), 0x20)) {
                    wchar_t* name2 = reinterpret_cast<wchar_t*>(
                        reinterpret_cast<uint8_t*>(secondEl) + 0x18);
                    if (IsReadable(name2, 4) && name2[0] >= L'A' && name2[0] <= L'z') {
                        char narrow2[32] = {};
                        wcstombs(narrow2, name2, 31);

                        LOG_INFO("*** GNames FOUND via data scan ***");
                        LOG_INFO("  TArray at .data+0x%llX, Count=%d", scan - dataStart, count);
                        LOG_INFO("  [0]='None', [1]='%s'", narrow2);

                        static GNamesArray gnArr;
                        gnArr.Data = reinterpret_cast<FNameEntry**>(val);
                        gnArr.Count = count;
                        gnArr.Max = maxv;
                        // But actually this IS the TArray at 'scan'
                        g_GNames = reinterpret_cast<GNamesArray*>(scan);
                        return true;
                    }
                }
            }
        }
    }

    LOG_ERR("GNames not found by any method");
    return false;
}

static bool FindGObjects() {
    LOG_INFO("Scanning for GObjects...");

    // Try pattern scan first
    for (int i = 0; GOBJECTS_SIGS[i].pattern; i++) {
        uintptr_t match = PatternScan::FindInModule(GOBJECTS_SIGS[i].pattern);
        if (!match) continue;

        LOG_INFO("GObjects pattern %d matched at 0x%llX", i, match);
        uintptr_t resolved = PatternScan::ResolveRIP(
            match, GOBJECTS_SIGS[i].dispOffset, GOBJECTS_SIGS[i].instrSize);
        LOG_INFO("  Resolved to 0x%llX", resolved);

        if (ValidateGObjects(resolved)) {
            LOG_INFO("*** GObjects found via pattern %d ***", i);
            return true;
        }
    }

    // Fallback: GObjects is typically 0x48 bytes after GNames
    if (g_GNames) {
        uintptr_t gnAddr = reinterpret_cast<uintptr_t>(g_GNames);
        LOG_INFO("Trying GNames+0x48 = 0x%llX", gnAddr + 0x48);
        if (ValidateGObjects(gnAddr + 0x48)) {
            LOG_INFO("*** GObjects found at GNames+0x48 ***");
            return true;
        }
        // Try nearby offsets
        for (int off = 0x10; off <= 0x100; off += 0x08) {
            if (ValidateGObjects(gnAddr + off)) {
                LOG_INFO("*** GObjects found at GNames+0x%X ***", off);
                return true;
            }
        }
    }

    LOG_ERR("GObjects not found");
    return false;
}

bool Initialize() {
    if (g_initialized) return true;

    LOG_INFO("=== UE3 Initialization ===");
    uintptr_t moduleBase;
    size_t moduleSize;
    if (!PatternScan::GetModuleInfo(moduleBase, moduleSize)) {
        LOG_ERR("Failed to get module info");
        return false;
    }
    LOG_INFO("Module base: 0x%llX, size: 0x%llX (%zu MB)",
             moduleBase, moduleSize, moduleSize / (1024 * 1024));

    if (!FindGNames()) return false;
    if (!FindGObjects()) return false;

    g_initialized = true;
    LOG_INFO("=== UE3 Initialized Successfully ===");

    // Log first 30 objects
    LOG_INFO("--- First 30 objects ---");
    int logged = 0;
    for (int i = 0; i < g_GObjects->Count && logged < 30; i++) {
        UObject* obj = g_GObjects->Data[i];
        if (!obj || !IsReadable(obj, sizeof(UObject))) continue;
        LOG_INFO("  [%d] %s", i, GetFullName(obj).c_str());
        logged++;
    }

    return true;
}

GNamesArray* GetGNames() { return g_GNames; }
TArray<UObject*>* GetGObjects() { return g_GObjects; }
bool IsInitialized() { return g_initialized; }

std::string GetName(const FName& name) {
    if (!g_GNames || !g_GNames->IsValid(name.Index)) return "None";
    FNameEntry* entry = g_GNames->Data[name.Index];
    if (!entry || !IsReadable(entry, 0x20)) return "None";

    // Convert wide string to narrow
    char narrow[256] = {};
    wcstombs(narrow, entry->Name, 255);

    std::string result = narrow;
    if (name.Number > 0) {
        result += "_" + std::to_string(name.Number - 1);
    }
    return result;
}

std::string GetObjectName(UObject* obj) {
    if (!obj) return "None";
    return GetName(obj->Name);
}

std::string GetFullName(UObject* obj) {
    if (!obj || !IsReadable(obj, sizeof(UObject))) return "None";

    std::string name = GetName(obj->Name);
    UObject* outer = obj->Outer;
    int depth = 0;
    while (outer && depth < 10) {
        if (!IsReadable(outer, sizeof(UObject))) break;
        name = GetName(outer->Name) + "." + name;
        outer = outer->Outer;
        depth++;
    }
    return name;
}

} // namespace UE3
