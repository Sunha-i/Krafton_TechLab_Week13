#include "pch.h"
#include "LuaComponentProxy.h"
#include "LuaObjectProxyHelpers.h"
#include "LuaArrayProxy.h"
#include "LuaMapProxy.h"
#include "LuaStructProxy.h"
#include "ObjectFactory.h"

// Global bound classes map for reflection-based access
TMap<UClass*, FBoundClassDesc> GBoundClasses;

// External function from LuaManager.cpp
extern sol::object MakeCompProxy(sol::state_view SolState, UObject* Instance, UClass* Class);

// ===== Core Functions =====

void BuildBoundClass(UClass* Class)
{
    if (!Class) return;
    if (GBoundClasses.count(Class)) return;

    FBoundClassDesc Desc;
    Desc.Class = Class;

    // Count bound properties
    int BoundCount = 0;
    for (auto& Property : Class->GetAllProperties())
    {
        FBoundProp BoundProp;
        BoundProp.Property = &Property;
        Desc.PropsByName.emplace(Property.Name, BoundProp);
        BoundCount++;
    }

    UE_LOG("[LuaProxy] Bound class: %s (%d/%d properties exposed to Lua)",
           Class->Name, BoundCount, (int)Class->GetAllProperties().size());
    GBoundClasses.emplace(Class, std::move(Desc));
}

bool LuaComponentProxy::IsValid() const
{
    return IsValidUObject(Instance);
}

// ===== Index (Property/Method Access) =====

sol::object LuaComponentProxy::Index(sol::this_state LuaState, LuaComponentProxy& Self, const char* Key)
{
    if (!Self.Instance)
    {
        UE_LOG("[LuaProxy] Index: Instance is null for key '%s'", Key);
        return sol::nil;
    }

    sol::state_view LuaView(LuaState);

    // Build bound class for reflection fallback
    BuildBoundClass(Self.Class);

    // ===== 1. Registry-based lookup (LuaBindHelpers bindings) =====
    // Search inheritance chain for methods
    for (const UClass* CurrentClass = Self.Class; CurrentClass != nullptr; CurrentClass = CurrentClass->Super)
    {
        sol::table& BindTable = FLuaBindRegistry::Get().EnsureTable(LuaView, CurrentClass);
        if (!BindTable.valid()) continue;

        sol::object Result = BindTable[Key];
        if (!Result.valid()) continue;

        // Check if it's a property descriptor
        if (Result.is<sol::table>())
        {
            sol::table propDesc = Result.as<sol::table>();
            sol::optional<bool> isProperty = propDesc["is_property"];

            if (isProperty && *isProperty)
            {
                // Property - call getter
                sol::object getterObj = propDesc["get"];
                if (getterObj.valid())
                {
                    sol::protected_function getter = getterObj.as<sol::protected_function>();
                    auto pfr = getter(Self);
                    if (pfr.valid())
                        return pfr.get<sol::object>();
                }
                return sol::nil;
            }
        }

        // Function - return directly
        if (Result.get_type() == sol::type::function)
            return Result;
    }

    // ===== 2. Reflection-based fallback (LuaReadWrite metadata) =====
    auto It = GBoundClasses.find(Self.Class);
    if (It == GBoundClasses.end()) return sol::nil;

    auto ItProp = It->second.PropsByName.find(Key);
    if (ItProp == It->second.PropsByName.end()) return sol::nil;

    const FProperty* Property = ItProp->second.Property;

    switch (Property->Type)
    {
    case EPropertyType::Bool:
        return sol::make_object(LuaView, *Property->GetValuePtr<bool>(Self.Instance));
    case EPropertyType::Float:
        return sol::make_object(LuaView, *Property->GetValuePtr<float>(Self.Instance));
    case EPropertyType::Int32:
        return sol::make_object(LuaView, *Property->GetValuePtr<int>(Self.Instance));
    case EPropertyType::FString:
    case EPropertyType::ScriptFile:
        return sol::make_object(LuaView, *Property->GetValuePtr<FString>(Self.Instance));
    case EPropertyType::FVector:
        return sol::make_object(LuaView, *Property->GetValuePtr<FVector>(Self.Instance));
    case EPropertyType::FLinearColor:
        return sol::make_object(LuaView, *Property->GetValuePtr<FLinearColor>(Self.Instance));
    case EPropertyType::FName:
        return sol::make_object(LuaView, Property->GetValuePtr<FName>(Self.Instance)->ToString());

    // UObject pointer types (supports recursive access)
    case EPropertyType::ObjectPtr:
    case EPropertyType::Texture:
    case EPropertyType::SkeletalMesh:
    case EPropertyType::StaticMesh:
    case EPropertyType::Material:
    case EPropertyType::Sound:
    {
        UObject** ObjPtr = Property->GetValuePtr<UObject*>(Self.Instance);
        if (!ObjPtr || !IsValidUObject(*ObjPtr))
            return sol::nil;

        UObject* TargetObj = *ObjPtr;
        return MakeCompProxy(LuaView, TargetObj, TargetObj->GetClass());
    }

    // Array types - return LuaArrayProxy
    case EPropertyType::Array:
        return sol::make_object(LuaView, LuaArrayProxy(Self.Instance, Property));

    // Map types - return LuaMapProxy
    case EPropertyType::Map:
        return sol::make_object(LuaView, LuaMapProxy(Self.Instance, Property));

    // Struct types - return LuaStructProxy for recursive access
    case EPropertyType::Struct:
    {
        UStruct* StructType = UStruct::FindStruct(Property->TypeName);
        if (!StructType)
        {
            UE_LOG("[Lua][error] Unknown struct type: %s", Property->TypeName);
            return sol::nil;
        }

        void* StructInstance = (char*)Self.Instance + Property->Offset;
        return sol::make_object(LuaView, LuaStructProxy(StructInstance, StructType));
    }

    default:
        return sol::nil;
    }
}

// ===== NewIndex (Property Assignment) =====

void LuaComponentProxy::NewIndex(LuaComponentProxy& Self, const char* Key, sol::object Obj)
{
    if (!Self.Instance || !Self.Class) return;

    sol::state_view LuaView = Obj.lua_state();

    // Build bound class for reflection fallback
    BuildBoundClass(Self.Class);

    // ===== 1. Registry-based lookup first =====
    sol::table& BindTable = FLuaBindRegistry::Get().EnsureTable(LuaView, Self.Class);
    if (BindTable.valid())
    {
        sol::object Property = BindTable[Key];
        if (Property.valid() && Property.is<sol::table>())
        {
            sol::table propDesc = Property.as<sol::table>();
            sol::optional<bool> isProperty = propDesc["is_property"];

            if (isProperty && *isProperty)
            {
                // Check read-only
                sol::optional<bool> readOnly = propDesc["read_only"];
                if (readOnly && *readOnly)
                {
                    UE_LOG("[LuaProxy] Attempted to set read-only property: %s", Key);
                    return;
                }

                // Call setter
                sol::optional<sol::function> setter = propDesc["set"];
                if (setter)
                {
                    (*setter)(Self, Obj);
                }
                return;
            }
        }
    }

    // ===== 2. Reflection-based fallback =====
    auto IterateClass = GBoundClasses.find(Self.Class);
    if (IterateClass == GBoundClasses.end()) return;

    auto It = IterateClass->second.PropsByName.find(Key);
    if (It == IterateClass->second.PropsByName.end()) return;

    const FProperty* Property = It->second.Property;

    switch (Property->Type)
    {
    case EPropertyType::Bool:
        if (Obj.get_type() == sol::type::boolean)
            *Property->GetValuePtr<bool>(Self.Instance) = Obj.as<bool>();
        break;
    case EPropertyType::Float:
        if (Obj.get_type() == sol::type::number)
            *Property->GetValuePtr<float>(Self.Instance) = static_cast<float>(Obj.as<double>());
        break;
    case EPropertyType::Int32:
        if (Obj.get_type() == sol::type::number)
            *Property->GetValuePtr<int>(Self.Instance) = static_cast<int>(Obj.as<double>());
        break;
    case EPropertyType::FString:
    case EPropertyType::ScriptFile:
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
    case EPropertyType::FLinearColor:
        if (Obj.is<FLinearColor>())
        {
            *Property->GetValuePtr<FLinearColor>(Self.Instance) = Obj.as<FLinearColor>();
        }
        else if (Obj.get_type() == sol::type::table)
        {
            sol::table t = Obj.as<sol::table>();
            FLinearColor tmp{
                static_cast<float>(t.get_or("R", 1.0)),
                static_cast<float>(t.get_or("G", 1.0)),
                static_cast<float>(t.get_or("B", 1.0)),
                static_cast<float>(t.get_or("A", 1.0))
            };
            *Property->GetValuePtr<FLinearColor>(Self.Instance) = tmp;
        }
        break;
    case EPropertyType::FName:
        if (Obj.get_type() == sol::type::string)
            *Property->GetValuePtr<FName>(Self.Instance) = FName(Obj.as<FString>());
        break;

    // UObject pointer types
    case EPropertyType::ObjectPtr:
    case EPropertyType::Texture:
    case EPropertyType::SkeletalMesh:
    case EPropertyType::StaticMesh:
    case EPropertyType::Material:
    case EPropertyType::Sound:
    {
        UObject** ObjPtr = Property->GetValuePtr<UObject*>(Self.Instance);
        if (!ObjPtr) break;

        // nil assignment
        if (Obj.get_type() == sol::type::nil || Obj.get_type() == sol::type::none)
        {
            *ObjPtr = nullptr;
            break;
        }

        // Must be a proxy
        if (!Obj.is<LuaComponentProxy>())
        {
            UE_LOG("[Lua][warning] Cannot assign non-UObject to property '%s'", Property->Name);
            break;
        }

        LuaComponentProxy& SourceProxy = Obj.as<LuaComponentProxy&>();
        UObject* SourceObj = SourceProxy.Instance;

        if (!SourceObj)
        {
            *ObjPtr = nullptr;
            break;
        }

        if (!IsValidUObject(SourceObj))
        {
            UE_LOG("[Lua][warning] Cannot assign deleted UObject to property '%s'", Property->Name);
            break;
        }

        // Type validation
        if (Property->Type != EPropertyType::ObjectPtr)
        {
            UClass* ExpectedClass = GetExpectedClassForPropertyType(Property->Type);
            if (ExpectedClass && !SourceObj->IsA(ExpectedClass))
            {
                UE_LOG("[Lua][warning] Type mismatch: cannot assign %s to %s property '%s'",
                       SourceObj->GetClass()->Name, ExpectedClass->Name, Property->Name);
                break;
            }
        }

        *ObjPtr = SourceObj;
        break;
    }

    // Array types
    case EPropertyType::Array:
    {
        if (!IsObjectPointerType(Property->InnerType))
            break;

        TArray<UObject*>* ArrayPtr = Property->GetValuePtr<TArray<UObject*>>(Self.Instance);
        if (!ArrayPtr) break;

        // nil → clear
        if (Obj.get_type() == sol::type::nil || Obj.get_type() == sol::type::none)
        {
            ArrayPtr->clear();
            break;
        }

        // Must be table
        if (Obj.get_type() != sol::type::table)
        {
            UE_LOG("[Lua][warning] Cannot assign non-table to array property '%s'", Property->Name);
            break;
        }

        sol::table SourceTable = Obj.as<sol::table>();
        TArray<UObject*> NewArray;

        for (size_t i = 1; i <= SourceTable.size(); ++i)
        {
            sol::object Element = SourceTable[i];

            if (Element.get_type() == sol::type::nil)
            {
                NewArray.push_back(nullptr);
                continue;
            }

            if (!Element.is<LuaComponentProxy>())
            {
                NewArray.push_back(nullptr);
                continue;
            }

            LuaComponentProxy& ElemProxy = Element.as<LuaComponentProxy&>();
            UObject* ElemObj = ElemProxy.Instance;

            if (ElemObj && !IsValidUObject(ElemObj))
            {
                NewArray.push_back(nullptr);
                continue;
            }

            NewArray.push_back(ElemObj);
        }

        *ArrayPtr = std::move(NewArray);
        break;
    }

    // Struct - cannot replace directly
    case EPropertyType::Struct:
        UE_LOG("[Lua][warning] Cannot assign to struct property '%s' directly. Modify its fields instead.", Key);
        break;

    default:
        break;
    }
}
