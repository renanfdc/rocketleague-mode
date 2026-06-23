#pragma once
#include <cstdint>
#include <cstring>
#include <cwchar>

// UE3 type definitions for Rocket League x64
// Offsets from RLSDK Season 22 v2.67 (smallest-cock/RLSDK, CodeRed-Generator v1.1.5)

struct FName {
    int32_t Index;
    int32_t Number;
};

struct UObject;
struct UField;
struct UStruct;
struct UClass;
struct UFunction;
struct UProperty;

template<typename T>
struct TArray {
    T* Data;
    int32_t Count;
    int32_t Max;
    bool IsValid(int i) const { return i >= 0 && i < Count && Data != nullptr; }
};

// FNameEntry: Name is wchar_t at offset 0x18
struct FNameEntry {
    uint64_t Flags;           // 0x00
    int32_t  Index;           // 0x08
    uint8_t  Pad0C[0x0C];    // 0x0C
    wchar_t  Name[0x400];    // 0x18 - UTF-16
};

using GNamesArray = TArray<FNameEntry*>;

// UObject - 0x60 bytes
struct UObject {
    void*    VfTableObject;       // 0x00
    void*    HashNext;            // 0x08
    uint64_t ObjectFlags;         // 0x10
    void*    HashOuterNext;       // 0x18
    void*    StateFrame;          // 0x20
    void*    Linker;              // 0x28
    void*    LinkerIndex;         // 0x30
    int32_t  ObjectInternalInteger; // 0x38
    int32_t  NetIndex;            // 0x3C
    UObject* Outer;               // 0x40
    FName    Name;                // 0x48
    UClass*  Class;               // 0x50
    UObject* ObjectArchetype;     // 0x58
};
static_assert(sizeof(UObject) == 0x60);

// UField - 0x70 bytes
struct UField : UObject {
    UField*  Next;                // 0x60
    uint8_t  Pad68[0x08];        // 0x68
};
static_assert(sizeof(UField) == 0x70);

// UStruct - 0x130 bytes
struct UStruct : UField {
    uint8_t  Pad70[0x10];        // 0x70
    UField*  SuperField;          // 0x80
    UField*  Children;            // 0x88
    uint32_t PropertySize;        // 0x90
    uint8_t  Pad94[0x9C];        // 0x94
};
static_assert(sizeof(UStruct) == 0x130);

// UFunction - 0x160 bytes
struct UFunction : UStruct {
    uint64_t FunctionFlags;       // 0x130
    uint16_t iNative;             // 0x138
    uint16_t RepOffset;           // 0x13A
    FName    FriendlyName;        // 0x13C
    uint8_t  OperPrecedence;      // 0x144
    uint8_t  NumParms;            // 0x145
    uint16_t ParmsSize;           // 0x146
    uint16_t ReturnValueOffset;   // 0x148
    uint8_t  Pad14A[0x06];       // 0x14A
    void*    FirstStructWithDefaults; // 0x150
    void*    Func;                // 0x158 <<<< NATIVE FUNCTION POINTER
};
static_assert(sizeof(UFunction) == 0x160);

struct UClass : UStruct {};

// UProperty - 0xC8 bytes
struct UProperty : UField {
    uint32_t ArrayDim;            // 0x70
    uint32_t ElementSize;         // 0x74
    uint64_t PropertyFlags;       // 0x78
    uint8_t  Pad80[0x18];        // 0x80
    uint32_t Offset;              // 0x98
    uint8_t  Pad9C[0x2C];        // 0x9C
};
static_assert(sizeof(UProperty) == 0xC8);

// ProcessEvent vtable index 67, 4 arguments (confirmed by multiple SDK generators)
// MUST be called from GAME THREAD (UE3 ignores calls from other threads!)
inline void CallProcessEvent(UObject* obj, UFunction* func, void* params) {
    auto vtable = *reinterpret_cast<void***>(obj);
    using PEFn = void(*)(UObject*, UFunction*, void*, void*);
    auto pe = reinterpret_cast<PEFn>(vtable[67]);
    pe(obj, func, params, nullptr);
}
