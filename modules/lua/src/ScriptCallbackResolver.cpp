#include "PerfUI/Lua/ScriptCallbackResolver.h"

namespace PerfUI {

ScriptCallbackResolver::ScriptCallbackResolver(ScriptHost& host)
    : m_host(host)
{
}

bool ScriptCallbackResolver::has(std::string_view name) const
{
    return m_host.hasFunction(name);
}

bool ScriptCallbackResolver::call(std::string_view name)
{
    return m_host.call(name);
}

bool ScriptCallbackResolver::callNumber(std::string_view name, double value)
{
    ScriptArgs args{ value };
    return m_host.call(name, args);
}

bool ScriptCallbackResolver::callString(std::string_view name, std::string_view value)
{
    ScriptArgs args{ std::string(value) };
    return m_host.call(name, args);
}

} // namespace PerfUI
