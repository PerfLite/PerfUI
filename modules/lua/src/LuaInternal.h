#pragma once

#include "PerfUI/Lua/ModuleBuilder.h"
#include "PerfUI/Lua/ScriptHost.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

extern "C" {
struct lua_State;
}

namespace PerfUI {

class UIContext;
class UIElement;

// Разделяемые внутренние детали модуля. Публичным заголовкам lua_State не виден.
struct ModuleSink {
    lua_State* L{ nullptr };
    int tableRef{ -2 };  // LUA_NOREF
    std::string moduleName;
};

struct ScriptAllocState {
    std::size_t used{ 0 };
    std::size_t limit{ 0 };
};

struct ScriptHost::Impl {
    struct RegisteredModule {
        std::string name;
        std::function<void(ModuleBuilder&)> setup;
    };

    std::string name;
    ScriptLimits limits;
    std::filesystem::path scriptRoot;
    std::vector<RegisteredModule> modules;

    lua_State* L{ nullptr };
    ScriptAllocState alloc;

    std::uint64_t instructionBudget{ 0 };
    std::chrono::steady_clock::time_point deadline{};

    int consecutiveErrors{ 0 };
    bool disabled{ false };
    std::string lastError;

    std::thread::id ownerThread;

    UIContext* boundContext{ nullptr };
    UIElement* boundRoot{ nullptr };

    struct TimerEntry {
        uint32_t id{ 0 };
        float interval{ 0.0f };
        float remaining{ 0.0f };
        bool repeating{ false };
        int funcRef{ -2 };
    };
    std::vector<TimerEntry> timers;
    uint32_t nextTimerId{ 1 };

    uint32_t addTimer(float delay, int funcRef, bool repeating);
    bool cancelTimer(uint32_t id);
    void clearTimers();
    void updateTimers(float dt);

    UIElement* findElement(std::string_view name) const;
    void pushElementHandle(UIElement* element);
    void registerUiLibrary();

    bool createState();
    void destroyState();
    void registerAllModules();
    void applyModule(const RegisteredModule& module);

    bool protectedCall(int nargs, int nresults);
    bool callFunction(const char* functionName, const ScriptArgs* args);
    void reportError(std::string message);
    void resetErrorStreak();

    bool isSafeModuleName(std::string_view moduleName) const;
    void log(ScriptLogLevel level, const std::string& body) const;

    static Impl* FromState(lua_State* state);
};

} // namespace PerfUI
