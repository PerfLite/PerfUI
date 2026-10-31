#pragma once

#include <string_view>

namespace PerfUI {

/**
 * @brief Абстрактный интерфейс для внешнего разрешения колбэков разметки (например, через Lua).
 * Позволяет PerfUI_Markup вызывать скриптовые функции без прямой зависимости от Lua.
 */
class ICallbackResolver {
public:
    virtual ~ICallbackResolver() = default;

    /// Проверить наличие функции по имени
    virtual bool has(std::string_view name) const = 0;

    /// Вызов без аргументов (onClick, onFocus)
    virtual bool call(std::string_view name) = 0;

    /// Вызов с числовым аргументом (onToggle: 0/1, onValueChanged: float, onSelectionChanged: index)
    virtual bool callNumber(std::string_view name, double value) = 0;

    /// Вызов со строковым аргументом (onTextChanged, onSubmit)
    virtual bool callString(std::string_view name, std::string_view value) = 0;
};

} // namespace PerfUI
