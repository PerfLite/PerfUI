#include "PerfUI/Diagnostics.h"
#include "ModuleLog.h"

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <fstream>

namespace PerfUI {
namespace {

constexpr size_t kMaxOverlayRows = 6;
constexpr float kPanelX = 16.0f;
constexpr float kPanelY = 16.0f;
constexpr float kPanelWidth = 400.0f;
constexpr float kPanelPadding = 12.0f;
constexpr float kLineHeight = 18.0f;
constexpr float kTitleHeight = 22.0f;

void ModuleAnchor() {}

std::string CurrentTimestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char buffer[32]{};
    if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local) == 0) {
        return "?";
    }
    return buffer;
}

Color SeverityColor(float milliseconds) {
    if (milliseconds >= 40.0f) return Color(240, 120, 120, 255);
    if (milliseconds >= 20.0f) return Color(235, 200, 110, 255);
    return Color(140, 220, 150, 255);
}

} // namespace

Diagnostics& Diagnostics::GetSingleton() {
    static Diagnostics instance;
    return instance;
}

void Diagnostics::setVisible(bool visible) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_visible = visible;
}

bool Diagnostics::isVisible() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_visible;
}

void Diagnostics::toggle() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_visible = !m_visible;
}

void Diagnostics::recordFrameInterval(float frameMs) {
    if (!(frameMs > 0.0f)) return;

    const auto now = std::chrono::steady_clock::now();
    std::string line;
    std::string logPath;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_intervals[m_intervalNext] = frameMs;
        m_intervalNext = (m_intervalNext + 1) % m_intervals.size();
        if (m_intervalCount < m_intervals.size()) {
            ++m_intervalCount;
        }
        m_lastIntervalMs = frameMs;

        if (!m_hasFrameTime) {
            m_hasFrameTime = true;
            m_firstFrameTime = now;
            return;
        }
        if (now - m_firstFrameTime < std::chrono::milliseconds(static_cast<int64_t>(m_startupGraceMs))) {
            return;
        }
        if (frameMs < m_hitchThresholdMs) {
            return;
        }

        ++m_hitchCount;
        if (m_lastHitchLog.time_since_epoch().count() != 0 && now - m_lastHitchLog < std::chrono::seconds(2)) {
            return;
        }
        m_lastHitchLog = now;

        const float uiMs = m_uiTiming.updateMs + m_uiTiming.layoutMs + m_uiTiming.renderMs;
        const OverlaySample* slowest = nullptr;
        for (const auto& sample : m_overlays) {
            if (!slowest || sample.lastMs > slowest->lastMs) {
                slowest = &sample;
            }
        }

        char buffer[320]{};
        if (slowest && slowest->lastMs > 0.0f) {
            std::snprintf(buffer, sizeof(buffer), "[%s] frame %.1f ms | ui %.2f ms | overlay '%s' %.2f ms\n",
                          CurrentTimestamp().c_str(), frameMs, uiMs, slowest->name.c_str(), slowest->lastMs);
        } else {
            std::snprintf(buffer, sizeof(buffer), "[%s] frame %.1f ms | ui %.2f ms | no overlays\n",
                          CurrentTimestamp().c_str(), frameMs, uiMs);
        }
        line = buffer;
        logPath = m_logFilePath;
    }

    writeHitchLog(logPath, line);
}

void Diagnostics::recordUiFrame(const UiFrameTiming& timing) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_uiTiming = timing;
}

void Diagnostics::recordDrawStats(const DrawStats& stats) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_drawStats = stats;
}

void Diagnostics::beginOverlayFrame() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& sample : m_overlays) {
        sample.seenThisFrame = false;
    }
}

void Diagnostics::recordOverlayTime(const char* name, float milliseconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const std::string overlayName = (name && *name) ? name : "unnamed";
    for (auto& sample : m_overlays) {
        if (sample.name == overlayName) {
            sample.lastMs = milliseconds;
            sample.averageMs = sample.averageMs * 0.8f + milliseconds * 0.2f;
            sample.seenThisFrame = true;
            return;
        }
    }
    OverlaySample sample;
    sample.name = overlayName;
    sample.lastMs = milliseconds;
    sample.averageMs = milliseconds;
    sample.seenThisFrame = true;
    m_overlays.push_back(std::move(sample));
}

void Diagnostics::endOverlayFrame() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_overlays.erase(
        std::remove_if(m_overlays.begin(), m_overlays.end(), [](const OverlaySample& sample) {
            return !sample.seenThisFrame;
        }),
        m_overlays.end());
}

void Diagnostics::writeHitchLog(const std::string& path, const std::string& line) {
    if (path.empty()) {
        detail::AppendLogNextToModule(reinterpret_cast<const void*>(&ModuleAnchor), "PerfUI_perf.log", line);
        return;
    }
    std::ofstream file(path, std::ios::app);
    if (file) {
        file << line;
    }
}

void Diagnostics::render(UIRenderBackend& backend, Dimensions screenSize) {
    (void)screenSize;

    UiFrameTiming ui;
    DrawStats draw;
    std::vector<OverlaySample> overlays;
    float lastMs = 0.0f;
    float fps = 0.0f;
    float maxMs = 0.0f;
    uint64_t hitches = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_visible) return;
        ui = m_uiTiming;
        draw = m_drawStats;
        overlays = m_overlays;
        hitches = m_hitchCount;
        lastMs = m_lastIntervalMs;
        if (m_intervalCount > 0) {
            float sum = 0.0f;
            for (size_t i = 0; i < m_intervalCount; ++i) {
                sum += m_intervals[i];
                maxMs = std::max(maxMs, m_intervals[i]);
            }
            const float average = sum / static_cast<float>(m_intervalCount);
            fps = average > 0.0f ? 1000.0f / average : 0.0f;
        }
    }

    std::sort(overlays.begin(), overlays.end(), [](const OverlaySample& a, const OverlaySample& b) {
        return a.averageMs > b.averageMs;
    });
    if (overlays.size() > kMaxOverlayRows) {
        overlays.resize(kMaxOverlayRows);
    }

    const size_t overlayRows = overlays.empty() ? 1 : overlays.size();
    const float bodyLines = 5.0f + 1.0f + static_cast<float>(overlayRows);
    const float panelHeight = kPanelPadding * 2.0f + kTitleHeight + kLineHeight * bodyLines;

    const Rect panelRect{ kPanelX, kPanelY, kPanelWidth, panelHeight };

    // --- Pure Liquid Glass (No muddy outer side-shadows, softer buttery blur) ---
    // 1. Hardware Dual Kawase blur with soft translucent obsidian glass tint
    backend.drawBackdropBlur(panelRect, 10.0f, Color(14, 18, 26, 115));

    // 2. Specular gloss reflection and top light refraction lip
    backend.pushClipRect(panelRect);
    // Upper specular highlight wash
    backend.drawGradientRect(Rect{ kPanelX, kPanelY, kPanelWidth, panelHeight * 0.48f },
                             Color(255, 255, 255, 28), Color(255, 255, 255, 12),
                             Color(160, 215, 255, 2), Color(160, 215, 255, 4));
    // Top-lip sharp highlight line
    backend.drawLine(Point{ kPanelX + 10.0f, kPanelY + 1.0f },
                     Point{ kPanelX + kPanelWidth - 10.0f, kPanelY + 1.0f },
                     Color(255, 255, 255, 130), 1.0f);
    backend.popClipRect();

    // 3. Clean refractive dual glass border (crisp crystal edges)
    backend.drawRectOutline(panelRect, Color(160, 200, 245, 95), 1.0f, 10.0f);
    backend.drawRectOutline(Rect{ kPanelX + 1.0f, kPanelY + 1.0f, kPanelWidth - 2.0f, panelHeight - 2.0f },
                            Color(255, 255, 255, 22), 1.0f, 9.0f);

    float cursorY = kPanelY + kPanelPadding;
    const float textX = kPanelX + kPanelPadding;

    TextStyle titleStyle;
    titleStyle.fontSize = 15.0f;
    titleStyle.bold = true;
    titleStyle.color = Color(240, 245, 255, 255);
    backend.drawText("PerfUI diagnostics", { textX, cursorY }, titleStyle);

    TextStyle hintStyle;
    hintStyle.fontSize = 13.0f;
    hintStyle.bold = true;
    hintStyle.color = Color(155, 180, 210, 255);
    backend.drawText("F7", { kPanelX + kPanelWidth - kPanelPadding - 18.0f, cursorY }, hintStyle);
    cursorY += kTitleHeight;

    TextStyle bodyStyle;
    bodyStyle.fontSize = 13.0f;
    bodyStyle.bold = true;
    bodyStyle.color = Color(225, 235, 248, 255);

    auto drawLine = [&](const char* text, Color color, float fontSize = 13.0f) {
        TextStyle style;
        style.fontSize = fontSize;
        style.bold = true;
        style.color = color;
        backend.drawText(text, { textX, cursorY }, style);
        cursorY += kLineHeight;
    };

    char buffer[192]{};

    std::snprintf(buffer, sizeof(buffer), "FPS %.1f    frame %.1f ms    max %.1f ms", fps, lastMs, maxMs);
    drawLine(buffer, SeverityColor(lastMs));

    std::snprintf(buffer, sizeof(buffer), "UI %.2f ms    upd %.2f    lay %.2f    rnd %.2f",
                  ui.updateMs + ui.layoutMs + ui.renderMs, ui.updateMs, ui.layoutMs, ui.renderMs);
    drawLine(buffer, bodyStyle.color);

    std::snprintf(buffer, sizeof(buffer), "Draw %u calls    %u verts", draw.drawCalls, draw.vertices);
    drawLine(buffer, bodyStyle.color);

    std::snprintf(buffer, sizeof(buffer), "Tex %u    atlas %ux%u    elems %u",
                  draw.textures, draw.fontAtlasWidth, draw.fontAtlasHeight, ui.elementCount);
    drawLine(buffer, bodyStyle.color);

    if (hitches > 0) {
        std::snprintf(buffer, sizeof(buffer), "Hitches %llu    log: PerfUI_perf.log",
                      static_cast<unsigned long long>(hitches));
        drawLine(buffer, Color(235, 200, 110, 255));
    } else {
        drawLine("Hitches 0", bodyStyle.color);
    }

    std::snprintf(buffer, sizeof(buffer), "Overlays (%zu):", overlays.size());
    drawLine(buffer, Color(170, 186, 206, 255));

    if (overlays.empty()) {
        drawLine("  none rendered", Color(140, 156, 176, 255));
    } else {
        for (const auto& sample : overlays) {
            std::string name = sample.name.size() > 22 ? sample.name.substr(0, 22) : sample.name;
            std::snprintf(buffer, sizeof(buffer), "%.2f / %.2f ms", sample.lastMs, sample.averageMs);

            TextStyle valueStyle;
            valueStyle.fontSize = 13.0f;
            valueStyle.bold = true;
            valueStyle.color = SeverityColor(sample.lastMs);
            const Dimensions valueSize = backend.measureText(buffer, valueStyle);

            TextStyle nameStyle;
            nameStyle.fontSize = 13.0f;
            nameStyle.bold = true;
            nameStyle.color = Color(210, 225, 242, 255);
            backend.drawText("  " + name, { textX, cursorY }, nameStyle);
            backend.drawText(buffer, { kPanelX + kPanelWidth - kPanelPadding - valueSize.width, cursorY }, valueStyle);
            cursorY += kLineHeight;
        }
    }
}

void Diagnostics::setHitchThresholdMs(float milliseconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hitchThresholdMs = milliseconds;
}

float Diagnostics::hitchThresholdMs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_hitchThresholdMs;
}

void Diagnostics::setStartupGraceMs(float milliseconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_startupGraceMs = milliseconds;
}

void Diagnostics::setLogFilePath(std::string path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logFilePath = std::move(path);
}

std::vector<OverlayTiming> Diagnostics::overlayTimings() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<OverlayTiming> result;
    result.reserve(m_overlays.size());
    for (const auto& sample : m_overlays) {
        result.push_back(OverlayTiming{ sample.name, sample.lastMs, sample.averageMs });
    }
    return result;
}

UiFrameTiming Diagnostics::uiFrame() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_uiTiming;
}

DrawStats Diagnostics::drawStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_drawStats;
}

float Diagnostics::lastFrameMs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastIntervalMs;
}

float Diagnostics::averageFps() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_intervalCount == 0) return 0.0f;
    float sum = 0.0f;
    for (size_t i = 0; i < m_intervalCount; ++i) {
        sum += m_intervals[i];
    }
    const float average = sum / static_cast<float>(m_intervalCount);
    return average > 0.0f ? 1000.0f / average : 0.0f;
}

float Diagnostics::maxFrameMs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    float maxMs = 0.0f;
    for (size_t i = 0; i < m_intervalCount; ++i) {
        maxMs = std::max(maxMs, m_intervals[i]);
    }
    return maxMs;
}

uint64_t Diagnostics::hitchCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_hitchCount;
}

} // namespace PerfUI
