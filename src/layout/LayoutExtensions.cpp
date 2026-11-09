#include "PerfUI/Layout.h"

#include <mutex>
#include <unordered_map>

namespace PerfUI {
namespace {
struct Anchors {
    std::optional<float> left, right, top, bottom;
    bool centerX{ false };
};
std::mutex g_anchorMutex;
std::unordered_map<const LayoutProps*, Anchors> g_anchors;
}

std::optional<float> LayoutProps::left() const {
    std::lock_guard lock(g_anchorMutex);
    auto it = g_anchors.find(this);
    return it == g_anchors.end() ? std::nullopt : it->second.left;
}
LayoutProps& LayoutProps::left(float px) {
    std::lock_guard lock(g_anchorMutex);
    g_anchors[this].left = px;
    return *this;
}
std::optional<float> LayoutProps::right() const {
    std::lock_guard lock(g_anchorMutex);
    auto it = g_anchors.find(this);
    return it == g_anchors.end() ? std::nullopt : it->second.right;
}
LayoutProps& LayoutProps::right(float px) {
    std::lock_guard lock(g_anchorMutex);
    g_anchors[this].right = px;
    return *this;
}
std::optional<float> LayoutProps::top() const {
    std::lock_guard lock(g_anchorMutex);
    auto it = g_anchors.find(this);
    return it == g_anchors.end() ? std::nullopt : it->second.top;
}
LayoutProps& LayoutProps::top(float px) {
    std::lock_guard lock(g_anchorMutex);
    g_anchors[this].top = px;
    return *this;
}
std::optional<float> LayoutProps::bottom() const {
    std::lock_guard lock(g_anchorMutex);
    auto it = g_anchors.find(this);
    return it == g_anchors.end() ? std::nullopt : it->second.bottom;
}
LayoutProps& LayoutProps::bottom(float px) {
    std::lock_guard lock(g_anchorMutex);
    g_anchors[this].bottom = px;
    return *this;
}
bool LayoutProps::centerX() const {
    std::lock_guard lock(g_anchorMutex);
    auto it = g_anchors.find(this);
    return it != g_anchors.end() && it->second.centerX;
}
LayoutProps& LayoutProps::centerX(bool enabled) {
    std::lock_guard lock(g_anchorMutex);
    g_anchors[this].centerX = enabled;
    return *this;
}
bool LayoutProps::hasAbsoluteAnchors() const {
    std::lock_guard lock(g_anchorMutex);
    auto it = g_anchors.find(this);
    if (it == g_anchors.end()) return false;
    const auto& a = it->second;
    return a.left || a.right || a.top || a.bottom || a.centerX;
}
void LayoutProps::clearAbsoluteAnchors() {
    std::lock_guard lock(g_anchorMutex);
    g_anchors.erase(this);
}
} // namespace PerfUI
