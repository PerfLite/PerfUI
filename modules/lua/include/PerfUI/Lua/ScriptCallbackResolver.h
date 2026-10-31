#pragma once

#include "PerfUI/Export.h"
#include "PerfUI/Markup/ICallbackResolver.h"
#include "PerfUI/Lua/ScriptHost.h"

namespace PerfUI {

/**
 * @brief Мост между разметкой PerfUI_Markup и средой скриптов ScriptHost.
 * Позволяет декларативно связывать события элементов (onClick, onToggle, и т.д.)
 * с функциями в Lua-скрипте мода.
 */
class PERFUI_API ScriptCallbackResolver : public ICallbackResolver {
public:
    explicit ScriptCallbackResolver(ScriptHost& host);
    ~ScriptCallbackResolver() override = default;

    bool has(std::string_view name) const override;
    bool call(std::string_view name) override;
    bool callNumber(std::string_view name, double value) override;
    bool callString(std::string_view name, std::string_view value) override;

    ScriptHost& host() { return m_host; }
    const ScriptHost& host() const { return m_host; }

private:
    ScriptHost& m_host;
};

} // namespace PerfUI
