#pragma once

#include <functional>
#include <sol/sol.hpp>

#include "LuaBindingRegistry.h"

// ===== Reflection System Support =====

struct FBoundProp
{
    const FProperty* Property = nullptr;
};

struct FBoundClassDesc
{
    UClass* Class = nullptr;
    TMap<FString, FBoundProp> PropsByName;
};

extern TMap<UClass*, FBoundClassDesc> GBoundClasses;

void BuildBoundClass(UClass* Class);

// ===== Main Proxy Class =====

/**
 * Proxy object that bridges C++ component instances to Lua
 * Supports both:
 * 1. Registry-based binding (LuaBindHelpers - AddProperty, AddMethod, etc.)
 * 2. Reflection-based binding (LuaReadWrite metadata)
 */
struct LuaComponentProxy
{
    UObject* Instance = nullptr;  // Type-safe UObject pointer
    UClass* Class = nullptr;

    // Validate if the UObject instance is still valid
    bool IsValid() const;

    // Get raw UObject pointer
    UObject* Get() const { return Instance; }

    static sol::object Index(sol::this_state LuaState, LuaComponentProxy& Self, const char* Key);
    static void        NewIndex(LuaComponentProxy& Self, const char* Key, sol::object Obj);
};
