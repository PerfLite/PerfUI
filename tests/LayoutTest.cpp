/**
 * @file LayoutTest.cpp
 * @brief Unit tests and benchmarks for PerfUI layout engine and focus management.
 *
 * Verifies:
 *  1. Flex-grow distribution in horizontal/vertical containers.
 *  2. Padding and margin offsetting and constraint bounds.
 *  3. Auto-sizing computation for nested container hierarchies.
 *  4. Focus management and focusable state transitions.
 *  5. 500-widget performance benchmark (Layout vs Dirty-Flag Cache vs Render).
 */

#include "PerfUI/PerfUI.h"
#include "../src/backends/mock/MockRenderBackend.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <cassert>
#include <cmath>

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

static bool approxEqual(float a, float b, float eps = 0.5f) {
    return std::fabs(a - b) <= eps;
}

int main() {
    std::cout << "=========================================================\n";
    std::cout << "     PerfUI Layout & Focus Test Suite + Benchmark        \n";
    std::cout << "=========================================================\n\n";

    // -------------------------------------------------------------
    // Test 1: Flex-Grow Distribution (Horizontal)
    // -------------------------------------------------------------
    std::cout << "Test 1: Flex-Grow Distribution (Horizontal)...\n";
    {
        PerfUI::UIContext context;
        context.setViewportSize({ 1000.0f, 600.0f });
        PerfUI::MockRenderBackend mock;

        auto* container = context.root()->add<PerfUI::Panel>("FlexContainer");
        container->layout()
            .direction(PerfUI::LayoutDirection::Horizontal)
            .width(PerfUI::DimensionConstraint::Fixed(300.0f))
            .height(PerfUI::DimensionConstraint::Fixed(100.0f))
            .padding(0.0f)
            .gap(0.0f);

        auto* child1 = container->add<PerfUI::Panel>("Child1");
        child1->layout().flex(1.0f).height(PerfUI::DimensionConstraint::Fixed(50.0f));

        auto* child2 = container->add<PerfUI::Panel>("Child2");
        child2->layout().flex(2.0f).height(PerfUI::DimensionConstraint::Fixed(50.0f));

        context.render(mock);

        TEST_ASSERT(approxEqual(child1->bounds().width, 100.0f), "Child1 flex(1.0) occupies 100px (1/3 of 300px)");
        TEST_ASSERT(approxEqual(child2->bounds().width, 200.0f), "Child2 flex(2.0) occupies 200px (2/3 of 300px)");
        TEST_ASSERT(approxEqual(child1->bounds().x, 0.0f), "Child1 starts at x = 0px");
        TEST_ASSERT(approxEqual(child2->bounds().x, 100.0f), "Child2 starts at x = 100px");
    }

    // -------------------------------------------------------------
    // Test 2: Padding & Margin Insets
    // -------------------------------------------------------------
    std::cout << "\nTest 2: Padding & Margin Insets...\n";
    {
        PerfUI::UIContext context;
        context.setViewportSize({ 1000.0f, 600.0f });
        PerfUI::MockRenderBackend mock;

        auto* container = context.root()->add<PerfUI::Panel>("PadContainer");
        container->layout()
            .direction(PerfUI::LayoutDirection::Vertical)
            .width(PerfUI::DimensionConstraint::Fixed(200.0f))
            .height(PerfUI::DimensionConstraint::Fixed(200.0f))
            .padding(15.0f, 25.0f, 10.0f, 20.0f) // L=15, T=25, R=10, B=20
            .gap(0.0f);

        auto* item = container->add<PerfUI::Panel>("Item");
        item->layout()
            .width(PerfUI::DimensionConstraint::Fixed(50.0f))
            .height(PerfUI::DimensionConstraint::Fixed(30.0f))
            .margin(5.0f, 10.0f, 0.0f, 0.0f); // margin L=5, T=10

        context.render(mock);

        // Expected X: 15 (padding) + 5 (margin) = 20
        // Expected Y: 25 (padding) + 10 (margin) = 35
        TEST_ASSERT(approxEqual(item->bounds().x, 20.0f), "Item X respects padding + margin (20px)");
        TEST_ASSERT(approxEqual(item->bounds().y, 35.0f), "Item Y respects padding + margin (35px)");
        TEST_ASSERT(approxEqual(item->bounds().width, 50.0f), "Item width fixed to 50px");
        TEST_ASSERT(approxEqual(item->bounds().height, 30.0f), "Item height fixed to 30px");
    }

    // -------------------------------------------------------------
    // Test 3: Auto-Sizing Hierarchy
    // -------------------------------------------------------------
    std::cout << "\nTest 3: Auto-Sizing Hierarchy...\n";
    {
        PerfUI::UIContext context;
        context.setViewportSize({ 1000.0f, 600.0f });
        PerfUI::MockRenderBackend mock;

        auto* autoPanel = context.root()->add<PerfUI::Panel>("AutoPanel");
        autoPanel->layout()
            .direction(PerfUI::LayoutDirection::Vertical)
            .alignment(PerfUI::Alignment::Start)
            .width(PerfUI::DimensionConstraint::Auto())
            .height(PerfUI::DimensionConstraint::Auto())
            .padding(10.0f)
            .gap(8.0f);

        auto* b1 = autoPanel->add<PerfUI::Button>("Small");
        b1->layout().width(120.0f).height(30.0f);

        auto* b2 = autoPanel->add<PerfUI::Button>("Wide Button");
        b2->layout().width(180.0f).height(40.0f);

        context.render(mock);

        // Expected Width: max(120, 180) + padding.horizontal (20) = 200px
        // Expected Height: 30 + 40 + gap(8) + padding.vertical (20) = 98px
        TEST_ASSERT(approxEqual(autoPanel->bounds().width, 200.0f), "Auto-width correctly computes 200px");
        TEST_ASSERT(approxEqual(autoPanel->bounds().height, 98.0f), "Auto-height correctly computes 98px");
    }

    // -------------------------------------------------------------
    // Test 4: Focus Graph & State Transitions
    // -------------------------------------------------------------
    std::cout << "\nTest 4: Focus Graph & State Transitions...\n";
    {
        PerfUI::UIContext context;
        context.setViewportSize({ 800.0f, 600.0f });

        auto* btn1 = context.root()->add<PerfUI::Button>("Button 1");
        auto* txt = context.root()->add<PerfUI::Text>("Static Text");
        auto* btn2 = context.root()->add<PerfUI::Button>("Button 2");

        TEST_ASSERT(btn1->isFocusable() == true, "Button 1 is focusable by default");
        TEST_ASSERT(txt->isFocusable() == false, "Text is not focusable by default");
        TEST_ASSERT(btn2->isFocusable() == true, "Button 2 is focusable by default");

        context.setFocus(btn1);
        TEST_ASSERT(context.focusedElement() == btn1, "UIContext tracks Button 1 as focused");
        TEST_ASSERT(btn1->isFocused() == true, "Button 1 reports isFocused() == true");
        TEST_ASSERT(btn2->isFocused() == false, "Button 2 is not focused");

        context.setFocus(btn2);
        TEST_ASSERT(context.focusedElement() == btn2, "UIContext transitions focus to Button 2");
        TEST_ASSERT(btn1->isFocused() == false, "Button 1 lost focus");
        TEST_ASSERT(btn2->isFocused() == true, "Button 2 gained focus");

        // Non-focusable element rejection
        context.setFocus(txt);
        TEST_ASSERT(context.focusedElement() == nullptr, "Non-focusable element rejected from focus");

        context.setFocus(btn1);
        context.clearFocus();
        TEST_ASSERT(context.focusedElement() == nullptr, "clearFocus() clears active focus");
        TEST_ASSERT(btn1->isFocused() == false, "Button 1 is no longer focused");
    }

    // -------------------------------------------------------------
    // Test 5: 500-Widget Performance Benchmark
    // -------------------------------------------------------------
    std::cout << "\nTest 5: 500-Widget Performance Benchmark...\n";
    {
        PerfUI::UIContext context;
        context.setViewportSize({ 1920.0f, 1080.0f });
        PerfUI::MockRenderBackend mock;

        auto* mainCard = context.root()->add<PerfUI::Panel>("BenchCard");
        mainCard->layout()
            .direction(PerfUI::LayoutDirection::Vertical)
            .width(PerfUI::DimensionConstraint::Fixed(1600.0f))
            .height(PerfUI::DimensionConstraint::Fixed(900.0f))
            .padding(10.0f)
            .gap(4.0f);

        // Build 50 rows of 10 widgets each = 500 widgets (+ containers)
        const int rowCount = 50;
        const int perRow = 10;
        for (int r = 0; r < rowCount; ++r) {
            auto* row = mainCard->add<PerfUI::Panel>("Row_" + std::to_string(r));
            row->layout()
                .direction(PerfUI::LayoutDirection::Horizontal)
                .width(PerfUI::DimensionConstraint::Fixed(1580.0f))
                .height(PerfUI::DimensionConstraint::Fixed(14.0f))
                .gap(2.0f);

            for (int c = 0; c < perRow; ++c) {
                if (c % 3 == 0) {
                    auto* btn = row->add<PerfUI::Button>("B");
                    btn->layout().flex(1.0f);
                } else if (c % 3 == 1) {
                    auto* bar = row->add<PerfUI::ProgressBar>(0.5f);
                    bar->layout().flex(1.0f);
                } else {
                    auto* txt = row->add<PerfUI::Text>("Val");
                    txt->layout().flex(1.0f);
                }
            }
        }

        // Pass 1: Initial layout calculation (cold)
        auto t0 = std::chrono::high_resolution_clock::now();
        context.render(mock);
        auto t1 = std::chrono::high_resolution_clock::now();
        double coldLayoutUs = std::chrono::duration<double, std::micro>(t1 - t0).count();

        // Pass 2: Cached layout (dirty-flag cache hit)
        auto t2 = std::chrono::high_resolution_clock::now();
        context.render(mock);
        auto t3 = std::chrono::high_resolution_clock::now();
        double cachedPassUs = std::chrono::duration<double, std::micro>(t3 - t2).count();

        // Pass 3: Invalidate single element layout and measure re-layout
        context.root()->markLayoutDirty();
        auto t4 = std::chrono::high_resolution_clock::now();
        context.render(mock);
        auto t5 = std::chrono::high_resolution_clock::now();
        double reLayoutUs = std::chrono::duration<double, std::micro>(t5 - t4).count();

        int totalElements = context.metrics().elementCount;
        TEST_ASSERT(totalElements >= 500, "Retained hierarchy contains 500+ elements");

        std::cout << "\n  --- Benchmark Results (550+ Elements) ---\n";
        std::cout << "  * Total Element Count   : " << totalElements << "\n";
        std::cout << "  * Cold Layout + Render  : " << std::fixed << std::setprecision(2) << coldLayoutUs << " us (" << (coldLayoutUs / 1000.0) << " ms)\n";
        std::cout << "  * Dirty-Flag Cached Pass: " << cachedPassUs << " us (" << (cachedPassUs / 1000.0) << " ms)\n";
        std::cout << "  * Full Tree Re-layout   : " << reLayoutUs << " us (" << (reLayoutUs / 1000.0) << " ms)\n";
        std::cout << "  * Mock Draw Calls       : " << mock.drawCallCount() << " calls\n\n";

        TEST_ASSERT(cachedPassUs < 250.0, "Cached pass takes < 0.25 ms for 500+ elements");
    }

    // -------------------------------------------------------------
    // Test Summary
    // -------------------------------------------------------------
    std::cout << "=========================================================\n";
    std::cout << "  Layout Test Results: " << g_testsPassed << " passed, " << g_testsFailed << " failed.\n";
    std::cout << "=========================================================\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
