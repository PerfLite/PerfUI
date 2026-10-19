#include <iostream>
#include <cassert>
#include <string>
#include "PerfUI/SvgService.h"
#include "PerfUI/UIContext.h"
#include "PerfUI/SvgImage.h"
#include "MockRenderBackend.h"

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " << (msg) << " (" << #cond << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

int main() {
    std::cout << "===================================================\n";
    std::cout << "  PerfUI: Native SVG Subsystem Verification Suite   \n";
    std::cout << "===================================================\n";

    // Test 1: In-memory SVG parsing and rasterization
    std::cout << "Test 1: SvgService::RasterizeString...\n";
    const char* sampleSvg =
        R"(<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 64 64">
             <rect x="0" y="0" width="64" height="64" fill="#1e1e1e"/>
             <circle cx="32" cy="32" r="20" fill="#ffd700"/>
           </svg>)";

    auto res = PerfUI::SvgService::RasterizeString(sampleSvg, 64.0f, 64.0f);
    TEST_ASSERT(res.valid(), "RasterizeString returned valid result");
    TEST_ASSERT(res.width == 64, "width matches target 64");
    TEST_ASSERT(res.height == 64, "height matches target 64");
    TEST_ASSERT(res.rgbaPixels.size() == 64 * 64 * 4, "pixel buffer size is 64x64x4 bytes");

    // Center pixel should be gold (#ffd700 -> R=255, G=215, B=0, A=255)
    size_t centerIdx = (32 * 64 + 32) * 4;
    uint8_t r = res.rgbaPixels[centerIdx + 0];
    uint8_t g = res.rgbaPixels[centerIdx + 1];
    uint8_t b = res.rgbaPixels[centerIdx + 2];
    uint8_t a = res.rgbaPixels[centerIdx + 3];
    TEST_ASSERT(r > 200 && g > 150 && b < 50 && a > 200, "center pixel has gold circle color");

    // Test 2: Aspect ratio preservation & scale
    std::cout << "Test 2: Aspect ratio scaling...\n";
    auto scaledRes = PerfUI::SvgService::RasterizeString(sampleSvg, 128.0f, 128.0f);
    TEST_ASSERT(scaledRes.valid(), "128x128 scale valid");
    TEST_ASSERT(scaledRes.width == 128 && scaledRes.height == 128, "dimensions scaled to 128");

    // Test 3: SvgImage widget lifecycle
    std::cout << "Test 3: SvgImage widget lifecycle...\n";
    PerfUI::MockRenderBackend mockBackend;
    PerfUI::UIContext context;
    context.setRenderBackend(&mockBackend);

    auto svgWidget = std::make_unique<PerfUI::SvgImage>("", 48.0f, 48.0f, "TestSvg");
    svgWidget->svgContent(sampleSvg);
    svgWidget->setBounds(PerfUI::Rect{ 10.0f, 10.0f, 48.0f, 48.0f });

    svgWidget->render(mockBackend);
    TEST_ASSERT(svgWidget->texture() > 0, "SvgImage successfully loaded and bound texture");
    TEST_ASSERT(mockBackend.drawCallCount() > 0, "drawImage was dispatched for SvgImage");

    std::cout << "\n[PASS] All SVG Subsystem tests passed successfully!\n";
    return 0;
}
