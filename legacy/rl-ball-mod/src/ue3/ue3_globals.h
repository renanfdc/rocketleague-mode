#pragma once
#include "ue3_types.h"
#include <string>

namespace UE3 {
    // Initialize: find GNames and GObjects via pattern scanning
    bool Initialize();

    // Accessors
    GNamesArray* GetGNames();
    TArray<UObject*>* GetGObjects();

    // Get name string from FName
    std::string GetName(const FName& name);

    // Get full path name of an object (e.g., "Engine.Texture2DDynamic")
    std::string GetFullName(UObject* obj);

    // Get object name only (no path)
    std::string GetObjectName(UObject* obj);

    // Check if engine is initialized
    bool IsInitialized();
}
