#include "PerfUI/Layout.h"

#include <mutex>
#include <unordered_map>

namespace PerfUI {
namespace {
struct Anchors {
    std::optional<Length> left, right, top, bottom;
    bool centerX{ false };
};
std::mutex g_anchorMutex;
std::unordered_map<const LayoutProps*, Anchors> g_anchors;
}

std::optional<float> LayoutProps::left() const {
    std::lock_guard lock(g_anchorMutex);
    auto it = g_anchors.find(this);
    if (it == g_anchors.end() || !it->second.left) return std::nullopt;
    return it->second.left->value;
}
LayoutProps& LayoutProps::left(float px) {
    return left(Length::Px(px));
}
LayoutProps& LayoutProps::left(Length len) {
    std::lock_guard lock(g_anchorMutex);
    g_anchors[this].left = len;
    return *this;
}
std::optional<Length> LayoutProps::leftLength() const {
    std::lock_guard lock(g_anchorMutex);
    auto it = g_anchors.find(this);
    return it == g_anchors.end() ? std::nullopt : it->second.left;
}

std::optional<float> LayoutProps::right() const {
    std::lock_guard lock(g_anchorMutex);
    auto it = g_anchors.find(this);
    if (it == g_anchors.end() || !it->second.right) return std::nullopt;
    return it->second.right->value;
}
LayoutProps& LayoutProps::right(float px) {
    return right(Length::Px(px));
}
LayoutProps& LayoutProps::right(Length len) {
    std::lock_guard lock(g_anchorMutex);
    g_anchors[this].right = len;
    return *this;
}
std::optional<Length> LayoutProps::rightLength() const {
    std::lock_guard lock(g_anchorMutex);
    auto it = g_anchors.find(this);
    return it == g_anchors.end() ? std::nullopt : it->second.right;
}

std::optional<float> LayoutProps::top() const {
    std::lock_guard lock(g_anchorMutex);
    auto it = g_anchors.find(this);
    if (it == g_anchors.end() || !it->second.top) return std::nullopt;
    return it->second.top->value;
}
LayoutProps& LayoutProps::top(float px) {
    return top(Length::Px(px));
}
LayoutProps& LayoutProps::top(Length len) {
    std::lock_guard lock(g_anchorMutex);
    g_anchors[this].top = len;
    return *this;
}
std::optional<Length> LayoutProps::topLength() const {
    std::lock_guard lock(g_anchorMutex);
    auto it = g_anchors.find(this);
    return it == g_anchors.end() ? std::nullopt : it->second.top;
}

std::optional<float> LayoutProps::bottom() const {
    std::lock_guard lock(g_anchorMutex);
    auto it = g_anchors.find(this);
    if (it == g_anchors.end() || !it->second.bottom) return std::nullopt;
    return it->second.bottom->value;
}
LayoutProps& LayoutProps::bottom(float px) {
    return bottom(Length::Px(px));
}
LayoutProps& LayoutProps::bottom(Length len) {
    std::lock_guard lock(g_anchorMutex);
    g_anchors[this].bottom = len;
    return *this;
}
std::optional<Length> LayoutProps::bottomLength() const {
    std::lock_guard lock(g_anchorMutex);
    auto it = g_anchors.find(this);
    return it == g_anchors.end() ? std::nullopt : it->second.bottom;
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
