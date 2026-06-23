#include "ue3_reflection.h"
#include "../util/logger.h"
#include <unordered_map>
#include <algorithm>

namespace UE3 {

// Cache for frequently looked-up objects
static std::unordered_map<std::string, UObject*> g_objectCache;
static std::unordered_map<std::string, UFunction*> g_functionCache;

UObject* FindObject(const std::string& fullName) {
    // Check cache
    auto it = g_objectCache.find(fullName);
    if (it != g_objectCache.end()) return it->second;

    auto* objects = GetGObjects();
    if (!objects) return nullptr;

    for (int i = 0; i < objects->Count; i++) {
        UObject* obj = objects->Data[i];
        if (!obj) continue;

        std::string objName = GetFullName(obj);
        if (objName == fullName) {
            g_objectCache[fullName] = obj;
            return obj;
        }
    }

    return nullptr;
}

UClass* FindClass(const std::string& className) {
    auto* objects = GetGObjects();
    if (!objects) return nullptr;

    for (int i = 0; i < objects->Count; i++) {
        UObject* obj = objects->Data[i];
        if (!obj || !obj->Class) continue;

        // Check if this object IS a Class and its name matches
        std::string clsName = GetName(obj->Class->Name);
        if (clsName != "Class") continue;

        std::string objName = GetObjectName(obj);
        if (objName == className) {
            return reinterpret_cast<UClass*>(obj);
        }
    }

    return nullptr;
}

std::vector<UObject*> FindAllInstances(const std::string& className) {
    std::vector<UObject*> results;
    auto* objects = GetGObjects();
    if (!objects) return results;

    UClass* targetClass = FindClass(className);
    if (!targetClass) {
        LOG_WARN("FindAllInstances: class '%s' not found", className.c_str());
        return results;
    }

    for (int i = 0; i < objects->Count; i++) {
        UObject* obj = objects->Data[i];
        if (!obj) continue;

        // Check if object is an instance of targetClass (including inheritance)
        UClass* cls = obj->Class;
        while (cls) {
            if (cls == targetClass) {
                results.push_back(obj);
                break;
            }
            cls = reinterpret_cast<UClass*>(cls->SuperField);
        }
    }

    return results;
}

UFunction* FindFunction(UClass* cls, const std::string& funcName) {
    if (!cls) return nullptr;

    // Walk class hierarchy
    UStruct* current = cls;
    while (current) {
        UField* child = current->Children;
        while (child) {
            std::string childName = GetName(child->Name);
            std::string childClassName = GetName(child->Class->Name);

            if (childClassName == "Function" && childName == funcName) {
                return reinterpret_cast<UFunction*>(child);
            }
            child = child->Next;
        }
        current = reinterpret_cast<UStruct*>(current->SuperField);
    }

    return nullptr;
}

UFunction* FindFunctionByFullName(const std::string& fullName) {
    // Check cache
    auto it = g_functionCache.find(fullName);
    if (it != g_functionCache.end()) return it->second;

    // Parse "Package.Class.Function" format
    auto* objects = GetGObjects();
    if (!objects) return nullptr;

    for (int i = 0; i < objects->Count; i++) {
        UObject* obj = objects->Data[i];
        if (!obj || !obj->Class) continue;

        std::string clsName = GetName(obj->Class->Name);
        if (clsName != "Function") continue;

        std::string objFullName = GetFullName(obj);
        if (objFullName == fullName) {
            auto* func = reinterpret_cast<UFunction*>(obj);
            g_functionCache[fullName] = func;
            return func;
        }
    }

    return nullptr;
}

UObject* FindDefaultObject(const std::string& className) {
    std::string defaultName = "Default__" + className;
    auto* objects = GetGObjects();
    if (!objects) return nullptr;

    for (int i = 0; i < objects->Count; i++) {
        UObject* obj = objects->Data[i];
        if (!obj) continue;

        std::string name = GetObjectName(obj);
        if (name == defaultName) {
            return obj;
        }
    }

    return nullptr;
}

int32_t GetPropertyOffset(UClass* cls, const std::string& propName) {
    if (!cls) return -1;

    UStruct* current = cls;
    while (current) {
        UField* child = current->Children;
        while (child) {
            std::string name = GetName(child->Name);
            std::string clsName = GetName(child->Class->Name);

            // Properties have class names like "ObjectProperty", "IntProperty", etc.
            if (name == propName && clsName.find("Property") != std::string::npos) {
                auto* prop = reinterpret_cast<UProperty*>(child);
                return prop->Offset;
            }
            child = child->Next;
        }
        current = reinterpret_cast<UStruct*>(current->SuperField);
    }

    return -1;
}

} // namespace UE3
