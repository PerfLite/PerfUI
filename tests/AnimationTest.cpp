#include "PerfUI/PerfUI.h"
#include "PerfUI/Animation.h"
#include "PerfUI/ProgressBar.h"
#include "../src/backends/mock/MockRenderBackend.h"

#include <iostream>
#include <iomanip>
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

static bool approxEqual(float a, float b, float eps = 0.01f) {
    return std::fabs(a - b) <= eps;
}

int main() {
    std::cout << "=========================================================\n";
    std::cout << "          PerfUI Animation & Timeline Test Suite         \n";
    std::cout << "=========================================================\n\n";

    // -------------------------------------------------------------
    // Test 1: Easing Curves Bounds and Monotonicity
    // -------------------------------------------------------------
    std::cout << "Test 1: Easing Curves...\n";
    {
        TEST_ASSERT(approxEqual(PerfUI::ApplyEasing(0.0f, PerfUI::Easing::Linear), 0.0f), "Linear at 0 is 0");
        TEST_ASSERT(approxEqual(PerfUI::ApplyEasing(1.0f, PerfUI::Easing::Linear), 1.0f), "Linear at 1 is 1");
        TEST_ASSERT(approxEqual(PerfUI::ApplyEasing(0.0f, PerfUI::Easing::EaseOutCubic), 0.0f), "EaseOutCubic at 0 is 0");
        TEST_ASSERT(approxEqual(PerfUI::ApplyEasing(1.0f, PerfUI::Easing::EaseOutCubic), 1.0f), "EaseOutCubic at 1 is 1");
        TEST_ASSERT(approxEqual(PerfUI::ApplyEasing(0.0f, PerfUI::Easing::EaseOutBounce), 0.0f), "EaseOutBounce at 0 is 0");
        TEST_ASSERT(approxEqual(PerfUI::ApplyEasing(1.0f, PerfUI::Easing::EaseOutBounce), 1.0f), "EaseOutBounce at 1 is 1");
        TEST_ASSERT(approxEqual(PerfUI::ApplyEasing(0.0f, PerfUI::Easing::EaseOutBack), 0.0f), "EaseOutBack at 0 is 0");
        TEST_ASSERT(approxEqual(PerfUI::ApplyEasing(1.0f, PerfUI::Easing::EaseOutBack), 1.0f), "EaseOutBack at 1 is 1");
    }

    // -------------------------------------------------------------
    // Test 2: Tween Position, Size, and Opacity
    // -------------------------------------------------------------
    std::cout << "\nTest 2: ElementAnimator (Tween Properties)...\n";
    {
        PerfUI::UIContext context;
        context.setViewportSize({ 800.0f, 600.0f });

        auto* panel = context.root()->add<PerfUI::Panel>("TestPanel");
        panel->layout()
            .width(PerfUI::DimensionConstraint::Fixed(100.0f))
            .height(PerfUI::DimensionConstraint::Fixed(50.0f));
        panel->setBounds(PerfUI::Rect{ 10.0f, 20.0f, 100.0f, 50.0f });
        panel->setOpacity(1.0f);

        bool completed = false;
        panel->animate()
            .position({ 210.0f, 120.0f }, 0.2f, PerfUI::Easing::Linear)
            .size({ 300.0f, 150.0f }, 0.2f, PerfUI::Easing::Linear)
            .alpha(0.5f, 0.2f, PerfUI::Easing::Linear)
            .onComplete([&completed]() { completed = true; });

        TEST_ASSERT(panel->isAnimating(), "Panel is animating after setup");

        // Advance 0.1s (halfway)
        context.update(0.1f);
        TEST_ASSERT(approxEqual(panel->bounds().x, 110.0f, 5.0f), "Halfway X position ~110");
        TEST_ASSERT(approxEqual(panel->bounds().y, 70.0f, 5.0f), "Halfway Y position ~70");
        TEST_ASSERT(approxEqual(panel->bounds().width, 200.0f, 5.0f), "Halfway width ~200");
        TEST_ASSERT(approxEqual(panel->opacity(), 0.75f, 0.05f), "Halfway opacity ~0.75");
        TEST_ASSERT(!completed, "Not completed halfway");

        // Advance remaining 0.15s (past end)
        context.update(0.15f);
        TEST_ASSERT(approxEqual(panel->bounds().x, 210.0f, 0.1f), "Final X is 210");
        TEST_ASSERT(approxEqual(panel->bounds().y, 120.0f, 0.1f), "Final Y is 120");
        TEST_ASSERT(approxEqual(panel->bounds().width, 300.0f, 0.1f), "Final width is 300");
        TEST_ASSERT(approxEqual(panel->opacity(), 0.5f, 0.01f), "Final opacity is 0.5");
        TEST_ASSERT(completed, "onComplete callback invoked");
        TEST_ASSERT(!panel->isAnimating(), "Animation is marked finished");
    }

    // -------------------------------------------------------------
    // Test 3: ProgressBar Animated Fill
    // -------------------------------------------------------------
    std::cout << "\nTest 3: ProgressBar Animate Fill...\n";
    {
        PerfUI::UIContext context;
        context.setViewportSize({ 800.0f, 600.0f });

        auto* bar = context.root()->add<PerfUI::ProgressBar>(0.0f, "TestBar");
        TEST_ASSERT(approxEqual(bar->progress(), 0.0f), "Initial bar progress is 0.0");

        bar->animate().fill(0.8f, 0.2f, PerfUI::Easing::Linear);

        // Advance 0.1s
        context.update(0.1f);
        TEST_ASSERT(approxEqual(bar->progress(), 0.4f, 0.05f), "Bar progress halfway ~0.4");

        // Advance 0.15s
        context.update(0.15f);
        TEST_ASSERT(approxEqual(bar->progress(), 0.8f, 0.01f), "Bar progress final is 0.8");
    }

    // -------------------------------------------------------------
    // Test 4: Spring Physics Dynamics
    // -------------------------------------------------------------
    std::cout << "\nTest 4: Spring Physics Convergence...\n";
    {
        PerfUI::UIContext context;
        context.setViewportSize({ 800.0f, 600.0f });

        auto* panel = context.root()->add<PerfUI::Panel>("SpringPanel");
        panel->setBounds(PerfUI::Rect{ 0.0f, 0.0f, 50.0f, 50.0f });

        panel->spring()
            .position({ 100.0f, 200.0f })
            .stiffness(200.0f)
            .damping(25.0f); // Critically damped

        TEST_ASSERT(panel->isAnimating(), "Spring is active");

        // Simulate 1 second of frames
        for (int i = 0; i < 60; ++i) {
            context.update(1.0f / 60.0f);
        }

        TEST_ASSERT(approxEqual(panel->bounds().x, 100.0f, 0.1f), "Spring settled at X = 100");
        TEST_ASSERT(approxEqual(panel->bounds().y, 200.0f, 0.1f), "Spring settled at Y = 200");
        TEST_ASSERT(!panel->isAnimating(), "Spring marked resting");
    }

    // -------------------------------------------------------------
    // Test 5: Timeline Sequencing & Callbacks
    // -------------------------------------------------------------
    std::cout << "\nTest 5: Timeline Sequencer...\n";
    {
        PerfUI::UIContext context;
        context.setViewportSize({ 800.0f, 600.0f });

        auto* panel = context.root()->add<PerfUI::Panel>("TimelinePanel");
        panel->setBounds(PerfUI::Rect{ 0.0f, 0.0f, 100.0f, 100.0f });
        panel->setOpacity(0.0f);

        int checkStep = 0;
        bool timelineComplete = false;

        auto timeline = context.createTimeline();
        timeline->sequence()
            .fadeTo(panel, 1.0f, 0.1f, PerfUI::Easing::Linear)
            .call([&checkStep]() { checkStep = 1; })
            .wait(0.1f)
            .call([&checkStep]() { checkStep = 2; })
            .moveTo(panel, { 50.0f, 50.0f }, 0.1f, PerfUI::Easing::Linear)
            .onComplete([&timelineComplete]() { timelineComplete = true; });

        timeline->play();

        // 1. Advance 0.05s: halfway in fadeTo
        context.update(0.05f);
        TEST_ASSERT(approxEqual(panel->opacity(), 0.5f, 0.05f), "Timeline: fade halfway ~0.5");
        TEST_ASSERT(checkStep == 0, "Timeline: step 0 before wait");

        // 2. Advance 0.06s: fadeTo completes, step 1 action triggers, enters wait
        context.update(0.06f);
        TEST_ASSERT(approxEqual(panel->opacity(), 1.0f, 0.02f), "Timeline: fade finished at 1.0");
        TEST_ASSERT(checkStep == 1, "Timeline: first action called (checkStep == 1)");

        // 3. Advance 0.11s: wait finishes, step 2 action triggers, enters moveTo
        context.update(0.11f);
        TEST_ASSERT(checkStep == 2, "Timeline: second action called after wait");

        // 4. Advance 0.15s: moveTo finishes, timeline completes
        context.update(0.15f);
        TEST_ASSERT(approxEqual(panel->bounds().x, 50.0f, 0.1f), "Timeline: moveTo finished at X=50");
        TEST_ASSERT(approxEqual(panel->bounds().y, 50.0f, 0.1f), "Timeline: moveTo finished at Y=50");
        TEST_ASSERT(timelineComplete, "Timeline: onComplete invoked");
        TEST_ASSERT(timeline->isFinished(), "Timeline state is Finished");
    }

    // -------------------------------------------------------------
    // Test 6: Weak Reference Safety (Deleting Element Mid-Timeline)
    // -------------------------------------------------------------
    std::cout << "\nTest 6: Weak Reference Cancellation Safety...\n";
    {
        PerfUI::UIContext context;
        context.setViewportSize({ 800.0f, 600.0f });

        auto* doomedPanel = context.root()->add<PerfUI::Panel>("DoomedPanel");
        doomedPanel->layout()
            .width(PerfUI::DimensionConstraint::Fixed(100.0f))
            .height(PerfUI::DimensionConstraint::Fixed(100.0f));
        doomedPanel->setBounds(PerfUI::Rect{ 10.0f, 10.0f, 100.0f, 100.0f });

        bool timelineFinished = false;
        auto timeline = context.createTimeline();
        timeline->sequence()
            .moveTo(doomedPanel, { 200.0f, 200.0f }, 0.5f, PerfUI::Easing::Linear)
            .wait(0.2f)
            .fadeTo(doomedPanel, 0.0f, 0.5f)
            .onComplete([&timelineFinished]() { timelineFinished = true; });

        timeline->play();

        // Advance 0.1s
        context.update(0.1f);
        TEST_ASSERT(doomedPanel->bounds().x > 20.0f, "DoomedPanel started moving towards target");

        // DESTROY doomedPanel right now!
        context.root()->removeChild(doomedPanel);
        doomedPanel = nullptr;

        // Advance another 0.8s without doomedPanel
        // This MUST NOT crash, MUST NOT access freed memory, and MUST cleanly finish!
        try {
            context.update(0.8f);
            context.update(0.5f);
            TEST_ASSERT(true, "No segfault or exception after destroying element mid-timeline");
            TEST_ASSERT(timelineFinished, "Timeline safely completed after element was deleted");
        } catch (...) {
            TEST_ASSERT(false, "Exception thrown on destroyed element timeline step!");
        }
    }

    std::cout << "\n=========================================================\n";
    std::cout << "Tests Passed: " << g_testsPassed << "\n";
    std::cout << "Tests Failed: " << g_testsFailed << "\n";
    std::cout << "=========================================================\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
