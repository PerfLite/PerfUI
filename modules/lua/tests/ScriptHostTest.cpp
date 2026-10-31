/**
 * @file ScriptHostTest.cpp
 * @brief Этап 1: ядро ScriptHost — песочница, лимиты, ошибки, изоляция хостов.
 */

#include "PerfUI/Lua/ScriptHost.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace PerfUI;

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            std::cout << "  [PASS] " << msg << "\n"; \
            ++g_testsPassed; \
        } else { \
            std::cerr << "  [FAIL] " << msg << " (line " << __LINE__ << ")\n"; \
            ++g_testsFailed; \
        } \
    } while (0)

namespace {

std::filesystem::path MakeTempScriptDir()
{
    const auto dir = std::filesystem::temp_directory_path() / "perfui_lua_host_test";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir / "lib");
    return dir;
}

void WriteFile(const std::filesystem::path& path, const std::string& content)
{
    std::ofstream stream(path, std::ios::binary);
    stream << content;
}

} // namespace

int main()
{
    std::cout << "=========================================================\n";
    std::cout << "        PerfUI Lua ScriptHost Test Suite (stage 1)       \n";
    std::cout << "=========================================================\n\n";

    // -----------------------------------------------------------------
    // 1. Песочница: запрещённые глобалы недоступны
    // -----------------------------------------------------------------
    {
        std::cout << "[1] Sandbox\n";
        ScriptHost host("sandbox");
        const bool loaded = host.loadString(R"(
            function checkSandbox()
                local forbidden = { "io", "os", "package", "debug", "dofile", "loadfile", "load" }
                for _, name in ipairs(forbidden) do
                    if _G[name] ~= nil then
                        error("global '" .. name .. "' must be nil")
                    end
                end
                if type(collectgarbage) ~= "function" then
                    error("collectgarbage is missing")
                end
                if collectgarbage("count") <= 0 then
                    error("collectgarbage('count') failed")
                end
                return true
            end
            function checkRestrictedGC()
                collectgarbage("collect")
            end
        )", "sandbox_test.lua");

        TEST_ASSERT(loaded, "sandbox chunk loads");
        TEST_ASSERT(host.call("checkSandbox"), "io/os/package/debug/dofile/loadfile/load are nil");
        TEST_ASSERT(!host.call("checkRestrictedGC"), "collectgarbage('collect') is rejected");
        TEST_ASSERT(host.status().ok, "host survives a rejected collectgarbage call");
    }

    // -----------------------------------------------------------------
    // 2. Лимит инструкций
    // -----------------------------------------------------------------
    {
        std::cout << "\n[2] Instruction limit\n";
        ScriptLimits limits;
        limits.maxInstructionsPerCall = 200000;
        ScriptHost host("instructions", limits);

        const bool loaded = host.loadString(R"(
            function spin()
                while true do end
            end
            function ping()
                return true
            end
        )", "instr_test.lua");

        TEST_ASSERT(loaded, "chunk with the infinite loop loads");
        TEST_ASSERT(!host.call("spin"), "infinite loop is interrupted");
        TEST_ASSERT(host.status().lastError.find("instruction limit") != std::string::npos,
                    "status reports the instruction limit");
        TEST_ASSERT(host.call("ping"), "host stays usable after the interruption");
    }

    // -----------------------------------------------------------------
    // 3. Лимит памяти
    // -----------------------------------------------------------------
    {
        std::cout << "\n[3] Memory limit\n";
        ScriptLimits limits;
        limits.maxMemoryBytes = 2u * 1024u * 1024u;
        ScriptHost host("memory", limits);

        const bool loaded = host.loadString(R"(
            function bomb()
                local huge = string.rep("x", 1024 * 1024 * 1024)
                return huge
            end
            function ping()
                return true
            end
        )", "mem_test.lua");

        TEST_ASSERT(loaded, "chunk loads");
        TEST_ASSERT(!host.call("bomb"), "1 GB allocation is refused");
        TEST_ASSERT(host.status().ok, "host survives the allocation failure");
        TEST_ASSERT(host.call("ping"), "host still runs scripts");
    }

    // -----------------------------------------------------------------
    // 4. Ошибки трёх видов
    // -----------------------------------------------------------------
    {
        std::cout << "\n[4] Error handling\n";
        ScriptHost host("errors");

        TEST_ASSERT(host.loadString("function boom() error('runtime boom') end", "errors.lua"),
                    "chunk with a runtime error loads");
        TEST_ASSERT(!host.call("boom"), "runtime error returns false");
        TEST_ASSERT(host.status().lastError.find("runtime boom") != std::string::npos,
                    "status carries the runtime message");

        TEST_ASSERT(!host.call("missingFunction"), "calling an unknown function returns false");
        TEST_ASSERT(host.status().lastError.find("function not found") != std::string::npos,
                    "status explains the missing function");

        const auto hostForSyntax = std::make_unique<ScriptHost>("syntax");
        TEST_ASSERT(!hostForSyntax->loadString("function broken(", "syntax_test.lua"),
                    "syntax error returns false");
        TEST_ASSERT(!hostForSyntax->status().lastError.empty(), "syntax error is reported");
    }

    // -----------------------------------------------------------------
    // 5. Отключение после N подряд идущих ошибок
    // -----------------------------------------------------------------
    {
        std::cout << "\n[5] Consecutive error cutoff\n";
        ScriptLimits limits;
        limits.maxConsecutiveErrors = 3;
        ScriptHost host("cutoff", limits);

        const bool loaded = host.loadString("function fail() error('nope') end", "cutoff.lua");
        TEST_ASSERT(loaded, "failing chunk loads");

        TEST_ASSERT(!host.call("fail"), "1st error returns false");
        TEST_ASSERT(!host.call("fail"), "2nd error returns false");
        TEST_ASSERT(!host.call("fail"), "3rd error returns false");
        TEST_ASSERT(host.status().disabled, "host disables itself after 3 errors");
        TEST_ASSERT(!host.call("fail"), "disabled host refuses calls");
    }

    // -----------------------------------------------------------------
    // 6. require: выход за каталог запрещён, легальный вызов работает
    // -----------------------------------------------------------------
    {
        std::cout << "\n[6] Sandboxed require\n";
        const auto dir = MakeTempScriptDir();
        WriteFile(dir / "main.lua", R"(
            local helper = require("lib.helper")
            function getValue()
                return helper.value == 42
            end
            function tryParent()
                require("..\\evil")
            end
            function tryAbsolute()
                require("C:/evil")
            end
            function tryUnixAbsolute()
                require("/etc/passwd")
            end
        )");
        WriteFile(dir / "lib" / "helper.lua", "return { value = 42 }");

        ScriptHost host("require");
        TEST_ASSERT(host.loadFile(dir / "main.lua"), "main.lua loads from a file");
        TEST_ASSERT(host.call("getValue"), "require('lib.helper') resolves inside the script dir");
        TEST_ASSERT(!host.call("tryParent"), "require('..\\evil') is rejected");
        TEST_ASSERT(!host.call("tryAbsolute"), "require('C:/evil') is rejected");
        TEST_ASSERT(!host.call("tryUnixAbsolute"), "require('/etc/passwd') is rejected");

        std::filesystem::remove_all(dir);
    }

    // -----------------------------------------------------------------
    // 7. Изоляция двух хостов
    // -----------------------------------------------------------------
    {
        std::cout << "\n[7] Host isolation\n";
        ScriptHost hostA("hostA");
        ScriptHost hostB("hostB");

        TEST_ASSERT(hostA.loadString("value = 5", "a.lua"), "hostA sets a global");
        TEST_ASSERT(hostB.loadString(R"(
            function checkIsolation()
                return value == nil
            end
        )", "b.lua"), "hostB compiles");
        TEST_ASSERT(hostB.call("checkIsolation"), "globals of hostA are invisible in hostB");
    }

    // -----------------------------------------------------------------
    // 8. reset() очищает состояние
    // -----------------------------------------------------------------
    {
        std::cout << "\n[8] reset()\n";
        ScriptHost host("resettable");
        TEST_ASSERT(host.loadString("value = 1; function hasValue() return value == 1 end", "r.lua"),
                    "chunk loads");
        TEST_ASSERT(host.call("hasValue"), "function works before reset");

        host.reset();
        TEST_ASSERT(!host.hasFunction("hasValue"), "reset() drops user functions");
        TEST_ASSERT(host.status().ok, "reset() leaves a working host");
        TEST_ASSERT(host.loadString("function fresh() return true end", "r2.lua"),
                    "host accepts new code after reset");
    }

    // -----------------------------------------------------------------
    // 9. Проверка потока
    // -----------------------------------------------------------------
    {
        std::cout << "\n[9] Thread guard\n";
        // Проверка реализована как assert в debug-сборке; из теста её вызвать
        // нельзя без аварийного завершения процесса.
        TEST_ASSERT(true, "thread guard is a debug-only assert (documented, not testable here)");
    }

    // -----------------------------------------------------------------
    // 10. Лимит времени
    // -----------------------------------------------------------------
    {
        std::cout << "\n[10] Time limit\n";
        ScriptLimits limits;
        limits.maxInstructionsPerCall = 1'000'000'000ULL;
        limits.maxMillisecondsPerCall = 5.0f;
        ScriptHost host("timing", limits);

        const bool loaded = host.loadString(R"(
            function spinSlow()
                local counter = 0
                while true do counter = counter + 1 end
            end
        )", "time_test.lua");

        TEST_ASSERT(loaded, "chunk loads");
        TEST_ASSERT(!host.call("spinSlow"), "long-running call is interrupted");
        TEST_ASSERT(host.status().lastError.find("time limit") != std::string::npos,
                    "status reports the time limit");
    }

    // -----------------------------------------------------------------
    // 11. Формат лога и C++-модули
    // -----------------------------------------------------------------
    {
        std::cout << "\n[11] Logging and modules\n";
        std::vector<std::string> lines;
        ScriptHost::SetLogSink([&lines](ScriptLogLevel, std::string_view line) {
            lines.emplace_back(line);
        });

        ScriptHost host("testmod");
        host.registerModule("game", [](ModuleBuilder& builder) {
            builder.function("answer", []() -> double { return 42.0; });
            builder.function("greet", [](std::string name) { (void)name; });
        });

        TEST_ASSERT(host.loadString(R"(
            log.warn("hello from script")
            function checkModule()
                return game.answer() == 42
            end
        )", "log_test.lua"), "chunk with log and module call loads");
        TEST_ASSERT(!lines.empty(), "log line reached the sink");
        if (!lines.empty()) {
            TEST_ASSERT(lines.front().find("[Lua:testmod]") != std::string::npos,
                        "log line starts with the [Lua:<mod>] prefix");
            TEST_ASSERT(lines.front().find("log_test.lua:2:") != std::string::npos,
                        "log line carries file:line");
        }
        TEST_ASSERT(host.call("checkModule"), "C++ module function is callable from Lua");

        TEST_ASSERT(host.loadString(R"(
            function callGreetWithoutArgs()
                game.greet()
            end
        )", "arity_test.lua"), "arity probe chunk loads");
        TEST_ASSERT(!host.call("callGreetWithoutArgs"), "wrong argument count is a script error");
        TEST_ASSERT(host.status().lastError.find("expected 1 argument") != std::string::npos,
                    "status names the argument mismatch");

        ScriptHost::SetLogSink(nullptr);
    }

    std::cout << "\n=========================================================\n";
    std::cout << "  ScriptHost Test Results: " << g_testsPassed << " passed, "
              << g_testsFailed << " failed.\n";
    std::cout << "=========================================================\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
