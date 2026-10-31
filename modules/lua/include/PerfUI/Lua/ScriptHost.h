#pragma once

#include "PerfUI/Export.h"
#include "PerfUI/Lua/ModuleBuilder.h"
#include "PerfUI/Lua/ScriptTypes.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace PerfUI {

// Изолированная среда исполнения Lua-скриптов одного мода.
// Каждый хост владеет собственным lua_State; общих глобалов между хостами нет.
// Все методы, кроме SetLogSink, обязаны вызываться в UI-потоке.
class PERFUI_API ScriptHost {
public:
    explicit ScriptHost(std::string name, ScriptLimits limits = {});
    ~ScriptHost();
    ScriptHost(const ScriptHost&) = delete;
    ScriptHost& operator=(const ScriptHost&) = delete;

    bool loadFile(const std::filesystem::path& file);
    bool loadString(std::string_view code, std::string_view chunkName);

    bool hasFunction(std::string_view name) const;
    bool call(std::string_view name);
    bool call(std::string_view name, const ScriptArgs& args);

    void registerModule(std::string name, std::function<void(ModuleBuilder&)> setup);

    void update(float dt);

    ScriptStatus status() const;
    void reset();

    const std::string& name() const;
    const ScriptLimits& limits() const;

    // Приёмник строк лога: в игре сюда ставится SKSE-лог, в тестах — stderr.
    static void SetLogSink(ScriptLogSink sink);

    // Внутреннее состояние модуля; определено в src/LuaInternal.h.
    struct Impl;

private:
    std::unique_ptr<Impl> m_impl;
};

} // namespace PerfUI
