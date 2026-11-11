/**
 * @file MarkupTest.cpp
 * @brief Unit tests for PerfUI_Markup: declarative XML UI parsing, layout mapping,
 * callbacks, error resilience, and hot-reload state restoration.
 */

#include "PerfUI/PerfUI.h"
#include "PerfUI/Markup/MarkupLoader.h"
#include "PerfUI/Markup/WidgetRegistry.h"
#include "PerfUI/LayoutEngine.h"
#include "PerfUI/MapCanvas.h"
#include "PerfUI/WorldAnchor.h"
#include "PerfUI/SettingsView.h"
#include "PerfUI/Localization.h"
#include "PerfUI/Style/StyleSheet.h"
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
    <Text text="Settings" font="title"/>
    <Slider name="Volume" min="0" max="100" bind="settings.volume"/>
    <Checkbox label="Show profiler" bind="settings.showProfiler"/>
    <Panel direction="row" gap="8" justify="end">
      <Button text="Cancel" onClick="cancel"/>
      <Button text="Save" onClick="save" class="primary"/>
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
    <Text name="LiveValue" bind="player.xp" text="waiting"/>
  </Panel>
</UI>
)";
        }

        UIElement* loadedV1 = loader.loadFile(xmlFile);
        TEST_ASSERT(loadedV1 != nullptr, "Loaded V1 from temp file");
        loader.setBinding("player.xp", "5/100");
        loader.enableHotReload(true);
        int beforeReloads = 0;
        int afterReloads = 0;
        loader.onBeforeReload([&](UIElement* oldRoot) {
            ++beforeReloads;
            TEST_ASSERT(oldRoot == loadedV1, "Before-reload callback receives live old root");
            TEST_ASSERT(loader.findByName("BtnFocusTarget") != nullptr, "Old elements remain available before reload");
        });
        loader.onAfterReload([&](UIElement* newRoot) {
            ++afterReloads;
            TEST_ASSERT(newRoot == loader.loadedRoot() && newRoot != loadedV1, "After-reload callback receives new root");
            TEST_ASSERT(context.focusedElement() == loader.findByName("BtnFocusTarget"), "Focus is restored before after-reload callback");
        });

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
                << "    <Text name=\"LiveValue\" bind=\"player.xp\" text=\"waiting\"/>\n"
                << "  </Panel>\n"
                << "</UI>\n";
        }

        // Poll to trigger hot-reload
        loader.poll();
        TEST_ASSERT(beforeReloads == 1 && afterReloads == 1, "Reload callbacks fire once in order");

        auto* newBtn = dynamic_cast<PerfUI::Button*>(loader.findByName("BtnFocusTarget"));
        TEST_ASSERT(newBtn != nullptr, "Found focus target in reloaded V2");
        TEST_ASSERT(newBtn->text() == "Target V2 (Reloaded)", "Button text updated via hot reload");
        TEST_ASSERT(context.focusedElement() == newBtn, "Focus preserved on BtnFocusTarget across hot-reload!");
        auto* liveValue = dynamic_cast<PerfUI::Text*>(loader.findByName("LiveValue"));
        TEST_ASSERT(liveValue && liveValue->text() == "5/100", "Bound value survives XML hot reload");

        auto* newSv = dynamic_cast<PerfUI::ScrollView*>(loader.findByName("MyScroll"));
        TEST_ASSERT(newSv != nullptr, "Found MyScroll in reloaded V2");
        TEST_ASSERT(approxEqual(newSv->scrollOffset(), 45.0f), "ScrollView scroll offset (45px) preserved!");

        // Cleanup temp file
        std::error_code ec;
        std::filesystem::remove_all(tempDir, ec);
    }

    // Absolute anchors position HUD elements without per-frame C++ coordinates.
    {
        PerfUI::UIContext context;
        context.setViewportSize({ 800.0f, 600.0f });
        PerfUI::MarkupLoader loader(context);
        loader.loadString(R"(<UI><Panel name="AnchorRoot" width="800" height="600"><Panel name="Anchored" position="absolute" width="200" height="50" center-x="true" bottom="24"/></Panel></UI>)");
        context.update(0.0f);
        auto* anchored = loader.findByName("Anchored");
        TEST_ASSERT(anchored != nullptr, "Anchored panel loaded");
        if (anchored) {
            TEST_ASSERT(approxEqual(anchored->bounds().x, 300.0f), "center-x positions absolute panel");
            TEST_ASSERT(approxEqual(anchored->bounds().y, 526.0f), "bottom offset positions absolute panel");
        }
    }

    {
        PerfUI::UIContext context;
        PerfUI::MockRenderBackend backend;
        context.setRenderBackend(&backend);
        context.setViewportSize({ 800.0f, 600.0f });
        PerfUI::MarkupLoader loader(context);
        const auto xmlFile = std::filesystem::temp_directory_path() / "perfui_panel_image_test.xml";
        {
            std::ofstream out(xmlFile);
            out << "<UI><Panel name=\"ImagePanel\" width=\"200\" height=\"80\" bg-image=\"frame.png\" bg-slice=\"10 10 10 10\" bg-corner=\"8 8 8 8\"/></UI>";
        }
        loader.loadFile(xmlFile);
        auto* panel = dynamic_cast<PerfUI::Panel*>(loader.findByName("ImagePanel"));
        TEST_ASSERT(panel && panel->backgroundImage() != 0, "Panel background image loaded from XML");
        TEST_ASSERT(backend.loadedTextureCount() == 1 &&
                    backend.mockTextures().begin()->second.filePath == (xmlFile.parent_path() / "frame.png").string(),
                    "Relative panel image path resolves beside XML");
        context.render(backend);
        int imageParts = 0;
        for (const auto& command : backend.commands()) {
            if (command.type == PerfUI::MockRenderBackend::DrawCommand::Type::ImageUV) ++imageParts;
        }
        TEST_ASSERT(imageParts == 9, "Nine-slice panel renders nine image regions");
        std::filesystem::remove(xmlFile);
    }
    {
        PerfUI::UIContext context;
        PerfUI::MarkupLoader loader(context);
        loader.setBinding("player.xp", "0/100");
        loader.loadString(R"(<UI><Text name="XpLabel" bind="player.xp" text="waiting"/></UI>)");
        auto* label = dynamic_cast<PerfUI::Text*>(loader.findByName("XpLabel"));
        TEST_ASSERT(label && label->text() == "0/100", "Initial bound value replaces placeholder");
        loader.setBinding("player.xp", "50/100");
        TEST_ASSERT(label && label->text() == "50/100", "Bound Text updates without rebuilding XML");
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
    // Test 9: MapCanvas tag registration
    // -------------------------------------------------------------
    std::cout << "\nTest 9: MapCanvas markup tag...\n";
    {
        TEST_ASSERT(WidgetRegistry::instance().hasWidget("MapCanvas"), "MapCanvas tag registered");

        PerfUI::UIContext context;
        context.setViewportSize({ 800.0f, 600.0f });
        PerfUI::MarkupLoader loader(context);

        const std::string_view xml = R"(
<UI>
  <Panel name="MapHost" direction="column" width="400" padding="10">
    <MapCanvas name="Radar" width="320" height="200" mode="world" radar-range="450" show-grid="true" show-distances="false"/>
  </Panel>
</UI>
)";

        UIElement* root = loader.loadString(xml);
        TEST_ASSERT(root != nullptr, "Panel with MapCanvas loaded");

        auto* map = dynamic_cast<PerfUI::MapCanvas*>(loader.findByName("Radar"));
        TEST_ASSERT(map != nullptr, "MapCanvas element created by tag");
        if (map) {
            // Source is not assigned: rendering must proceed without throwing or crashing.
            PerfUI::MockRenderBackend mock;
            context.render(mock);

            TEST_ASSERT(map->mode() == PerfUI::MapMode::World, "mode=\"world\" applied");
            TEST_ASSERT(approxEqual(map->radarRange(), 450.0f), "radar-range=\"450\" applied");
            TEST_ASSERT(map->showsGrid(), "show-grid=\"true\" applied");
            TEST_ASSERT(!map->showsDistances(), "show-distances=\"false\" applied");
            TEST_ASSERT(approxEqual(map->bounds().width, 320.0f), "MapCanvas honours width=\"320\"");
            TEST_ASSERT(approxEqual(map->bounds().height, 200.0f), "MapCanvas honours height=\"200\"");
            TEST_ASSERT(map->projectedCount() == 0, "Canvas without data source projects nothing");
        }
    }

    // -------------------------------------------------------------
    // Test 10: WorldAnchor Tag Parsing and Attributes
    // -------------------------------------------------------------
    std::cout << "\nTest 10: WorldAnchor Tag Parsing...\n";
    {
        PerfUI::UIContext context;
        PerfUI::MarkupLoader loader(context);

        const std::string xml = R"(
<UI>
  <WorldAnchor name="BossAnchor" targetId="100" align="center" offset="10 -20"
               distanceScaling="true" minDist="50" maxDist="2000" minScale="0.25" maxScale="1.5"
               distanceAlpha="true" minAlpha="0.1" maxAlpha="0.9"
               edgeClamping="true" screenMargin="45">
    <Text name="BossTitle" text="Alduin" />
  </WorldAnchor>
</UI>
)";

        UIElement* root = loader.loadString(xml);
        TEST_ASSERT(root != nullptr, "WorldAnchor loaded successfully");

        auto* anchor = dynamic_cast<PerfUI::WorldAnchor*>(loader.findByName("BossAnchor"));
        TEST_ASSERT(anchor != nullptr, "WorldAnchor element created by tag");
        if (anchor) {
            TEST_ASSERT(anchor->targetId() == 100, "targetId=\"100\" parsed");
            TEST_ASSERT(anchor->anchorAlignment() == PerfUI::AnchorAlignment::Center, "align=\"center\" parsed");
            TEST_ASSERT(approxEqual(anchor->screenOffset().x, 10.0f) && approxEqual(anchor->screenOffset().y, -20.0f), "offset=\"10 -20\" parsed");
            TEST_ASSERT(anchor->isDistanceScalingEnabled(), "distanceScaling=\"true\" parsed");
            TEST_ASSERT(anchor->isDistanceAlphaEnabled(), "distanceAlpha=\"true\" parsed");
            TEST_ASSERT(anchor->isEdgeClampingEnabled(), "edgeClamping=\"true\" parsed");
            TEST_ASSERT(approxEqual(anchor->screenMargin(), 45.0f), "screenMargin=\"45\" parsed");
            TEST_ASSERT(anchor->children().size() == 1, "Child Text element attached to WorldAnchor");
        }
    }

    // -------------------------------------------------------------
    // Test 11: Image UV and FlipY Tag Parsing
    // -------------------------------------------------------------
    std::cout << "\nTest 11: Image UV and FlipY Tag Parsing...\n";
    {
        PerfUI::UIContext context;
        PerfUI::MarkupLoader loader(context);

        const std::string xml = R"(
<UI>
  <Panel>
    <Image name="Preview3D" texture="1234" uv="0.1 0.2 0.9 0.8" flip-y="true" />
  </Panel>
</UI>
)";

        UIElement* root = loader.loadString(xml);
        TEST_ASSERT(root != nullptr, "Image markup loaded");

        auto* img = dynamic_cast<PerfUI::Image*>(loader.findByName("Preview3D"));
        TEST_ASSERT(img != nullptr, "Image found");
        if (img) {
            TEST_ASSERT(img->texture() == 1234, "texture=\"1234\" parsed");
            TEST_ASSERT(img->flipY() == true, "flip-y=\"true\" parsed");
            TEST_ASSERT(approxEqual(img->uv0().x, 0.1f) && approxEqual(img->uv0().y, 0.2f), "uv0 parsed");
            TEST_ASSERT(approxEqual(img->uv1().x, 0.9f) && approxEqual(img->uv1().y, 0.8f), "uv1 parsed");
        }
    }

    // -------------------------------------------------------------
    // Test 12: Localization in Markup ($KEY expansion)
    // -------------------------------------------------------------
    std::cout << "\nTest 12: Localization in Markup ($KEY expansion)...\n";
    {
        auto& loc = PerfUI::LocalizationService::instance();
        loc.clear();
        loc.setLanguage("en");
        loc.setTranslation("en", "BT_START", "Start Adventure");
        loc.setTranslation("en", "PL_SEARCH", "Type to filter items...");
        loc.setTranslation("ru", "BT_START", "Начать приключение");
        loc.setTranslation("ru", "PL_SEARCH", "Поиск предметов...");

        PerfUI::UIContext context;
        PerfUI::MarkupLoader loader(context);

        const std::string xml = R"(
<UI>
  <Panel>
    <Button name="BtnStart" text="$BT_START" />
    <TextInput name="SearchBox" placeholder="$PL_SEARCH" />
  </Panel>
</UI>
)";

        loader.loadString(xml);
        auto* btn = dynamic_cast<PerfUI::Button*>(loader.findByName("BtnStart"));
        auto* input = dynamic_cast<PerfUI::TextInput*>(loader.findByName("SearchBox"));

        TEST_ASSERT(btn && btn->text() == "Start Adventure", "Button text localized in EN");
        TEST_ASSERT(input && input->placeholder() == "Type to filter items...", "TextInput placeholder localized in EN");

        // Switch to Russian and reload
        loc.setLanguage("ru");
        loader.loadString(xml);
        auto* btnRu = dynamic_cast<PerfUI::Button*>(loader.findByName("BtnStart"));
        auto* inputRu = dynamic_cast<PerfUI::TextInput*>(loader.findByName("SearchBox"));

        TEST_ASSERT(btnRu && btnRu->text() == "Начать приключение", "Button text localized in RU");
        TEST_ASSERT(inputRu && inputRu->placeholder() == "Поиск предметов...", "TextInput placeholder localized in RU");

        // Test JSON dictionary content parsing
        const std::string jsonDict = R"({
            "UI_SAVE": "Сохранить игру",
            "UI_EXIT": "Выйти в меню"
        })";
        loc.loadDictionaryContent("ru", jsonDict);
        TEST_ASSERT(loc.translate("UI_SAVE") == "Сохранить игру", "JSON dictionary parsed UI_SAVE");
        TEST_ASSERT(loc.translate("UI_EXIT") == "Выйти в меню", "JSON dictionary parsed UI_EXIT");

        // Test SKSE key-value tab dictionary parsing
        const std::string skseDict = "$QUEST_NAME\tДраконоборец\n$GOLD_AMOUNT\t1000 золота\n";
        loc.loadDictionaryContent("ru", skseDict);
        TEST_ASSERT(loc.translate("QUEST_NAME") == "Драконоборец", "SKSE tab dictionary parsed QUEST_NAME");
        TEST_ASSERT(loc.translate("$GOLD_AMOUNT") == "1000 золота", "SKSE tab dictionary parsed GOLD_AMOUNT");

        // Test loadTranslationsFromFolder with multi-mod JSON merge
        std::filesystem::path tempTransDir = std::filesystem::temp_directory_path() / "PerfUI_Test_Trans";
        std::filesystem::create_directories(tempTransDir);
        {
            std::ofstream f1(tempTransDir / "ru.json");
            f1 << R"({ "MOD_ITEM": "Двемерский шлем" })";
        }
        {
            std::ofstream f2(tempTransDir / "ExtraMod_ru.json");
            f2 << R"({ "EXTRA_GOLD": "500 монет" })";
        }
        loc.loadTranslationsFromFolder(tempTransDir);
        TEST_ASSERT(loc.translate("MOD_ITEM") == "Двемерский шлем", "Folder loaded ru.json MOD_ITEM");
        TEST_ASSERT(loc.translate("EXTRA_GOLD") == "500 монет", "Folder merged ExtraMod_ru.json EXTRA_GOLD");
        std::error_code ec;
        std::filesystem::remove_all(tempTransDir, ec);
    }

    // -------------------------------------------------------------
    // Test 13: SettingsView & SettingsSection Markup Parsing
    // -------------------------------------------------------------
    {
        std::cout << "\nTest 13: SettingsView & SettingsSection Markup Parsing...\n";
        PerfUI::UIContext context;
        context.setViewportSize({ 1920.0f, 1080.0f });
        PerfUI::MarkupLoader loader(context);
        const std::string xml = R"(
            <Panel name="Root" width="800" height="600">
                <SettingsView name="MySettings" flex="1">
                    <SettingsSection name="GfxSection" title="GRAPHICS & VISUALS">
                        <SettingToggle name="BlurToggle" label="Motion Blur" value="true" tooltip="Enable blur" />
                        <SettingSlider name="OpacitySlider" label="Window Opacity" min="0.2" max="1.0" value="0.85" format="%.2f" />
                        <SettingChoice name="ThemeChoice" label="Active Theme" options="Nordic Gold, Imperial Ruby, Winterhold Frost" selected="1" />
                        <SettingButton name="ResetBtn" label="Reset Options" button="Reset All" />
                    </SettingsSection>
                </SettingsView>
            </Panel>
        )";

        PerfUI::UIElement* root = loader.loadString(xml);
        TEST_ASSERT(root != nullptr, "SettingsView XML root loaded");

        auto* sv = dynamic_cast<PerfUI::SettingsView*>(loader.findByName("MySettings"));
        TEST_ASSERT(sv != nullptr, "SettingsView element created");

        auto* sec = dynamic_cast<PerfUI::SettingsSection*>(loader.findByName("GfxSection"));
        TEST_ASSERT(sec != nullptr, "SettingsSection element created");
        TEST_ASSERT(sec && sec->title() == "GRAPHICS & VISUALS", "SettingsSection title matches");

        auto* toggle = dynamic_cast<PerfUI::Checkbox*>(loader.findByName("BlurToggle"));
        TEST_ASSERT(toggle != nullptr && toggle->isChecked() == true, "SettingToggle created and checked");

        auto* slider = dynamic_cast<PerfUI::Slider*>(loader.findByName("OpacitySlider"));
        TEST_ASSERT(slider != nullptr && std::abs(slider->value() - 0.85f) < 0.01f, "SettingSlider value initialized to 0.85");

        auto* choice = dynamic_cast<PerfUI::ComboBox*>(loader.findByName("ThemeChoice"));
        TEST_ASSERT(choice != nullptr && choice->selectedIndex() == 1, "SettingChoice options parsed and index selected");

        auto* btn = dynamic_cast<PerfUI::Button*>(loader.findByName("ResetBtn"));
        TEST_ASSERT(btn != nullptr && btn->text() == "Reset All", "SettingButton created with button text");
    }

    // -------------------------------------------------------------
    // Test 14: XML Themes Loading, Inheritance & Token Resolution
    // -------------------------------------------------------------
    std::cout << "\nTest 14: XML Themes Loading, Inheritance & Token Resolution...\n";
    {
        PerfUI::UIContext context;
        PerfUI::MarkupLoader loader(context);

        const std::string themeXml = R"(
            <Theme name="Dwemer Brass" extends="Nordic Gold" apply="true">
                <Colors surfaceBase="#14100c" borderFocus="#ebaa32" textAccent="#ffc34b" />
                <Metrics radiusSmall="3" radiusMedium="6" radiusLarge="9" spacingSM="10" />
                <Typography fontTitle="22" fontBody="15" />
            </Theme>
        )";

        loader.loadString(themeXml);
        TEST_ASSERT(PerfUI::Theme::HasTheme("Dwemer Brass"), "Dwemer Brass registered in Theme registry");
        TEST_ASSERT(PerfUI::Theme::Current().colors.borderFocus == PerfUI::Color(0xeb, 0xaa, 0x32, 255), "Active theme borderFocus matches XML hex #ebaa32");
        TEST_ASSERT(PerfUI::Theme::Current().colors.textAccent == PerfUI::Color(0xff, 0xc3, 0x4b, 255), "Active theme textAccent matches XML hex #ffc34b");
        TEST_ASSERT(PerfUI::Theme::Current().metrics.spacingSM == 10.0f, "Active theme spacingSM matches XML 10.0");
        TEST_ASSERT(PerfUI::Theme::Current().metrics.radiusSmall == 3.0f, "Active theme radiusSmall matches XML 3.0");
        TEST_ASSERT(PerfUI::Theme::Current().typography.fontTitle == 22.0f, "Active theme fontTitle matches XML 22.0");

        // Now test that layout parser resolves tokens dynamically to the active XML theme
        const std::string layoutXml = R"(
            <Panel name="ThemeTokenTest" gap="$spacingSM" padding="0">
                <Text name="DwemerTitle" color="$borderFocus" font="$fontTitle" text="Dwemer Ruins" />
            </Panel>
        )";

        auto* root = loader.loadString(layoutXml);
        TEST_ASSERT(root != nullptr, "Layout using dynamic theme tokens loaded");
        auto* panel = dynamic_cast<PerfUI::Panel*>(root);
        TEST_ASSERT(panel != nullptr && panel->layout().gap() == 10.0f, "Panel gap='$spacingSM' dynamically resolved to 10.0f");

        auto* text = dynamic_cast<PerfUI::Text*>(loader.findByName("DwemerTitle"));
        TEST_ASSERT(text != nullptr && text->style().color == PerfUI::Color(0xeb, 0xaa, 0x32, 255), "Text color='$borderFocus' dynamically resolved to #ebaa32");
        TEST_ASSERT(text != nullptr && text->style().fontSize == 22.0f, "Text font='$fontTitle' dynamically resolved to 22.0f");
    }

    // -------------------------------------------------------------
    // Test 15: Stage 0 Markup Fast Fixes (opacity, transforms, id, image src)
    // -------------------------------------------------------------
    {
        std::cout << "\nTest 15: Stage 0 Markup Fast Fixes (opacity, transforms, id, image src)...\n";

        PerfUI::UIContext ctx;
        PerfUI::MarkupLoader loader(ctx);

        const std::string testXml = R"(
            <Panel id="RootPanel" opacity="0.8" width="200" height="100"
                   transform-offset="20px 50%" transform-scale="1.5 2.0" transform-pivot="center">
                <Panel id="ChildPanel" name="FallbackName" opacity="0.5" width="100" height="50"
                       transform-offset="10 -5" transform-scale="120%" transform-pivot="top left"
                       blend-mode="additive">
                    <Image id="BannerImg" src="textures/test_banner.png" tint="#FF8800" />
                </Panel>
                <Panel id="ClampedPanel" opacity="2.5" />
            </Panel>
        )";

        auto* root = loader.loadString(testXml);
        TEST_ASSERT(root != nullptr, "Stage 0 test markup loaded successfully");
        TEST_ASSERT(loader.lastResult().success, "Markup loaded without errors");
        TEST_ASSERT(loader.lastResult().warnings.empty(), "No warnings for Stage 0 attributes (opacity, transform-*, id, src)");

        // 1. id as name alias
        auto* rootPanel = loader.findByName("RootPanel");
        TEST_ASSERT(rootPanel != nullptr, "id='RootPanel' found via findByName");
        TEST_ASSERT(rootPanel->name() == "RootPanel", "element->name() is 'RootPanel'");

        // 2. id takes priority over name
        auto* childPanel = loader.findByName("ChildPanel");
        TEST_ASSERT(childPanel != nullptr, "id='ChildPanel' found despite name='FallbackName'");
        TEST_ASSERT(loader.findByName("FallbackName") == nullptr, "FallbackName was overridden by id");

        // 3. Opacity and effectiveOpacity multiplication
        TEST_ASSERT(std::abs(rootPanel->opacity() - 0.8f) < 0.001f, "RootPanel opacity is 0.8");
        TEST_ASSERT(std::abs(childPanel->opacity() - 0.5f) < 0.001f, "ChildPanel opacity is 0.5");
        TEST_ASSERT(std::abs(childPanel->effectiveOpacity() - (0.8f * 0.5f)) < 0.001f, "ChildPanel effectiveOpacity is 0.4 (0.8 * 0.5)");

        auto* clamped = loader.findByName("ClampedPanel");
        TEST_ASSERT(clamped != nullptr && clamped->opacity() == 1.0f, "Opacity > 1.0 clamped to 1.0");

        // 4. Transform scale & pivot
        TEST_ASSERT(rootPanel->renderScale().x == 1.5f && rootPanel->renderScale().y == 2.0f, "RootPanel scale parsed 1.5 2.0");
        TEST_ASSERT(rootPanel->pivot().x == 0.5f && rootPanel->pivot().y == 0.5f, "RootPanel pivot parsed center (0.5 0.5)");

        TEST_ASSERT(std::abs(childPanel->renderScale().x - 1.2f) < 0.001f, "ChildPanel scale parsed 120% as 1.2");
        TEST_ASSERT(childPanel->pivot().x == 0.0f && childPanel->pivot().y == 0.0f, "ChildPanel pivot parsed top left (0.0 0.0)");

        // 5. Transform offset with % evaluation
        // Root has width=200, height=100. transform-offset="20px 50%" -> x=20, y=50 (50% of 100)
        rootPanel->setBounds(PerfUI::Rect{ 0, 0, 200, 100 });
        Point rootOffset = rootPanel->renderOffset();
        TEST_ASSERT(rootOffset.x == 20.0f, "RootPanel offset X is 20px");
        TEST_ASSERT(rootOffset.y == 50.0f, "RootPanel offset Y is 50px (50% of 100)");

        // Child offset "10 -5"
        Point childOffset = childPanel->renderOffset();
        TEST_ASSERT(childOffset.x == 10.0f && childOffset.y == -5.0f, "ChildPanel offset is {10, -5}");

        // 6. BlendMode on child
        TEST_ASSERT(childPanel->blendMode() == PerfUI::BlendMode::Additive, "ChildPanel blendMode is Additive");

        // 7. Image src and tint
        auto* img = dynamic_cast<PerfUI::Image*>(loader.findByName("BannerImg"));
        TEST_ASSERT(img != nullptr, "Image found with id='BannerImg'");
        TEST_ASSERT(img->tint() == PerfUI::Color(0xFF, 0x88, 0x00, 0xFF), "Image tint #FF8800 applied");
    }

    // -------------------------------------------------------------
    // Test 16: Stage 1 PSS (PerfUI Style Sheets) Engine & Cascade
    // -------------------------------------------------------------
    {
        std::cout << "\nTest 16: Stage 1 PSS Engine (Selectors, Specificity, Cascade, Variables, Invalidation)...\n";
        UIContext context;
        MarkupLoader loader(context);

        const std::string pssXml = R"(
            <UI>
                <Styles>
                    /* Comments support */
                    :root {
                        --gold: #c8a45a;
                        --card-bg: rgba(10, 10, 14, 0.9);
                        --pad: 12;
                    }

                    Panel {
                        opacity: 0.95;
                    }

                    .card {
                        background: var(--card-bg);
                        radius: 6;
                        padding: var(--pad);
                    }

                    Panel.card.big {
                        width: 350;
                    }

                    #myBtn {
                        width: 500;
                        color: var(--gold);
                    }

                    Button {
                        width: 120;
                    }

                    Button {
                        width: 220; /* Later rule wins at equal specificity */
                    }

                    Panel > Text.title {
                        font-size: 24;
                        color: var(--gold);
                    }

                    Panel .desc {
                        font-size: 14;
                        color: var(--missing-token, #aabbcc);
                    }

                    Button, .clickable {
                        height: 40;
                    }
                </Styles>

                <Panel id="MainCard" class="card big">
                    <Text id="TitleText" class="title" text="Card Title" />
                    <Panel id="Inner">
                        <Text id="DescText" class="desc" text="Inner Description" />
                    </Panel>
                    <Button id="myBtn" class="clickable" text="Click Me" />
                    <Button id="standardBtn" text="Standard Button" />
                    <Button id="inlineOverrideBtn" width="333" text="Inline Override" />
                </Panel>
            </UI>
        )";

        auto* root = loader.loadString(pssXml);
        TEST_ASSERT(root != nullptr, "PSS markup loaded successfully");

        auto* mainCard = loader.findByName("MainCard");
        TEST_ASSERT(mainCard != nullptr, "MainCard found");
        TEST_ASSERT(mainCard->computedStyle().width.value_or(0.0f) == 350.0f, "MainCard width 350 from compound selector 'Panel.card.big'");
        TEST_ASSERT(mainCard->computedStyle().cornerRadius.value_or(0.0f) == 6.0f, "MainCard radius 6 from class selector '.card'");
        TEST_ASSERT(mainCard->computedStyle().opacity.value_or(0.0f) == 0.95f, "MainCard opacity 0.95 from tag selector 'Panel'");
        TEST_ASSERT(mainCard->computedStyle().padding.has_value() && mainCard->computedStyle().padding->left == 12.0f, "MainCard padding 12 from var(--pad)");

        // Specificity: ID (#myBtn = 500) beats Tag (Button = 220)
        auto* myBtn = loader.findByName("myBtn");
        TEST_ASSERT(myBtn != nullptr, "myBtn found");
        TEST_ASSERT(myBtn->computedStyle().width.value_or(0.0f) == 500.0f, "#myBtn width 500 (ID beats Tag)");
        TEST_ASSERT(myBtn->computedStyle().height.value_or(0.0f) == 40.0f, "#myBtn height 40 from comma selector 'Button, .clickable'");
        TEST_ASSERT(myBtn->computedStyle().color == Color::FromHex("#c8a45a"), "#myBtn color is var(--gold)");

        // Later rule wins at same specificity
        auto* standardBtn = loader.findByName("standardBtn");
        TEST_ASSERT(standardBtn != nullptr, "standardBtn found");
        TEST_ASSERT(standardBtn->computedStyle().width.value_or(0.0f) == 220.0f, "standardBtn width 220 (later rule won over 120)");

        // Source priority: Inline XML attribute beats PSS
        auto* inlineOverrideBtn = loader.findByName("inlineOverrideBtn");
        TEST_ASSERT(inlineOverrideBtn != nullptr, "inlineOverrideBtn found");
        TEST_ASSERT(inlineOverrideBtn->computedStyle().width.value_or(0.0f) == 333.0f, "inlineOverrideBtn width 333 (inline XML beats PSS 220)");

        // Child selector (Panel > Text.title)
        auto* titleText = loader.findByName("TitleText");
        TEST_ASSERT(titleText != nullptr, "TitleText found");
        TEST_ASSERT(titleText->computedStyle().fontSize.value_or(0.0f) == 24.0f, "TitleText font-size 24 from child selector 'Panel > Text.title'");
        TEST_ASSERT(titleText->computedStyle().color == Color::FromHex("#c8a45a"), "TitleText color var(--gold)");

        // Descendant selector (Panel .desc) with fallback in var()
        auto* descText = loader.findByName("DescText");
        TEST_ASSERT(descText != nullptr, "DescText found");
        TEST_ASSERT(descText->computedStyle().fontSize.value_or(0.0f) == 14.0f, "DescText font-size 14 from descendant selector 'Panel .desc'");
        TEST_ASSERT(descText->computedStyle().color == Color::FromHex("#aabbcc"), "DescText color var(--missing-token, #aabbcc) fallback");

        // Dynamic variable update in runtime
        context.setCssVariable("--gold", "#ffffff");
        TEST_ASSERT(titleText->computedStyle().color == Color::White(), "Runtime variable update (--gold -> #ffffff) recalculated dependent styles");

        // Class manipulation via C++ API
        mainCard->removeClass("big");
        context.root()->updateStyleCascade();
        TEST_ASSERT(!mainCard->computedStyle().width.has_value() || mainCard->computedStyle().width.value() != 350.0f, "Removing class 'big' clears width 350");

        mainCard->addClass("big");
        context.root()->updateStyleCascade();
        TEST_ASSERT(mainCard->computedStyle().width.value_or(0.0f) == 350.0f, "Adding class 'big' reapplies width 350");

        // Performance benchmark: 1000 elements cascade recalculation
        auto benchRoot = std::make_unique<UIElement>("BenchRoot");
        benchRoot->setContext(&context);
        for (int i = 0; i < 500; ++i) {
            auto p = std::make_unique<Panel>("Card" + std::to_string(i));
            p->addClass("card");
            auto b = std::make_unique<Button>("Btn" + std::to_string(i));
            p->addChild(std::move(b));
            benchRoot->addChild(std::move(p));
        }
        auto t0 = std::chrono::high_resolution_clock::now();
        benchRoot->invalidateStyleSubtree();
        benchRoot->updateStyleCascade();
        auto t1 = std::chrono::high_resolution_clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        std::cout << "  [Benchmark] 1000 elements style cascade recalculation time: " << elapsedMs << " ms\n";
        TEST_ASSERT(elapsedMs < 30.0, "1000 elements style cascade recalculated under 30ms");
    }

    // -------------------------------------------------------------
    // Test 17: Stage 2 PSS Pseudo-Classes & State Invalidation
    // -------------------------------------------------------------
    std::cout << "\nTest 17: Stage 2 PSS Pseudo-Classes & State Invalidation...\n";
    {
        PerfUI::UIContext context;
        PerfUI::MarkupLoader loader(context);

        const std::string stage2Xml = R"(
            <UI>
                <Styles>
                    Button {
                        background: #111111;
                        color: #ffffff;
                    }
                    Button:hover {
                        background: #444444;
                        color: #ffd700;
                    }
                    Button:active {
                        background: #ff0000;
                    }
                    Button:disabled {
                        opacity: 0.4;
                        background: #222222;
                    }

                    Checkbox {
                        color: #ffffff;
                    }
                    Checkbox:checked {
                        color: #00ffff;
                    }

                    Panel.card:hover {
                        background: #2a2a2a;
                    }

                    .item:first-child {
                        border: 2 #ff00ff;
                    }
                    .item:last-child {
                        border: 2 #ffff00;
                    }
                    .item:focus {
                        border: 2 #00ff00;
                    }

                    .expander {
                        width: 400;
                    }
                </Styles>

                <Panel id="Container" class="card">
                    <Button id="InteractiveBtn" class="item" text="Click" />
                    <Button id="DisabledBtn" text="Disabled" enabled="false" />
                    <Checkbox id="MyCheck" text="Option" checked="false" />
                    <Button id="LastBtn" class="item" text="Last" />
                </Panel>
            </UI>
        )";

        auto* root = loader.loadString(stage2Xml);
        TEST_ASSERT(root != nullptr, "Stage 2 XML loaded successfully");

        auto* btn = dynamic_cast<PerfUI::Button*>(loader.findByName("InteractiveBtn"));
        auto* disabledBtn = dynamic_cast<PerfUI::Button*>(loader.findByName("DisabledBtn"));
        auto* check = dynamic_cast<PerfUI::Checkbox*>(loader.findByName("MyCheck"));
        auto* lastBtn = dynamic_cast<PerfUI::Button*>(loader.findByName("LastBtn"));
        auto* container = dynamic_cast<PerfUI::Panel*>(loader.findByName("Container"));

        TEST_ASSERT(btn != nullptr, "InteractiveBtn found");
        TEST_ASSERT(disabledBtn != nullptr, "DisabledBtn found");
        TEST_ASSERT(check != nullptr, "MyCheck found");
        TEST_ASSERT(lastBtn != nullptr, "LastBtn found");
        TEST_ASSERT(container != nullptr, "Container found");

        // 1. Initial normal state
        TEST_ASSERT(btn->computedStyle().backgroundColor == PerfUI::Color::FromHex("#111111"), "InteractiveBtn initial background #111111");
        TEST_ASSERT(btn->computedStyle().color == PerfUI::Color::White(), "InteractiveBtn initial color White");
        TEST_ASSERT(btn->computedStyle().borderColor == PerfUI::Color::FromHex("#ff00ff"), "InteractiveBtn first-child border #ff00ff");
        TEST_ASSERT(lastBtn->computedStyle().borderColor == PerfUI::Color::FromHex("#ffff00"), "LastBtn last-child border #ffff00");

        // 2. Hover state
        btn->clearLayoutDirty();
        btn->onPointerEnter();
        TEST_ASSERT(btn->isStyleDirty(), "onPointerEnter marks element style dirty");
        context.root()->updateStyleCascade();
        TEST_ASSERT(btn->computedStyle().backgroundColor == PerfUI::Color::FromHex("#444444"), "InteractiveBtn :hover background #444444");
        TEST_ASSERT(btn->computedStyle().color == PerfUI::Color::FromHex("#ffd700"), "InteractiveBtn :hover color #ffd700");
        TEST_ASSERT(!btn->isLayoutDirty(), "Visual-only hover change did NOT dirty layout!");

        // 3. Active (press) state
        btn->onPointerDown(PerfUI::Point{ 10.0f, 10.0f });
        context.root()->updateStyleCascade();
        TEST_ASSERT(btn->computedStyle().backgroundColor == PerfUI::Color::FromHex("#ff0000"), "InteractiveBtn :active background #ff0000");

        // 4. Release active state
        btn->onPointerUp(PerfUI::Point{ 10.0f, 10.0f });
        context.root()->updateStyleCascade();
        TEST_ASSERT(btn->computedStyle().backgroundColor == PerfUI::Color::FromHex("#444444"), "Releasing press reverts to :hover background #444444");

        // 5. Leave hover
        btn->onPointerLeave();
        context.root()->updateStyleCascade();
        TEST_ASSERT(btn->computedStyle().backgroundColor == PerfUI::Color::FromHex("#111111"), "onPointerLeave reverts to normal background #111111");

        // 6. Focus state
        btn->setFocusable(true);
        context.setFocus(btn);
        context.root()->updateStyleCascade();
        TEST_ASSERT(btn->computedStyle().borderColor == PerfUI::Color::FromHex("#00ff00"), "InteractiveBtn :focus border #00ff00");
        context.clearFocus();
        context.root()->updateStyleCascade();
        TEST_ASSERT(btn->computedStyle().borderColor == PerfUI::Color::FromHex("#ff00ff"), "Clearing focus reverts border to first-child #ff00ff");

        // 7. Disabled state
        TEST_ASSERT(disabledBtn->computedStyle().opacity.value_or(1.0f) == 0.4f, "DisabledBtn :disabled opacity 0.4");
        TEST_ASSERT(disabledBtn->computedStyle().backgroundColor == PerfUI::Color::FromHex("#222222"), "DisabledBtn :disabled background #222222");

        // 8. Checked state on Checkbox
        TEST_ASSERT(check->computedStyle().color == PerfUI::Color::White(), "MyCheck unchecked color White");
        check->checked(true);
        context.root()->updateStyleCascade();
        TEST_ASSERT(check->computedStyle().color == PerfUI::Color::FromHex("#00ffff"), "MyCheck :checked color #00ffff");

        // 9. Panel :hover
        container->onPointerEnter();
        context.root()->updateStyleCascade();
        TEST_ASSERT(container->computedStyle().backgroundColor == PerfUI::Color::FromHex("#2a2a2a"), "Container :hover background #2a2a2a");
        container->onPointerLeave();
        context.root()->updateStyleCascade();
        TEST_ASSERT(!container->computedStyle().backgroundColor.has_value() || container->computedStyle().backgroundColor.value() != PerfUI::Color::FromHex("#2a2a2a"), "Container unhover cleared #2a2a2a");

        // 10. Layout dirtying on layout property change
        btn->clearLayoutDirty();
        btn->addClass("expander");
        context.root()->updateStyleCascade();
        TEST_ASSERT(btn->computedStyle().width.value_or(0.0f) == 400.0f, "Adding class 'expander' sets width 400");
        TEST_ASSERT(btn->isLayoutDirty(), "Layout property change correctly marked layout dirty");

        // 11. Large-scale Benchmarks: 5000 elements
        auto bench5k = std::make_unique<PerfUI::UIElement>("Bench5K");
        bench5k->setContext(&context);
        for (int i = 0; i < 2500; ++i) {
            auto p = std::make_unique<PerfUI::Panel>("Card" + std::to_string(i));
            p->addClass("card");
            auto b = std::make_unique<PerfUI::Button>("Btn" + std::to_string(i));
            p->addChild(std::move(b));
            bench5k->addChild(std::move(p));
        }

        // Cold start benchmark
        auto tStart = std::chrono::high_resolution_clock::now();
        bench5k->invalidateStyleSubtree();
        bench5k->updateStyleCascade();
        auto tEnd = std::chrono::high_resolution_clock::now();
        double coldMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
        std::cout << "  [Benchmark 5000 elements] Cold start cascade time: " << coldMs << " ms\n";
        TEST_ASSERT(coldMs < 100.0, "5000 elements cold cascade under 100ms");

        // Single class change on single element (dirty subtree)
        auto* targetCard = dynamic_cast<PerfUI::Panel*>(bench5k->children()[1234].get());
        auto tSingleStart = std::chrono::high_resolution_clock::now();
        targetCard->addClass("expander");
        bench5k->updateStyleCascade();
        auto tSingleEnd = std::chrono::high_resolution_clock::now();
        double singleMs = std::chrono::duration<double, std::milli>(tSingleEnd - tSingleStart).count();
        std::cout << "  [Benchmark 5000 elements] Single element class change cascade time: " << singleMs << " ms\n";
        TEST_ASSERT(singleMs < 1.0, "Single element class change cascade under 1ms");
    }

    // -------------------------------------------------------------
    // Test Summary
    // -------------------------------------------------------------
    std::cout << "\n=========================================================\n";
    std::cout << "  Markup Test Results: " << g_testsPassed << " passed, " << g_testsFailed << " failed.\n";
    std::cout << "=========================================================\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
