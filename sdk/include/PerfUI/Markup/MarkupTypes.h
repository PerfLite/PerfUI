#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>
#include "PerfUI/Types.h"

namespace PerfUI {

struct LoadResult {
    bool success{ true };
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

using AttrMap = std::unordered_map<std::string, std::string>;
using CallbackMap = std::unordered_map<std::string, std::function<void()>>;

// Stub for Stage 3 Binding system
class BindingRegistry {
public:
    static BindingRegistry& instance() {
        static BindingRegistry s_instance;
        return s_instance;
    }

    void registerBinding(std::string_view path, std::function<void(const std::string&)> setter) {
        // Reserved for Phase 3
        (void)path;
        (void)setter;
    }
};

} // namespace PerfUI
