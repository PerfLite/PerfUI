/**
 * @file DiagnosticsTest.cpp
 * @brief Verification for the built-in diagnostics panel and hitch log:
 *        1. Hidden by default, visible after toggle
 *        2. UI/draw statistics round-trip
 *        3. Overlay timings appear and disappear with the rendered set
 *        4. Hitch counting, log writing and throttle
 */

#include "PerfUI/Diagnostics.h"
#include "../src/backends/mock/MockRenderBackend.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            std::cout << "  [PASS] " << msg << "\n"; \
            ++g_testsPassed; \
        } else { \
            std::cerr << "  [FAIL] " << msg << " (line " << __LINE__ << ")\n"; \
            ++g_testsFailed; \
        } \
    } while(0)

static std::string CollectText(const PerfUI::MockRenderBackend& backend) {
    std::string text;
    for (const auto& command : backend.commands()) {
        text += command.text;
        text += '\n';
    }
    return text;
}

static int CountLines(const std::string& path) {
    std::ifstream file(path);
    int lines = 0;
    std::string line;
    while (std::getline(file, line)) {
        ++lines;
    }
    return lines;
}

int main() {
    std::cout << "=========================================================\n";
    std::cout << "  PerfUI Clean Client: Diagnostics Test Suite            \n";
    std::cout << "=========================================================\n\n";

    auto& diagnostics = PerfUI::Diagnostics::GetSingleton();
    PerfUI::MockRenderBackend mockBackend;

    // -------------------------------------------------------------
    // Test 1: Hidden By Default
    // -------------------------------------------------------------
    std::cout << "Test 1: Panel Hidden By Default...\n";
    mockBackend.beginFrame();
    diagnostics.render(mockBackend, { 1920, 1080 });
    mockBackend.endFrame();
    TEST_ASSERT(mockBackend.commands().empty(), "No draw commands while the panel is off");
    TEST_ASSERT(!diagnostics.isVisible(), "Panel reports hidden");

    diagnostics.toggle();
    TEST_ASSERT(diagnostics.isVisible(), "Toggle makes the panel visible");

    mockBackend.beginFrame();
    diagnostics.render(mockBackend, { 1920, 1080 });
    mockBackend.endFrame();
    const std::string text = CollectText(mockBackend);
    TEST_ASSERT(text.find("PerfUI diagnostics") != std::string::npos, "Panel draws its title");
    TEST_ASSERT(text.find("FPS") != std::string::npos, "Panel draws FPS");
    TEST_ASSERT(text.find("F7") != std::string::npos, "Panel shows the hotkey");

    // -------------------------------------------------------------
    // Test 2: UI & Draw Statistics
    // -------------------------------------------------------------
    std::cout << "\nTest 2: UI & Draw Statistics...\n";
    diagnostics.recordUiFrame({ 0.40f, 0.10f, 0.70f, 42 });
    diagnostics.recordDrawStats({ 12, 480, 5, 512, 512 });

    const auto ui = diagnostics.uiFrame();
    TEST_ASSERT(ui.updateMs == 0.40f && ui.layoutMs == 0.10f && ui.renderMs == 0.70f, "UI timings stored");
    TEST_ASSERT(ui.elementCount == 42, "Element count stored");

    const auto draw = diagnostics.drawStats();
    TEST_ASSERT(draw.drawCalls == 12 && draw.vertices == 480, "Draw calls and vertices stored");
    TEST_ASSERT(draw.textures == 5 && draw.fontAtlasWidth == 512 && draw.fontAtlasHeight == 512, "Texture and atlas stats stored");

    // -------------------------------------------------------------
    // Test 3: Overlay Timings
    // -------------------------------------------------------------
    std::cout << "\nTest 3: Overlay Timings...\n";
    diagnostics.beginOverlayFrame();
    diagnostics.recordOverlayTime("BossBars", 1.50f);
    diagnostics.recordOverlayTime("RecentLoot", 0.25f);
    diagnostics.endOverlayFrame();

    auto overlays = diagnostics.overlayTimings();
    TEST_ASSERT(overlays.size() == 2, "Both overlays recorded");
    TEST_ASSERT(overlays[0].lastMs == 1.50f && overlays[0].name == "BossBars", "Overlay last time stored");

    diagnostics.beginOverlayFrame();
    diagnostics.recordOverlayTime("BossBars", 0.75f);
    diagnostics.endOverlayFrame();
    overlays = diagnostics.overlayTimings();
    TEST_ASSERT(overlays.size() == 1, "Overlay that stopped rendering is dropped");

    // -------------------------------------------------------------
    // Test 4: Hitch Counting & Log
    // -------------------------------------------------------------
    std::cout << "\nTest 4: Hitch Counting & Log...\n";
    const auto logPath = std::filesystem::temp_directory_path() / "perfui_diagnostics_test.log";
    std::error_code ec;
    std::filesystem::remove(logPath, ec);

    diagnostics.setLogFilePath(logPath.string());
    diagnostics.setStartupGraceMs(0.0f);
    diagnostics.setHitchThresholdMs(30.0f);

    diagnostics.recordFrameInterval(10.0f); // first sample starts the clock
    TEST_ASSERT(diagnostics.hitchCount() == 0, "Normal frame is not a hitch");

    diagnostics.recordFrameInterval(45.0f);
    TEST_ASSERT(diagnostics.hitchCount() == 1, "Slow frame counted as a hitch");
    TEST_ASSERT(diagnostics.maxFrameMs() == 45.0f, "Max frame time tracked");
    TEST_ASSERT(diagnostics.averageFps() > 0.0f, "Average FPS computed");
    TEST_ASSERT(CountLines(logPath.string()) == 1, "Hitch written to the log");

    std::ifstream logFile(logPath);
    std::string firstLine;
    std::getline(logFile, firstLine);
    logFile.close();
    TEST_ASSERT(firstLine.find("frame 45.0 ms") != std::string::npos, "Log line carries the frame time");
    TEST_ASSERT(firstLine.find("BossBars") != std::string::npos, "Log line names the slowest overlay");

    diagnostics.recordFrameInterval(60.0f);
    TEST_ASSERT(diagnostics.hitchCount() == 2, "Second hitch counted");
    TEST_ASSERT(CountLines(logPath.string()) == 1, "Log is throttled between hitches");

    diagnostics.recordFrameInterval(12.0f);
    TEST_ASSERT(diagnostics.hitchCount() == 2, "Fast frame does not inflate the hitch count");

    std::filesystem::remove(logPath, ec);
    diagnostics.setVisible(false);

    std::cout << "\n=========================================================\n";
    std::cout << "  Summary: " << g_testsPassed << " passed, " << g_testsFailed << " failed.\n";
    std::cout << "=========================================================\n";

    return g_testsFailed == 0 ? 0 : 1;
}
