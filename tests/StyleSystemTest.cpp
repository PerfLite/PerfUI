#include <iostream>
#include <cassert>
#include <cmath>
#include <string>
#include "PerfUI/PerfUI.h"
#include "PerfUI/Markup/MarkupLoader.h"
#include "MockRenderBackend.h"

using namespace PerfUI;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion FAILED at line " << __LINE__ << ": " << (msg) << std::endl; \
            std::exit(1); \
        } \
    } while(0)

void TestThemePresetsAndTokens() {
    std::cout << "[Test] Running TestThemePresetsAndTokens..." << std::endl;

    // 1. Nordic Fantasy
    Theme nordic = Theme::NordicFantasy();
    TEST_ASSERT(nordic.colors.borderFocus == Color(212, 175, 55, 255), "Nordic gold border focus");
    TEST_ASSERT(nordic.metrics.radiusMedium == 8.0f, "Nordic medium radius");

    // 2. Dark Fantasy
    Theme dark = Theme::DarkFantasy();
    TEST_ASSERT(dark.colors.borderFocus == Color(180, 30, 40, 255), "Dark fantasy crimson focus");
    TEST_ASSERT(dark.colors.surfaceBase == Color(10, 8, 12, 245), "Dark fantasy obsidian base");

    // 3. Minimalist
    Theme mini = Theme::Minimalist();
    TEST_ASSERT(mini.colors.borderFocus == Color(90, 120, 220, 255), "Minimalist indigo focus");
    TEST_ASSERT(mini.metrics.radiusSmall == 2.0f, "Minimalist radius small");

    // 4. Vanilla Skyrim
    Theme vanilla = Theme::VanillaSkyrim();
    TEST_ASSERT(vanilla.colors.borderFocus == Color(216, 185, 100, 255), "Skyrim gold border focus");
    TEST_ASSERT(vanilla.metrics.radiusSmall == 0.0f, "Vanilla Skyrim sharp iron corners");

    // 5. Futuristic
    Theme future = Theme::Futuristic();
    TEST_ASSERT(future.colors.borderFocus == Color(0, 229, 255, 255), "Futuristic cyan focus");

    // ApplyPreset
    Theme::ApplyPreset("Dark Fantasy");
    TEST_ASSERT(Theme::Current().colors.borderFocus == Color(180, 30, 40, 255), "Current theme is Dark Fantasy");

    Theme::ApplyPreset("Minimalist");
    TEST_ASSERT(Theme::Current().colors.borderFocus == Color(90, 120, 220, 255), "Current theme is Minimalist");

    Theme::ApplyPreset("Nordic Gold");
    TEST_ASSERT(Theme::Current().colors.borderFocus == Color(212, 175, 55, 255), "Current theme restored to Nordic Gold");

    std::cout << "  -> Passed TestThemePresetsAndTokens" << std::endl;
}

void TestStyleCascadeAndResolution() {
    std::cout << "[Test] Running TestStyleCascadeAndResolution..." << std::endl;

    Theme::ApplyPreset("Nordic Gold");

    // 1. Fallback cascade in StateStyle
    StateStyle<BoxStyle> stateBox;
    stateBox.normal = BoxStyle{}.bg(Color(50, 50, 50));
    // Pressed not set -> falls back to normal
    BoxStyle resolvedPress = stateBox.resolve(WidgetState::Pressed, BoxStyle{}.bg(Color::Black()));
    TEST_ASSERT(resolvedPress.backgroundColor.has_value() && *resolvedPress.backgroundColor == Color(50, 50, 50),
        "Pressed falls back to normal");

    // If hover is set, Pressed falls back to hover
    stateBox.hover = BoxStyle{}.bg(Color(80, 80, 80));
    resolvedPress = stateBox.resolve(WidgetState::Pressed, BoxStyle{}.bg(Color::Black()));
    TEST_ASSERT(resolvedPress.backgroundColor.has_value() && *resolvedPress.backgroundColor == Color(80, 80, 80),
        "Pressed falls back to hover before normal");

    // If pressed explicitly set, it has highest state priority
    stateBox.pressed = BoxStyle{}.bg(Color(120, 120, 120));
    resolvedPress = stateBox.resolve(WidgetState::Pressed, BoxStyle{}.bg(Color::Black()));
    TEST_ASSERT(resolvedPress.backgroundColor.has_value() && *resolvedPress.backgroundColor == Color(120, 120, 120),
        "Explicit pressed state resolved");

    // 2. Component Style & Local Override on Button
    Button btn("Click Me");
    // Before local override, theme button style applies
    BoxStyle btnBox = btn.resolveBoxStyle(WidgetState::Normal);
    TEST_ASSERT(btnBox.backgroundColor.has_value() == false, "No local box style yet");

    // Apply local override
    btn.style().box.normal = BoxStyle{}.bg(Color(255, 0, 0)).radius(10.0f);
    btnBox = btn.resolveBoxStyle(WidgetState::Normal);
    TEST_ASSERT(btnBox.backgroundColor.has_value() && *btnBox.backgroundColor == Color(255, 0, 0), "Local override bg applied");
    TEST_ASSERT(btnBox.cornerRadius.has_value() && *btnBox.cornerRadius == 10.0f, "Local override radius applied");

    std::cout << "  -> Passed TestStyleCascadeAndResolution" << std::endl;
}

void TestAnimationAndEasing() {
    std::cout << "[Test] Running TestAnimationAndEasing..." << std::endl;

    // Easing evaluations
    TEST_ASSERT(ApplyEasing(0.0f, Easing::Linear) == 0.0f, "Linear 0.0");
    TEST_ASSERT(ApplyEasing(1.0f, Easing::Linear) == 1.0f, "Linear 1.0");
    TEST_ASSERT(ApplyEasing(0.5f, Easing::Linear) == 0.5f, "Linear 0.5");

    TEST_ASSERT(ApplyEasing(0.5f, Easing::EaseInQuad) == 0.25f, "EaseInQuad 0.5 -> 0.25");
    TEST_ASSERT(ApplyEasing(0.5f, Easing::EaseOutQuad) == 0.75f, "EaseOutQuad 0.5 -> 0.75");
    TEST_ASSERT(ApplyEasing(1.0f, Easing::EaseOutCubic) == 1.0f, "EaseOutCubic 1.0 -> 1.0");

    // AnimatedColor
    AnimatedColor animCol(Color(0, 0, 0, 255), 10.0f);
    TEST_ASSERT(animCol.value() == Color(0, 0, 0, 255), "Initial color black");
    TEST_ASSERT(animCol.isAtTarget(), "Initially at target");

    animCol.setTarget(Color(100, 100, 100, 255));
    TEST_ASSERT(!animCol.isAtTarget(), "Not at target after setting new target");

    animCol.update(0.05f); // partial interpolation
    TEST_ASSERT(animCol.value().r > 0 && animCol.value().r < 100, "Color smoothly interpolating");

    animCol.snapTo(Color(200, 200, 200, 255));
    TEST_ASSERT(animCol.value() == Color(200, 200, 200, 255), "SnapTo sets color immediately");
    TEST_ASSERT(animCol.isAtTarget(), "At target after snapTo");

    std::cout << "  -> Passed TestAnimationAndEasing" << std::endl;
}

void TestNewComponents() {
    std::cout << "[Test] Running TestNewComponents..." << std::endl;

    MockRenderBackend backend;

    // 1. Divider
    Divider divH(LayoutDirection::Horizontal);
    divH.thickness(2.0f);
    divH.color(Color(100, 150, 200, 255));
    divH.measure(Dimensions{ 200.0f, 100.0f });
    TEST_ASSERT(divH.desiredSize().height == 2.0f, "Horizontal divider height is thickness");
    TEST_ASSERT(divH.desiredSize().width == 200.0f, "Horizontal divider width matches available");

    divH.setBounds(Rect{ 10.0f, 20.0f, 200.0f, 2.0f });
    backend.beginFrame();
    divH.render(backend);
    backend.endFrame();
    TEST_ASSERT(backend.drawCallCount() == 1, "Divider rendered via drawRect");

    // 2. Spacer
    Spacer spacer(24.0f);
    spacer.measure(Dimensions{ 500.0f, 500.0f });
    TEST_ASSERT(spacer.desiredSize().width == 24.0f && spacer.desiredSize().height == 24.0f, "Spacer fixed size");

    // 3. Switch
    Switch toggle(false);
    TEST_ASSERT(!toggle.isChecked(), "Switch initially unchecked");
    bool toggled = false;
    toggle.onToggle([&](bool) {
        toggled = true;
    });

    toggle.onPointerDown(Point{ 5.0f, 5.0f });
    TEST_ASSERT(toggle.isChecked(), "Switch toggled to checked on click");
    TEST_ASSERT(toggled, "Switch onToggle callback triggered");

    toggle.measure(Dimensions{ 100.0f, 100.0f });
    TEST_ASSERT(toggle.desiredSize().width == 44.0f && toggle.desiredSize().height == 24.0f, "Switch default size");

    toggle.setBounds(Rect{ 0.0f, 0.0f, 44.0f, 24.0f });
    backend.beginFrame();
    toggle.render(backend);
    backend.endFrame();
    TEST_ASSERT(backend.drawCallCount() >= 2, "Switch renders track and thumb");

    // 4. RadioButton and RadioGroup
    RadioGroup group;
    auto* rb1 = group.addOption(1, "Option 1");
    auto* rb2 = group.addOption(2, "Option 2");

    int currentGroupValue = -1;
    group.onSelectionChanged([&](int val) {
        currentGroupValue = val;
    });

    group.select(1);
    TEST_ASSERT(rb1->isSelected(), "rb1 is selected");
    TEST_ASSERT(!rb2->isSelected(), "rb2 is not selected");
    TEST_ASSERT(currentGroupValue == 1, "Group reports selection 1");

    // Clicking rb2 should deselect rb1
    rb2->onPointerDown(Point{ 5.0f, 5.0f });
    TEST_ASSERT(!rb1->isSelected(), "rb1 is deselected after rb2 clicked");
    TEST_ASSERT(rb2->isSelected(), "rb2 is now selected");
    TEST_ASSERT(currentGroupValue == 2, "Group reports selection 2");

    // 5. Label
    Label heading("Title", LabelVariant::Heading);
    TEST_ASSERT(heading.text() == "Title", "Label text set");
    TEST_ASSERT(heading.textStyle().fontSize == Theme::Current().typography.fontTitle, "Label Heading variant font size");

    Label caption("Small details", LabelVariant::Caption);
    TEST_ASSERT(caption.textStyle().fontSize == Theme::Current().typography.fontSmall, "Label Caption variant font size");

    // 6. Panel clipChildren
    Panel panel;
    panel.clipChildren(true);
    TEST_ASSERT(panel.clipChildren(), "Panel clipChildren enabled");

    panel.setBounds(Rect{ 50.0f, 50.0f, 200.0f, 100.0f });
    panel.add<Button>("Inside Button");

    backend.beginFrame();
    panel.render(backend);
    backend.endFrame();

    bool hasPushClip = false;
    bool hasPopClip = false;
    for (const auto& cmd : backend.commands()) {
        if (cmd.type == MockRenderBackend::DrawCommand::Type::PushClip) {
            hasPushClip = true;
            TEST_ASSERT(cmd.bounds.x == 50.0f && cmd.bounds.y == 50.0f &&
                        cmd.bounds.width == 200.0f && cmd.bounds.height == 100.0f, "PushClip bounds match panel");
        }
        if (cmd.type == MockRenderBackend::DrawCommand::Type::PopClip) {
            hasPopClip = true;
        }
    }
    TEST_ASSERT(hasPushClip, "MockRenderBackend recorded PushClip");
    TEST_ASSERT(hasPopClip, "MockRenderBackend recorded PopClip");

    std::cout << "  -> Passed TestNewComponents" << std::endl;
}

void TestMarkupIntegration() {
    std::cout << "[Test] Running TestMarkupIntegration..." << std::endl;

    const std::string xml = R"(
        <Panel name="RootPanel" clip-children="true" bg="#1a1a24" corner-radius="10">
            <Label name="HeaderLabel" text="Settings" variant="heading" />
            <Divider name="Sep" thickness="2" color="#445566" />
            <Switch name="SoundToggle" checked="true" />
            <Spacer name="Gap" size="12" />
            <RadioGroup name="DifficultyGroup" selected="2">
                <RadioButton name="Easy" value="1" label="Easy" />
                <RadioButton name="Normal" value="2" label="Normal" />
                <RadioButton name="Hard" value="3" label="Hard" />
            </RadioGroup>
        </Panel>
    )";

    UIContext ctx;
    MarkupLoader loader(ctx);
    UIElement* root = loader.loadString(xml);
    TEST_ASSERT(root != nullptr, "Root element loaded from XML");

    auto* panel = dynamic_cast<Panel*>(root);
    TEST_ASSERT(panel != nullptr, "Root is Panel");
    TEST_ASSERT(panel->clipChildren() == true, "Panel clipChildren parsed from XML");

    // Find children
    Label* label = nullptr;
    Divider* divider = nullptr;
    Switch* switchWidget = nullptr;
    Spacer* spacer = nullptr;
    RadioGroup* radioGroup = nullptr;

    for (const auto& child : panel->children()) {
        if (child->name() == "HeaderLabel") label = dynamic_cast<Label*>(child.get());
        if (child->name() == "Sep") divider = dynamic_cast<Divider*>(child.get());
        if (child->name() == "SoundToggle") switchWidget = dynamic_cast<Switch*>(child.get());
        if (child->name() == "Gap") spacer = dynamic_cast<Spacer*>(child.get());
        if (child->name() == "DifficultyGroup") radioGroup = dynamic_cast<RadioGroup*>(child.get());
    }

    TEST_ASSERT(label != nullptr, "Label parsed");
    TEST_ASSERT(label->text() == "Settings", "Label text correct");
    TEST_ASSERT(label->variant() == LabelVariant::Heading, "Label variant heading");

    TEST_ASSERT(divider != nullptr, "Divider parsed");
    TEST_ASSERT(divider->thickness() == 2.0f, "Divider thickness parsed");

    TEST_ASSERT(switchWidget != nullptr, "Switch parsed");
    TEST_ASSERT(switchWidget->isChecked() == true, "Switch checked state parsed");

    TEST_ASSERT(spacer != nullptr, "Spacer parsed");

    TEST_ASSERT(radioGroup != nullptr, "RadioGroup parsed");
    TEST_ASSERT(radioGroup->selectedValue() == 2, "RadioGroup initial selected value parsed");
    TEST_ASSERT(radioGroup->children().size() == 3, "RadioGroup has 3 RadioButton children");

    std::cout << "  -> Passed TestMarkupIntegration" << std::endl;
}

int main() {
    std::cout << "=== Running PerfUI Style System & Components Tests ===" << std::endl;

    TestThemePresetsAndTokens();
    TestStyleCascadeAndResolution();
    TestAnimationAndEasing();
    TestNewComponents();
    TestMarkupIntegration();

    std::cout << "=== ALL STYLE SYSTEM & COMPONENT TESTS PASSED ===" << std::endl;
    return 0;
}
