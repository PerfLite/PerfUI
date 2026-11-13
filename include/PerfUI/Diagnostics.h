#pragma once

#include "Export.h"
#include "Types.h"
#include "UIRenderBackend.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace PerfUI {

class UIContext;
class MarkupLoader;
class UIElement;

struct OverlayTiming {
    std::string name;
    float lastMs{ 0.0f };
    float averageMs{ 0.0f };
};

struct UiFrameTiming {
    float updateMs{ 0.0f };
    float layoutMs{ 0.0f };
    float renderMs{ 0.0f };
    uint32_t elementCount{ 0 };
};

struct DrawStats {
    uint32_t drawCalls{ 0 };
    uint32_t vertices{ 0 };
    uint32_t textures{ 0 };
    uint32_t fontAtlasWidth{ 0 };
    uint32_t fontAtlasHeight{ 0 };
};

// Built-in performance panel and hitch log.
//
// Collection is always on and cheap (one clock read per overlay per frame);
// the panel is toggled with F7 and the hitch log only writes when a frame
// exceeds the threshold, so a normal game session produces no output.
class PERFUI_API Diagnostics {
public:
    static Diagnostics& GetSingleton();

    Diagnostics(const Diagnostics&) = delete;
    Diagnostics& operator=(const Diagnostics&) = delete;

    void setVisible(bool visible);
    bool isVisible() const;
    void toggle();
    void setSuppressed(bool suppressed) { m_suppressed = suppressed; }
    bool isSuppressed() const { return m_suppressed; }

    // Engine feed: Present-to-Present interval of the render thread.
    void recordFrameInterval(float frameMs);
    // Engine feed: cost of the last UI update/layout/render pass.
    void recordUiFrame(const UiFrameTiming& timing);
    // Engine feed: draw statistics of the last submitted frame.
    void recordDrawStats(const DrawStats& stats);

    // Overlay feed, maintained by OverlayManager.
    void beginOverlayFrame();
    void recordOverlayTime(const char* name, float milliseconds);
    void endOverlayFrame();

    // Draws the panel when visible. Call once per rendered frame, after overlays.
    void render(UIRenderBackend& backend, Dimensions screenSize, UIContext* context = nullptr);

    uint32_t toggleHotkey() const { return m_toggleHotkey; }
    bool enableGlassBlur() const { return m_enableGlassBlur; }
    float blurScale() const { return m_blurScale; }

    void setHitchThresholdMs(float milliseconds);
    float hitchThresholdMs() const;
    // Startup frames (loading, first shader compiles) are not counted as hitches.
    void setStartupGraceMs(float milliseconds);
    // Empty path means "PerfUI_perf.log next to PerfUI.dll".
    void setLogFilePath(std::string path);

    std::vector<OverlayTiming> overlayTimings() const;
    UiFrameTiming uiFrame() const;
    DrawStats drawStats() const;
    float lastFrameMs() const;
    float averageFps() const;
    float maxFrameMs() const;
    uint64_t hitchCount() const;

private:
    Diagnostics();
    ~Diagnostics();

    struct OverlaySample {
        std::string name;
        float lastMs{ 0.0f };
        float averageMs{ 0.0f };
        bool seenThisFrame{ false };
    };

    void writeHitchLog(const std::string& path, const std::string& line);

    mutable std::mutex m_mutex;
    bool m_visible{ false };
    bool m_suppressed{ false };
    float m_hitchThresholdMs{ 50.0f };
    float m_startupGraceMs{ 10000.0f };
    uint64_t m_hitchCount{ 0 };
    std::array<float, 120> m_intervals{};
    size_t m_intervalNext{ 0 };
    size_t m_intervalCount{ 0 };
    float m_lastIntervalMs{ 0.0f };
    UiFrameTiming m_uiTiming;
    DrawStats m_drawStats;
    std::vector<OverlaySample> m_overlays;
    bool m_hasFrameTime{ false };
    std::chrono::steady_clock::time_point m_firstFrameTime{};
    std::chrono::steady_clock::time_point m_lastHitchLog{};
    std::string m_logFilePath;

    void loadIni(const std::string& baseDir);
    void ensureMarkup(UIContext* context);

    std::unique_ptr<UIElement> m_isolatedRoot;
    std::unique_ptr<MarkupLoader> m_markupLoader;
    bool m_markupAttempted{ false };
    bool m_enableGlassBlur{ true };
    float m_blurScale{ 1.0f };
    uint32_t m_toggleHotkey{ 0x76 };
    std::string m_templatePath{ "Interface/PerfUI/diagnostics.xml" };
};

} // namespace PerfUI
