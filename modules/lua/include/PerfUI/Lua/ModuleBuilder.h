#pragma once

#include "PerfUI/Export.h"
#include "PerfUI/Lua/ScriptTypes.h"

#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace PerfUI {

namespace detail {

class ScriptArgumentError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

template <typename Fn>
struct ScriptFunctionTraits : ScriptFunctionTraits<decltype(&Fn::operator())> {};

template <typename C, typename R, typename... A>
struct ScriptFunctionTraits<R (C::*)(A...)> {
    using ReturnType = R;
    using ArgsTuple = std::tuple<A...>;
    static constexpr std::size_t kArity = sizeof...(A);
};

template <typename C, typename R, typename... A>
struct ScriptFunctionTraits<R (C::*)(A...) const> {
    using ReturnType = R;
    using ArgsTuple = std::tuple<A...>;
    static constexpr std::size_t kArity = sizeof...(A);
};

template <typename R, typename... A>
struct ScriptFunctionTraits<R (*)(A...)> {
    using ReturnType = R;
    using ArgsTuple = std::tuple<A...>;
    static constexpr std::size_t kArity = sizeof...(A);
};

template <typename R, typename... A>
struct ScriptFunctionTraits<std::function<R(A...)>> {
    using ReturnType = R;
    using ArgsTuple = std::tuple<A...>;
    static constexpr std::size_t kArity = sizeof...(A);
};

template <typename T>
T ScriptArgFromValue(const ScriptValue& a_value)
{
    if constexpr (std::is_same_v<T, std::string>) {
        if (const auto* s = std::get_if<std::string>(&a_value)) {
            return *s;
        }
        throw ScriptArgumentError("expected string");
    } else if constexpr (std::is_same_v<T, std::string_view>) {
        if (const auto* s = std::get_if<std::string>(&a_value)) {
            return std::string_view(*s);
        }
        throw ScriptArgumentError("expected string");
    } else if constexpr (std::is_same_v<T, bool>) {
        if (const auto* b = std::get_if<bool>(&a_value)) {
            return *b;
        }
        throw ScriptArgumentError("expected boolean");
    } else if constexpr (std::is_arithmetic_v<T>) {
        if (const auto* d = std::get_if<double>(&a_value)) {
            return static_cast<T>(*d);
        }
        throw ScriptArgumentError("expected number");
    } else {
        static_assert(!sizeof(T), "unsupported script function argument type");
    }
}

template <typename T>
ScriptValue ScriptValueFromResult(T&& a_result)
{
    using Result = std::decay_t<T>;
    if constexpr (std::is_same_v<Result, std::string>) {
        return ScriptValue{ std::move(a_result) };
    } else if constexpr (std::is_same_v<Result, const char*>) {
        return ScriptValue{ std::string(a_result ? a_result : "") };
    } else if constexpr (std::is_same_v<Result, bool>) {
        return ScriptValue{ a_result };
    } else if constexpr (std::is_arithmetic_v<Result>) {
        return ScriptValue{ static_cast<double>(a_result) };
    } else {
        static_assert(!sizeof(Result), "unsupported script function return type");
    }
}

template <typename Fn, std::size_t... I>
ScriptValue InvokeScriptFunction(Fn& a_fn, ScriptArgs& a_args, std::index_sequence<I...>)
{
    using Traits = ScriptFunctionTraits<Fn>;
    if (a_args.size() != Traits::kArity) {
        throw ScriptArgumentError("expected " + std::to_string(Traits::kArity) + " argument(s), got " +
                                  std::to_string(a_args.size()));
    }

    if constexpr (std::is_void_v<typename Traits::ReturnType>) {
        a_fn(ScriptArgFromValue<std::tuple_element_t<I, typename Traits::ArgsTuple>>(a_args[I])...);
        return ScriptValue{};
    } else {
        return ScriptValueFromResult(
            a_fn(ScriptArgFromValue<std::tuple_element_t<I, typename Traits::ArgsTuple>>(a_args[I])...));
    }
}

template <typename Fn>
std::function<ScriptValue(ScriptArgs&)> MakeScriptInvoker(Fn a_fn)
{
    return [fn = std::move(a_fn)](ScriptArgs& a_args) mutable -> ScriptValue {
        using Traits = ScriptFunctionTraits<Fn>;
        return InvokeScriptFunction(fn, a_args, std::make_index_sequence<Traits::kArity>{});
    };
}

} // namespace detail

// Собирает таблицу C++-функций, доступную скрипту как модуль.
// Экземпляр живёт только внутри ScriptHost::registerModule().
class PERFUI_API ModuleBuilder {
public:
    ~ModuleBuilder();

    ModuleBuilder(const ModuleBuilder&) = delete;
    ModuleBuilder& operator=(const ModuleBuilder&) = delete;

    template <typename Fn>
    ModuleBuilder& function(std::string a_name, Fn a_fn)
    {
        addFunction(std::move(a_name), detail::MakeScriptInvoker(std::move(a_fn)));
        return *this;
    }

private:
    friend class ScriptHost;

    explicit ModuleBuilder(void* a_sink) : m_sink(a_sink) {}

    void addFunction(std::string a_name, std::function<ScriptValue(ScriptArgs&)> a_invoker);

    void* m_sink{ nullptr };
};

} // namespace PerfUI
