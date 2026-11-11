#include "PerfUI/Lua/ScriptHost.h"

#include "LuaInternal.h"
#include "PerfUI/UIContext.h"
#include "PerfUI/UIElement.h"
#include "PerfUI/Text.h"
#include "PerfUI/Label.h"
#include "PerfUI/Button.h"
#include "PerfUI/Slider.h"
#include "PerfUI/ProgressBar.h"
#include "PerfUI/Checkbox.h"
#include "PerfUI/ComboBox.h"
#include "PerfUI/TabBar.h"
#include "PerfUI/TextInput.h"
#include "PerfUI/Panel.h"
#include "PerfUI/Theme.h"
#include "PerfUI/Toast.h"
#include "PerfUI/Animation.h"
#include "PerfUI/WorldAnchor.h"
#include "PerfUI/Image.h"
#include "PerfUI/SvgImage.h"
#include "PerfUI/Localization.h"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include <algorithm>
#include <cassert>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <variant>

namespace PerfUI {
namespace {

using Clock = std::chrono::steady_clock;

constexpr int kHookStep = 2000;
constexpr std::uint64_t kMinInstructionBudget = kHookStep;
constexpr std::size_t kEmergencyBytes = 256 * 1024;
constexpr const char* kRequireCacheKey = "PerfUI.RequireCache";

#ifndef NDEBUG
#define PERFUI_LUA_ASSERT_THREAD(impl) \
    assert((impl)->ownerThread == std::this_thread::get_id() && "ScriptHost must be used from its owning UI thread")
#else
#define PERFUI_LUA_ASSERT_THREAD(impl) ((void)0)
#endif

ScriptLogSink& LogSinkStorage()
{
    static ScriptLogSink sink;
    return sink;
}

void* LuaAllocator(void* userdata, void* pointer, std::size_t oldSize, std::size_t newSize)
{
    auto* state = static_cast<ScriptAllocState*>(userdata);
    if (newSize == 0) {
        std::free(pointer);
        state->used -= (std::min)(state->used, oldSize);
        return nullptr;
    }

    if (pointer == nullptr) {
        oldSize = 0;
    }

    const std::size_t nextUsed = state->used - oldSize + newSize;
    if (state->limit > 0 && nextUsed > state->limit) {
        // Разрешаем небольшой запас сверх лимита: без него Lua не сможет
        // создать объект ошибки и уйдёт в panic вместо MEMERR.
        if (nextUsed > state->limit + kEmergencyBytes) {
            return nullptr;
        }
    }

    void* result = std::realloc(pointer, newSize);
    if (result) {
        state->used = nextUsed;
    }
    return result;
}

void PushScriptValue(lua_State* L, const ScriptValue& value)
{
    if (const auto* flag = std::get_if<bool>(&value)) {
        lua_pushboolean(L, *flag ? 1 : 0);
    } else if (const auto* number = std::get_if<double>(&value)) {
        lua_pushnumber(L, static_cast<lua_Number>(*number));
    } else if (const auto* text = std::get_if<std::string>(&value)) {
        lua_pushlstring(L, text->data(), text->size());
    } else {
        lua_pushnil(L);
    }
}

std::string ShortSourceName(const char* shortSource)
{
    if (!shortSource) {
        return {};
    }
    std::string_view source(shortSource);
    if (!source.empty() && source.front() == '=') {
        source.remove_prefix(1);
    } else if (source.rfind("[string \"", 0) == 0) {
        source.remove_prefix(9);
        if (!source.empty() && source.back() == ']') {
            source.remove_suffix(1);
        }
        if (!source.empty() && source.back() == '"') {
            source.remove_suffix(1);
        }
    }
    return std::string(source);
}

std::string NormalizeLuaMessage(std::string message)
{
    static constexpr std::string_view kChunkPrefix = "[string \"";
    if (message.rfind(kChunkPrefix.data(), 0) != 0) {
        return message;
    }
    const auto closing = message.find("\"]:");
    if (closing == std::string::npos) {
        return message;
    }
    const std::string name = message.substr(kChunkPrefix.size(), closing - kChunkPrefix.size());
    return name + message.substr(closing + 2);
}

int LL_LogLine(lua_State* L)
{
    ScriptHost::Impl* host = ScriptHost::Impl::FromState(L);
    const auto level = static_cast<ScriptLogLevel>(lua_tointeger(L, lua_upvalueindex(1)));

    std::string prefix;
    lua_Debug frame{};
    if (lua_getstack(L, 1, &frame) != 0 && lua_getinfo(L, "Sl", &frame) != 0 && frame.currentline > 0) {
        prefix = ShortSourceName(frame.short_src) + ":" + std::to_string(frame.currentline) + ": ";
    }

    std::string message;
    const int top = lua_gettop(L);
    for (int i = 1; i <= top; ++i) {
        if (i > 1) {
            message += '\t';
        }
        std::size_t length = 0;
        const char* text = luaL_tolstring(L, i, &length);
        message.append(text ? text : "", text ? length : 0);
        lua_pop(L, 1);
    }

    host->log(level, prefix + message);
    return 0;
}

int LL_LogCall(lua_State* L)
{
    lua_remove(L, 1);  // drop the log table passed to __call
    return LL_LogLine(L);
}

int LL_CollectGarbage(lua_State* L)
{
    const char* option = luaL_optstring(L, 1, "count");
    if (std::strcmp(option, "count") != 0) {
        return luaL_error(L, "collectgarbage('%s') is not allowed; only 'count' is available", option);
    }
    const int kilobytes = lua_gc(L, LUA_GCCOUNT, 0);
    const int remainder = lua_gc(L, LUA_GCCOUNTB, 0);
    lua_pushnumber(L, static_cast<lua_Number>(kilobytes) + static_cast<lua_Number>(remainder) / 1024.0);
    return 1;
}

int LL_Require(lua_State* L)
{
    ScriptHost::Impl* host = ScriptHost::Impl::FromState(L);

    std::size_t length = 0;
    const char* rawName = luaL_checklstring(L, 1, &length);
    const std::string name(rawName, length);

    if (!host->isSafeModuleName(name)) {
        return luaL_error(L, "require('%s'): invalid module name", name.c_str());
    }
    if (host->scriptRoot.empty()) {
        return luaL_error(L, "require('%s'): no script directory is known yet (load a file first)", name.c_str());
    }

    std::string relative;
    relative.reserve(name.size() + 4);
    for (char ch : name) {
        relative += (ch == '.') ? '/' : ch;
    }
    relative += ".lua";

    std::error_code ec;
    const std::filesystem::path root = std::filesystem::weakly_canonical(host->scriptRoot, ec);
    if (ec) {
        return luaL_error(L, "require('%s'): %s", name.c_str(), ec.message().c_str());
    }
    const std::filesystem::path full = std::filesystem::weakly_canonical(root / relative, ec);
    if (ec) {
        return luaL_error(L, "require('%s'): %s", name.c_str(), ec.message().c_str());
    }

    std::string rootKey = root.generic_string();
    if (!rootKey.empty() && rootKey.back() != '/') {
        rootKey += '/';
    }
    std::string fullKey = full.generic_string();
#if defined(_WIN32)
    std::string rootLower = rootKey;
    std::string fullLower = fullKey;
    std::transform(rootLower.begin(), rootLower.end(), rootLower.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    std::transform(fullLower.begin(), fullLower.end(), fullLower.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    const bool insideRoot = fullLower.compare(0, rootLower.size(), rootLower) == 0;
#else
    const bool insideRoot = fullKey.compare(0, rootKey.size(), rootKey) == 0;
#endif
    if (!insideRoot) {
        return luaL_error(L, "require('%s'): path escapes the script directory", name.c_str());
    }

    lua_getfield(L, LUA_REGISTRYINDEX, kRequireCacheKey);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, kRequireCacheKey);
    }

    lua_getfield(L, -1, fullKey.c_str());
    if (!lua_isnil(L, -1)) {
        return 1;
    }
    lua_pop(L, 1);

    std::ifstream file(full, std::ios::binary);
    if (!file) {
        return luaL_error(L, "require('%s'): cannot open %s", name.c_str(), fullKey.c_str());
    }
    std::string code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    if (luaL_loadbufferx(L, code.data(), code.size(), fullKey.c_str(), "t") != LUA_OK) {
        return lua_error(L);
    }
    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        return lua_error(L);
    }

    lua_pushvalue(L, -1);
    lua_setfield(L, -3, fullKey.c_str());
    return 1;
}

void RegisterLogTable(lua_State* L)
{
    lua_newtable(L);

    const auto addLevelFunction = [L](const char* field, ScriptLogLevel level) {
        lua_pushinteger(L, static_cast<lua_Integer>(level));
        lua_pushcclosure(L, LL_LogLine, 1);
        lua_setfield(L, -2, field);
    };
    addLevelFunction("info", ScriptLogLevel::Info);
    addLevelFunction("warn", ScriptLogLevel::Warn);
    addLevelFunction("error", ScriptLogLevel::Error);

    lua_newtable(L);
    lua_pushcfunction(L, LL_LogCall);
    lua_setfield(L, -2, "__call");
    lua_setmetatable(L, -2);

    lua_setglobal(L, "log");

    lua_getglobal(L, "log");
    lua_getfield(L, -1, "info");
    lua_setglobal(L, "print");
    lua_pop(L, 1);
}

void ApplySandbox(lua_State* L)
{
    for (const char* name : { "dofile", "loadfile", "load", "package", "io", "os", "debug" }) {
        lua_pushnil(L);
        lua_setglobal(L, name);
    }

    lua_pushcfunction(L, LL_CollectGarbage);
    lua_setglobal(L, "collectgarbage");

    lua_pushcfunction(L, LL_Require);
    lua_setglobal(L, "require");

    RegisterLogTable(L);
}

// ---------------------------------------------------------------------------
// UI Element Handles & 'ui' Library
// ---------------------------------------------------------------------------

struct LuaElementHandle {
    ElementId id{ 0 };
    std::string name;
    WeakElementRef ref;
};

constexpr const char* kElementHandleMeta = "PerfUI.ElementHandle";

UIElement* CheckHandleElement(lua_State* L, int argIdx, const char* methodName)
{
    auto* handle = static_cast<LuaElementHandle*>(luaL_checkudata(L, argIdx, kElementHandleMeta));
    if (!handle) {
        luaL_error(L, "bad self to '%s' (expected UIElement handle)", methodName);
        return nullptr;
    }
    UIElement* elem = handle->ref.get();
    if (!elem) {
        ScriptHost::Impl* impl = ScriptHost::Impl::FromState(L);
        if (impl) {
            impl->log(ScriptLogLevel::Warn,
                      "attempt to use destroyed element '" + handle->name + "' in method '" + methodName + "'");
        }
        return nullptr;
    }
    return elem;
}

int Handle_id(lua_State* L)
{
    auto* handle = static_cast<LuaElementHandle*>(luaL_checkudata(L, 1, kElementHandleMeta));
    lua_pushinteger(L, static_cast<lua_Integer>(handle->id));
    return 1;
}

int Handle_name(lua_State* L)
{
    auto* handle = static_cast<LuaElementHandle*>(luaL_checkudata(L, 1, kElementHandleMeta));
    lua_pushstring(L, handle->name.c_str());
    return 1;
}

int Handle_isValid(lua_State* L)
{
    auto* handle = static_cast<LuaElementHandle*>(luaL_checkudata(L, 1, kElementHandleMeta));
    lua_pushboolean(L, handle->ref.isValid() ? 1 : 0);
    return 1;
}

int Handle_isVisible(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "isVisible");
    if (!elem) {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_pushboolean(L, elem->isVisible() ? 1 : 0);
    return 1;
}

int Handle_setVisible(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "setVisible");
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    bool visible = lua_toboolean(L, 2) != 0;
    if (elem) {
        elem->setVisible(visible);
    }
    return 0;
}

int Handle_isEnabled(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "isEnabled");
    if (!elem) {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_pushboolean(L, elem->isEnabled() ? 1 : 0);
    return 1;
}

int Handle_setEnabled(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "setEnabled");
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    bool enabled = lua_toboolean(L, 2) != 0;
    if (elem) {
        elem->setEnabled(enabled);
    }
    return 0;
}

int Handle_isFocused(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "isFocused");
    if (!elem) {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_pushboolean(L, elem->isFocused() ? 1 : 0);
    return 1;
}

int Handle_opacity(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "opacity");
    if (!elem) {
        lua_pushnumber(L, 0.0);
        return 1;
    }
    lua_pushnumber(L, static_cast<lua_Number>(elem->opacity()));
    return 1;
}

int Handle_setOpacity(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "setOpacity");
    lua_Number opacity = luaL_checknumber(L, 2);
    if (elem) {
        elem->setOpacity(static_cast<float>(opacity));
    }
    return 0;
}

int Handle_text(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "text");
    if (!elem) {
        lua_pushnil(L);
        return 1;
    }
    if (auto* t = dynamic_cast<Text*>(elem)) {
        lua_pushstring(L, t->text().c_str());
        return 1;
    }
    if (auto* b = dynamic_cast<Button*>(elem)) {
        lua_pushstring(L, b->text().c_str());
        return 1;
    }
    if (auto* pb = dynamic_cast<ProgressBar*>(elem)) {
        lua_pushstring(L, pb->text().c_str());
        return 1;
    }
    if (auto* ti = dynamic_cast<TextInput*>(elem)) {
        lua_pushstring(L, ti->text().c_str());
        return 1;
    }
    if (auto* cb = dynamic_cast<Checkbox*>(elem)) {
        lua_pushstring(L, cb->label().c_str());
        return 1;
    }
    lua_pushstring(L, "");
    return 1;
}

int Handle_setText(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "setText");
    const char* str = luaL_checkstring(L, 2);
    if (elem) {
        if (auto* t = dynamic_cast<Text*>(elem)) {
            t->text(str);
        } else if (auto* pb = dynamic_cast<ProgressBar*>(elem)) {
            pb->setText(str);
        } else if (auto* b = dynamic_cast<Button*>(elem)) {
            b->text(str);
        } else if (auto* ti = dynamic_cast<TextInput*>(elem)) {
            ti->text(str);
        } else if (auto* cb = dynamic_cast<Checkbox*>(elem)) {
            cb->label(str);
        } else if (auto* sl = dynamic_cast<Slider*>(elem)) {
            sl->label(str);
        }
    }
    return 0;
}

int Handle_value(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "value");
    if (!elem) {
        lua_pushnil(L);
        return 1;
    }
    if (auto* sl = dynamic_cast<Slider*>(elem)) {
        lua_pushnumber(L, static_cast<lua_Number>(sl->value()));
        return 1;
    }
    if (auto* pb = dynamic_cast<ProgressBar*>(elem)) {
        lua_pushnumber(L, static_cast<lua_Number>(pb->value()));
        return 1;
    }
    if (auto* cb = dynamic_cast<Checkbox*>(elem)) {
        lua_pushnumber(L, cb->isChecked() ? 1.0 : 0.0);
        return 1;
    }
    lua_pushnumber(L, 0.0);
    return 1;
}

int Handle_setValue(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "setValue");
    lua_Number val = luaL_checknumber(L, 2);
    if (elem) {
        if (auto* sl = dynamic_cast<Slider*>(elem)) {
            sl->value(static_cast<float>(val));
        } else if (auto* pb = dynamic_cast<ProgressBar*>(elem)) {
            pb->value(static_cast<float>(val));
        } else if (auto* cb = dynamic_cast<Checkbox*>(elem)) {
            cb->checked(val != 0.0);
        }
    }
    return 0;
}

int Handle_isChecked(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "isChecked");
    if (!elem) {
        lua_pushboolean(L, 0);
        return 1;
    }
    if (auto* cb = dynamic_cast<Checkbox*>(elem)) {
        lua_pushboolean(L, cb->isChecked() ? 1 : 0);
        return 1;
    }
    lua_pushboolean(L, 0);
    return 1;
}

int Handle_setChecked(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "setChecked");
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    bool chk = lua_toboolean(L, 2) != 0;
    if (elem) {
        if (auto* cb = dynamic_cast<Checkbox*>(elem)) {
            cb->checked(chk);
        }
    }
    return 0;
}

int Handle_selectedIndex(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "selectedIndex");
    if (!elem) {
        lua_pushnil(L);
        return 1;
    }
    if (auto* cb = dynamic_cast<ComboBox*>(elem)) {
        lua_pushinteger(L, static_cast<lua_Integer>(cb->selectedIndex()));
        return 1;
    }
    if (auto* tb = dynamic_cast<TabBar*>(elem)) {
        lua_pushinteger(L, static_cast<lua_Integer>(tb->selectedTab()));
        return 1;
    }
    lua_pushinteger(L, 0);
    return 1;
}

int Handle_selectIndex(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "selectIndex");
    lua_Integer idx = luaL_checkinteger(L, 2);
    if (idx < 0) {
        return luaL_error(L, "index must be non-negative");
    }
    if (elem) {
        if (auto* cb = dynamic_cast<ComboBox*>(elem)) {
            cb->selectIndex(static_cast<size_t>(idx));
        } else if (auto* tb = dynamic_cast<TabBar*>(elem)) {
            tb->selectTab(static_cast<size_t>(idx));
        }
    }
    return 0;
}

int Handle_setColor(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "setColor");
    if (!elem) return 0;
    Color col;
    if (lua_isstring(L, 2) && !lua_isnumber(L, 2)) {
        const char* str = lua_tostring(L, 2);
        if (str && str[0] == '#') {
            unsigned int hexVal = 0;
            if (sscanf_s(str + 1, "%x", &hexVal) == 1) {
                if (strlen(str + 1) <= 6) {
                    col = Color(static_cast<uint8_t>((hexVal >> 16) & 0xFF),
                                static_cast<uint8_t>((hexVal >> 8) & 0xFF),
                                static_cast<uint8_t>(hexVal & 0xFF),
                                255);
                } else {
                    col = Color(static_cast<uint8_t>((hexVal >> 24) & 0xFF),
                                static_cast<uint8_t>((hexVal >> 16) & 0xFF),
                                static_cast<uint8_t>((hexVal >> 8) & 0xFF),
                                static_cast<uint8_t>(hexVal & 0xFF));
                }
            }
        }
    } else {
        lua_Number r = luaL_checknumber(L, 2);
        lua_Number g = luaL_checknumber(L, 3);
        lua_Number b = luaL_checknumber(L, 4);
        lua_Number a = lua_isnumber(L, 5) ? lua_tonumber(L, 5) : 255.0;
        col = Color(static_cast<uint8_t>(std::clamp(r, 0.0, 255.0)),
                    static_cast<uint8_t>(std::clamp(g, 0.0, 255.0)),
                    static_cast<uint8_t>(std::clamp(b, 0.0, 255.0)),
                    static_cast<uint8_t>(std::clamp(a, 0.0, 255.0)));
    }

    if (auto* txt = dynamic_cast<Text*>(elem)) {
        txt->color(col);
    } else if (auto* pb = dynamic_cast<ProgressBar*>(elem)) {
        pb->fillColor(col);
    } else if (auto* pnl = dynamic_cast<Panel*>(elem)) {
        pnl->backgroundColor(col);
    } else if (auto* svg = dynamic_cast<SvgImage*>(elem)) {
        svg->tint(col);
    }
    return 0;
}

int Handle_setSize(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "setSize");
    if (!elem) return 0;
    lua_Number w = luaL_checknumber(L, 2);
    lua_Number h = luaL_checknumber(L, 3);
    float fw = static_cast<float>(w);
    float fh = static_cast<float>(h);
    elem->setSize(Dimensions{ fw, fh });
    elem->layout().width(fw);
    elem->layout().height(fh);
    elem->markLayoutDirty();
    return 0;
}

int Handle_setPosition(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "setPosition");
    if (!elem) return 0;
    float fx = static_cast<float>(luaL_checknumber(L, 2));
    float fy = static_cast<float>(luaL_checknumber(L, 3));
    elem->setPosition(Point{ fx, fy });
    return 0;
}

int Handle_position(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "position");
    if (!elem) {
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
        return 2;
    }
    const auto pos = elem->position();
    lua_pushnumber(L, static_cast<lua_Number>(pos.x));
    lua_pushnumber(L, static_cast<lua_Number>(pos.y));
    return 2;
}

int Handle_setWidth(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "setWidth");
    if (!elem) return 0;
    float fw = static_cast<float>(luaL_checknumber(L, 2));
    elem->layout().width(fw);
    elem->setSize(Dimensions{ fw, elem->size().height });
    elem->markLayoutDirty();
    return 0;
}

int Handle_setHeight(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "setHeight");
    if (!elem) return 0;
    float fh = static_cast<float>(luaL_checknumber(L, 2));
    elem->layout().height(fh);
    elem->setSize(Dimensions{ elem->size().width, fh });
    elem->markLayoutDirty();
    return 0;
}

int Handle_setFontSize(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "setFontSize");
    if (!elem) return 0;
    lua_Number size = luaL_checknumber(L, 2);
    float fs = static_cast<float>(size);
    if (auto* txt = dynamic_cast<Text*>(elem)) {
        txt->fontSize(fs);
    } else if (auto* pb = dynamic_cast<ProgressBar*>(elem)) {
        pb->fontSize(fs);
    }
    return 0;
}

int Handle_targetId(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "targetId");
    if (!elem) {
        lua_pushnil(L);
        return 1;
    }
    if (auto* wa = dynamic_cast<WorldAnchor*>(elem)) {
        lua_pushinteger(L, static_cast<lua_Integer>(wa->targetId()));
        return 1;
    }
    lua_pushinteger(L, 0);
    return 1;
}

int Handle_setTargetId(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "setTargetId");
    lua_Integer id = luaL_checkinteger(L, 2);
    if (elem) {
        if (auto* wa = dynamic_cast<WorldAnchor*>(elem)) {
            wa->setTargetId(static_cast<uint32_t>(id));
        }
    }
    return 0;
}

int Handle_setScreenOffset(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "setScreenOffset");
    lua_Number ox = luaL_checknumber(L, 2);
    lua_Number oy = luaL_checknumber(L, 3);
    if (elem) {
        if (auto* wa = dynamic_cast<WorldAnchor*>(elem)) {
            wa->setScreenOffset(Point{ static_cast<float>(ox), static_cast<float>(oy) });
        }
    }
    return 0;
}

int Handle_isClamped(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "isClamped");
    if (!elem) {
        lua_pushboolean(L, 0);
        return 1;
    }
    if (auto* wa = dynamic_cast<WorldAnchor*>(elem)) {
        lua_pushboolean(L, wa->isClamped() ? 1 : 0);
        return 1;
    }
    lua_pushboolean(L, 0);
    return 1;
}

int Handle_clampedAngle(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "clampedAngle");
    if (!elem) {
        lua_pushnumber(L, 0.0);
        return 1;
    }
    if (auto* wa = dynamic_cast<WorldAnchor*>(elem)) {
        lua_pushnumber(L, static_cast<lua_Number>(wa->clampedAngle()));
        return 1;
    }
    lua_pushnumber(L, 0.0);
    return 1;
}

int Handle_currentScale(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "currentScale");
    if (!elem) {
        lua_pushnumber(L, 1.0);
        return 1;
    }
    if (auto* wa = dynamic_cast<WorldAnchor*>(elem)) {
        lua_pushnumber(L, static_cast<lua_Number>(wa->currentScale()));
        return 1;
    }
    lua_pushnumber(L, 1.0);
    return 1;
}

int Handle_setTexture(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "setTexture");
    lua_Integer texId = luaL_checkinteger(L, 2);
    if (elem) {
        if (auto* img = dynamic_cast<Image*>(elem)) {
            img->texture(static_cast<TextureId>(texId));
        }
    }
    return 0;
}

int Handle_setUV(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "setUV");
    lua_Number u0 = luaL_checknumber(L, 2);
    lua_Number v0 = luaL_checknumber(L, 3);
    lua_Number u1 = luaL_checknumber(L, 4);
    lua_Number v1 = luaL_checknumber(L, 5);
    if (elem) {
        if (auto* img = dynamic_cast<Image*>(elem)) {
            img->uv({ static_cast<float>(u0), static_cast<float>(v0) },
                    { static_cast<float>(u1), static_cast<float>(v1) });
        }
    }
    return 0;
}

int Handle_setFlipY(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "setFlipY");
    bool flip = lua_toboolean(L, 2) != 0;
    if (elem) {
        if (auto* img = dynamic_cast<Image*>(elem)) {
            img->flipY(flip);
        }
    }
    return 0;
}

int Handle_addClass(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "addClass");
    const char* cls = luaL_checkstring(L, 2);
    if (elem && cls) {
        elem->addClass(cls);
    }
    return 0;
}

int Handle_removeClass(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "removeClass");
    const char* cls = luaL_checkstring(L, 2);
    if (elem && cls) {
        elem->removeClass(cls);
    }
    return 0;
}

int Handle_toggleClass(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "toggleClass");
    const char* cls = luaL_checkstring(L, 2);
    if (elem && cls) {
        elem->toggleClass(cls);
    }
    return 0;
}

int Handle_hasClass(lua_State* L)
{
    UIElement* elem = CheckHandleElement(L, 1, "hasClass");
    const char* cls = luaL_checkstring(L, 2);
    if (elem && cls) {
        lua_pushboolean(L, elem->hasClass(cls) ? 1 : 0);
    } else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

int Handle_tostring(lua_State* L)
{
    auto* handle = static_cast<LuaElementHandle*>(luaL_checkudata(L, 1, kElementHandleMeta));
    std::string str = "UIElement('" + handle->name + "', id=" + std::to_string(handle->id) +
                      ", alive=" + (handle->ref.isValid() ? "true" : "false") + ")";
    lua_pushstring(L, str.c_str());
    return 1;
}

int Handle_gc(lua_State* L)
{
    auto* handle = static_cast<LuaElementHandle*>(luaL_checkudata(L, 1, kElementHandleMeta));
    if (handle) {
        handle->~LuaElementHandle();
    }
    return 0;
}

void RegisterElementHandleMeta(lua_State* L)
{
    if (luaL_newmetatable(L, kElementHandleMeta)) {
        lua_pushcfunction(L, Handle_gc);
        lua_setfield(L, -2, "__gc");

        lua_pushcfunction(L, Handle_tostring);
        lua_setfield(L, -2, "__tostring");

        lua_newtable(L);

        lua_pushcfunction(L, Handle_id);
        lua_setfield(L, -2, "id");
        lua_pushcfunction(L, Handle_name);
        lua_setfield(L, -2, "name");
        lua_pushcfunction(L, Handle_isValid);
        lua_setfield(L, -2, "isValid");
        lua_pushcfunction(L, Handle_isVisible);
        lua_setfield(L, -2, "isVisible");
        lua_pushcfunction(L, Handle_setVisible);
        lua_setfield(L, -2, "setVisible");
        lua_pushcfunction(L, Handle_isEnabled);
        lua_setfield(L, -2, "isEnabled");
        lua_pushcfunction(L, Handle_setEnabled);
        lua_setfield(L, -2, "setEnabled");
        lua_pushcfunction(L, Handle_isFocused);
        lua_setfield(L, -2, "isFocused");
        lua_pushcfunction(L, Handle_opacity);
        lua_setfield(L, -2, "opacity");
        lua_pushcfunction(L, Handle_setOpacity);
        lua_setfield(L, -2, "setOpacity");
        lua_pushcfunction(L, Handle_text);
        lua_setfield(L, -2, "text");
        lua_pushcfunction(L, Handle_setText);
        lua_setfield(L, -2, "setText");
        lua_pushcfunction(L, Handle_value);
        lua_setfield(L, -2, "value");
        lua_pushcfunction(L, Handle_setValue);
        lua_setfield(L, -2, "setValue");
        lua_pushcfunction(L, Handle_isChecked);
        lua_setfield(L, -2, "isChecked");
        lua_pushcfunction(L, Handle_setChecked);
        lua_setfield(L, -2, "setChecked");
        lua_pushcfunction(L, Handle_selectedIndex);
        lua_setfield(L, -2, "selectedIndex");
        lua_pushcfunction(L, Handle_selectIndex);
        lua_setfield(L, -2, "selectIndex");
        lua_pushcfunction(L, Handle_setColor);
        lua_setfield(L, -2, "setColor");
        lua_pushcfunction(L, Handle_setColor);
        lua_setfield(L, -2, "setTint");
        lua_pushcfunction(L, Handle_setSize);
        lua_setfield(L, -2, "setSize");
        lua_pushcfunction(L, Handle_setPosition);
        lua_setfield(L, -2, "setPosition");
        lua_pushcfunction(L, Handle_position);
        lua_setfield(L, -2, "position");
        lua_pushcfunction(L, Handle_setWidth);
        lua_setfield(L, -2, "setWidth");
        lua_pushcfunction(L, Handle_setHeight);
        lua_setfield(L, -2, "setHeight");
        lua_pushcfunction(L, Handle_setFontSize);
        lua_setfield(L, -2, "setFontSize");

        // WorldAnchor
        lua_pushcfunction(L, Handle_targetId);
        lua_setfield(L, -2, "targetId");
        lua_pushcfunction(L, Handle_setTargetId);
        lua_setfield(L, -2, "setTargetId");
        lua_pushcfunction(L, Handle_setScreenOffset);
        lua_setfield(L, -2, "setScreenOffset");
        lua_pushcfunction(L, Handle_isClamped);
        lua_setfield(L, -2, "isClamped");
        lua_pushcfunction(L, Handle_clampedAngle);
        lua_setfield(L, -2, "clampedAngle");
        lua_pushcfunction(L, Handle_currentScale);
        lua_setfield(L, -2, "currentScale");

        // Image
        lua_pushcfunction(L, Handle_setTexture);
        lua_setfield(L, -2, "setTexture");
        lua_pushcfunction(L, Handle_setUV);
        lua_setfield(L, -2, "setUV");
        lua_pushcfunction(L, Handle_setFlipY);
        lua_setfield(L, -2, "setFlipY");

        // CSS Classes
        lua_pushcfunction(L, Handle_addClass);
        lua_setfield(L, -2, "addClass");
        lua_pushcfunction(L, Handle_removeClass);
        lua_setfield(L, -2, "removeClass");
        lua_pushcfunction(L, Handle_toggleClass);
        lua_setfield(L, -2, "toggleClass");
        lua_pushcfunction(L, Handle_hasClass);
        lua_setfield(L, -2, "hasClass");

        lua_setfield(L, -2, "__index");
    }
    lua_pop(L, 1);
}

int LL_UiFind(lua_State* L)
{
    ScriptHost::Impl* impl = ScriptHost::Impl::FromState(L);
    const char* name = luaL_checkstring(L, 1);
    UIElement* elem = impl ? impl->findElement(name) : nullptr;
    if (!elem) {
        lua_pushnil(L);
        return 1;
    }
    impl->pushElementHandle(elem);
    return 1;
}

int LL_UiToast(lua_State* L)
{
    ScriptHost::Impl* impl = ScriptHost::Impl::FromState(L);
    int top = lua_gettop(L);
    if (top < 1) {
        return luaL_error(L, "ui.toast requires at least 1 argument");
    }

    std::string title;
    std::string message;
    std::string typeStr = "info";

    if (top == 1) {
        title = impl ? impl->name : "PerfUI";
        message = luaL_checkstring(L, 1);
    } else if (top == 2) {
        message = luaL_checkstring(L, 1);
        typeStr = luaL_checkstring(L, 2);
        title = impl ? impl->name : "PerfUI";
    } else {
        title = luaL_checkstring(L, 1);
        message = luaL_checkstring(L, 2);
        typeStr = luaL_checkstring(L, 3);
    }

    ToastType type = ToastType::Info;
    if (typeStr == "success") type = ToastType::Success;
    else if (typeStr == "warning") type = ToastType::Warning;
    else if (typeStr == "error") type = ToastType::Error;

    if (impl && impl->boundContext) {
        impl->boundContext->showToast(std::move(title), std::move(message), type);
    } else if (impl) {
        impl->log(ScriptLogLevel::Info, "toast [" + typeStr + "] " + message);
    }
    return 0;
}

int LL_UiTimer(lua_State* L)
{
    ScriptHost::Impl* impl = ScriptHost::Impl::FromState(L);
    lua_Number delay = luaL_checknumber(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_pushvalue(L, 2);
    int funcRef = luaL_ref(L, LUA_REGISTRYINDEX);

    uint32_t id = impl->addTimer(static_cast<float>(delay), funcRef, false);
    lua_pushinteger(L, static_cast<lua_Integer>(id));
    return 1;
}

int LL_UiInterval(lua_State* L)
{
    ScriptHost::Impl* impl = ScriptHost::Impl::FromState(L);
    lua_Number interval = luaL_checknumber(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_pushvalue(L, 2);
    int funcRef = luaL_ref(L, LUA_REGISTRYINDEX);

    uint32_t id = impl->addTimer(static_cast<float>(interval), funcRef, true);
    lua_pushinteger(L, static_cast<lua_Integer>(id));
    return 1;
}

int LL_UiCancel(lua_State* L)
{
    ScriptHost::Impl* impl = ScriptHost::Impl::FromState(L);
    lua_Integer id = luaL_checkinteger(L, 1);
    bool cancelled = impl->cancelTimer(static_cast<uint32_t>(id));
    lua_pushboolean(L, cancelled ? 1 : 0);
    return 1;
}

int LL_UiThemeColor(lua_State* L)
{
    const char* token = luaL_checkstring(L, 1);
    const auto& c = Theme::Current().colors;
    Color foundColor;
    bool found = false;

    if (std::strcmp(token, "surfaceBase") == 0) { foundColor = c.surfaceBase; found = true; }
    else if (std::strcmp(token, "surfaceElevated") == 0) { foundColor = c.surfaceElevated; found = true; }
    else if (std::strcmp(token, "surfaceActive") == 0) { foundColor = c.surfaceActive; found = true; }
    else if (std::strcmp(token, "borderSubtle") == 0) { foundColor = c.borderSubtle; found = true; }
    else if (std::strcmp(token, "borderStrong") == 0) { foundColor = c.borderStrong; found = true; }
    else if (std::strcmp(token, "borderFocus") == 0) { foundColor = c.borderFocus; found = true; }
    else if (std::strcmp(token, "textPrimary") == 0) { foundColor = c.textPrimary; found = true; }
    else if (std::strcmp(token, "textSecondary") == 0) { foundColor = c.textSecondary; found = true; }
    else if (std::strcmp(token, "textAccent") == 0) { foundColor = c.textAccent; found = true; }
    else if (std::strcmp(token, "textSuccess") == 0) { foundColor = c.textSuccess; found = true; }
    else if (std::strcmp(token, "textDisabled") == 0) { foundColor = c.textDisabled; found = true; }
    else if (std::strcmp(token, "focusGlow") == 0) { foundColor = c.focusGlow; found = true; }
    else if (std::strcmp(token, "selectionFill") == 0) { foundColor = c.selectionFill; found = true; }

    if (!found) {
        lua_pushnil(L);
        return 1;
    }

    char hexBuf[16];
    std::snprintf(hexBuf, sizeof(hexBuf), "#%02X%02X%02X%02X", foundColor.r, foundColor.g, foundColor.b, foundColor.a);

    lua_createtable(L, 0, 5);
    lua_pushinteger(L, foundColor.r);
    lua_setfield(L, -2, "r");
    lua_pushinteger(L, foundColor.g);
    lua_setfield(L, -2, "g");
    lua_pushinteger(L, foundColor.b);
    lua_setfield(L, -2, "b");
    lua_pushinteger(L, foundColor.a);
    lua_setfield(L, -2, "a");
    lua_pushstring(L, hexBuf);
    lua_setfield(L, -2, "hex");

    return 1;
}

int LL_UiTranslate(lua_State* L)
{
    const char* key = luaL_checkstring(L, 1);
    const char* fallback = lua_isstring(L, 2) ? lua_tostring(L, 2) : "";
    std::string res = LocalizationService::instance().translate(key, fallback);
    lua_pushstring(L, res.c_str());
    return 1;
}

int LL_UiSetLanguage(lua_State* L)
{
    const char* lang = luaL_checkstring(L, 1);
    LocalizationService::instance().setLanguage(lang);
    return 0;
}

int LL_UiGetLanguage(lua_State* L)
{
    const std::string& lang = LocalizationService::instance().currentLanguage();
    lua_pushstring(L, lang.c_str());
    return 1;
}

int LL_UiLoadDictionary(lua_State* L)
{
    const char* lang = luaL_checkstring(L, 1);
    const char* path = luaL_checkstring(L, 2);
    bool ok = LocalizationService::instance().loadDictionary(lang, path);
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
}

static UIElement* ResolveElementArg(lua_State* L, int idx, ScriptHost::Impl* impl)
{
    if (lua_isstring(L, idx)) {
        const char* name = lua_tostring(L, idx);
        return impl ? impl->findElement(name) : nullptr;
    }
    auto* handle = static_cast<LuaElementHandle*>(luaL_checkudata(L, idx, kElementHandleMeta));
    return (handle && handle->ref.isValid()) ? handle->ref.get() : nullptr;
}

int LL_UiAddClass(lua_State* L)
{
    ScriptHost::Impl* impl = ScriptHost::Impl::FromState(L);
    UIElement* elem = ResolveElementArg(L, 1, impl);
    const char* cls = luaL_checkstring(L, 2);
    if (elem && cls) {
        elem->addClass(cls);
    }
    return 0;
}

int LL_UiRemoveClass(lua_State* L)
{
    ScriptHost::Impl* impl = ScriptHost::Impl::FromState(L);
    UIElement* elem = ResolveElementArg(L, 1, impl);
    const char* cls = luaL_checkstring(L, 2);
    if (elem && cls) {
        elem->removeClass(cls);
    }
    return 0;
}

int LL_UiToggleClass(lua_State* L)
{
    ScriptHost::Impl* impl = ScriptHost::Impl::FromState(L);
    UIElement* elem = ResolveElementArg(L, 1, impl);
    const char* cls = luaL_checkstring(L, 2);
    if (elem && cls) {
        elem->toggleClass(cls);
    }
    return 0;
}

int LL_UiHasClass(lua_State* L)
{
    ScriptHost::Impl* impl = ScriptHost::Impl::FromState(L);
    UIElement* elem = ResolveElementArg(L, 1, impl);
    const char* cls = luaL_checkstring(L, 2);
    if (elem && cls) {
        lua_pushboolean(L, elem->hasClass(cls) ? 1 : 0);
    } else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

int LL_UiSetVar(lua_State* L)
{
    ScriptHost::Impl* impl = ScriptHost::Impl::FromState(L);
    const char* name = luaL_checkstring(L, 1);
    const char* val = luaL_checkstring(L, 2);
    if (impl && impl->boundContext && name && val) {
        impl->boundContext->setCssVariable(name, val);
    }
    return 0;
}

int LL_UiGetVar(lua_State* L)
{
    ScriptHost::Impl* impl = ScriptHost::Impl::FromState(L);
    const char* name = luaL_checkstring(L, 1);
    if (impl && impl->boundContext && name) {
        std::string val = impl->boundContext->getCssVariable(name);
        lua_pushstring(L, val.c_str());
    } else {
        lua_pushstring(L, "");
    }
    return 1;
}

int LL_UiReloadStyles(lua_State* L)
{
    ScriptHost::Impl* impl = ScriptHost::Impl::FromState(L);
    if (impl && impl->boundContext) {
        impl->boundContext->reloadStyles();
    }
    return 0;
}

void RegisterUiTable(lua_State* L)
{
    RegisterElementHandleMeta(L);

    lua_newtable(L);

    lua_pushcfunction(L, LL_UiFind);
    lua_setfield(L, -2, "find");

    lua_pushcfunction(L, LL_UiToast);
    lua_setfield(L, -2, "toast");

    lua_pushcfunction(L, LL_UiTimer);
    lua_setfield(L, -2, "timer");

    lua_pushcfunction(L, LL_UiInterval);
    lua_setfield(L, -2, "interval");

    lua_pushcfunction(L, LL_UiCancel);
    lua_setfield(L, -2, "cancel");

    // Localization
    lua_pushcfunction(L, LL_UiTranslate);
    lua_setfield(L, -2, "translate");
    lua_pushcfunction(L, LL_UiTranslate);
    lua_setfield(L, -2, "tr");
    lua_pushcfunction(L, LL_UiSetLanguage);
    lua_setfield(L, -2, "setLanguage");
    lua_pushcfunction(L, LL_UiGetLanguage);
    lua_setfield(L, -2, "getLanguage");
    lua_pushcfunction(L, LL_UiLoadDictionary);
    lua_setfield(L, -2, "loadDictionary");

    // PSS / CSS Style API
    lua_pushcfunction(L, LL_UiAddClass);
    lua_setfield(L, -2, "addClass");
    lua_pushcfunction(L, LL_UiRemoveClass);
    lua_setfield(L, -2, "removeClass");
    lua_pushcfunction(L, LL_UiToggleClass);
    lua_setfield(L, -2, "toggleClass");
    lua_pushcfunction(L, LL_UiHasClass);
    lua_setfield(L, -2, "hasClass");
    lua_pushcfunction(L, LL_UiSetVar);
    lua_setfield(L, -2, "setVar");
    lua_pushcfunction(L, LL_UiGetVar);
    lua_setfield(L, -2, "getVar");
    lua_pushcfunction(L, LL_UiReloadStyles);
    lua_setfield(L, -2, "reloadStyles");

    lua_newtable(L);
    lua_pushcfunction(L, LL_UiThemeColor);
    lua_setfield(L, -2, "color");
    lua_setfield(L, -2, "theme");

    lua_setglobal(L, "ui");
}

UIElement* FindElementRecursive(UIElement* current, std::string_view name)
{
    if (!current) return nullptr;
    if (current->name() == name) return current;
    for (const auto& child : current->children()) {
        if (UIElement* found = FindElementRecursive(child.get(), name)) {
            return found;
        }
    }
    return nullptr;
}

void LimitHook(lua_State* L, lua_Debug*)
{
    ScriptHost::Impl* host = ScriptHost::Impl::FromState(L);
    if (host->instructionBudget >= static_cast<std::uint64_t>(kHookStep)) {
        host->instructionBudget -= static_cast<std::uint64_t>(kHookStep);
    } else {
        host->instructionBudget = 0;
    }

    if (host->instructionBudget == 0) {
        luaL_error(L, "instruction limit exceeded (max %I per call)",
                   static_cast<lua_Integer>(host->limits.maxInstructionsPerCall));
    }
    if (Clock::now() >= host->deadline) {
        luaL_error(L, "time limit exceeded (max %f ms per call)",
                   static_cast<lua_Number>(host->limits.maxMillisecondsPerCall));
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------

ScriptHost::Impl* ScriptHost::Impl::FromState(lua_State* state)
{
    return *static_cast<Impl**>(lua_getextraspace(state));
}

bool ScriptHost::Impl::createState()
{
    alloc = ScriptAllocState{};
    alloc.limit = limits.maxMemoryBytes;

    L = lua_newstate(&LuaAllocator, &alloc);
    if (!L) {
        disabled = true;
        lastError = "[Lua:" + name + "] failed to create Lua state within " +
                    std::to_string(limits.maxMemoryBytes) + " bytes";
        return false;
    }

    *static_cast<Impl**>(lua_getextraspace(L)) = this;

    struct Library {
        const char* name;
        lua_CFunction open;
    };
    const Library libraries[] = {
        { LUA_GNAME, luaopen_base },
        { LUA_TABLIBNAME, luaopen_table },
        { LUA_STRLIBNAME, luaopen_string },
        { LUA_MATHLIBNAME, luaopen_math },
        { LUA_UTF8LIBNAME, luaopen_utf8 },
        { LUA_COLIBNAME, luaopen_coroutine }
    };
    for (const auto& library : libraries) {
        luaL_requiref(L, library.name, library.open, 1);
        lua_pop(L, 1);
    }

    ApplySandbox(L);
    registerUiLibrary();
    return true;
}

void ScriptHost::Impl::destroyState()
{
    clearTimers();
    if (L) {
        lua_close(L);
        L = nullptr;
    }
}

void ScriptHost::Impl::registerAllModules()
{
    for (const auto& module : modules) {
        applyModule(module);
    }
}

void ScriptHost::Impl::applyModule(const RegisteredModule& module)
{
    if (!L) {
        return;
    }

    lua_newtable(L);
    const int tableRef = luaL_ref(L, LUA_REGISTRYINDEX);

    ModuleSink sink{ L, tableRef, module.name };
    try {
        ModuleBuilder builder(&sink);
        module.setup(builder);
    } catch (const std::exception& exception) {
        reportError("module '" + module.name + "': " + exception.what());
    } catch (...) {
        reportError("module '" + module.name + "': unknown exception");
    }

    std::string modName = module.name;
    auto dotPos = modName.find('.');
    if (dotPos != std::string::npos) {
        std::string parentName = modName.substr(0, dotPos);
        std::string childName = modName.substr(dotPos + 1);

        lua_getglobal(L, parentName.c_str());
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_setglobal(L, parentName.c_str());
        }

        lua_rawgeti(L, LUA_REGISTRYINDEX, tableRef);
        lua_setfield(L, -2, childName.c_str());
        lua_pop(L, 1);
    } else {
        lua_getglobal(L, module.name.c_str());
        if (lua_istable(L, -1)) {
            int globalIdx = lua_gettop(L);
            lua_rawgeti(L, LUA_REGISTRYINDEX, tableRef);
            int newTableIdx = lua_gettop(L);
            lua_pushnil(L);
            while (lua_next(L, newTableIdx) != 0) {
                lua_pushvalue(L, -2);
                lua_pushvalue(L, -2);
                lua_settable(L, globalIdx);
                lua_pop(L, 1);
            }
            lua_pop(L, 2);
        } else {
            lua_pop(L, 1);
            lua_rawgeti(L, LUA_REGISTRYINDEX, tableRef);
            lua_setglobal(L, module.name.c_str());
        }
    }
    luaL_unref(L, LUA_REGISTRYINDEX, tableRef);
}

bool ScriptHost::Impl::protectedCall(int nargs, int nresults)
{
    if (!L) {
        return false;
    }

    instructionBudget = (std::max)(limits.maxInstructionsPerCall, kMinInstructionBudget);
    deadline = Clock::now() + std::chrono::duration_cast<Clock::duration>(
                                  std::chrono::duration<double, std::milli>(limits.maxMillisecondsPerCall));
    lua_sethook(L, &LimitHook, LUA_MASKCOUNT, kHookStep);

    const int status = lua_pcall(L, nargs, nresults, 0);
    lua_sethook(L, nullptr, 0, 0);

    if (status != LUA_OK) {
        const char* message = lua_tostring(L, -1);
        reportError(message ? message : "unknown Lua error");
        lua_pop(L, 1);
        return false;
    }

    resetErrorStreak();
    return true;
}

bool ScriptHost::Impl::callFunction(const char* functionName, const ScriptArgs* args)
{
    if (!L || disabled) {
        return false;
    }

    lua_getglobal(L, functionName);
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        reportError(std::string(functionName) + ": function not found");
        return false;
    }

    int nargs = 0;
    if (args) {
        for (const auto& value : *args) {
            PushScriptValue(L, value);
            ++nargs;
        }
    }

    return protectedCall(nargs, 0);
}

void ScriptHost::Impl::reportError(std::string message)
{
    message = NormalizeLuaMessage(std::move(message));
    lastError = "[Lua:" + name + "] " + message;
    ++consecutiveErrors;
    log(ScriptLogLevel::Error, message);

    if (!disabled && limits.maxConsecutiveErrors > 0 && consecutiveErrors >= limits.maxConsecutiveErrors) {
        disabled = true;
        log(ScriptLogLevel::Error, "disabled after " + std::to_string(consecutiveErrors) + " consecutive errors");
    }
}

void ScriptHost::Impl::resetErrorStreak()
{
    consecutiveErrors = 0;
    lastError.clear();
}

bool ScriptHost::Impl::isSafeModuleName(std::string_view moduleName) const
{
    if (moduleName.empty() || moduleName.size() > 256) {
        return false;
    }
    for (char ch : moduleName) {
        const bool allowed = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                             (ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '.';
        if (!allowed) {
            return false;
        }
    }
    if (moduleName.find("..") != std::string_view::npos) {
        return false;
    }
    if (moduleName.front() == '.' || moduleName.back() == '.') {
        return false;
    }
    return true;
}

void ScriptHost::Impl::log(ScriptLogLevel level, const std::string& body) const
{
    const std::string line = "[Lua:" + name + "] " + body;
    auto& sink = LogSinkStorage();
    if (sink) {
        sink(level, line);
        return;
    }
    std::fprintf(stderr, "%s\n", line.c_str());
}

uint32_t ScriptHost::Impl::addTimer(float delay, int funcRef, bool repeating)
{
    uint32_t id = nextTimerId++;
    float d = (repeating && delay <= 0.0f) ? 0.001f : delay;
    timers.push_back(TimerEntry{ id, d, d, repeating, funcRef });
    return id;
}

bool ScriptHost::Impl::cancelTimer(uint32_t id)
{
    auto it = std::find_if(timers.begin(), timers.end(), [id](const TimerEntry& e) { return e.id == id; });
    if (it != timers.end()) {
        if (L && it->funcRef >= 0) {
            luaL_unref(L, LUA_REGISTRYINDEX, it->funcRef);
        }
        timers.erase(it);
        return true;
    }
    return false;
}

void ScriptHost::Impl::clearTimers()
{
    if (L) {
        for (const auto& t : timers) {
            if (t.funcRef >= 0) {
                luaL_unref(L, LUA_REGISTRYINDEX, t.funcRef);
            }
        }
    }
    timers.clear();
}

void ScriptHost::Impl::updateTimers(float dt)
{
    if (timers.empty() || !L || disabled) {
        return;
    }

    std::vector<uint32_t> expiredIds;
    for (auto& t : timers) {
        t.remaining -= dt;
        if (t.remaining <= 0.0f) {
            expiredIds.push_back(t.id);
        }
    }

    for (uint32_t id : expiredIds) {
        if (disabled || !L) {
            break;
        }
        auto it = std::find_if(timers.begin(), timers.end(), [id](const TimerEntry& e) { return e.id == id; });
        if (it == timers.end()) {
            continue;
        }

        int funcRef = it->funcRef;
        bool repeating = it->repeating;
        if (repeating) {
            it->remaining = (std::max)(0.001f, it->interval);
        } else {
            timers.erase(it);
        }

        lua_rawgeti(L, LUA_REGISTRYINDEX, funcRef);
        if (!repeating) {
            luaL_unref(L, LUA_REGISTRYINDEX, funcRef);
        }

        if (lua_isfunction(L, -1)) {
            protectedCall(0, 0);
        } else {
            lua_pop(L, 1);
        }
    }
}

UIElement* ScriptHost::Impl::findElement(std::string_view elementName) const
{
    if (boundRoot) {
        if (UIElement* found = FindElementRecursive(boundRoot, elementName)) {
            return found;
        }
    }
    if (boundContext && boundContext->root()) {
        if (UIElement* found = FindElementRecursive(boundContext->root(), elementName)) {
            return found;
        }
    }
    return nullptr;
}

void ScriptHost::Impl::pushElementHandle(UIElement* element)
{
    if (!element || !L) {
        lua_pushnil(L);
        return;
    }

    void* mem = lua_newuserdata(L, sizeof(LuaElementHandle));
    auto* handle = new (mem) LuaElementHandle();
    handle->id = element->id();
    handle->name = element->name();
    handle->ref = WeakElementRef(element);

    luaL_getmetatable(L, kElementHandleMeta);
    lua_setmetatable(L, -2);
}

void ScriptHost::Impl::registerUiLibrary()
{
    if (L) {
        RegisterUiTable(L);
    }
}

// ---------------------------------------------------------------------------
// ScriptHost
// ---------------------------------------------------------------------------

ScriptHost::ScriptHost(std::string name, ScriptLimits limits)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->name = std::move(name);
    m_impl->limits = limits;
    m_impl->ownerThread = std::this_thread::get_id();
    if (!m_impl->createState()) {
        return;
    }
    m_impl->registerAllModules();
}

ScriptHost::~ScriptHost()
{
    m_impl->destroyState();
}

bool ScriptHost::loadFile(const std::filesystem::path& file)
{
    PERFUI_LUA_ASSERT_THREAD(m_impl);

    std::error_code ec;
    const std::filesystem::path absolute = std::filesystem::absolute(file, ec);
    if (ec) {
        m_impl->reportError("cannot resolve script path: " + file.string());
        return false;
    }

    m_impl->scriptRoot = absolute.parent_path();

    std::ifstream stream(absolute, std::ios::binary);
    if (!stream) {
        m_impl->reportError("cannot open script: " + absolute.string());
        return false;
    }
    std::string code((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

    return loadString(code, absolute.filename().string());
}

bool ScriptHost::loadString(std::string_view code, std::string_view chunkName)
{
    PERFUI_LUA_ASSERT_THREAD(m_impl);

    if (!m_impl->L || m_impl->disabled) {
        return false;
    }

    const std::string chunk(chunkName);
    if (luaL_loadbufferx(m_impl->L, code.data(), code.size(), chunk.c_str(), "t") != LUA_OK) {
        const char* message = lua_tostring(m_impl->L, -1);
        m_impl->reportError(message ? message : "syntax error");
        lua_pop(m_impl->L, 1);
        return false;
    }

    return m_impl->protectedCall(0, 0);
}

bool ScriptHost::hasFunction(std::string_view name) const
{
    PERFUI_LUA_ASSERT_THREAD(m_impl);

    if (!m_impl->L) {
        return false;
    }
    const std::string functionName(name);
    lua_getglobal(m_impl->L, functionName.c_str());
    const bool exists = lua_isfunction(m_impl->L, -1) != 0;
    lua_pop(m_impl->L, 1);
    return exists;
}

bool ScriptHost::call(std::string_view name)
{
    PERFUI_LUA_ASSERT_THREAD(m_impl);

    const std::string functionName(name);
    return m_impl->callFunction(functionName.c_str(), nullptr);
}

bool ScriptHost::call(std::string_view name, const ScriptArgs& args)
{
    PERFUI_LUA_ASSERT_THREAD(m_impl);

    const std::string functionName(name);
    return m_impl->callFunction(functionName.c_str(), &args);
}

void ScriptHost::registerModule(std::string name, std::function<void(ModuleBuilder&)> setup)
{
    PERFUI_LUA_ASSERT_THREAD(m_impl);

    Impl::RegisteredModule module{ std::move(name), std::move(setup) };
    m_impl->modules.push_back(module);
    if (m_impl->L) {
        m_impl->applyModule(module);
    }
}

void ScriptHost::bindContext(UIContext* context, UIElement* rootScope)
{
    PERFUI_LUA_ASSERT_THREAD(m_impl);
    m_impl->boundContext = context;
    m_impl->boundRoot = rootScope;
}

UIContext* ScriptHost::boundContext() const
{
    PERFUI_LUA_ASSERT_THREAD(m_impl);
    return m_impl->boundContext;
}

UIElement* ScriptHost::boundRoot() const
{
    PERFUI_LUA_ASSERT_THREAD(m_impl);
    return m_impl->boundRoot;
}

void ScriptHost::clearTimers()
{
    PERFUI_LUA_ASSERT_THREAD(m_impl);
    m_impl->clearTimers();
}

void ScriptHost::update(float dt)
{
    PERFUI_LUA_ASSERT_THREAD(m_impl);
    m_impl->updateTimers(dt);
}

ScriptStatus ScriptHost::status() const
{
    PERFUI_LUA_ASSERT_THREAD(m_impl);

    ScriptStatus result;
    result.ok = !m_impl->disabled && m_impl->L != nullptr;
    result.disabled = m_impl->disabled;
    result.lastError = m_impl->lastError;
    return result;
}

void ScriptHost::reset()
{
    PERFUI_LUA_ASSERT_THREAD(m_impl);

    m_impl->destroyState();
    m_impl->consecutiveErrors = 0;
    m_impl->disabled = false;
    m_impl->lastError.clear();
    m_impl->scriptRoot.clear();

    if (m_impl->createState()) {
        m_impl->registerAllModules();
    }
}

const std::string& ScriptHost::name() const
{
    return m_impl->name;
}

const ScriptLimits& ScriptHost::limits() const
{
    return m_impl->limits;
}

void ScriptHost::SetLogSink(ScriptLogSink sink)
{
    LogSinkStorage() = std::move(sink);
}

} // namespace PerfUI
