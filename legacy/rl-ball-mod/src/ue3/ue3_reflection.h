#pragma once
#include "ue3_types.h"
#include "ue3_globals.h"
#include <string>
#include <vector>

namespace UE3 {

    // Find any UObject by full name (e.g., "TAGame.Ball_TA")
    UObject* FindObject(const std::string& fullName);

    // Find a UClass by name (e.g., "Texture2DDynamic")
    UClass* FindClass(const std::string& className);

    // Find all instances of a given class name
    std::vector<UObject*> FindAllInstances(const std::string& className);

    // Find a UFunction within a class by function name
    UFunction* FindFunction(UClass* cls, const std::string& funcName);

    // Find a UFunction by full path (e.g., "Engine.MaterialInstanceConstant.SetTextureParameterValue")
    UFunction* FindFunctionByFullName(const std::string& fullName);

    // Find default object for a class
    UObject* FindDefaultObject(const std::string& className);

    // Get a property offset within an object by property name
    int32_t GetPropertyOffset(UClass* cls, const std::string& propName);

    // Read a property value from an object
    template<typename T>
    T* GetProperty(UObject* obj, int32_t offset) {
        return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(obj) + offset);
    }
}
