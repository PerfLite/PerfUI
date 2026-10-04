/**
 * @file MarkupTest.cpp
 * @brief Unit tests for PerfUI_Markup: declarative XML UI parsing, layout mapping,
 * callbacks, error resilience, and hot-reload state restoration.
 */

#include "PerfUI/PerfUI.h"
#include "PerfUI/Markup/MarkupLoader.h"
#include "PerfUI/Markup/WidgetRegistry.h"
#include "PerfUI/LayoutEngine.h"
#include "../src/backends/mock/MockRenderBackend.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <thread>

using namespace PerfUI;

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

static bool approxEqual(float a, float b, float eps = 0.001f) {
    return std::abs(a - b) < eps;
}

int main() {
    std::cout << "=========================================================\n";
    std::cout << "       PerfUI Markup & Hot-Reload Test Suite             \n";
    std::cout << "=========================================================\n\n";

    // -------------------------------------------------------------
    // Test 1: Load Spec Example (Tree structure, children, layout props)
    // -------------------------------------------------------------
    std::cout << "Test 1: Spec Example XML Loading...\n";
    {
        PerfUI::UIContext context;
        context.setViewportSize({ 1920.0f, 1080.0f });
        PerfUI::MarkupLoader loader(context);

        const std::string_view xml = R"(
<UI>
  <Panel name="Main" direction="column" width="400" padding="16" gap="8">
    <Text text="Настройки" font="title"/>
    <Slider name="Volume" min="0" max="100" bind="settings.volume"/>
    <Checkbox label="Показывать профайлер" bind="settings.showProfiler"/>
    <Panel direction="row" gap="8" justify="end">
      <Button text="Отмена" onClick="cancel"/>
      <Button text="Сохранить" onClick="save" class="primary"/>
    </Panel>
  </Panel>
</UI>
)";

        UIElement* root = loader.loadString(xml);
        TEST_ASSERT(root != nullptr, "Markup loaded root element");
        TEST_ASSERT(root->name() == "Main", "Root panel name is 'Main'");
        TEST_ASSERT(root->layout().width().mode == PerfUI::SizeMode::Fixed, "Width mode is Fixed");
        TEST_ASSERT(approxEqual(root->layout().width().value, 400.0f), "Width value is 400px");
        TEST_ASSERT(approxEqual(root->layout().padding().left, 16.0f), "Padding uniform is 16px");
        TEST_ASSERT(approxEqual(root->layout().gap(), 8.0f), "Gap is 8px");
        TEST_ASSERT(root->layout().direction() == PerfUI::LayoutDirection::Vertical, "Direction is column");
        TEST_ASSERT(root->children().size() == 4, "Root has 4 children (Text, Slider, Checkbox, Panel)");

        UIElement* volumeSlider = loader.findByName("Volume");
        TEST_ASSERT(volumeSlider != nullptr, "findByName found 'Volume' slider");

        UIElement* saveBtn = nullptr;
        for (const auto& child : root->children()) {
            if (auto* rowPanel = dynamic_cast<PerfUI::Panel*>(child.get())) {
                for (const auto& btnChild : rowPanel->children()) {
                    if (btnChild->hasClass("primary")) {
                        saveBtn = btnChild.get();
                    }
                }
            }
        }
        TEST_ASSERT(saveBtn != nullptr, "Found button with class 'primary'");
        TEST_ASSERT(saveBtn->hasClass("primary"), "Button reports hasClass('primary') == true");
    }

    // -------------------------------------------------------------
    // Test 2: Layout Attributes & Theme Tokens
    // -------------------------------------------------------------
    std::cout << "\nTest 2: Layout Attributes, Constraints & Theme Tokens...\n";
    {
        PerfUI::UIContext context;
        context.setViewportSize({ 1920.0f, 1080.0f });
        PerfUI::MarkupLoader loader(context);

        const std::string_view xml = R"(
<UI>
  <Panel name="TokenTest" width="50%" flex="2" padding="1 2 3 4" gap="$spacingSM">
    <Text text="Token Label" font="$fontTitle" color="$borderFocus"/>
  </Panel>
</UI>
)";

        UIElement* root = loader.loadString(xml);
        TEST_ASSERT(root != nullptr, "Token test root created");
        TEST_ASSERT(root->layout().width().mode == PerfUI::SizeMode::Percent, "Width mode is Percent");
        TEST_ASSERT(approxEqual(root->layout().width().value, 50.0f), "Width value is 50%");
        TEST_ASSERT(approxEqual(root->layout().flexGrow(), 2.0f), "flex is 2.0");
        TEST_ASSERT(approxEqual(root->layout().padding().left, 1.0f), "Padding left is 1");
        TEST_ASSERT(approxEqual(root->layout().padding().top, 2.0f), "Padding top is 2");
        TEST_ASSERT(approxEqual(root->layout().padding().right, 3.0f), "Padding right is 3");
        TEST_ASSERT(approxEqual(root->layout().padding().bottom, 4.0f), "Padding bottom is 4");
        TEST_ASSERT(approxEqual(root->layout().gap(), 8.0f), "gap='$spacingSM' resolved to 8.0");

        auto* txt = dynamic_cast<PerfUI::Text*>(root->children()[0].get());
        TEST_ASSERT(txt != nullptr, "First child is Text");
        TEST_ASSERT(approxEqual(txt->style().fontSize, 20.0f), "font='$fontTitle' resolved to 20.0");
        TEST_ASSERT(txt->style().color.r == 212 && txt->style().color.g == 175, "color='$borderFocus' resolved to Nordic Gold");
    }

    // -------------------------------------------------------------
    // Test 3: Unknown Tag and Attribute Resilience
    // -------------------------------------------------------------
    std::cout << "\nTest 3: Resilience on Unknown Tags and Attributes...\n";
    {
        PerfUI::UIContext context;
        PerfUI::MarkupLoader loader(context);

        const std::string_view xml = R"(
<UI>
  <Panel name="RobustPanel">
    <FutureAlienWidget fancyFeature="true"/>
    <Button name="ValidBtn" unknownAttribute="random_value" text="Click Me"/>
  </Panel>
</UI>
)";

        UIElement* root = loader.loadString(xml);
        TEST_ASSERT(root != nullptr, "Parsing continues despite unknown tag");
        auto result = loader.lastResult();
        TEST_ASSERT(!result.warnings.empty(), "Warnings recorded for unknown tag and attribute");

        UIElement* btn = loader.findByName("ValidBtn");
        TEST_ASSERT(btn != nullptr, "Valid Button element was created and found");
        auto* buttonWidget = dynamic_cast<PerfUI::Button*>(btn);
        TEST_ASSERT(buttonWidget && buttonWidget->text() == "Click Me", "Button retained valid text");
    }

    // -------------------------------------------------------------
    // Test 4: Callback Binding & Simulated Click Dispatch
    // -------------------------------------------------------------
    std::cout << "\nTest 4: Callback Binding (bindCallback) & Click Dispatch...\n";
    {
        PerfUI::UIContext context;
        PerfUI::MarkupLoader loader(context);

        bool saveCalled = false;
        loader.bindCallback("saveAction", [&saveCalled]() {
            saveCalled = true;
        });

        const std::string_view xml = R"(
<UI>
  <Panel name="Actions">
    <Button name="BtnSave" text="Save" onClick="saveAction"/>
  </Panel>
</UI>
)";

        UIElement* root = loader.loadString(xml);
        TEST_ASSERT(root != nullptr, "Loaded action panel");

        auto* btn = dynamic_cast<PerfUI::Button*>(loader.findByName("BtnSave"));
        TEST_ASSERT(btn != nullptr, "Found BtnSave");

        // Simulate click
        btn->onPointerDown({ 10.0f, 10.0f });
        btn->onPointerUp({ 10.0f, 10.0f });

        TEST_ASSERT(saveCalled, "Bound callback 'saveAction' successfully executed on click");
    }

    // -------------------------------------------------------------
    // Test 5: Unregistered Callback Graceful Fallback
    // -------------------------------------------------------------
    std::cout << "\nTest 5: Unregistered Callback Graceful Handling...\n";
    {
        PerfUI::UIContext context;
        PerfUI::MarkupLoader loader(context);

        const std::string_view xml = R"(
<UI>
  <Panel>
    <Button name="BtnUnbound" text="Noop" onClick="nonExistentCallback"/>
  </Panel>
</UI>
)";

        UIElement* root = loader.loadString(xml);
        TEST_ASSERT(root != nullptr, "Loaded tree with unbound callback");
        auto* btn = dynamic_cast<PerfUI::Button*>(loader.findByName("BtnUnbound"));
        TEST_ASSERT(btn != nullptr, "Found button");

        // Simulating click should NOT crash
        btn->onPointerDown({ 5.0f, 5.0f });
        btn->onPointerUp({ 5.0f, 5.0f });
        TEST_ASSERT(true, "Clicked button with unbound callback without throwing or crashing");

        auto result = loader.lastResult();
        bool hasUnboundWarn = false;
        for (const auto& w : result.warnings) {
            if (w.find("unbound callback") != std::string::npos) {
                hasUnboundWarn = true;
                break;
            }
        }
        TEST_ASSERT(hasUnboundWarn, "Warning logged for unbound callback");
    }

    // -------------------------------------------------------------
    // Test 6: Broken XML -> Returns nullptr & Preserves Previous Tree
    // -------------------------------------------------------------
    std::cout << "\nTest 6: Malformed XML Error Recovery...\n";
    {
        PerfUI::UIContext context;
        PerfUI::MarkupLoader loader(context);

        // Load valid initial tree
        UIElement* initial = loader.loadString("<UI><Panel name='InitialTree'><Text text='Healthy'/></Panel></UI>");
        TEST_ASSERT(initial != nullptr, "Initial valid tree loaded");
        TEST_ASSERT(loader.findByName("InitialTree") != nullptr, "InitialTree exists");

        // Attempt to load malformed XML
        UIElement* broken = loader.loadString("<UI><Panel name='Broken'><Text>unclosed tags");
        TEST_ASSERT(broken == nullptr, "Malformed XML returned nullptr");
        TEST_ASSERT(!loader.lastResult().success, "lastResult().success is false");
        TEST_ASSERT(!loader.lastResult().errors.empty(), "Error message recorded with line information");

        // Previous tree must remain untouched
        TEST_ASSERT(loader.findByName("InitialTree") != nullptr, "Previous tree remains valid and intact");
    }

    // -------------------------------------------------------------
    // Test 7: Hot-Reload Polling & State Restoration
    // -------------------------------------------------------------
    std::cout << "\nTest 7: Hot-Reload Polling & Focus / Scroll State Preservation...\n";
    {
        PerfUI::UIContext context;
        context.setViewportSize({ 1920.0f, 1080.0f });
        PerfUI::MarkupLoader loader(context);
        loader.setPollInterval(std::chrono::milliseconds(0)); // disable throttle for unit test

        auto tempDir = std::filesystem::temp_directory_path() / "perfui_test_markup";
        std::filesystem::create_directories(tempDir);
        auto xmlFile = tempDir / "hot_reload_menu.xml";

        // Version 1 of file
        {
            std::ofstream ofs(xmlFile);
            ofs << R"(
<UI>
  <Panel name="RootHot">
    <ScrollView name="MyScroll" offset="120.0">
      <Button name="BtnFocusTarget" text="Target V1"/>
    </ScrollView>
  </Panel>
</UI>
)";
        }

        UIElement* loadedV1 = loader.loadFile(xmlFile);
        TEST_ASSERT(loadedV1 != nullptr, "Loaded V1 from temp file");
        loader.enableHotReload(true);

        auto* btnTarget = loader.findByName("BtnFocusTarget");
        TEST_ASSERT(btnTarget != nullptr, "Found focus target in V1");
        btnTarget->setFocusable(true);
        context.setFocus(btnTarget);
        TEST_ASSERT(context.focusedElement() == btnTarget, "BtnFocusTarget is focused in context");

        auto* sv = dynamic_cast<PerfUI::ScrollView*>(loader.findByName("MyScroll"));
        TEST_ASSERT(sv != nullptr, "Found MyScroll");
        sv->scrollTo(45.0f); // Set custom scroll offset

        // Version 2 of file with updated button text
        // Ensure write time change by waiting a brief moment
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        {
            std::ofstream ofs(xmlFile, std::ios::trunc);
            ofs << "<UI>\n"
                << "  <Panel name=\"RootHot\">\n"
                << "    <ScrollView name=\"MyScroll\" offset=\"0.0\">\n"
                << "      <Button name=\"BtnFocusTarget\" text=\"Target V2 (Reloaded)\"/>\n"
                << "    </ScrollView>\n"
                << "  </Panel>\n"
                << "</UI>\n";
        }

        // Poll to trigger hot-reload
        loader.poll();

        auto* newBtn = dynamic_cast<PerfUI::Button*>(loader.findByName("BtnFocusTarget"));
        TEST_ASSERT(newBtn != nullptr, "Found focus target in reloaded V2");
        TEST_ASSERT(newBtn->text() == "Target V2 (Reloaded)", "Button text updated via hot reload");
        TEST_ASSERT(context.focusedElement() == newBtn, "Focus preserved on BtnFocusTarget across hot-reload!");

        auto* newSv = dynamic_cast<PerfUI::ScrollView*>(loader.findByName("MyScroll"));
        TEST_ASSERT(newSv != nullptr, "Found MyScroll in reloaded V2");
        TEST_ASSERT(approxEqual(newSv->scrollOffset(), 45.0f), "ScrollView scroll offset (45px) preserved!");

        // Cleanup temp file
        std::error_code ec;
        std::filesystem::remove_all(tempDir, ec);
    }

    // -------------------------------------------------------------
    // Test 8: Layout Equivalence (Markup vs. Manual C++ Hierarchy)
    // -------------------------------------------------------------
    std::cout << "\nTest 8: Layout Equivalence (XML Markup vs Manual C++ Tree)...\n";
    {
        PerfUI::Dimensions viewport{ 800.0f, 600.0f };

        // 1. Manual C++ tree
        PerfUI::UIContext manualCtx;
        manualCtx.setViewportSize(viewport);
        auto* manualPanel = manualCtx.root()->add<PerfUI::Panel>("Card");
        manualPanel->layout()
            .direction(PerfUI::LayoutDirection::Vertical)
            .width(PerfUI::DimensionConstraint::Fixed(300.0f))
            .padding(10.0f)
            .gap(5.0f);
        auto* manualBtn1 = manualPanel->add<PerfUI::Button>("Btn1");
        manualBtn1->layout().height(PerfUI::DimensionConstraint::Fixed(30.0f));
        auto* manualBtn2 = manualPanel->add<PerfUI::Button>("Btn2");
        manualBtn2->layout().height(PerfUI::DimensionConstraint::Fixed(40.0f));

        PerfUI::MockRenderBackend mock1;
        manualCtx.render(mock1);

        // 2. XML Markup tree
        PerfUI::UIContext xmlCtx;
        xmlCtx.setViewportSize(viewport);
        PerfUI::MarkupLoader loader(xmlCtx);

        const std::string_view xml = R"(
<UI>
  <Panel name="Card" direction="column" width="300" padding="10" gap="5">
    <Button name="Btn1" text="Btn1" height="30"/>
    <Button name="Btn2" text="Btn2" height="40"/>
  </Panel>
</UI>
)";

        UIElement* xmlRoot = loader.loadString(xml);
        PerfUI::MockRenderBackend mock2;
        xmlCtx.render(mock2);

        TEST_ASSERT(xmlRoot != nullptr, "XML tree loaded");
        auto* xmlBtn1 = loader.findByName("Btn1");
        auto* xmlBtn2 = loader.findByName("Btn2");

        TEST_ASSERT(approxEqual(xmlRoot->bounds().width, manualPanel->bounds().width), "Panel widths match (300px)");
        TEST_ASSERT(approxEqual(xmlRoot->bounds().height, manualPanel->bounds().height), "Panel heights match");
        TEST_ASSERT(approxEqual(xmlBtn1->bounds().x, manualBtn1->bounds().x), "Btn1 X positions match");
        TEST_ASSERT(approxEqual(xmlBtn1->bounds().y, manualBtn1->bounds().y), "Btn1 Y positions match");
        TEST_ASSERT(approxEqual(xmlBtn1->bounds().height, manualBtn1->bounds().height), "Btn1 heights match (30px)");
        TEST_ASSERT(approxEqual(xmlBtn2->bounds().y, manualBtn2->bounds().y), "Btn2 Y positions match");
        TEST_ASSERT(approxEqual(xmlBtn2->bounds().height, manualBtn2->bounds().height), "Btn2 heights match (40px)");
    }

    // -------------------------------------------------------------
    // Test Summary
    // -------------------------------------------------------------
    std::cout << "\n=========================================================\n";
    std::cout << "  Markup Test Results: " << g_testsPassed << " passed, " << g_testsFailed << " failed.\n";
    std::cout << "=========================================================\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
