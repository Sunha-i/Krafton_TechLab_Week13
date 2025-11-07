#include "pch.h"
#include "LuaComponentProxy.h"

TMap<UClass*, FBoundClassDesc> GBoundClasses;
TMap<UClass*, sol::table>      GComponentFunctionTables;