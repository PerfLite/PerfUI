#include "PerfUI/Lua/ScriptHost.h"

#include "LuaInternal.h"

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
    return true;
}

void ScriptHost::Impl::destroyState()
{
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

    lua_rawgeti(L, LUA_REGISTRYINDEX, tableRef);
    lua_setglobal(L, module.name.c_str());
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

void ScriptHost::update(float dt)
{
    PERFUI_LUA_ASSERT_THREAD(m_impl);

    // Таймеры (ui.timer/interval) появятся вместе с API интерфейса на Этапе 2.
    (void)dt;
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
