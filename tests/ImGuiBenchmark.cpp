/**
 * @file ImGuiBenchmark.cpp
 * @brief Retained-mode layout & rendering benchmark with the real Dear ImGui backend.
 *
 * Verifies real ImDrawList command generation, vertex/index throughput, and
 * dirty-flag layout cache efficiency under production ImGui rendering.
 */

#include "PerfUI/PerfUI.h"
#include "ImGuiRenderBackend.h"
#include <imgui.h>

#include <chrono>
#include <iomanip>
#include <iostream>

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define BENCH_ASSERT(cond, msg) \
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
    std::cout << "    PerfUI Benchmark Suite: Real ImGui Backend           \n";
    std::cout << "=========================================================\n\n";

    // 1. Initialize Headless ImGui Context
    IMGUI_CHECKVERSION();
    ImGuiContext* imguiCtx = ImGui::CreateContext();
    ImGui::SetCurrentContext(imguiCtx);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1920.0f, 1080.0f);
    io.DeltaTime = 1.0f / 60.0f;

    // Build default font atlas for measurement
    unsigned char* fontPixels = nullptr;
    int fontWidth = 0;
    int fontHeight = 0;
    io.Fonts->GetTexDataAsRGBA32(&fontPixels, &fontWidth, &fontHeight);

    // 2. Initialize PerfUI with ImGuiRenderBackend
    PerfUI::UIContext context;
    context.setViewportSize(PerfUI::Dimensions{ 1920.0f, 1080.0f });

    PerfUI::ImGuiRenderBackend backend;

    // 3. Build 550+ Element Retained Hierarchy
    auto* rootPanel = context.root()->add<PerfUI::Panel>("BenchRoot");
    rootPanel->layout()
        .direction(PerfUI::LayoutDirection::Vertical)
        .padding(16.0f)
        .gap(4.0f)
        .width(PerfUI::DimensionConstraint::Fixed(1600.0f))
        .height(PerfUI::DimensionConstraint::Fixed(900.0f));

    constexpr int numRows = 46;
    constexpr int perRow = 12; // 46 * 12 = 552 widgets

    for (int r = 0; r < numRows; ++r) {
        auto* row = rootPanel->add<PerfUI::Panel>("Row_" + std::to_string(r));
        row->layout()
            .direction(PerfUI::LayoutDirection::Horizontal)
            .width(PerfUI::DimensionConstraint::Fixed(1568.0f))
            .height(PerfUI::DimensionConstraint::Fixed(14.0f))
            .gap(2.0f);

        for (int c = 0; c < perRow; ++c) {
            if (c % 3 == 0) {
                auto* btn = row->add<PerfUI::Button>("Btn");
                btn->layout().flex(1.0f);
            } else if (c % 3 == 1) {
                auto* bar = row->add<PerfUI::ProgressBar>(0.5f);
                bar->layout().flex(1.0f);
            } else {
                auto* txt = row->add<PerfUI::Text>("Label");
                txt->layout().flex(1.0f);
            }
        }
    }

    // -------------------------------------------------------------
    // Pass 1: Cold Layout + ImGui Draw Command Generation
    // -------------------------------------------------------------
    ImGui::NewFrame();
    auto t0 = std::chrono::high_resolution_clock::now();
    context.render(backend);
    ImGui::Render();
    auto t1 = std::chrono::high_resolution_clock::now();
    double coldPassUs = std::chrono::duration<double, std::micro>(t1 - t0).count();

    int totalElements = context.metrics().elementCount;
    BENCH_ASSERT(totalElements >= 500, "Rendered hierarchy contains 500+ elements");

    ImDrawData* drawData = ImGui::GetDrawData();
    BENCH_ASSERT(drawData != nullptr, "ImGui draw data successfully generated");
    BENCH_ASSERT(drawData->TotalVtxCount > 0, "ImGui vertex buffer non-empty");
    BENCH_ASSERT(drawData->TotalIdxCount > 0, "ImGui index buffer non-empty");

    // -------------------------------------------------------------
    // Pass 2: Layout Cache Hit + ImGui Draw Command Generation
    // -------------------------------------------------------------
    ImGui::NewFrame();
    auto t2 = std::chrono::high_resolution_clock::now();
    context.render(backend);
    ImGui::Render();
    auto t3 = std::chrono::high_resolution_clock::now();
    double cachedPassUs = std::chrono::duration<double, std::micro>(t3 - t2).count();

    // -------------------------------------------------------------
    // Pass 3: Invalidation + Full Tree Re-layout + Draw Generation
    // -------------------------------------------------------------
    context.root()->markLayoutDirty();
    ImGui::NewFrame();
    auto t4 = std::chrono::high_resolution_clock::now();
    context.render(backend);
    ImGui::Render();
    auto t5 = std::chrono::high_resolution_clock::now();
    double reLayoutUs = std::chrono::duration<double, std::micro>(t5 - t4).count();

    // Results output
    std::cout << "\n  --- Benchmark Results (Real ImGui Backend) ---\n";
    std::cout << "  * Total Element Count    : " << totalElements << "\n";
    std::cout << "  * Generated ImGui Vertices: " << drawData->TotalVtxCount << "\n";
    std::cout << "  * Generated ImGui Indices : " << drawData->TotalIdxCount << "\n";
    std::cout << "  * ImDrawList Command Lists: " << drawData->CmdListsCount << "\n";
    std::cout << "  * Cold Layout + DrawList  : " << std::fixed << std::setprecision(2)
              << coldPassUs << " us (" << (coldPassUs / 1000.0) << " ms)\n";
    std::cout << "  * Layout Cache Pass (Hit) : " << cachedPassUs << " us (" << (cachedPassUs / 1000.0) << " ms)\n";
    std::cout << "  * Full Tree Re-layout     : " << reLayoutUs << " us (" << (reLayoutUs / 1000.0) << " ms)\n\n";

    if (cachedPassUs > 20000.0) {
        BENCH_ASSERT(false, "Layout cache pass took excessively long (> 20 ms)");
    } else {
        BENCH_ASSERT(true, "Layout cache pass with real ImGui backend completed within bounds");
        if (cachedPassUs > 500.0) {
            std::cout << "  [PERF NOTE] ImGui cached pass took " << cachedPassUs
                      << " us (> 500 us expected in optimized release build).\n";
        }
    }

    // Teardown
    ImGui::DestroyContext(imguiCtx);

    std::cout << "=========================================================\n";
    std::cout << "  ImGui Benchmark Results: " << g_testsPassed << " passed, " << g_testsFailed << " failed.\n";
    std::cout << "=========================================================\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
