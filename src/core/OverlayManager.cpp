#include "PerfUI/OverlayManager.h"
#include <algorithm>

namespace PerfUI {

OverlayId OverlayManager::registerOverlay(const char* name, OverlayCallback cb, int zOrder, bool alwaysVisible) {
    if (!cb) return 0;

    std::lock_guard<std::mutex> lock(m_mutex);
    OverlayId id = m_nextId++;
    OverlayEntry entry;
    entry.id = id;
    entry.name = name ? name : "UnnamedOverlay";
    entry.callback = std::move(cb);
    entry.zOrder = zOrder;
    entry.visible = true;
    entry.alwaysVisible = alwaysVisible;
    entry.enabled = true;

    m_overlays.push_back(std::move(entry));
    sortOverlaysLocked();
    return id;
}

void OverlayManager::unregisterOverlay(OverlayId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_overlays.erase(
        std::remove_if(m_overlays.begin(), m_overlays.end(), [id](const OverlayEntry& e) {
            return e.id == id;
        }),
        m_overlays.end()
    );
}

void OverlayManager::setOverlayVisible(OverlayId id, bool visible) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& e : m_overlays) {
        if (e.id == id) {
            e.visible = visible;
            break;
        }
    }
}

bool OverlayManager::isOverlayVisible(OverlayId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& e : m_overlays) {
        if (e.id == id) {
            return e.visible;
        }
    }
    return false;
}

bool OverlayManager::isOverlayEnabled(OverlayId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& e : m_overlays) {
        if (e.id == id) {
            return e.enabled;
        }
    }
    return false;
}

void OverlayManager::setOverlayEnabled(OverlayId id, bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& e : m_overlays) {
        if (e.id == id) {
            e.enabled = enabled;
            break;
        }
    }
}

bool OverlayManager::hasVisibleOverlays() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& e : m_overlays) {
        if (e.enabled && e.visible) {
            return true;
        }
    }
    return false;
}

void OverlayManager::renderOverlays(OverlayContext& ctx, bool isUIVisible) {
    std::vector<OverlayEntry> copy;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        copy = m_overlays;
    }

    for (const auto& e : copy) {
        if (!e.enabled || !e.visible) continue;
        if (!e.alwaysVisible && !isUIVisible) continue;

        try {
            e.callback(ctx);
        } catch (const std::exception& ex) {
            setOverlayEnabled(e.id, false);
            if (m_errorCallback) {
                m_errorCallback(e.name, ex.what());
            }
        } catch (...) {
            setOverlayEnabled(e.id, false);
            if (m_errorCallback) {
                m_errorCallback(e.name, "Unknown exception");
            }
        }
    }
}

void OverlayManager::setErrorCallback(ErrorCallback cb) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_errorCallback = std::move(cb);
}

size_t OverlayManager::overlayCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_overlays.size();
}

void OverlayManager::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_overlays.clear();
}

void OverlayManager::sortOverlaysLocked() {
    std::stable_sort(m_overlays.begin(), m_overlays.end(), [](const OverlayEntry& a, const OverlayEntry& b) {
        return a.zOrder < b.zOrder;
    });
}

} // namespace PerfUI
