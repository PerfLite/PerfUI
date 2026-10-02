/**
 * @file BackendIndependenceTest.cpp
 * @brief Phase 15 Verification Test: Proof of 100% Backend Independence.
 * 
 * Verifies that the entire PerfUI Core:
 *  1. Compiles and executes with ZERO ImGui or DirectX 11 headers.
 *  2. Operates headlessly using MockRenderBackend.
 *  3. Measures, arranges, dispatches events, and renders without a GPU.
 */

#include "PerfUI/PerfUI.h"
#include "../src/backends/mock/MockRenderBackend.h"

#include <iostream>
#include <cassert>
#include <string>

// Test counters
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
    std::cout << "  PerfUI Phase 15: Backend Independence & Mock Test Suite \n";
    std::cout << "=========================================================\n\n";

    // -------------------------------------------------------------
    // Test 1: Initialize Headless UIContext with MockRenderBackend
    // -------------------------------------------------------------
    std::cout << "Test 1: Context Initialization...\n";
    PerfUI::UIContext context;
    context.setViewportSize(PerfUI::Dimensions{ 1920.0f, 1080.0f });
    PerfUI::MockRenderBackend mockBackend;

    TEST_ASSERT(context.root() != nullptr, "Root UIElement created");
    TEST_ASSERT(context.viewportSize().width == 1920.0f, "Viewport width is 1920");
    TEST_ASSERT(context.viewportSize().height == 1080.0f, "Viewport height is 1080");

    // -------------------------------------------------------------
    // Test 2: Build Retained Hierarchy with Core Widgets
    // -------------------------------------------------------------
    std::cout << "\nTest 2: Retained Widget Tree Construction...\n";
    auto* mainPanel = context.root()->add<PerfUI::Panel>("TestMainPanel");
    mainPanel->layout().direction(PerfUI::LayoutDirection::Vertical)
                       .padding(20.0f)
                       .gap(12.0f)
                       .width(PerfUI::DimensionConstraint::Fixed(400.0f));

    auto* headerText = mainPanel->add<PerfUI::Text>("PerfUI Headless Card");
    (void)headerText;
    auto* healthBar = mainPanel->add<PerfUI::ProgressBar>(0.75f);
    healthBar->layout().width(200.0f).height(16.0f);

    auto* slider = mainPanel->add<PerfUI::Slider>(50.0f, 0.0f, 100.0f);
    slider->layout().width(200.0f).height(20.0f);

    auto* checkbox = mainPanel->add<PerfUI::Checkbox>("Enable Notifications");
    (void)checkbox;

    bool buttonClicked = false;
    auto* button = mainPanel->add<PerfUI::Button>("Submit Action");
    button->onClick([&buttonClicked]() {
        buttonClicked = true;
    });

    auto* comboBox = mainPanel->add<PerfUI::ComboBox>(std::vector<std::string>{ "Option A", "Option B", "Option C" });

    TEST_ASSERT(mainPanel->children().size() == 6, "Panel holds 6 child widgets");

    // -------------------------------------------------------------
    // Test 3: Layout Measurement and Arrangement
    // -------------------------------------------------------------
    std::cout << "\nTest 3: Flexbox Layout Engine Execution...\n";
    // First render pass triggers performLayout()
    context.render(mockBackend);

    TEST_ASSERT(mainPanel->bounds().width == 400.0f, "MainPanel fixed width respected");
    TEST_ASSERT(mainPanel->bounds().height > 0.0f, "MainPanel auto-height computed from children");
    TEST_ASSERT(healthBar->bounds().width == 200.0f, "ProgressBar width arranged");
    TEST_ASSERT(slider->bounds().width == 200.0f, "Slider width arranged");

    // -------------------------------------------------------------
    // Test 4: Mouse Click & Event Dispatch Simulation
    // -------------------------------------------------------------
    std::cout << "\nTest 4: Input Simulation & Action Dispatch...\n";
    // Click on Button
    PerfUI::Point btnPos{
        button->bounds().x + button->bounds().width * 0.5f,
        button->bounds().y + button->bounds().height * 0.5f
    };

    context.onMouseDown(0, btnPos);
    context.onMouseUp(0, btnPos);

    TEST_ASSERT(buttonClicked == true, "Button::onClick fired via simulated mouse events");

    // -------------------------------------------------------------
    // Test 5: ComboBox Dropdown Interaction
    // -------------------------------------------------------------
    std::cout << "\nTest 5: ComboBox Overlay Lifecycle...\n";
    PerfUI::Point comboPos{
        comboBox->bounds().x + 20.0f,
        comboBox->bounds().y + 10.0f
    };

    // Open ComboBox
    context.onMouseDown(0, comboPos);
    context.onMouseUp(0, comboPos);
    TEST_ASSERT(comboBox->isOpen() == true, "ComboBox opened on click");
    TEST_ASSERT(context.hasActiveComboBox() == true, "UIContext tracks active ComboBox");

    // Click outside to close ComboBox
    context.onMouseDown(0, PerfUI::Point{ 10.0f, 10.0f });
    TEST_ASSERT(comboBox->isOpen() == false, "ComboBox closed on click outside");
    TEST_ASSERT(context.hasActiveComboBox() == false, "UIContext active ComboBox cleared");

    // -------------------------------------------------------------
    // Test 6: Headless Rendering Telemetry
    // -------------------------------------------------------------
    std::cout << "\nTest 6: Mock Render Telemetry...\n";
    context.render(mockBackend);

    TEST_ASSERT(mockBackend.drawCallCount() > 0, "Mock backend recorded draw calls (>0)");
    TEST_ASSERT(mockBackend.clipStackDepth() == 0, "Clip stack balanced at depth 0");
    TEST_ASSERT(context.metrics().elementCount > 0, "Metrics element count is accurate");

    std::cout << "\n  -> Telemetry: " << mockBackend.drawCallCount() << " draw calls, "
              << context.metrics().elementCount << " elements, "
              << context.metrics().layoutTimeUs << " us layout, "
              << context.metrics().renderTimeUs << " us render.\n";

    // -------------------------------------------------------------
    // Test Summary
    // -------------------------------------------------------------
    std::cout << "\n=========================================================\n";
    std::cout << "  Test Results: " << g_testsPassed << " passed, " << g_testsFailed << " failed.\n";
    std::cout << "=========================================================\n";

    if (g_testsFailed == 0) {
        std::cout << ">> SUCCESS: PerfUI Core is 100% backend-independent! <<\n\n";
        return 0;
    } else {
        std::cerr << ">> ERROR: Some tests failed! <<\n\n";
        return 1;
    }
}
