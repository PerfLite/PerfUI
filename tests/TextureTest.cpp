/**
 * @file TextureTest.cpp
 * @brief Clean Client Stage 2 Verification Tests:
 *        1. Texture loading, sizing, dynamic creation, updating, and destruction
 *        2. UIContext delegation and mock backend recording
 */

#include "PerfUI/PerfUI.h"
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
    std::cout << "  PerfUI Clean Client: Texture Management Test Suite     \n";
    std::cout << "=========================================================\n\n";

    PerfUI::MockRenderBackend mockBackend;
    PerfUI::UIContext context;
    context.setRenderBackend(&mockBackend);

    // -------------------------------------------------------------
    // Test 1: Load Texture via Backend and UIContext
    // -------------------------------------------------------------
    std::cout << "Test 1: Texture Loading and Dimensions...\n";
    PerfUI::TextureId tex1 = context.loadTexture("Data/Textures/Icons/Health.png");
    TEST_ASSERT(tex1 > 0, "loadTexture returned valid TextureId");

    PerfUI::Dimensions size1 = context.getTextureSize(tex1);
    TEST_ASSERT(size1.width == 64.0f && size1.height == 64.0f, "getTextureSize returned expected dimensions");

    // -------------------------------------------------------------
    // Test 2: Dynamic Texture Creation and Updating
    // -------------------------------------------------------------
    std::cout << "\nTest 2: Dynamic Texture Lifecycle...\n";
    std::vector<uint8_t> pixels(32 * 32 * 4, 255);
    PerfUI::TextureId dynTex = context.createDynamicTexture(32, 32, pixels.data());
    TEST_ASSERT(dynTex > 0, "createDynamicTexture returned valid TextureId");

    PerfUI::Dimensions dynSize = context.getTextureSize(dynTex);
    TEST_ASSERT(dynSize.width == 32.0f && dynSize.height == 32.0f, "dynamic texture has correct initial size");

    // Update dynamic texture
    std::vector<uint8_t> newPixels(64 * 64 * 4, 128);
    bool updated = context.updateDynamicTexture(dynTex, 64, 64, newPixels.data());
    TEST_ASSERT(updated, "updateDynamicTexture succeeded");

    PerfUI::Dimensions updatedSize = context.getTextureSize(dynTex);
    TEST_ASSERT(updatedSize.width == 64.0f && updatedSize.height == 64.0f, "updateDynamicTexture updated dimensions");

    // -------------------------------------------------------------
    // Test 3: Destruction
    // -------------------------------------------------------------
    std::cout << "\nTest 3: Texture Destruction...\n";
    TEST_ASSERT(mockBackend.loadedTextureCount() == 2, "2 textures currently active");
    context.destroyTexture(tex1);
    TEST_ASSERT(mockBackend.loadedTextureCount() == 1, "destroyTexture decremented active count");

    PerfUI::Dimensions destroyedSize = context.getTextureSize(tex1);
    TEST_ASSERT(destroyedSize.width == 0.0f && destroyedSize.height == 0.0f, "destroyed texture returns 0x0 size");

    // -------------------------------------------------------------
    // Test 4: Drawing with TextureId
    // -------------------------------------------------------------
    std::cout << "\nTest 4: Draw Calls with TextureId...\n";
    mockBackend.beginFrame();
    mockBackend.drawImage(dynTex, PerfUI::Rect{ 10, 10, 64, 64 });
    mockBackend.drawImageRotated(dynTex, PerfUI::Point{ 100, 100 }, PerfUI::Dimensions{ 64, 64 }, 1.57f);
    mockBackend.drawImageUV(dynTex, PerfUI::Rect{ 200, 200, 32, 32 }, PerfUI::Point{ 0, 0 }, PerfUI::Point{ 0.5f, 0.5f });
    mockBackend.drawImageQuad(dynTex, { 0, 0 }, { 10, 0 }, { 10, 10 }, { 0, 10 });
    mockBackend.endFrame();

    TEST_ASSERT(mockBackend.drawCallCount() == 4, "all 4 texture draw calls recorded");

    std::cout << "\n=========================================================\n";
    std::cout << "  Summary: " << g_testsPassed << " passed, " << g_testsFailed << " failed.\n";
    std::cout << "=========================================================\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
