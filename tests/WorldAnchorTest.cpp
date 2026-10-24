#include "PerfUI/WorldAnchor.h"
#include "PerfUI/UIContext.h"
#include <iostream>
#include <cassert>
#include <cmath>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[FAILED] " << msg << " (" << #cond << ") at line " << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

static constexpr float EPSILON = 0.01f;

static bool approxEqual(float a, float b, float eps = EPSILON) {
    return std::abs(a - b) <= eps;
}

bool test_WorldAnchorInit() {
    PerfUI::WorldAnchor anchor(42);
    TEST_ASSERT(anchor.targetId() == 42, "Target ID should match constructor argument");
    TEST_ASSERT(approxEqual(anchor.bounds().width, 100.0f), "Default width should be 100");
    TEST_ASSERT(approxEqual(anchor.bounds().height, 40.0f), "Default height should be 40");
    TEST_ASSERT(anchor.anchorAlignment() == PerfUI::AnchorAlignment::BottomCenter, "Default alignment should be BottomCenter");
    TEST_ASSERT(!anchor.isEdgeClampingEnabled(), "Edge clamping should be disabled by default");
    TEST_ASSERT(anchor.hideBehindCamera(), "Hide behind camera should be enabled by default");
    TEST_ASSERT(!anchor.hideWhenOccluded(), "Hide when occluded should be disabled by default");
    return true;
}

bool test_WorldAnchorAlignments() {
    PerfUI::WorldAnchor anchor;
    anchor.setBaseSize({ 100.0f, 40.0f });

    // Center alignment
    anchor.setAnchorAlignment(PerfUI::AnchorAlignment::Center);
    anchor.setScreenData({ 500.0f, 300.0f }, 100.0f, true);
    anchor.update(0.016f);
    TEST_ASSERT(approxEqual(anchor.bounds().x, 450.0f), "Center align: X should be 500 - 50 = 450");
    TEST_ASSERT(approxEqual(anchor.bounds().y, 280.0f), "Center align: Y should be 300 - 20 = 280");

    // BottomCenter alignment
    anchor.setAnchorAlignment(PerfUI::AnchorAlignment::BottomCenter);
    anchor.update(0.016f);
    TEST_ASSERT(approxEqual(anchor.bounds().x, 450.0f), "BottomCenter: X should be 450");
    TEST_ASSERT(approxEqual(anchor.bounds().y, 260.0f), "BottomCenter: Y should be 300 - 40 = 260");

    // TopCenter alignment
    anchor.setAnchorAlignment(PerfUI::AnchorAlignment::TopCenter);
    anchor.update(0.016f);
    TEST_ASSERT(approxEqual(anchor.bounds().x, 450.0f), "TopCenter: X should be 450");
    TEST_ASSERT(approxEqual(anchor.bounds().y, 300.0f), "TopCenter: Y should be 300");

    // TopLeft alignment
    anchor.setAnchorAlignment(PerfUI::AnchorAlignment::TopLeft);
    anchor.update(0.016f);
    TEST_ASSERT(approxEqual(anchor.bounds().x, 500.0f), "TopLeft: X should be 500");
    TEST_ASSERT(approxEqual(anchor.bounds().y, 300.0f), "TopLeft: Y should be 300");

    return true;
}

bool test_WorldAnchorScreenOffset() {
    PerfUI::WorldAnchor anchor;
    anchor.setBaseSize({ 100.0f, 40.0f });
    anchor.setAnchorAlignment(PerfUI::AnchorAlignment::TopLeft);
    anchor.setScreenOffset({ 15.0f, -25.0f });
    anchor.setScreenData({ 200.0f, 200.0f }, 100.0f, true);
    anchor.update(0.016f);

    TEST_ASSERT(approxEqual(anchor.bounds().x, 215.0f), "Offset X should add 15px");
    TEST_ASSERT(approxEqual(anchor.bounds().y, 175.0f), "Offset Y should subtract 25px");
    return true;
}

bool test_WorldAnchorDistanceScaling() {
    PerfUI::WorldAnchor anchor;
    anchor.setBaseSize({ 100.0f, 50.0f });
    anchor.setDistanceScaling(100.0f, 1000.0f, 0.5f, 2.0f, true);

    // Close: dist = 100 -> scale = 2.0
    anchor.setScreenData({ 500.0f, 500.0f }, 100.0f, true);
    anchor.update(0.016f);
    TEST_ASSERT(approxEqual(anchor.currentScale(), 2.0f), "Scale at minDist should be maxScale (2.0)");
    TEST_ASSERT(approxEqual(anchor.bounds().width, 200.0f), "Width at scale 2.0 should be 200");
    TEST_ASSERT(approxEqual(anchor.bounds().height, 100.0f), "Height at scale 2.0 should be 100");

    // Far: dist = 1000 -> scale = 0.5
    anchor.setScreenData({ 500.0f, 500.0f }, 1000.0f, true);
    anchor.update(0.016f);
    TEST_ASSERT(approxEqual(anchor.currentScale(), 0.5f), "Scale at maxDist should be minScale (0.5)");
    TEST_ASSERT(approxEqual(anchor.bounds().width, 50.0f), "Width at scale 0.5 should be 50");
    TEST_ASSERT(approxEqual(anchor.bounds().height, 25.0f), "Height at scale 0.5 should be 25");

    // Middle: dist = 550 -> scale = 1.25
    anchor.setScreenData({ 500.0f, 500.0f }, 550.0f, true);
    anchor.update(0.016f);
    TEST_ASSERT(approxEqual(anchor.currentScale(), 1.25f), "Scale at midDist should be 1.25");
    TEST_ASSERT(approxEqual(anchor.bounds().width, 125.0f), "Width at scale 1.25 should be 125");

    // Clamping beyond range
    anchor.setScreenData({ 500.0f, 500.0f }, 50.0f, true);
    anchor.update(0.016f);
    TEST_ASSERT(approxEqual(anchor.currentScale(), 2.0f), "Scale below minDist must clamp to 2.0");

    anchor.setScreenData({ 500.0f, 500.0f }, 5000.0f, true);
    anchor.update(0.016f);
    TEST_ASSERT(approxEqual(anchor.currentScale(), 0.5f), "Scale above maxDist must clamp to 0.5");

    return true;
}

bool test_WorldAnchorDistanceAlpha() {
    PerfUI::WorldAnchor anchor;
    anchor.setDistanceAlpha(500.0f, 1500.0f, 0.2f, 1.0f, true);

    anchor.setScreenData({ 500.0f, 500.0f }, 500.0f, true);
    anchor.update(0.016f);
    TEST_ASSERT(approxEqual(anchor.opacity(), 1.0f), "Opacity at minDist should be 1.0");

    anchor.setScreenData({ 500.0f, 500.0f }, 1500.0f, true);
    anchor.update(0.016f);
    TEST_ASSERT(approxEqual(anchor.opacity(), 0.2f), "Opacity at maxDist should be 0.2");

    anchor.setScreenData({ 500.0f, 500.0f }, 1000.0f, true);
    anchor.update(0.016f);
    TEST_ASSERT(approxEqual(anchor.opacity(), 0.6f), "Opacity at midDist should be 0.6");

    return true;
}

bool test_WorldAnchorBehindCameraHiding() {
    PerfUI::WorldAnchor anchor;
    anchor.setHideBehindCamera(true);

    anchor.setScreenData({ 500.0f, 500.0f }, 100.0f, true);
    anchor.update(0.016f);
    TEST_ASSERT(anchor.isVisible(), "Anchor should be visible in front of camera");

    anchor.setScreenData({ 500.0f, 500.0f }, 100.0f, false); // Behind camera
    anchor.update(0.016f);
    TEST_ASSERT(!anchor.isVisible(), "Anchor should hide when behind camera and hideBehindCamera is true");

    anchor.setHideBehindCamera(false);
    anchor.update(0.016f);
    TEST_ASSERT(anchor.isVisible(), "Anchor should remain visible if hideBehindCamera is false");

    return true;
}

bool test_WorldAnchorOcclusionLoS() {
    PerfUI::WorldAnchor anchor;
    anchor.setHideWhenOccluded(true);

    anchor.setScreenData({ 500.0f, 500.0f }, 100.0f, true, true, true);
    anchor.update(0.016f);
    TEST_ASSERT(anchor.isVisible(), "Anchor should be visible when unobstructed");

    anchor.setScreenData({ 500.0f, 500.0f }, 100.0f, true, true, false); // Occluded
    anchor.update(0.016f);
    TEST_ASSERT(!anchor.isVisible(), "Anchor should hide when occluded");

    anchor.setHideWhenOccluded(false);
    anchor.update(0.016f);
    TEST_ASSERT(anchor.isVisible(), "Anchor should ignore occlusion when hideWhenOccluded is false");

    return true;
}

bool test_WorldAnchorEdgeClamping() {
    PerfUI::UIContext context;
    context.setViewportSize({ 1920.0f, 1080.0f });

    auto* anchor = context.root()->add<PerfUI::WorldAnchor>();
    anchor->setBaseSize({ 100.0f, 40.0f });
    anchor->setAnchorAlignment(PerfUI::AnchorAlignment::Center);
    anchor->setEdgeClamping(true, 50.0f); // margin = 50

    // 1. Inside screen: (960, 540)
    anchor->setScreenData({ 960.0f, 540.0f }, 100.0f, true);
    context.update(0.016f);
    TEST_ASSERT(!anchor->isClamped(), "Center of screen must NOT be clamped");
    TEST_ASSERT(approxEqual(anchor->bounds().x, 910.0f), "Bounds X should be 960 - 50 = 910");
    TEST_ASSERT(approxEqual(anchor->bounds().y, 520.0f), "Bounds Y should be 540 - 20 = 520");

    // 2. Far Right (offscreen): (3000, 540)
    anchor->setScreenData({ 3000.0f, 540.0f }, 100.0f, true);
    context.update(0.016f);
    TEST_ASSERT(anchor->isClamped(), "Target to the right must be clamped");
    // Clamped screen X = 1920 - 50 = 1870. Center align: 1870 - 50 = 1820
    TEST_ASSERT(approxEqual(anchor->bounds().x, 1820.0f), "Clamped right X bounds should be 1820");
    TEST_ASSERT(approxEqual(anchor->bounds().y, 520.0f), "Clamped right Y bounds should be 520");
    TEST_ASSERT(approxEqual(anchor->clampedAngle(), 0.0f, 0.05f), "Angle to right should be ~0 rad");

    // 3. Far Left (offscreen): (-1000, 540)
    anchor->setScreenData({ -1000.0f, 540.0f }, 100.0f, true);
    context.update(0.016f);
    TEST_ASSERT(anchor->isClamped(), "Target to the left must be clamped");
    // Clamped screen X = 50. Center align: 50 - 50 = 0
    TEST_ASSERT(approxEqual(anchor->bounds().x, 0.0f), "Clamped left X bounds should be 0");
    TEST_ASSERT(approxEqual(anchor->bounds().y, 520.0f), "Clamped left Y bounds should be 520");
    TEST_ASSERT(approxEqual(std::abs(anchor->clampedAngle()), 3.14159f, 0.05f), "Angle to left should be ~PI rad");

    // 4. Far Top (offscreen): (960, -800)
    anchor->setScreenData({ 960.0f, -800.0f }, 100.0f, true);
    context.update(0.016f);
    TEST_ASSERT(anchor->isClamped(), "Target to top must be clamped");
    // Clamped screen Y = 50. Center align: 50 - 20 = 30
    TEST_ASSERT(approxEqual(anchor->bounds().x, 910.0f), "Clamped top X bounds should be 910");
    TEST_ASSERT(approxEqual(anchor->bounds().y, 30.0f), "Clamped top Y bounds should be 30");
    TEST_ASSERT(approxEqual(anchor->clampedAngle(), -1.5708f, 0.05f), "Angle to top should be ~ -PI/2 rad");

    // 5. Behind Camera: edge clamping keeps it visible on the border with inverted direction
    anchor->setScreenData({ 3000.0f, 540.0f }, 100.0f, false); // Behind camera
    context.update(0.016f);
    TEST_ASSERT(anchor->isVisible(), "Anchor behind camera with edge clamping MUST stay visible");
    TEST_ASSERT(anchor->isClamped(), "Anchor behind camera must be clamped");

    return true;
}

bool test_WorldAnchorContextSnapshot() {
    PerfUI::UIContext context;
    context.setViewportSize({ 1920.0f, 1080.0f });

    auto* anchor1 = context.root()->add<PerfUI::WorldAnchor>(101);
    anchor1->setBaseSize({ 100.0f, 40.0f });
    anchor1->setAnchorAlignment(PerfUI::AnchorAlignment::Center);

    auto* anchor2 = context.root()->add<PerfUI::WorldAnchor>(102);
    anchor2->setBaseSize({ 80.0f, 30.0f });
    anchor2->setAnchorAlignment(PerfUI::AnchorAlignment::TopLeft);

    // Initial update before snapshot arrives: target not found -> hidden
    context.update(0.016f);
    TEST_ASSERT(!anchor1->isVisible(), "Anchor1 should be hidden when no snapshot exists");
    TEST_ASSERT(!anchor2->isVisible(), "Anchor2 should be hidden when no snapshot exists");

    // Publish snapshot from game thread
    auto snapshot = std::make_shared<PerfUI::WorldAnchorSnapshot>();
    snapshot->screenWidth = 1920.0f;
    snapshot->screenHeight = 1080.0f;
    snapshot->frameNumber = 1;

    PerfUI::WorldAnchorPoint pt1;
    pt1.targetId = 101;
    pt1.screenPos = { 800.0f, 400.0f };
    pt1.distance = 300.0f;
    pt1.inFrontOfCamera = true;
    pt1.isValid = true;
    snapshot->points[101] = pt1;

    PerfUI::WorldAnchorPoint pt2;
    pt2.targetId = 102;
    pt2.screenPos = { 250.0f, 150.0f };
    pt2.distance = 600.0f;
    pt2.inFrontOfCamera = true;
    pt2.isValid = true;
    snapshot->points[102] = pt2;

    context.setWorldAnchorSnapshot(snapshot);
    context.update(0.016f);

    TEST_ASSERT(anchor1->isVisible(), "Anchor1 should be visible now");
    TEST_ASSERT(approxEqual(anchor1->bounds().x, 750.0f), "Anchor1 X: 800 - 50 = 750");
    TEST_ASSERT(approxEqual(anchor1->bounds().y, 380.0f), "Anchor1 Y: 400 - 20 = 380");

    TEST_ASSERT(anchor2->isVisible(), "Anchor2 should be visible now");
    TEST_ASSERT(approxEqual(anchor2->bounds().x, 250.0f), "Anchor2 X: 250");
    TEST_ASSERT(approxEqual(anchor2->bounds().y, 150.0f), "Anchor2 Y: 150");

    // Invalidate target 101 (e.g. actor dead / deleted)
    auto snapshot2 = std::make_shared<PerfUI::WorldAnchorSnapshot>(*snapshot);
    snapshot2->points[101].isValid = false;
    context.setWorldAnchorSnapshot(snapshot2);
    context.update(0.016f);

    TEST_ASSERT(!anchor1->isVisible(), "Anchor1 should hide when target becomes invalid");
    TEST_ASSERT(anchor2->isVisible(), "Anchor2 should remain visible");

    return true;
}

int main() {
    std::cout << "Running PerfUI WorldAnchor Unit Tests..." << std::endl;

    int passed = 0;
    int failed = 0;

    auto run = [&](const char* name, bool(*fn)()) {
        std::cout << "  [TEST] " << name << "... ";
        if (fn()) {
            std::cout << "PASSED" << std::endl;
            passed++;
        } else {
            std::cout << "FAILED" << std::endl;
            failed++;
        }
    };

    run("WorldAnchorInit", test_WorldAnchorInit);
    run("WorldAnchorAlignments", test_WorldAnchorAlignments);
    run("WorldAnchorScreenOffset", test_WorldAnchorScreenOffset);
    run("WorldAnchorDistanceScaling", test_WorldAnchorDistanceScaling);
    run("WorldAnchorDistanceAlpha", test_WorldAnchorDistanceAlpha);
    run("WorldAnchorBehindCameraHiding", test_WorldAnchorBehindCameraHiding);
    run("WorldAnchorOcclusionLoS", test_WorldAnchorOcclusionLoS);
    run("WorldAnchorEdgeClamping", test_WorldAnchorEdgeClamping);
    run("WorldAnchorContextSnapshot", test_WorldAnchorContextSnapshot);

    std::cout << "\nResults: " << passed << " passed, " << failed << " failed." << std::endl;
    return failed == 0 ? 0 : 1;
}
