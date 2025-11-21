#pragma once

#include "LuaComponentProxy.h"

// LuaObjectProxy is an alias for LuaComponentProxy
// Used by LuaArrayProxy, LuaMapProxy, LuaStructProxy for type checking
using LuaObjectProxy = LuaComponentProxy;
