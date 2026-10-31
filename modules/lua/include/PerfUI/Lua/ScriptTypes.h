#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace PerfUI {

// Значение, которым обмениваются скрипт и C++: nil, boolean, число или строка.
using ScriptValue = std::variant<std::monostate, bool, double, std::string>;

using ScriptArgs = std::vector<ScriptValue>;

struct ScriptLimits {
    std::uint64_t maxInstructionsPerCall{ 1'000'000 };
    float maxMillisecondsPerCall{ 50.0f };
    std::size_t maxMemoryBytes{ 16u * 1024u * 1024u };
    int maxConsecutiveErrors{ 5 };
};

struct ScriptStatus {
    bool ok{ true };
    bool disabled{ false };
    std::string lastError;
};

enum class ScriptLogLevel {
    Info,
    Warn,
    Error
};

// Строка приходит уже с префиксом "[Lua:<mod>] файл:строка:".
using ScriptLogSink = std::function<void(ScriptLogLevel, std::string_view)>;

} // namespace PerfUI
