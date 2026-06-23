#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <vector>
#include <string>

namespace PatternScan {
    // Scan for a byte pattern in a memory region
    // Pattern format: "48 8D 0D ?? ?? ?? ?? E8" where ?? is wildcard
    uintptr_t Find(uintptr_t start, size_t size, const char* pattern);

    // Scan within the main module (.text section)
    uintptr_t FindInModule(const char* pattern);

    // Resolve a RIP-relative address (LEA instruction)
    // instructionAddr points to the start of the LEA, ripOffset is the offset
    // within the instruction where the 32-bit relative displacement starts
    uintptr_t ResolveRIP(uintptr_t instructionAddr, int ripOffset, int instructionSize);

    // Get main module base and size
    bool GetModuleInfo(uintptr_t& base, size_t& size);
}
