#pragma once

#include "Overlay.h"
#include "Export.h"
#include <vector>
#include <mutex>
#include <functional>
#include <string>

namespace PerfUI {

class PERFUI_API OverlayManager {
public:
    using ErrorCallback = std::function<void(const std::string& overlayName, const std::string& error)>;

    OverlayManager() = default;
    ~OverlayManager() = default;

    OverlayId registerOverlay(const char* name, OverlayCallback cb, int zOrder = 0, bool alwaysVisible = true);
    void unregisterOverlay(OverlayId id);
    void setOverlayVisible(OverlayId id, bool visible);
    bool isOverlayVisible(OverlayId id) const;

    bool isOverlayEnabled(OverlayId id) const;
    void setOverlayEnabled(OverlayId id, bool enabled);

    bool hasVisibleOverlays() const;
    void renderOverlays(OverlayContext& ctx, bool isUIVisible);

    void setErrorCallback(ErrorCallback cb);
    size_t overlayCount() const;
    void clear();

private:
    void sortOverlaysLocked();

    mutable std::mutex m_mutex;
    OverlayId m_nextId{ 1 };
    std::vector<OverlayEntry> m_overlays;
    ErrorCallback m_errorCallback;
};

} // namespace PerfUI
