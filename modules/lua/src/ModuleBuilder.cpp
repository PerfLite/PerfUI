#include "PerfUI/Lua/ModuleBuilder.h"

#include "LuaInternal.h"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
}

#include <exception>
#include <new>
#include <string>
#include <variant>

namespace PerfUI {
namespace {

constexpr const char* kInvokerMeta = "PerfUI.ScriptInvoker";

struct InvokerHolder {
    std::function<ScriptValue(ScriptArgs&)> invoker;
};

int InvokerGC(lua_State* L)
{
    auto* holder = static_cast<InvokerHolder*>(lua_touserdata(L, 1));
    if (holder) {
        holder->~InvokerHolder();
    }
    return 0;
}

int LuaInvoker(lua_State* L)
{
    auto* holder = static_cast<InvokerHolder*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!holder) {
        return luaL_error(L, "internal error: script function holder is missing");
    }

    const int top = lua_gettop(L);
    ScriptArgs args;
    args.reserve(static_cast<std::size_t>(top));

    for (int i = 1; i <= top; ++i) {
        switch (lua_type(L, i)) {
        case LUA_TBOOLEAN:
            args.emplace_back(lua_toboolean(L, i) != 0);
            break;
        case LUA_TNUMBER:
            args.emplace_back(static_cast<double>(lua_tonumber(L, i)));
            break;
        case LUA_TSTRING: {
            std::size_t length = 0;
            const char* text = lua_tolstring(L, i, &length);
            args.emplace_back(std::string(text, length));
            break;
        }
        default:
            return luaL_error(L, "argument %d has unsupported type '%s'", i, luaL_typename(L, i));
        }
    }

    try {
        ScriptValue result = holder->invoker(args);
        if (std::holds_alternative<std::monostate>(result)) {
            return 0;
        }
        if (const auto* flag = std::get_if<bool>(&result)) {
            lua_pushboolean(L, *flag ? 1 : 0);
            return 1;
        }
        if (const auto* number = std::get_if<double>(&result)) {
            lua_pushnumber(L, static_cast<lua_Number>(*number));
            return 1;
        }
        const auto& text = std::get<std::string>(result);
        lua_pushlstring(L, text.data(), text.size());
        return 1;
    } catch (const std::exception& exception) {
        return luaL_error(L, "%s", exception.what());
    } catch (...) {
        return luaL_error(L, "unknown C++ exception");
    }
}

void EnsureInvokerMeta(lua_State* L)
{
    if (luaL_newmetatable(L, kInvokerMeta)) {
        lua_pushcfunction(L, InvokerGC);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);
}

} // namespace

ModuleBuilder::~ModuleBuilder() = default;

void ModuleBuilder::addFunction(std::string a_name, std::function<ScriptValue(ScriptArgs&)> a_invoker)
{
    auto* sink = static_cast<ModuleSink*>(m_sink);
    if (!sink || !sink->L || sink->tableRef == -2) {
        return;
    }

    lua_State* L = sink->L;
    EnsureInvokerMeta(L);

    void* memory = lua_newuserdatauv(L, sizeof(InvokerHolder), 0);
    new (memory) InvokerHolder{ std::move(a_invoker) };
    luaL_setmetatable(L, kInvokerMeta);
    lua_pushcclosure(L, LuaInvoker, 1);

    lua_rawgeti(L, LUA_REGISTRYINDEX, sink->tableRef);
    lua_pushvalue(L, -2);
    lua_setfield(L, -2, a_name.c_str());
    lua_pop(L, 2);
}

} // namespace PerfUI
