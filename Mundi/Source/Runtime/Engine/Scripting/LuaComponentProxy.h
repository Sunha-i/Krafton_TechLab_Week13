#pragma once

#include <functional>
#include <sol/sol.hpp>

#include "LuaBindingRegistry.h"

struct FBoundProp
{
    const FProperty* Property = nullptr;  // TODO: editable/readonly flags
};

struct FBoundClassDesc   // Property list per class
{
    UClass* Class = nullptr;
    TMap<FString, FBoundProp> PropsByName;
};

extern TMap<UClass*, FBoundClassDesc> GBoundClasses;
extern TMap<UClass*, sol::table>      GComponentFunctionTables;

struct LuaTypeIO
{
    std::function<sol::object(sol::state_view, const void*, const FProperty&)> Read;   // C++ -> Lua
    std::function<void(sol::object, void*, const FProperty&)> Write;                   // Lua -> C++
};

struct LuaComponentProxy
{
    void*   Instance = nullptr;  // object address
    UClass* Class    = nullptr;  // meta info

    static sol::object Index(sol::this_state LuaState, LuaComponentProxy& Self, const char* Key)
    {
        if (!Self.Instance) return sol::nil;
        sol::state_view LuaView(LuaState);

        sol::table& FuncTable = FLuaBindRegistry::Get().EnsureTable(LuaView, Self.Class);
        sol::object Func = (FuncTable.valid() ? FuncTable.raw_get<sol::object>(Key) : sol::object());
        if (!Func.valid())
        {
            if (auto ItLegacy = GComponentFunctionTables.find(Self.Class); ItLegacy != GComponentFunctionTables.end())
            {
                Func = ItLegacy->second.raw_get<sol::object>(Key);
            }
        }

        if (Func.valid() && Func.get_type() == sol::type::function)
            return Func;

        auto It = GBoundClasses.find(Self.Class);
        if (It == GBoundClasses.end()) return sol::nil;

        auto ItProp = It->second.PropsByName.find(Key);
        if (ItProp == It->second.PropsByName.end()) return sol::nil;

        const FProperty* Property = ItProp->second.Property;
        switch (Property->Type)
        {
            case EPropertyType::Float:   return sol::make_object(LuaView, *Property->GetValuePtr<float>(Self.Instance));
            case EPropertyType::Int32:   return sol::make_object(LuaView, *Property->GetValuePtr<int>(Self.Instance));
            case EPropertyType::FString: return sol::make_object(LuaView, *Property->GetValuePtr<FString>(Self.Instance));
            case EPropertyType::FVector: return sol::make_object(LuaView, *Property->GetValuePtr<FVector>(Self.Instance));
            default: return sol::nil;
        }
    }

    static void NewIndex(LuaComponentProxy& Self, const char* Key, sol::object Obj)
    {
        auto IterateClass = GBoundClasses.find(Self.Class);
        if (IterateClass == GBoundClasses.end()) return;

        auto It = IterateClass->second.PropsByName.find(Key);
        if (It == IterateClass->second.PropsByName.end()) return;

        const FProperty* Property = It->second.Property;

        switch (Property->Type)
        {
            case EPropertyType::Float:
                if (Obj.get_type() == sol::type::number)
                    *Property->GetValuePtr<float>(Self.Instance) = static_cast<float>(Obj.as<double>());
                break;
            case EPropertyType::Int32:
                if (Obj.get_type() == sol::type::number)
                    *Property->GetValuePtr<int>(Self.Instance) = static_cast<int>(Obj.as<double>());
                break;
            case EPropertyType::FString:
                if (Obj.get_type() == sol::type::string)
                    *Property->GetValuePtr<FString>(Self.Instance) = Obj.as<FString>();
                break;
            case EPropertyType::FVector:
                if (Obj.is<FVector>())
                {
                    *Property->GetValuePtr<FVector>(Self.Instance) = Obj.as<FVector>();
                }
                else if (Obj.get_type() == sol::type::table)
                {
                    sol::table t = Obj.as<sol::table>();
                    FVector tmp{
                        static_cast<float>(t.get_or("X", 0.0)),
                        static_cast<float>(t.get_or("Y", 0.0)),
                        static_cast<float>(t.get_or("Z", 0.0))
                    };
                    *Property->GetValuePtr<FVector>(Self.Instance) = tmp;
                }
                break;
            default:
                break;
        }
    }
};
