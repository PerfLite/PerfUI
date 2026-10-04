/**
 * @file OverlayTest.cpp
 * @brief Clean Client Stage 1 Verification Tests:
 *        1. All 12 new primitives on MockRenderBackend
 *        2. OverlayManager registration, z-ordering, visibility, exception safety, unregistering
 */

#include "PerfUI/PerfUI.h"
#include "PerfUI/OverlayManager.h"
#include "../src/backends/mock/MockRenderBackend.h"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>

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

int main() {
    std::cout << "=========================================================\n";
    std::cout << "  PerfUI Clean Client: Overlay & Primitives Test Suite   \n";
    std::cout << "=========================================================\n\n";

    // -------------------------------------------------------------
    // Test 1: New Primitives on MockRenderBackend
    // -------------------------------------------------------------
    std::cout << "Test 1: Verification of 12 New Render Primitives...\n";
    PerfUI::MockRenderBackend mockBackend;
    mockBackend.beginFrame();

    mockBackend.drawLine({ 10.0f, 10.0f }, { 100.0f, 10.0f }, PerfUI::Color::White(), 2.0f);
    
    std::vector<PerfUI::Point> polyPts = { { 0, 0 }, { 50, 20 }, { 100, 0 } };
    mockBackend.drawPolyline(polyPts, PerfUI::Color(255, 0, 0), 1.5f, false);

    mockBackend.drawCircle({ 200.0f, 200.0f }, 30.0f, PerfUI::Color(0, 0, 255), PerfUI::Color::White(), 1.0f);
    mockBackend.drawArc({ 200.0f, 200.0f }, 25.0f, 0.0f, 3.14159f, PerfUI::Color(255, 255, 0), 2.0f);
    mockBackend.drawTriangle({ 10, 10 }, { 50, 10 }, { 30, 40 }, PerfUI::Color(0, 255, 0));
    mockBackend.drawQuad({ 0, 0 }, { 20, 5 }, { 20, 25 }, { 0, 20 }, PerfUI::Color(0, 255, 255));

    std::vector<PerfUI::Point> polygonPts = { { 0, 0 }, { 30, 0 }, { 40, 20 }, { 15, 35 }, { 0, 20 } };
    mockBackend.drawPolygon(polygonPts, PerfUI::Color(255, 0, 255));

    mockBackend.drawRectOutline(PerfUI::Rect{ 100, 100, 200, 50 }, PerfUI::Color::White(), 1.5f, 4.0f);
    mockBackend.drawImageRotated(101, { 300, 300 }, { 64, 64 }, 0.785f, PerfUI::Color::White());
    mockBackend.drawImageUV(102, PerfUI::Rect{ 0, 0, 100, 100 }, { 0.2f, 0.2f }, { 0.8f, 0.8f });
    mockBackend.drawImageQuad(103, { 0, 0 }, { 50, 0 }, { 60, 50 }, { 10, 50 }, { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 });
    mockBackend.drawGradientRect(PerfUI::Rect{ 0, 0, 100, 20 }, PerfUI::Color(255, 0, 0), PerfUI::Color(0, 255, 0), PerfUI::Color(0, 0, 255), PerfUI::Color::White());

    mockBackend.endFrame();

    TEST_ASSERT(mockBackend.drawCallCount() == 12, "All 12 primitives recorded by MockRenderBackend");
    const auto& cmds = mockBackend.commands();
    TEST_ASSERT(cmds.size() == 12, "Command list has 12 entries");
    TEST_ASSERT(cmds[0].type == PerfUI::MockRenderBackend::DrawCommand::Type::Line, "Command 0 is Line");
    TEST_ASSERT(cmds[1].type == PerfUI::MockRenderBackend::DrawCommand::Type::Polyline, "Command 1 is Polyline");
    TEST_ASSERT(cmds[2].type == PerfUI::MockRenderBackend::DrawCommand::Type::Circle, "Command 2 is Circle");
    TEST_ASSERT(cmds[3].type == PerfUI::MockRenderBackend::DrawCommand::Type::Arc, "Command 3 is Arc");
    TEST_ASSERT(cmds[4].type == PerfUI::MockRenderBackend::DrawCommand::Type::Triangle, "Command 4 is Triangle");
    TEST_ASSERT(cmds[5].type == PerfUI::MockRenderBackend::DrawCommand::Type::Quad, "Command 5 is Quad");
    TEST_ASSERT(cmds[6].type == PerfUI::MockRenderBackend::DrawCommand::Type::Polygon, "Command 6 is Polygon");
    TEST_ASSERT(cmds[7].type == PerfUI::MockRenderBackend::DrawCommand::Type::RectOutline, "Command 7 is RectOutline");
    TEST_ASSERT(cmds[8].type == PerfUI::MockRenderBackend::DrawCommand::Type::ImageRotated, "Command 8 is ImageRotated");
    TEST_ASSERT(cmds[9].type == PerfUI::MockRenderBackend::DrawCommand::Type::ImageUV, "Command 9 is ImageUV");
    TEST_ASSERT(cmds[10].type == PerfUI::MockRenderBackend::DrawCommand::Type::ImageQuad, "Command 10 is ImageQuad");
    TEST_ASSERT(cmds[11].type == PerfUI::MockRenderBackend::DrawCommand::Type::GradientRect, "Command 11 is GradientRect");

    // -------------------------------------------------------------
    // Test 2: OverlayManager Z-Ordering
    // -------------------------------------------------------------
    std::cout << "\nTest 2: Overlay Z-Ordering Execution Order...\n";
    PerfUI::OverlayManager mgr;
    std::vector<int> callOrder;

    mgr.registerOverlay("OverlayMiddle", [&](PerfUI::OverlayContext&) {
        callOrder.push_back(0);
    }, /*zOrder=*/0);

    mgr.registerOverlay("OverlayBack", [&](PerfUI::OverlayContext&) {
        callOrder.push_back(-10);
    }, /*zOrder=*/-10);

    mgr.registerOverlay("OverlayFront", [&](PerfUI::OverlayContext&) {
        callOrder.push_back(50);
    }, /*zOrder=*/50);

    PerfUI::OverlayContext ctx{ mockBackend, { 1920, 1080 }, 1.0f, 0.016f };
    mgr.renderOverlays(ctx, true);

    TEST_ASSERT(callOrder.size() == 3, "All 3 overlays executed");
    TEST_ASSERT(callOrder[0] == -10, "First is zOrder -10");
    TEST_ASSERT(callOrder[1] == 0, "Second is zOrder 0");
    TEST_ASSERT(callOrder[2] == 50, "Third is zOrder 50");

    // -------------------------------------------------------------
    // Test 3: Overlay Visibility & alwaysVisible Flag
    // -------------------------------------------------------------
    std::cout << "\nTest 3: Overlay Visibility & alwaysVisible Filter...\n";
    mgr.clear();
    bool hudRan = false;
    bool menuOnlyRan = false;

    auto hudId = mgr.registerOverlay("HUDCompass", [&](PerfUI::OverlayContext&) {
        hudRan = true;
    }, 0, /*alwaysVisible=*/true);

    mgr.registerOverlay("MenuStats", [&](PerfUI::OverlayContext&) {
        menuOnlyRan = true;
    }, 0, /*alwaysVisible=*/false);

    // Render in HUD mode (UI is not visible)
    hudRan = false;
    menuOnlyRan = false;
    mgr.renderOverlays(ctx, /*isUIVisible=*/false);
    TEST_ASSERT(hudRan == true, "HUD overlay executed when isUIVisible=false");
    TEST_ASSERT(menuOnlyRan == false, "Menu overlay was skipped when isUIVisible=false");

    // Now hide HUD overlay explicitly
    mgr.setOverlayVisible(hudId, false);
    TEST_ASSERT(!mgr.isOverlayVisible(hudId), "HUD overlay marked hidden");
    hudRan = false;
    mgr.renderOverlays(ctx, /*isUIVisible=*/false);
    TEST_ASSERT(hudRan == false, "Hidden HUD overlay was not executed");

    // -------------------------------------------------------------
    // Test 4: Exception Isolation (Faulty Overlay Disabled Without Crash)
    // -------------------------------------------------------------
    std::cout << "\nTest 4: Exception Handling & Fault Isolation...\n";
    std::string loggedError;
    mgr.setErrorCallback([&](const std::string& name, const std::string& err) {
        loggedError = name + ": " + err;
    });

    auto faultyId = mgr.registerOverlay("BuggyOverlay", [](PerfUI::OverlayContext&) {
        throw std::runtime_error("Simulated crash in mod overlay!");
    }, 10, true);

    bool crashCaught = false;
    try {
        mgr.renderOverlays(ctx, true);
        crashCaught = true;
    } catch (...) {
        crashCaught = false;
    }

    TEST_ASSERT(crashCaught == true, "Exception was safely intercepted without rethrowing");
    TEST_ASSERT(!mgr.isOverlayEnabled(faultyId), "Faulty overlay was automatically disabled");
    TEST_ASSERT(loggedError.find("BuggyOverlay") != std::string::npos, "Error callback received overlay name");

    // -------------------------------------------------------------
    // Test 5: Unregister Overlay
    // -------------------------------------------------------------
    std::cout << "\nTest 5: Unregister Overlay...\n";
    auto tempId = mgr.registerOverlay("Temp", [](PerfUI::OverlayContext&) {}, 0, true);
    size_t countBefore = mgr.overlayCount();
    mgr.unregisterOverlay(tempId);
    TEST_ASSERT(mgr.overlayCount() == countBefore - 1, "Unregister reduced overlay count");

    // -------------------------------------------------------------
    // Test 6: UIContext Integration
    // -------------------------------------------------------------
    std::cout << "\nTest 6: UIContext Integration...\n";
    PerfUI::UIContext uiCtx;
    bool uiOverlayCalled = false;
    uiCtx.registerOverlay("ContextOverlay", [&](PerfUI::OverlayContext& c) {
        c.renderer.drawCircle({ 50, 50 }, 10, PerfUI::Color::White());
        uiOverlayCalled = true;
    });

    mockBackend.beginFrame();
    uiCtx.update(0.016f);
    uiCtx.render(mockBackend, /*renderTree=*/false); // HUD mode
    TEST_ASSERT(uiOverlayCalled == true, "UIContext render rendered overlay in HUD mode");

    std::cout << "\n=========================================================\n";
    std::cout << "  Summary: " << g_testsPassed << " passed, " << g_testsFailed << " failed.\n";
    std::cout << "=========================================================\n";

    return g_testsFailed == 0 ? 0 : 1;
}
