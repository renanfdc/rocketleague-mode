#include "pattern_scan.h"
#include "logger.h"
#include <psapi.h>
#include <sstream>

namespace PatternScan {

struct PatternByte {
    uint8_t value;
    bool wildcard;
};

static std::vector<PatternByte> ParsePattern(const char* pattern) {
    std::vector<PatternByte> bytes;
    std::istringstream stream(pattern);
    std::string token;

    while (stream >> token) {
        if (token == "??" || token == "?") {
            bytes.push_back({0, true});
        } else {
            bytes.push_back({(uint8_t)strtoul(token.c_str(), nullptr, 16), false});
        }
    }
    return bytes;
}

uintptr_t Find(uintptr_t start, size_t size, const char* pattern) {
    auto bytes = ParsePattern(pattern);
    if (bytes.empty()) return 0;

    const uint8_t* data = reinterpret_cast<const uint8_t*>(start);
    size_t patLen = bytes.size();

    for (size_t i = 0; i <= size - patLen; i++) {
        bool found = true;
        for (size_t j = 0; j < patLen; j++) {
            if (!bytes[j].wildcard && data[i + j] != bytes[j].value) {
                found = false;
                break;
            }
        }
        if (found) {
            return start + i;
        }
    }
    return 0;
}

uintptr_t FindInModule(const char* pattern) {
    uintptr_t base;
    size_t size;
    if (!GetModuleInfo(base, size)) return 0;
    return Find(base, size, pattern);
}

uintptr_t ResolveRIP(uintptr_t instructionAddr, int ripOffset, int instructionSize) {
    int32_t displacement = *reinterpret_cast<int32_t*>(instructionAddr + ripOffset);
    return instructionAddr + instructionSize + displacement;
}

bool GetModuleInfo(uintptr_t& base, size_t& size) {
    HMODULE hModule = GetModuleHandle(NULL);
    if (!hModule) return false;

    MODULEINFO modInfo;
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo))) {
        return false;
    }

    base = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
    size = modInfo.SizeOfImage;
    return true;
}

} // namespace PatternScan
