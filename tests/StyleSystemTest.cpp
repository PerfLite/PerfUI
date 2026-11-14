#include <iostream>
#include <cassert>
#include <cmath>
#include <string>
#include "PerfUI/PerfUI.h"
#include "PerfUI/Style/StyleSheet.h"
#include "../src/style/StyleTransition.h"
#include "../src/style/StyleKeyframeAnimation.h"
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

    // 7. ItemSlot
    ItemSlot slot("TestSlot");
    slot.count(5).rarity(ItemRarity::Legendary).equipped(true);
    TEST_ASSERT(slot.count() == 5, "ItemSlot count is 5");
    TEST_ASSERT(slot.rarity() == ItemRarity::Legendary, "ItemSlot rarity is Legendary");
    TEST_ASSERT(slot.isEquipped() == true, "ItemSlot is equipped");
    TEST_ASSERT(slot.getRarityBorderColor() == Color(245, 185, 45, 255), "ItemSlot legendary border color");

    bool slotClicked = false;
    slot.onClick([&]() { slotClicked = true; });
    slot.onPointerDown(Point{ 10.0f, 10.0f });
    TEST_ASSERT(slotClicked == true, "ItemSlot clicked");

    backend.beginFrame();
    slot.setBounds(Rect{ 0.0f, 0.0f, 50.0f, 50.0f });
    slot.render(backend);
    backend.endFrame();
    TEST_ASSERT(backend.drawCallCount() >= 2, "ItemSlot renders background and badges");

    // 8. KeybindSelector
    KeybindSelector kb(0x45, "Interact", "KeybindInteract"); // 'E'
    TEST_ASSERT(kb.keyCode() == 0x45, "Keybind is 'E'");
    TEST_ASSERT(KeybindSelector::KeyCodeToString(0x45) == "E", "KeyCodeToString('E') is 'E'");
    TEST_ASSERT(KeybindSelector::KeyCodeToString(0x09) == "Tab", "KeyCodeToString(Tab) is 'Tab'");

    kb.startListening();
    TEST_ASSERT(kb.isListening() == true, "Keybind listening");
    kb.onKeyDown(0x20); // Space
    TEST_ASSERT(!kb.isListening(), "Keybind stopped listening on key down");
    TEST_ASSERT(kb.keyCode() == 0x20, "Keybind changed to Space (0x20)");
    TEST_ASSERT(KeybindSelector::KeyCodeToString(kb.keyCode()) == "Space", "Keybind string is Space");

    // 9. Accordion
    Accordion accordion("TestAccordion");
    accordion.exclusive(true);
    TEST_ASSERT(accordion.isExclusive(), "Accordion is exclusive");

    auto* sec1 = accordion.addSection("Audio", std::make_unique<Button>("Mute"), true);
    auto* sec2 = accordion.addSection("Video", std::make_unique<Button>("V-Sync"), false);
    TEST_ASSERT(sec1->isExpanded() == true, "sec1 is initially expanded");
    TEST_ASSERT(sec2->isExpanded() == false, "sec2 is initially collapsed");

    // Opening sec2 should close sec1 in exclusive mode
    sec2->setExpanded(true);
    TEST_ASSERT(sec2->isExpanded() == true, "sec2 is now expanded");
    TEST_ASSERT(sec1->isExpanded() == false, "sec1 automatically closed in exclusive mode");

    // 10. SearchableList
    SearchableList sList("Search weapons...", "WeaponList");
    sList.addItem(1, "Iron Sword");
    sList.addItem(2, "Steel Dagger");
    sList.addItem(3, "Elven Bow");

    int chosenId = -1;
    std::string chosenName;
    sList.onItemSelected([&](int id, const std::string& name) {
        chosenId = id;
        chosenName = name;
    });

    sList.select(2);
    TEST_ASSERT(chosenId == 2, "SearchableList selected id 2");
    TEST_ASSERT(chosenName == "Steel Dagger", "SearchableList selected text 'Steel Dagger'");

    // Filter
    sList.filterItems("iron");
    // Only item 1 is matching

    // 11. TextArea
    TextArea textArea("Line 1\nLine 2\nLine 3", "TestTextArea");
    textArea.fontSize(14.0f);
    textArea.measure(Dimensions{ 300.0f, 150.0f });
    TEST_ASSERT(textArea.desiredSize().width == 300.0f, "TextArea desired width");
    TEST_ASSERT(textArea.desiredSize().height == 150.0f, "TextArea desired height");

    backend.beginFrame();
    textArea.setBounds(Rect{ 0.0f, 0.0f, 300.0f, 150.0f });
    textArea.render(backend);
    backend.endFrame();
    TEST_ASSERT(backend.drawCallCount() >= 4, "TextArea renders background and lines");

    // 12. SegmentedControl
    SegmentedControl segCtrl("FilterControl");
    segCtrl.addSegment("All", "all").addSegment("Weapons", "wpn").addSegment("Armor", "arm");
    TEST_ASSERT(segCtrl.segmentCount() == 3, "SegmentedControl has 3 segments");
    TEST_ASSERT(segCtrl.selectedIndex() == 0, "Default selected index is 0");
    TEST_ASSERT(segCtrl.selectedText() == "All", "Default selected text is 'All'");
    TEST_ASSERT(segCtrl.selectedValue() == "all", "Default selected value is 'all'");

    size_t segChangedIdx = 999;
    std::string segChangedText;
    segCtrl.onSelectionChanged([&](size_t idx, const std::string& text) {
        segChangedIdx = idx;
        segChangedText = text;
    });

    segCtrl.setSelectedIndex(1);
    TEST_ASSERT(segCtrl.selectedIndex() == 1, "Selected index is 1");
    TEST_ASSERT(segCtrl.selectedText() == "Weapons", "Selected text is 'Weapons'");
    TEST_ASSERT(segChangedIdx == 1, "onSelectionChanged fired with index 1");
    TEST_ASSERT(segChangedText == "Weapons", "onSelectionChanged fired with 'Weapons'");

    segCtrl.measure(Dimensions{ 300.0f, 32.0f });
    segCtrl.arrange(Rect{ 0.0f, 0.0f, 300.0f, 32.0f });
    backend.beginFrame();
    segCtrl.render(backend);
    backend.endFrame();
    TEST_ASSERT(backend.drawCallCount() >= 4, "SegmentedControl renders track, thumb, and labels");

    // 13. CategoryList
    CategoryList catList("MCMCategories");
    catList.addCategory("general", "General Settings")
           .addCategory("graphics", "Graphics", "", "NEW")
           .addCategory("audio", "Audio");
    TEST_ASSERT(catList.count() == 3, "CategoryList has 3 items");
    TEST_ASSERT(catList.selectedId() == "general", "Initial selected id is 'general'");

    std::string catSelected;
    catList.onCategoryChanged([&](const std::string& id) {
        catSelected = id;
    });

    catList.selectCategory("graphics");
    TEST_ASSERT(catList.selectedId() == "graphics", "Selected id changed to 'graphics'");
    TEST_ASSERT(catSelected == "graphics", "onCategoryChanged fired with 'graphics'");

    catList.measure(Dimensions{ 200.0f, 400.0f });
    catList.arrange(Rect{ 0.0f, 0.0f, 200.0f, 150.0f });
    backend.beginFrame();
    catList.render(backend);
    backend.endFrame();
    TEST_ASSERT(backend.drawCallCount() >= 3, "CategoryList renders items and badge");

    // 14. QuestTracker
    QuestTracker quest("MainQuestHUD");
    quest.setQuestTitle("The Way of the Voice")
         .setQuestCategory("Main Quest")
         .addObjective("Speak to the Greybeards", false)
         .addObjective("Demonstrate Unrelenting Force", false, 0, 3)
         .addObjective("Listen to Arngeir (Optional)", false, 0, 0, true);

    TEST_ASSERT(quest.questTitle() == "The Way of the Voice", "Quest title matches");
    TEST_ASSERT(quest.questCategory() == "Main Quest", "Quest category matches");
    TEST_ASSERT(quest.objectiveCount() == 3, "Quest has 3 objectives");
    TEST_ASSERT(!quest.isAllCompleted(), "Quest is not yet completed");

    quest.setObjectiveCompleted(0, true);
    quest.setObjectiveProgress(1, 3, 3); // 3 of 3 completes obj 1
    // obj 2 is optional, so quest should now be considered all completed!
    TEST_ASSERT(quest.isAllCompleted() == true, "Quest all non-optional objectives completed");

    quest.measure(Dimensions{ 300.0f, 200.0f });
    quest.arrange(Rect{ 10.0f, 10.0f, 300.0f, 150.0f });
    backend.beginFrame();
    quest.render(backend);
    backend.endFrame();
    TEST_ASSERT(backend.drawCallCount() >= 4, "QuestTracker renders header, diamond, and objectives");

    // 15. Notification
    Notification notif("LevelUpNotification");
    notif.setTitle("Level Up!")
         .setMessage("You have reached Level 25")
         .setLevel(NotificationLevel::Success)
         .setDuration(0.0f) // persistent
         .setClosable(true);

    bool notifClosed = false;
    notif.onClose([&]() { notifClosed = true; });

    bool actionInvoked = false;
    notif.setAction("Open Skills", [&]() { actionInvoked = true; });

    TEST_ASSERT(notif.title() == "Level Up!", "Notification title matches");
    TEST_ASSERT(notif.level() == NotificationLevel::Success, "Notification level matches");
    TEST_ASSERT(!notif.isDismissed(), "Notification is not dismissed");

    notif.measure(Dimensions{ 320.0f, 100.0f });
    notif.arrange(Rect{ 0.0f, 0.0f, 320.0f, 80.0f });
    backend.beginFrame();
    notif.render(backend);
    backend.endFrame();
    TEST_ASSERT(backend.drawCallCount() >= 4, "Notification renders frame, badge, text, and action button");

    notif.dismiss();
    TEST_ASSERT(notif.isDismissed() == true, "Notification is dismissed");
    TEST_ASSERT(notifClosed == true, "Notification onClose fired");

    // 16. Sidebar
    Sidebar sidebar("ModSidebar");
    sidebar.setTitle("Skyrim UI")
           .addItem("home", "Overview")
           .addItem("mods", "Mods", "", "5")
           .addItem("settings", "Preferences");

    TEST_ASSERT(sidebar.title() == "Skyrim UI", "Sidebar title matches");
    TEST_ASSERT(sidebar.count() == 3, "Sidebar count is 3");
    TEST_ASSERT(sidebar.selectedId() == "home", "Default selected item is 'home'");
    TEST_ASSERT(!sidebar.isCollapsed(), "Sidebar is not collapsed");

    sidebar.setCollapsed(true);
    TEST_ASSERT(sidebar.isCollapsed() == true, "Sidebar is collapsed");

    sidebar.toggleCollapsed();
    TEST_ASSERT(sidebar.isCollapsed() == false, "Sidebar is uncollapsed via toggle");

    sidebar.selectItem("mods");
    TEST_ASSERT(sidebar.selectedId() == "mods", "Selected item is 'mods'");

    sidebar.measure(Dimensions{ 220.0f, 600.0f });
    sidebar.arrange(Rect{ 0.0f, 0.0f, 220.0f, 600.0f });
    backend.beginFrame();
    sidebar.render(backend);
    backend.endFrame();
    TEST_ASSERT(backend.drawCallCount() >= 4, "Sidebar renders background, title, toggle, and items");

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
            <ItemSlot name="PotionSlot" count="3" rarity="epic" equipped="true" />
            <KeybindSelector name="CastSpell" key="82" label="Cast" />
            <Accordion name="MenuAccordion" exclusive="true">
                <AccordionSection name="SecAudio" title="Audio" expanded="true" />
            </Accordion>
            <SearchableList name="SpellsList" placeholder="Filter..." />
            <TextArea name="ConsoleLog" text="Hello Skyrim" />
            <SegmentedControl name="FilterTabs" selected="1" />
            <CategoryList name="McmCats" item-height="38" />
            <QuestTracker name="HudTracker" title="Dragon Rising" category="Main Quest" />
            <Notification name="Alert" title="Notice" message="Mod initialized" level="success" />
            <Sidebar name="Nav" title="Menu" collapsed="false" />
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
    ItemSlot* itemSlot = nullptr;
    KeybindSelector* keybindSelector = nullptr;
    Accordion* accordion = nullptr;
    SearchableList* searchableList = nullptr;
    TextArea* textArea = nullptr;
    SegmentedControl* segmentedControl = nullptr;
    CategoryList* categoryList = nullptr;
    QuestTracker* questTracker = nullptr;
    Notification* notification = nullptr;
    Sidebar* sidebar = nullptr;

    for (const auto& child : panel->children()) {
        if (child->name() == "HeaderLabel") label = dynamic_cast<Label*>(child.get());
        if (child->name() == "Sep") divider = dynamic_cast<Divider*>(child.get());
        if (child->name() == "SoundToggle") switchWidget = dynamic_cast<Switch*>(child.get());
        if (child->name() == "Gap") spacer = dynamic_cast<Spacer*>(child.get());
        if (child->name() == "DifficultyGroup") radioGroup = dynamic_cast<RadioGroup*>(child.get());
        if (child->name() == "PotionSlot") itemSlot = dynamic_cast<ItemSlot*>(child.get());
        if (child->name() == "CastSpell") keybindSelector = dynamic_cast<KeybindSelector*>(child.get());
        if (child->name() == "MenuAccordion") accordion = dynamic_cast<Accordion*>(child.get());
        if (child->name() == "SpellsList") searchableList = dynamic_cast<SearchableList*>(child.get());
        if (child->name() == "ConsoleLog") textArea = dynamic_cast<TextArea*>(child.get());
        if (child->name() == "FilterTabs") segmentedControl = dynamic_cast<SegmentedControl*>(child.get());
        if (child->name() == "McmCats") categoryList = dynamic_cast<CategoryList*>(child.get());
        if (child->name() == "HudTracker") questTracker = dynamic_cast<QuestTracker*>(child.get());
        if (child->name() == "Alert") notification = dynamic_cast<Notification*>(child.get());
        if (child->name() == "Nav") sidebar = dynamic_cast<Sidebar*>(child.get());
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

    TEST_ASSERT(itemSlot != nullptr, "ItemSlot parsed from XML");
    TEST_ASSERT(itemSlot->count() == 3, "ItemSlot count is 3");
    TEST_ASSERT(itemSlot->rarity() == ItemRarity::Epic, "ItemSlot rarity is Epic");
    TEST_ASSERT(itemSlot->isEquipped() == true, "ItemSlot equipped is true");

    TEST_ASSERT(keybindSelector != nullptr, "KeybindSelector parsed from XML");
    TEST_ASSERT(keybindSelector->keyCode() == 82, "KeybindSelector key is 82 ('R')");
    TEST_ASSERT(keybindSelector->label() == "Cast", "KeybindSelector label is 'Cast'");

    TEST_ASSERT(accordion != nullptr, "Accordion parsed from XML");
    TEST_ASSERT(accordion->isExclusive() == true, "Accordion exclusive is true");
    TEST_ASSERT(accordion->children().size() == 1, "Accordion has 1 section");

    TEST_ASSERT(searchableList != nullptr, "SearchableList parsed from XML");
    TEST_ASSERT(textArea != nullptr, "TextArea parsed from XML");
    TEST_ASSERT(textArea->text() == "Hello Skyrim", "TextArea text parsed");

    TEST_ASSERT(segmentedControl != nullptr, "SegmentedControl parsed from XML");
    TEST_ASSERT(categoryList != nullptr, "CategoryList parsed from XML");
    TEST_ASSERT(categoryList->itemHeight() == 38.0f, "CategoryList item-height is 38");

    TEST_ASSERT(questTracker != nullptr, "QuestTracker parsed from XML");
    TEST_ASSERT(questTracker->questTitle() == "Dragon Rising", "QuestTracker title is Dragon Rising");
    TEST_ASSERT(questTracker->questCategory() == "Main Quest", "QuestTracker category is Main Quest");

    TEST_ASSERT(notification != nullptr, "Notification parsed from XML");
    TEST_ASSERT(notification->title() == "Notice", "Notification title is Notice");
    TEST_ASSERT(notification->level() == NotificationLevel::Success, "Notification level is Success");

    TEST_ASSERT(sidebar != nullptr, "Sidebar parsed from XML");
    TEST_ASSERT(sidebar->title() == "Menu", "Sidebar title is Menu");
    TEST_ASSERT(sidebar->isCollapsed() == false, "Sidebar collapsed is false");

    std::cout << "  -> Passed TestMarkupIntegration" << std::endl;
}


void TestTransitionsAndInterpolation() {
    std::cout << "[Test] Running TestTransitionsAndInterpolation..." << std::endl;

    // 1. Shorthand Parsing
    {
        StyleSheet sheet;
        sheet.loadString(R"(
            Button {
                background: #000000;
                opacity: 1.0;
                transform-scale: 1.0;
                transition: background 0.2s ease, opacity 150ms linear 50ms, transform-scale 0.3s cubic-bezier(0.25, 0.1, 0.25, 1.0);
            }
        )");

        UIContext ctx;
        ctx.styleSheet() = sheet;

        auto btn = std::make_unique<Button>("TestBtn");
        btn->setContext(&ctx);
        btn->updateStyleCascade();

        const auto& c = btn->computedStyle();
        TEST_ASSERT(c.transitions.size() == 3, "Parsed 3 transition specs");

        // Spec 0: background 0.2s ease
        TEST_ASSERT(c.transitions[0].property == TransitionProperty::Background, "Spec 0 property is Background");
        TEST_ASSERT(std::abs(c.transitions[0].duration - 0.2f) < 0.001f, "Spec 0 duration is 0.2s");
        TEST_ASSERT(c.transitions[0].delay == 0.0f, "Spec 0 delay is 0.0s");

        // Spec 1: opacity 150ms linear 50ms
        TEST_ASSERT(c.transitions[1].property == TransitionProperty::Opacity, "Spec 1 property is Opacity");
        TEST_ASSERT(std::abs(c.transitions[1].duration - 0.15f) < 0.001f, "Spec 1 duration is 150ms (0.15s)");
        TEST_ASSERT(std::abs(c.transitions[1].delay - 0.05f) < 0.001f, "Spec 1 delay is 50ms (0.05s)");
        TEST_ASSERT(c.transitions[1].easing.type == Easing::Linear, "Spec 1 easing is Linear");

        // Spec 2: transform-scale 0.3s cubic-bezier(0.25, 0.1, 0.25, 1.0)
        TEST_ASSERT(c.transitions[2].property == TransitionProperty::TransformScale, "Spec 2 property is TransformScale");
        TEST_ASSERT(std::abs(c.transitions[2].duration - 0.3f) < 0.001f, "Spec 2 duration is 0.3s");
        TEST_ASSERT(c.transitions[2].easing.type == Easing::Custom, "Spec 2 easing is Custom Bezier");
        TEST_ASSERT(std::abs(c.transitions[2].easing.bezier.x1 - 0.25f) < 0.001f, "Bezier x1 is 0.25");
        TEST_ASSERT(std::abs(c.transitions[2].easing.bezier.y1 - 0.1f) < 0.001f, "Bezier y1 is 0.1");
    }

    // 2. Longhands Parsing and transition: none
    {
        StyleSheet sheet;
        sheet.loadString(R"(
            Panel {
                transition-property: background, border-color;
                transition-duration: 200ms, 400ms;
                transition-timing-function: ease-in, ease-out;
                transition-delay: 10ms, 20ms;
            }
            Panel.notrans {
                transition: none;
            }
        )");

        UIContext ctx;
        ctx.styleSheet() = sheet;

        auto panel = std::make_unique<Panel>();
        panel->setContext(&ctx);
        panel->updateStyleCascade();

        const auto& c1 = panel->computedStyle();
        TEST_ASSERT(c1.transitions.size() == 2, "Longhands created 2 transition specs");
        TEST_ASSERT(c1.transitions[0].property == TransitionProperty::Background, "Panel spec 0 is Background");
        TEST_ASSERT(std::abs(c1.transitions[0].duration - 0.2f) < 0.001f, "Panel spec 0 duration is 0.2s");
        TEST_ASSERT(c1.transitions[1].property == TransitionProperty::BorderColor, "Panel spec 1 is BorderColor");
        TEST_ASSERT(std::abs(c1.transitions[1].duration - 0.4f) < 0.001f, "Panel spec 1 duration is 0.4s");

        panel->addClass("notrans");
        panel->markStyleDirty();
        panel->updateStyleCascade();
        TEST_ASSERT(panel->computedStyle().transitions.empty(), "transition: none cleared transitions");
    }

    // 3. Cold mount vs Dynamic Hover Transitions
    {
        StyleSheet sheet;
        sheet.loadString(R"(
            Button {
                background: #000000;
                opacity: 1.0;
                transition: background 0.2s linear, opacity 0.2s linear;
            }
            Button:hover {
                background: #ff0000;
                opacity: 0.5;
            }
        )");

        UIContext ctx;
        ctx.styleSheet() = sheet;

        auto btn = std::make_unique<Button>("HoverBtn");
        btn->setContext(&ctx);

        // Cold mount: updateStyleCascade immediately applies resting styles
        btn->updateStyleCascade();
        TEST_ASSERT(btn->style().box.normal.has_value() && *btn->style().box.normal->backgroundColor == Color(0, 0, 0, 255),
            "Cold mount establishes background immediately");
        TEST_ASSERT(std::abs(btn->opacity() - 1.0f) < 0.001f, "Cold mount establishes opacity immediately");
        TEST_ASSERT(btn->transitions() == nullptr || !btn->transitions()->hasActiveTransitions(),
            "No active transitions on cold mount");

        // Hover: mouse enter
        btn->onPointerEnter();
        btn->updateStyleCascade();

        TEST_ASSERT(btn->transitions() != nullptr, "Transition controller allocated");
        TEST_ASSERT(btn->transitions()->hasActiveTransitions(), "Transitions active on hover");
        TEST_ASSERT(btn->transitions()->trackBackground.active, "Background transition active");
        TEST_ASSERT(btn->transitions()->trackOpacity.active, "Opacity transition active");

        // Tick 0.1s (halfway through 0.2s linear)
        btn->update(0.1f);

        Color midCol = *btn->style().box.normal->backgroundColor;
        TEST_ASSERT(midCol.r > 100 && midCol.r < 155, "Background R lerped halfway (~127)");
        TEST_ASSERT(std::abs(btn->opacity() - 0.75f) < 0.05f, "Opacity lerped halfway (0.75)");

        // Tick remaining 0.15s (past 0.2s total)
        btn->update(0.15f);

        Color finalCol = *btn->style().box.normal->backgroundColor;
        TEST_ASSERT(finalCol == Color(255, 0, 0, 255), "Background reached final target #ff0000");
        TEST_ASSERT(std::abs(btn->opacity() - 0.5f) < 0.001f, "Opacity reached final target 0.5");
        TEST_ASSERT(!btn->transitions()->hasActiveTransitions(), "All transitions finished cleanly");
    }

    // 4. Seamless Mid-Transition Reversal
    {
        StyleSheet sheet;
        sheet.loadString(R"(
            Button {
                background: #000000;
                transition: background 0.2s linear;
            }
            Button:hover {
                background: #ff0000;
            }
        )");

        UIContext ctx;
        ctx.styleSheet() = sheet;

        auto btn = std::make_unique<Button>("ReversalBtn");
        btn->setContext(&ctx);
        btn->updateStyleCascade(); // Cold mount: #000000

        // Hover in
        btn->onPointerEnter();
        btn->updateStyleCascade();

        // Tick halfway (0.1s) -> color is roughly 127
        btn->update(0.1f);
        uint8_t midR = btn->style().box.normal->backgroundColor->r;
        TEST_ASSERT(midR > 115 && midR < 140, "Mid-transition R is ~127");

        // Unhover immediately mid-transition!
        btn->onPointerLeave();
        btn->updateStyleCascade();

        // Verify startVal of new reversed track is the current visual value (midR), NOT 255 and NOT 0!
        TEST_ASSERT(btn->transitions()->trackBackground.active, "Reversed track is active");
        Color newStart = btn->transitions()->trackBackground.startVal;
        TEST_ASSERT(newStart.r == midR, "Seamless reversal started from current visual value without teleporting");
        TEST_ASSERT(btn->transitions()->trackBackground.targetVal == Color(0, 0, 0, 255), "Target is base #000000");

        // Tick 0.1s of the reversal
        btn->update(0.1f);
        uint8_t revR = btn->style().box.normal->backgroundColor->r;
        TEST_ASSERT(revR < midR, "Seamlessly lerping backwards towards 0");

        // Tick to completion
        btn->update(0.15f);
        TEST_ASSERT(*btn->style().box.normal->backgroundColor == Color(0, 0, 0, 255), "Smoothly returned to base color");
    }

    // 5. Explicit Animation Priority (ElementAnimator beats PSS Transition)
    {
        StyleSheet sheet;
        sheet.loadString(R"(
            Button {
                opacity: 1.0;
                transition: opacity 0.2s linear;
            }
            Button:hover {
                opacity: 0.2;
            }
        )");

        UIContext ctx;
        ctx.styleSheet() = sheet;

        auto btn = std::make_unique<Button>("PriorityBtn");
        btn->setContext(&ctx);
        btn->updateStyleCascade();

        // Explicit C++ animation starts on alpha
        btn->animate().alpha(0.8f, 1.0f);
        TEST_ASSERT(btn->isAlphaAnimating(), "Explicit alpha animation active");

        // Now button is hovered, which normally triggers opacity transition to 0.2
        btn->onPointerEnter();
        btn->updateStyleCascade();

        // PSS opacity transition MUST be suppressed by explicit animator
        TEST_ASSERT(btn->transitions() == nullptr || !btn->transitions()->trackOpacity.active,
            "PSS opacity transition suppressed while explicit animator is active");
    }

    std::cout << "  -> Passed TestTransitionsAndInterpolation" << std::endl;
}

void TestOutlineAndBorderImage() {
    std::cout << "[Test] Running TestOutlineAndBorderImage..." << std::endl;

    // 1. PSS Outline shorthand and individual properties
    StyleSheet sheet;
    sheet.loadString(R"(
        .card {
            width: 200px;
            height: 300px;
            border-radius: 8px;
            border: 2px solid #303a4a;
            outline: 1.5px solid rgba(218, 175, 90, 0.4);
            outline-offset: -4px;
        }
        .card-custom {
            outline-color: #ff0000;
            outline-width: 3px;
            outline-offset: 5px;
        }
    )");

    UIContext ctx;
    ctx.styleSheet() = sheet;

    Panel card("TestCard");
    card.setContext(&ctx);
    card.addClass("card");
    card.updateStyleCascade();

    const auto& style = card.computedStyle();
    TEST_ASSERT(style.outlineWidth.has_value() && std::abs(*style.outlineWidth - 1.5f) < 0.001f, "Outline width parsed");
    TEST_ASSERT(style.outlineOffset.has_value() && std::abs(*style.outlineOffset - (-4.0f)) < 0.001f, "Outline negative offset parsed");
    TEST_ASSERT(style.outlineColor.has_value(), "Outline color parsed");
    TEST_ASSERT(style.outlineColor->r == 218 && style.outlineColor->g == 175 && style.outlineColor->b == 90, "Outline RGB parsed");

    Panel cardCustom("CustomCard");
    cardCustom.setContext(&ctx);
    cardCustom.addClass("card-custom");
    cardCustom.updateStyleCascade();
    const auto& styleCust = cardCustom.computedStyle();
    TEST_ASSERT(styleCust.outlineWidth.has_value() && *styleCust.outlineWidth == 3.0f, "Custom outline width");
    TEST_ASSERT(styleCust.outlineOffset.has_value() && *styleCust.outlineOffset == 5.0f, "Custom outline offset");
    TEST_ASSERT(styleCust.outlineColor.has_value() && *styleCust.outlineColor == Color(255, 0, 0, 255), "Custom outline color");

    // 2. Render Panel with Outline
    MockRenderBackend backend;
    card.setBounds(Rect{ 100.0f, 100.0f, 200.0f, 300.0f });
    card.render(backend);

    bool foundOutline = false;
    for (const auto& cmd : backend.commands()) {
        if (cmd.type == MockRenderBackend::DrawCommand::Type::RectOutline) {
            if (std::abs(cmd.bounds.x - 104.0f) < 0.1f && std::abs(cmd.bounds.width - 192.0f) < 0.1f) {
                foundOutline = true;
                TEST_ASSERT(std::abs(cmd.thickness - 1.5f) < 0.01f, "Outline thickness matches 1.5px");
                TEST_ASSERT(std::abs(cmd.radius - 4.0f) < 0.01f, "Outline radius matches (radius + offset) = 4px");
            }
        }
    }
    TEST_ASSERT(foundOutline, "Rendered inner outline via drawRectOutline with negative offset");

    // 3. Border Image 9-Slice parsing
    StyleSheet imgSheet;
    imgSheet.loadString(R"(
        .frame {
            border-image: "textures/frame.png" 16px;
        }
        .frame-custom {
            border-image: url("textures/complex_frame.png") 10px 12px 14px 16px;
        }
    )");
    ctx.setRenderBackend(&backend);
    ctx.styleSheet() = imgSheet;

    Panel frame("FramePanel");
    frame.setContext(&ctx);
    frame.addClass("frame");
    frame.updateStyleCascade();

    const auto& fStyle = frame.computedStyle();
    TEST_ASSERT(fStyle.borderImagePath.has_value() && *fStyle.borderImagePath == "textures/frame.png", "Border image path");
    TEST_ASSERT(fStyle.bgSlice.has_value(), "bgSlice is set");
    TEST_ASSERT(fStyle.bgSlice->left == 16.0f && fStyle.bgSlice->top == 16.0f &&
                fStyle.bgSlice->right == 16.0f && fStyle.bgSlice->bottom == 16.0f, "16px uniform slice");
    TEST_ASSERT(frame.backgroundImage() != 0, "Texture was resolved and loaded automatically via context");

    Panel customFrame("CustomFrame");
    customFrame.setContext(&ctx);
    customFrame.addClass("frame-custom");
    customFrame.updateStyleCascade();

    const auto& cStyle = customFrame.computedStyle();
    TEST_ASSERT(cStyle.borderImagePath.has_value() && *cStyle.borderImagePath == "textures/complex_frame.png", "Complex border image path");
    TEST_ASSERT(cStyle.bgSlice.has_value(), "Complex slice is set");
    TEST_ASSERT(cStyle.bgSlice->top == 10.0f && cStyle.bgSlice->right == 12.0f &&
                cStyle.bgSlice->bottom == 14.0f && cStyle.bgSlice->left == 16.0f, "4-value border slice correctly mapped");

    std::cout << "  -> Passed TestOutlineAndBorderImage" << std::endl;
}

void TestLinearGradients() {
    std::cout << "[Test] Running TestLinearGradients..." << std::endl;

    // 1. Two-stop gradient
    StyleSheet sheet;
    sheet.loadString(R"(
        .fade-bottom {
            background: linear-gradient(to bottom, transparent, rgba(11, 14, 20, 0.95));
        }
        .split-three-stop {
            background: linear-gradient(to right, transparent, #daaf5a, transparent);
        }
    )");

    UIContext ctx;
    ctx.styleSheet() = sheet;

    Panel fade("FadeBottom");
    fade.setContext(&ctx);
    fade.addClass("fade-bottom");
    fade.updateStyleCascade();

    const auto& fStyle = fade.computedStyle();
    TEST_ASSERT(fStyle.backgroundGradient.has_value(), "Background gradient parsed");
    TEST_ASSERT(fStyle.backgroundGradient->direction == GradientDirection::ToBottom, "To bottom direction");
    TEST_ASSERT(fStyle.backgroundGradient->startColor == Color::Transparent(), "Start transparent");
    TEST_ASSERT(fStyle.backgroundGradient->endColor.r == 11 && fStyle.backgroundGradient->endColor.g == 14, "End color RGB");

    // 2. Render two-stop gradient
    MockRenderBackend backend;
    fade.setBounds(Rect{ 0.0f, 0.0f, 100.0f, 200.0f });
    fade.render(backend);

    bool foundGrad = false;
    for (const auto& cmd : backend.commands()) {
        if (cmd.type == MockRenderBackend::DrawCommand::Type::GradientRect) {
            foundGrad = true;
            TEST_ASSERT(cmd.bounds == Rect(0.0f, 0.0f, 100.0f, 200.0f), "Gradient drawn on panel bounds");
        }
    }
    TEST_ASSERT(foundGrad, "Rendered 2-stop gradient via drawGradientRect");

    // 3. Three-stop gradient
    Panel split("SplitCard");
    split.setContext(&ctx);
    split.addClass("split-three-stop");
    split.updateStyleCascade();

    const auto& sStyle = split.computedStyle();
    TEST_ASSERT(sStyle.backgroundGradient.has_value(), "3-stop gradient parsed");
    TEST_ASSERT(sStyle.backgroundGradient->direction == GradientDirection::ToRight, "To right direction");
    TEST_ASSERT(sStyle.backgroundGradient->midColor.has_value(), "Mid color parsed for 3-stop split");
    TEST_ASSERT(sStyle.backgroundGradient->midColor->r == 218 && sStyle.backgroundGradient->midColor->g == 175, "Mid color RGB");

    // Render 3-stop gradient
    MockRenderBackend backend3;
    split.setBounds(Rect{ 0.0f, 0.0f, 200.0f, 50.0f });
    split.render(backend3);

    int gradCalls = 0;
    for (const auto& cmd : backend3.commands()) {
        if (cmd.type == MockRenderBackend::DrawCommand::Type::GradientRect) {
            gradCalls++;
        }
    }
    TEST_ASSERT(gradCalls == 2, "3-stop gradient rendered as 2 half-width gradient rects");

    std::cout << "  -> Passed TestLinearGradients" << std::endl;
}

void TestSpringTransitions() {
    std::cout << "[Test] Running TestSpringTransitions..." << std::endl;

    StyleSheet sheet;
    sheet.loadString(R"(
        .menu-card {
            transform-offset-y: 0px;
            transform-scale: 1.0;
            outline: 1px solid rgba(218, 175, 90, 0.25);
            transition: transform-offset-y 0.35s spring(stiffness: 190, damping: 18),
                        transform-scale 0.35s spring(200, 15),
                        outline-color 0.2s linear;
        }
        .menu-card:hover {
            transform-offset-y: -22px;
            transform-scale: 1.03;
            outline: 1px solid rgba(218, 175, 90, 0.65);
        }
    )");

    UIContext ctx;
    ctx.styleSheet() = sheet;

    Panel card("CardSpring");
    card.setContext(&ctx);
    card.addClass("menu-card");
    card.updateStyleCascade(); // Cold mount

    TEST_ASSERT(card.renderOffset().y == 0.0f, "Initial offset Y is 0");
    TEST_ASSERT(card.renderScale().x == 1.0f, "Initial scale is 1.0");

    // Hover in
    card.onPointerEnter();
    card.updateStyleCascade();

    auto* trans = card.transitions();
    TEST_ASSERT(trans != nullptr && trans->hasActiveTransitions(), "Transitions active on hover");
    TEST_ASSERT(trans->trackTransformOffset.active, "Offset transition active");
    TEST_ASSERT(trans->trackTransformOffset.timingTypeY == TransitionTimingType::Spring, "Y transition timing is Spring");
    TEST_ASSERT(trans->trackTransformOffset.springYCfg.stiffness == 190.0f, "Stiffness parsed 190");
    TEST_ASSERT(trans->trackTransformOffset.springYCfg.damping == 18.0f, "Damping parsed 18");
    TEST_ASSERT(trans->trackTransformScale.timingTypeX == TransitionTimingType::Spring, "Scale transition timing is Spring");

    // Tick 0.1s
    card.update(0.1f);
    float midY = card.renderOffset().y;
    TEST_ASSERT(midY < -2.0f && midY > -22.0f, "Spring moving towards target (-22px)");

    // Tick further to peak / overshoot
    bool hadOvershoot = false;
    for (int i = 0; i < 20; ++i) {
        card.update(0.02f);
        if (card.renderOffset().y < -22.01f) {
            hadOvershoot = true;
            break;
        }
    }
    TEST_ASSERT(hadOvershoot, "Physical spring exhibited realistic overshoot beyond target!");

    // Settle to rest
    for (int i = 0; i < 50; ++i) {
        card.update(0.02f);
    }
    TEST_ASSERT(std::abs(card.renderOffset().y - (-22.0f)) < 0.05f, "Spring settled on -22px target");
    TEST_ASSERT(std::abs(card.renderScale().x - 1.03f) < 0.01f, "Scale settled on 1.03 target");

    // Test mid-transition reversal with spring
    card.onPointerLeave();
    card.updateStyleCascade();
    TEST_ASSERT(trans->trackTransformOffset.active, "Reversal active");
    TEST_ASSERT(trans->trackTransformOffset.targetVal.y == 0.0f, "Target is back to 0");

    // Tick return
    for (int i = 0; i < 60; ++i) {
        card.update(0.02f);
    }
    TEST_ASSERT(std::abs(card.renderOffset().y - 0.0f) < 0.05f, "Spring successfully settled back to 0px");
    TEST_ASSERT(std::abs(card.renderScale().x - 1.0f) < 0.01f, "Scale back to 1.0");

    std::cout << "  -> Passed TestSpringTransitions" << std::endl;
}

void TestViewportUnits() {
    std::cout << "[Test] Running TestViewportUnits..." << std::endl;

    // 1. Direct DimensionConstraint and Length resolution
    Dimensions vp1080{ 1920.0f, 1080.0f };
    auto dcW = DimensionConstraint::Vw(50.0f);
    TEST_ASSERT(std::abs(dcW.resolve(0.0f, vp1080) - 960.0f) < 0.001f, "50vw on 1080p is 960px");

    auto dcH = DimensionConstraint::Vh(25.0f);
    TEST_ASSERT(std::abs(dcH.resolve(0.0f, vp1080) - 270.0f) < 0.001f, "25vh on 1080p is 270px");

    auto dcMin = DimensionConstraint::Vmin(10.0f);
    TEST_ASSERT(std::abs(dcMin.resolve(0.0f, vp1080) - 108.0f) < 0.001f, "10vmin on 1080p is 108px");

    auto dcMax = DimensionConstraint::Vmax(10.0f);
    TEST_ASSERT(std::abs(dcMax.resolve(0.0f, vp1080) - 192.0f) < 0.001f, "10vmax on 1080p is 192px");

    // 2. 4K screen test
    Dimensions vp4k{ 3840.0f, 2160.0f };
    TEST_ASSERT(std::abs(dcW.resolve(0.0f, vp4k) - 1920.0f) < 0.001f, "50vw on 4K is 1920px");
    TEST_ASSERT(std::abs(dcH.resolve(0.0f, vp4k) - 540.0f) < 0.001f, "25vh on 4K is 540px");

    // 3. LayoutEngine measure with viewport units
    UIContext ctx;
    ctx.setViewportSize(Dimensions{ 1920.0f, 1080.0f });

    Panel panel;
    panel.setContext(&ctx);
    panel.layout().width(DimensionConstraint::Vw(30.0f));
    panel.layout().height(DimensionConstraint::Vh(40.0f));
    panel.measure(Dimensions{ 1920.0f, 1080.0f });

    TEST_ASSERT(std::abs(panel.desiredSize().width - 576.0f) < 0.1f, "30vw desired width is 576px");
    TEST_ASSERT(std::abs(panel.desiredSize().height - 432.0f) < 0.1f, "40vh desired height is 432px");

    // 4. PSS parsing of viewport units
    StyleSheet sheet;
    sheet.loadString(R"(
        .responsive-card {
            width: 25vw;
            height: 50vh;
            left: 5vw;
            top: 10vh;
        }
    )");

    Panel card;
    card.setContext(&ctx);
    card.addClass("responsive-card");
    card.applyStyle(sheet);

    TEST_ASSERT(card.layout().width().mode == SizeMode::Vw, "Width constraint mode is Vw");
    TEST_ASSERT(card.layout().width().value == 25.0f, "Width constraint value is 25");
    TEST_ASSERT(card.layout().height().mode == SizeMode::Vh, "Height constraint mode is Vh");
    TEST_ASSERT(card.layout().height().value == 50.0f, "Height constraint value is 50");

    card.measure(Dimensions{ 1920.0f, 1080.0f });
    TEST_ASSERT(std::abs(card.desiredSize().width - 480.0f) < 0.1f, "25vw desired width is 480px on 1080p");
    TEST_ASSERT(std::abs(card.desiredSize().height - 540.0f) < 0.1f, "50vh desired height is 540px on 1080p");

    // 5. Dynamic screen resize adaptation
    ctx.setViewportSize(Dimensions{ 2560.0f, 1440.0f }); // 2K QHD
    card.measure(Dimensions{ 2560.0f, 1440.0f });
    TEST_ASSERT(std::abs(card.desiredSize().width - 640.0f) < 0.1f, "25vw desired width dynamically updates to 640px on 1440p");
    TEST_ASSERT(std::abs(card.desiredSize().height - 720.0f) < 0.1f, "50vh desired height dynamically updates to 720px on 1440p");

    // 6. Ultrawide screen tests: 21:9 (3440x1440) and 32:9 (5120x1440)
    Dimensions vp21_9{ 3440.0f, 1440.0f };
    TEST_ASSERT(std::abs(dcW.resolve(0.0f, vp21_9) - 1720.0f) < 0.001f, "50vw on 3440x1440 is 1720px");
    TEST_ASSERT(std::abs(dcH.resolve(0.0f, vp21_9) - 360.0f) < 0.001f, "25vh on 3440x1440 is 360px");
    TEST_ASSERT(std::abs(dcMin.resolve(0.0f, vp21_9) - 144.0f) < 0.001f, "10vmin on 3440x1440 is 144px (min is 1440)");
    TEST_ASSERT(std::abs(dcMax.resolve(0.0f, vp21_9) - 344.0f) < 0.001f, "10vmax on 3440x1440 is 344px (max is 3440)");

    ctx.setViewportSize(vp21_9);
    card.measure(vp21_9);
    TEST_ASSERT(std::abs(card.desiredSize().width - 860.0f) < 0.1f, "25vw desired width on 21:9 (3440x1440) is 860px");
    TEST_ASSERT(std::abs(card.desiredSize().height - 720.0f) < 0.1f, "50vh desired height on 21:9 (3440x1440) is 720px");

    Dimensions vp32_9{ 5120.0f, 1440.0f };
    TEST_ASSERT(std::abs(dcW.resolve(0.0f, vp32_9) - 2560.0f) < 0.001f, "50vw on 5120x1440 is 2560px");
    TEST_ASSERT(std::abs(dcH.resolve(0.0f, vp32_9) - 360.0f) < 0.001f, "25vh on 5120x1440 is 360px");
    TEST_ASSERT(std::abs(dcMin.resolve(0.0f, vp32_9) - 144.0f) < 0.001f, "10vmin on 32:9 is 144px (min is 1440)");
    TEST_ASSERT(std::abs(dcMax.resolve(0.0f, vp32_9) - 512.0f) < 0.001f, "10vmax on 32:9 is 512px (max is 5120)");

    ctx.setViewportSize(vp32_9);
    card.measure(vp32_9);
    TEST_ASSERT(std::abs(card.desiredSize().width - 1280.0f) < 0.1f, "25vw desired width on 32:9 (5120x1440) is 1280px");
    TEST_ASSERT(std::abs(card.desiredSize().height - 720.0f) < 0.1f, "50vh desired height on 32:9 (5120x1440) is 720px");

    std::cout << "  -> Passed TestViewportUnits" << std::endl;
}

void TestKeyframeAnimations() {
    std::cout << "[Test] Running TestKeyframeAnimations..." << std::endl;

    // 1. Parse @keyframes block and animation shorthand in PSS
    StyleSheet sheet;
    bool ok = sheet.loadString(R"(
        @keyframes pulse {
            0% {
                opacity: 0.2;
                transform-scale: 0.8;
                background-color: #000000;
            }
            50% {
                opacity: 1.0;
                transform-scale: 1.2;
                background-color: #ffd700;
            }
            100% {
                opacity: 0.2;
                transform-scale: 0.8;
                background-color: #000000;
            }
        }

        Button.pulsing {
            animation: pulse 2s infinite linear;
        }

        Panel.slide-in {
            animation-name: slide;
            animation-duration: 500ms;
            animation-timing-function: ease-out;
            animation-fill-mode: forwards;
        }

        @keyframes slide {
            from {
                transform-offset-y: 50px;
                opacity: 0.0;
            }
            to {
                transform-offset-y: 0px;
                opacity: 1.0;
            }
        }
    )");
    TEST_ASSERT(ok, "StyleSheet loaded @keyframes without errors");

    // 2. Verify keyframes registered in sheet
    const auto* kfPulse = sheet.findKeyframes("pulse");
    TEST_ASSERT(kfPulse != nullptr, "Keyframes 'pulse' registered");
    TEST_ASSERT(kfPulse->steps.size() == 3, "Pulse has 3 keyframe steps");
    TEST_ASSERT(kfPulse->steps[0].offset == 0.0f, "Step 0 offset is 0.0");
    TEST_ASSERT(kfPulse->steps[1].offset == 0.5f, "Step 1 offset is 0.5");
    TEST_ASSERT(kfPulse->steps[2].offset == 1.0f, "Step 2 offset is 1.0");
    TEST_ASSERT(kfPulse->steps[1].backgroundColor == Color(255, 215, 0, 255), "Step 1 bg is gold");

    const auto* kfSlide = sheet.findKeyframes("slide");
    TEST_ASSERT(kfSlide != nullptr, "Keyframes 'slide' registered");
    TEST_ASSERT(kfSlide->steps.size() == 2, "Slide has from/to (2 steps)");

    // 3. Verify element receives animation spec
    Button btn("TestBtn");
    btn.addClass("pulsing");
    btn.applyStyle(sheet);

    TEST_ASSERT(btn.keyframeAnimations() != nullptr, "Button allocated keyframe animation controller");
    TEST_ASSERT(btn.keyframeAnimations()->hasActiveAnimations(), "Button has active keyframe animation");

    // 4. Runtime update at t = 0.0s (start)
    btn.update(0.0f);
    TEST_ASSERT(std::abs(btn.opacity() - 0.2f) < 0.02f, "Initial opacity is 0.2");
    TEST_ASSERT(std::abs(btn.renderScale().x - 0.8f) < 0.02f, "Initial scale is 0.8");

    // 5. Runtime update at t = 1.0s (50% midpoint)
    btn.update(1.0f);
    TEST_ASSERT(std::abs(btn.opacity() - 1.0f) < 0.02f, "Midpoint opacity reached 1.0");
    TEST_ASSERT(std::abs(btn.renderScale().x - 1.2f) < 0.02f, "Midpoint scale reached 1.2");
    BoxStyle bs = btn.resolveBoxStyle(WidgetState::Normal);
    TEST_ASSERT(bs.backgroundColor == Color(255, 215, 0, 255), "Midpoint background color is gold");

    // 6. Runtime update at t = 2.0s (100% full cycle)
    btn.update(1.0f);
    TEST_ASSERT(std::abs(btn.opacity() - 0.2f) < 0.05f, "End-of-cycle opacity returned to 0.2");
    TEST_ASSERT(std::abs(btn.renderScale().x - 0.8f) < 0.05f, "End-of-cycle scale returned to 0.8");

    // 7. Verify infinite loop continues into second cycle (t = 2.5s -> 25% of second cycle)
    btn.update(0.5f);
    TEST_ASSERT(btn.keyframeAnimations()->hasActiveAnimations(), "Infinite animation still running");
    TEST_ASSERT(std::abs(btn.opacity() - 0.6f) < 0.05f, "Second cycle 25% opacity is ~0.6");

    // 8. Test Slide panel with fill-mode: forwards
    Panel slidePanel;
    slidePanel.addClass("slide-in");
    slidePanel.applyStyle(sheet);

    // Initial state before update: tick to 0.55s (end of 500ms duration)
    slidePanel.update(0.55f);
    TEST_ASSERT(std::abs(slidePanel.opacity() - 1.0f) < 0.02f, "Forwards fill mode holds opacity 1.0");
    TEST_ASSERT(std::abs(slidePanel.renderOffset().y - 0.0f) < 0.05f, "Forwards fill mode holds offset Y 0.0");
    TEST_ASSERT(!slidePanel.keyframeAnimations()->hasActiveAnimations(), "Finished non-infinite animation stopped");

    // 9. Test animation without fill-mode: must restore base style upon completion
    StyleSheet testSheet1;
    testSheet1.loadString(R"(
        @keyframes fadeOutIn {
            0% { opacity: 0.1; transform-scale: 0.5; background-color: #ff0000; }
            100% { opacity: 0.9; transform-scale: 1.5; background-color: #00ff00; }
        }
        .base-box {
            opacity: 0.75;
            transform-scale: 1.0;
            background-color: #0000ff;
            animation: fadeOutIn 1.0s linear 1 none;
        }
    )");
    Panel basePanel;
    basePanel.addClass("base-box");
    basePanel.applyStyle(testSheet1);
    basePanel.update(0.0f);
    TEST_ASSERT(std::abs(basePanel.opacity() - 0.1f) < 0.05f, "Start frame applied opacity 0.1");
    TEST_ASSERT(std::abs(basePanel.renderScale().x - 0.5f) < 0.05f, "Start frame applied scale 0.5");
    basePanel.update(1.1f); // finished
    TEST_ASSERT(!basePanel.keyframeAnimations()->hasActiveAnimations(), "Animation finished");
    TEST_ASSERT(std::abs(basePanel.opacity() - 0.75f) < 0.05f, "Without fill-forwards, restored to base opacity 0.75");
    TEST_ASSERT(std::abs(basePanel.renderScale().x - 1.0f) < 0.05f, "Without fill-forwards, restored to base scale 1.0");
    TEST_ASSERT(basePanel.resolveBoxStyle(WidgetState::Normal).backgroundColor == Color(0, 0, 255, 255), "Without fill-forwards, restored to base blue bg");

    // 10. Test class removal / animation cancellation midway returns to base style
    Panel cancelPanel;
    cancelPanel.addClass("base-box");
    cancelPanel.applyStyle(testSheet1);
    cancelPanel.update(0.5f); // midway
    TEST_ASSERT(std::abs(cancelPanel.opacity() - 0.5f) < 0.05f, "Midway opacity is ~0.5");
    cancelPanel.removeClass("base-box");
    cancelPanel.applyStyle(testSheet1); // re-compute without animation
    TEST_ASSERT(std::abs(cancelPanel.opacity() - 1.0f) < 0.05f || std::abs(cancelPanel.opacity() - 0.75f) < 0.05f, "After class removal, opacity restored from base");

    // 11. Test implicit keyframes (Points 2 & 3): missing from/to and intermediate omissions
    StyleSheet implicitSheet;
    implicitSheet.loadString(R"(
        @keyframes fadeTo {
            to { opacity: 1.0; }
        }
        @keyframes scaleFrom {
            from { transform-scale: 0.5; }
        }
        @keyframes colorTo {
            to { background-color: #ffffff; }
        }
        @keyframes demoGap {
            0%   { opacity: 0.0; }
            50%  { transform-scale: 1.2; }
            100% { opacity: 1.0; }
        }
        .fade-to-box {
            opacity: 0.2;
            animation: fadeTo 1.0s linear 1 forwards;
        }
        .scale-from-box {
            transform-scale: 1.0;
            animation: scaleFrom 1.0s linear 1 forwards;
        }
        .color-to-box {
            background-color: #000000;
            animation: colorTo 1.0s linear 1 forwards;
        }
        .gap-box {
            opacity: 0.2;
            transform-scale: 1.0;
            animation: demoGap 1.0s linear 1 forwards;
        }
    )");

    Panel fadePanel;
    fadePanel.addClass("fade-to-box");
    fadePanel.applyStyle(implicitSheet);
    fadePanel.update(0.5f); // 50%
    TEST_ASSERT(std::abs(fadePanel.opacity() - 0.6f) < 0.05f, "Implicit 'from' uses base opacity 0.2 -> 0.6 at 50%");

    Panel scalePanel;
    scalePanel.addClass("scale-from-box");
    scalePanel.applyStyle(implicitSheet);
    scalePanel.update(0.5f); // 50%
    TEST_ASSERT(std::abs(scalePanel.renderScale().x - 0.75f) < 0.05f, "Implicit 'to' uses base scale 1.0 -> 0.75 at 50%");

    Panel colorPanel;
    colorPanel.addClass("color-to-box");
    colorPanel.applyStyle(implicitSheet);
    colorPanel.update(0.5f); // 50%
    Color curBg = colorPanel.resolveBoxStyle(WidgetState::Normal).backgroundColor.value_or(Color::Black());
    TEST_ASSERT(std::abs((int)curBg.r - 127) <= 2 && std::abs((int)curBg.g - 127) <= 2, "Implicit 'from' uses base black color -> ~127 at 50%");

    Panel gapPanel;
    gapPanel.addClass("gap-box");
    gapPanel.applyStyle(implicitSheet);
    gapPanel.update(0.25f);
    TEST_ASSERT(std::abs(gapPanel.opacity() - 0.25f) < 0.05f, "Continuous opacity across 50% gap is ~0.25 at 25%");
    TEST_ASSERT(std::abs(gapPanel.renderScale().x - 1.1f) < 0.05f, "Scale with implicit from/to reaches ~1.1 at 25%");
    gapPanel.update(0.25f); // now at 0.5s
    TEST_ASSERT(std::abs(gapPanel.opacity() - 0.5f) < 0.05f, "Continuous opacity is ~0.5 at 50%");
    TEST_ASSERT(std::abs(gapPanel.renderScale().x - 1.2f) < 0.05f, "Scale peak at 50% is 1.2");

    // 12. Test per-segment easing (Point 4): midpoint 0.5 hits 50% keyframe exactly with ease-in-out
    StyleSheet easingSheet;
    easingSheet.loadString(R"(
        @keyframes customTiming {
            0% {
                opacity: 0.0;
                animation-timing-function: linear;
            }
            50% {
                opacity: 0.8;
                animation-timing-function: ease-in-out;
            }
            100% {
                opacity: 1.0;
            }
        }
        .timing-box {
            animation: customTiming 1.0s ease-in-out 1 forwards;
        }
    )");

    Panel timingPanel;
    timingPanel.addClass("timing-box");
    timingPanel.applyStyle(easingSheet);

    // At t = 0.25s (midway between 0% and 50% where timing-function is linear):
    // Opacity goes from 0.0 to 0.8 with linear -> at segT = 0.5 it should be exactly 0.4!
    timingPanel.update(0.25f);
    TEST_ASSERT(std::abs(timingPanel.opacity() - 0.4f) < 0.02f, "Linear segment easing at 25% gives exactly 0.4 (half of 0.8)");

    // At t = 0.5s (exact 50% stop):
    // Even with ease-in-out on the animation, 50% time hits 50% keyframe value 0.8!
    timingPanel.update(0.25f); // now 0.5s
    TEST_ASSERT(std::abs(timingPanel.opacity() - 0.8f) < 0.02f, "Exact 50% keyframe reached at progress 0.5 (opacity 0.8)");

    // 13. Test Point 5 details: fractional iterations, width constraint restore, sorting/merging, warnings, reducedMotion
    StyleSheet detailsSheet;
    detailsSheet.loadString(R"(
        @keyframes unorderedDup {
            100% { opacity: 1.0; }
            from { opacity: 0.1; transform-scale: 1.0; }
            50%  { opacity: 0.5; }
            0%   { opacity: 0.3; } /* duplicate offset 0%: should override opacity 0.1 to 0.3 while keeping scale 1.0 */
            to   { font-family: "Arial"; } /* non-animatable warning */
        }
        @keyframes fractionalAnim {
            0%   { opacity: 0.0; }
            100% { opacity: 1.0; }
        }
        @keyframes animWidth {
            0%   { width: 100px; }
            100% { width: 300px; }
        }
        .frac-normal {
            animation: fractionalAnim 1.0s linear 1.5 normal forwards;
        }
        .frac-alt {
            animation: fractionalAnim 1.0s linear 1.5 alternate forwards;
        }
        .width-box {
            animation: animWidth 1.0s linear 1 none;
        }
    )", "test_style.pss");

    // 5.4 Check sorting and duplicate merging
    const auto* kfMerged = detailsSheet.findKeyframes("unorderedDup");
    TEST_ASSERT(kfMerged != nullptr, "Keyframes unorderedDup parsed");
    TEST_ASSERT(kfMerged->steps.size() == 3, "Merged duplicate 0% and 100% offsets into 3 unique steps (0%, 50%, 100%)");
    TEST_ASSERT(kfMerged->steps[0].offset == 0.0f, "Step 0 is 0%");
    TEST_ASSERT(kfMerged->steps[0].opacity == 0.3f, "Later duplicate 0% step overrode opacity to 0.3");
    TEST_ASSERT(kfMerged->steps[0].transformScale == Point(1.0f, 1.0f), "Preserved transformScale 1.0 from earlier 0% step");
    TEST_ASSERT(kfMerged->steps[1].offset == 0.5f, "Step 1 is 50%");
    TEST_ASSERT(kfMerged->steps[2].offset == 1.0f, "Step 2 is 100%");

    // 5.5 Check warning with file:line:col format
    bool foundWarning = false;
    for (const auto& w : detailsSheet.warnings()) {
        if (w.find("Property 'font-family' in @keyframes is not animatable") != std::string::npos &&
            w.find("test_style.pss:") != std::string::npos) {
            foundWarning = true;
            break;
        }
    }
    TEST_ASSERT(foundWarning, "Warning generated with file:line:col for non-animatable property");

    // 5.2 Fractional iteration count
    Panel fracPanel;
    fracPanel.addClass("frac-normal");
    fracPanel.applyStyle(detailsSheet);
    fracPanel.update(1.6f); // 1.5 iterations completed (1.5s)
    TEST_ASSERT(std::abs(fracPanel.opacity() - 0.5f) < 0.05f, "Normal 1.5 iterations finishes at 50% frame (0.5 opacity)");

    Panel fracAltPanel;
    fracAltPanel.addClass("frac-alt");
    fracAltPanel.applyStyle(detailsSheet);
    fracAltPanel.update(1.6f);
    TEST_ASSERT(std::abs(fracAltPanel.opacity() - 0.5f) < 0.05f, "Alternate 1.5 iterations finishes at 50% backward frame (0.5 opacity)");

    // 5.3 Width DimensionConstraint restoration
    Panel widthPanel;
    widthPanel.layout().width(DimensionConstraint::Percent(60.0f));
    widthPanel.addClass("width-box");
    widthPanel.applyStyle(detailsSheet);
    widthPanel.update(0.5f);
    TEST_ASSERT(widthPanel.layout().width().mode == SizeMode::Fixed, "During animation width is fixed pixel constraint");
    widthPanel.update(0.6f); // finished (1.1s > 1.0s)
    TEST_ASSERT(widthPanel.layout().width().mode == SizeMode::Percent, "After animation without forwards fill, restored to Percent mode");
    TEST_ASSERT(std::abs(widthPanel.layout().width().value - 60.0f) < 0.01f, "Restored original percent width constraint 60%");

    // 5.1 reducedMotion handling
    UIContext reducedCtx;
    reducedCtx.setReducedMotion(true);

    Panel rmPanel;
    rmPanel.setContext(&reducedCtx);
    rmPanel.addClass("frac-normal");
    rmPanel.applyStyle(detailsSheet);
    rmPanel.update(0.0f); // with reducedMotion, finishes instantly
    TEST_ASSERT(!rmPanel.keyframeAnimations()->hasActiveAnimations(), "Finite animation finishes instantly in reducedMotion");
    TEST_ASSERT(std::abs(rmPanel.opacity() - 0.5f) < 0.05f, "Finite animation with forwards fill takes final fractional frame in reducedMotion");

    // 14. Edge cases: empty keyframes, single keyframe, paused play-state, infinite alternate
    StyleSheet edgeSheet;
    edgeSheet.loadString(R"(
        @keyframes emptyAnim {
        }
        @keyframes singleKeyframeAnim {
            50% { opacity: 0.8; }
        }
        .empty-box {
            animation: emptyAnim 1s linear infinite;
        }
        .single-box {
            opacity: 0.2;
            animation: singleKeyframeAnim 1.0s linear 1 forwards;
        }
        .paused-box {
            opacity: 0.2;
            animation: singleKeyframeAnim 1.0s linear 1 forwards;
            animation-play-state: paused;
        }
        .alt-inf-box {
            opacity: 0.2;
            animation: singleKeyframeAnim 1.0s linear infinite alternate;
        }
    )");

    Panel emptyPanel;
    emptyPanel.addClass("empty-box");
    emptyPanel.applyStyle(edgeSheet);
    emptyPanel.update(0.5f);
    TEST_ASSERT(emptyPanel.keyframeAnimations() == nullptr || !emptyPanel.keyframeAnimations()->hasActiveAnimations(), "Empty keyframes does not create active running animation");

    Panel singlePanel;
    singlePanel.addClass("single-box");
    singlePanel.applyStyle(edgeSheet);
    singlePanel.update(0.25f);
    TEST_ASSERT(std::abs(singlePanel.opacity() - 0.5f) < 0.05f, "Single frame at 50% uses implicit base at 0% (0.5 at 25%)");
    singlePanel.update(0.25f);
    TEST_ASSERT(std::abs(singlePanel.opacity() - 0.8f) < 0.05f, "Single frame reaches 0.8 at 50%");
    singlePanel.update(0.5f);
    TEST_ASSERT(std::abs(singlePanel.opacity() - 0.2f) < 0.05f, "Single frame returns to base 0.2 at 100%");

    Panel pausedPanel;
    pausedPanel.addClass("paused-box");
    pausedPanel.applyStyle(edgeSheet);
    pausedPanel.update(0.5f);
    TEST_ASSERT(std::abs(pausedPanel.opacity() - 0.2f) < 0.01f, "Paused animation does not advance opacity");

    Panel altInfPanel;
    altInfPanel.addClass("alt-inf-box");
    altInfPanel.applyStyle(edgeSheet);
    altInfPanel.update(1.25f);
    TEST_ASSERT(altInfPanel.keyframeAnimations()->hasActiveAnimations(), "Infinite alternate still active");
    TEST_ASSERT(std::abs(altInfPanel.opacity() - 0.5f) < 0.05f, "Infinite alternate in reverse phase correctly interpolates");

    std::cout << "  -> Passed TestKeyframeAnimations" << std::endl;
}

void TestKeyframeAnimationBenchmark() {
    std::cout << "[Benchmark] Running Keyframe Animation Benchmark (100 elements, 1000 frames)..." << std::endl;

    StyleSheet sheet;
    sheet.loadString(R"(
        @keyframes complexPulse {
            0% {
                opacity: 0.2;
                transform-scale: 0.8;
                background-color: #ff0000;
            }
            50% {
                opacity: 1.0;
                transform-scale: 1.2;
                background-color: #00ff00;
            }
            100% {
                opacity: 0.2;
                transform-scale: 0.8;
                background-color: #ff0000;
            }
        }
        .bench-box {
            animation: complexPulse 1.0s linear infinite;
        }
    )");

    constexpr int kElementCount = 100;
    constexpr int kFrameCount = 1000;
    constexpr float kDt = 0.016f; // ~60 FPS

    std::vector<std::unique_ptr<Panel>> elements;
    elements.reserve(kElementCount);
    for (int i = 0; i < kElementCount; ++i) {
        auto p = std::make_unique<Panel>("BenchPanel" + std::to_string(i));
        p->addClass("bench-box");
        p->applyStyle(sheet);
        elements.push_back(std::move(p));
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int frame = 0; frame < kFrameCount; ++frame) {
        for (auto& el : elements) {
            el->update(kDt);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double totalUs = std::chrono::duration<double, std::micro>(end - start).count();
    double perFrameUs = totalUs / kFrameCount;
    double perElementPerFrameUs = perFrameUs / kElementCount;

    std::cout << "  * Total Time (1000 frames): " << (totalUs / 1000.0) << " ms" << std::endl;
    std::cout << "  * Cost per frame (100 elements): " << perFrameUs << " us (" << (perFrameUs / 1000.0) << " ms)" << std::endl;
    std::cout << "  * Cost per element per frame: " << perElementPerFrameUs << " us" << std::endl;

    TEST_ASSERT(perFrameUs < 500.0, "100 animated elements update cost under 500 us (0.5 ms) per frame");
    std::cout << "  -> Passed TestKeyframeAnimationBenchmark" << std::endl;
}

void TestKeyframeBaseRestorationAndTracks() {
    std::cout << "[Test] Running TestKeyframeBaseRestorationAndTracks..." << std::endl;

    StyleSheet sheet;
    sheet.loadString(R"(
        @keyframes fadeToFull {
            to { opacity: 1.0; }
        }
        @keyframes animBorderColor {
            to { border-color: #ff0000; }
        }
        @keyframes animBgRed {
            to { background: #ff0000; }
        }
        @keyframes animScaleOnly {
            0%   { transform-scale: 1.0; }
            50%  { transform-scale: 2.0; }
            100% { transform-scale: 1.0; }
        }
        @keyframes animBgOnly {
            0%   { background-color: #0000ff; }
            100% { background-color: #00ff00; }
        }

        .base-op-20 {
            opacity: 0.2;
            animation: fadeToFull 1.0s linear 1 none;
        }
        .base-op-60 {
            opacity: 0.6;
            animation: fadeToFull 1.0s linear 1 none;
        }
        .scale-box {
            animation: animScaleOnly 1.0s linear 1 none;
        }
        .multi-anim-box {
            animation: animScaleOnly 1.0s linear 1 none, animBgOnly 2.0s linear 1 none;
        }
        .border-box {
            animation: animBorderColor 1.0s linear 1 forwards;
        }
        .bg-box {
            animation: animBgRed 1.0s linear 1 none;
        }
    )");

    // A1. Test base snapshot freshness when class/style changes during animation
    Panel pA1;
    pA1.addClass("base-op-20");
    pA1.applyStyle(sheet);
    pA1.update(0.5f); // halfway
    // Change base class midway
    pA1.removeClass("base-op-20");
    pA1.addClass("base-op-60");
    pA1.applyStyle(sheet);
    pA1.update(0.6f); // finished (total 1.1s > 1.0s)
    TEST_ASSERT(!pA1.keyframeAnimations()->hasActiveAnimations(), "Animation finished");
    TEST_ASSERT(std::abs(pA1.opacity() - 0.6f) < 0.05f, "After finish without forwards, restored to updated base opacity 0.6 (not old 0.2)");

    // A2. Selective restoration & layout dirty flag
    Panel pA2;
    pA2.addClass("base-op-20");
    pA2.applyStyle(sheet);
    pA2.measure({ 100, 100 });
    pA2.arrange({ 0, 0, 100, 100 });
    TEST_ASSERT(!pA2.isLayoutDirty(), "Layout clean before animation update");
    // Change scale manually from C++ during opacity animation
    pA2.setRenderScale(Point{ 3.0f, 3.0f });
    pA2.update(1.1f); // finish opacity animation
    TEST_ASSERT(!pA2.isLayoutDirty(), "Pure visual animation finish did NOT mark layout dirty");
    TEST_ASSERT(std::abs(pA2.renderScale().x - 3.0f) < 0.01f, "Manual C++ scale 3.0 was NOT wiped by opacity animation finish");

    // A2. Multi-animation: finish of one does not wipe property animated by other
    Panel pMulti;
    pMulti.addClass("multi-anim-box");
    pMulti.applyStyle(sheet);
    pMulti.update(1.1f); // animScaleOnly finished (1.0s), animBgOnly still running (2.0s)
    TEST_ASSERT(pMulti.keyframeAnimations()->hasActiveAnimations(), "Multi-anim still has animBgOnly running");
    TEST_ASSERT(std::abs(pMulti.renderScale().x - 1.0f) < 0.05f, "Finished scale animation restored scale");

    // A3. Missing border: at t=0 border is not visible (alpha 0), at t=1 it is red
    Panel pBorder;
    pBorder.addClass("border-box");
    pBorder.applyStyle(sheet);
    pBorder.update(0.0f); // start
    BoxStyle bsBorder0 = pBorder.resolveBoxStyle(WidgetState::Normal);
    Color col0 = bsBorder0.borderColor.value_or(Color::Transparent());
    TEST_ASSERT(col0.a == 0, "Element without base border starts at border alpha 0 (invisible at t=0)");

    pBorder.update(1.0f); // end
    BoxStyle bsBorder1 = pBorder.resolveBoxStyle(WidgetState::Normal);
    Color col1 = bsBorder1.borderColor.value_or(Color::Transparent());
    TEST_ASSERT(col1.r == 255 && col1.g == 0 && col1.b == 0 && col1.a == 255, "At t=1 border is fully red");

    // A3. Missing border-width animates from 0.0px, not 1.0px
    StyleSheet bwSheet;
    bwSheet.loadString(R"(
        @keyframes animBw {
            to { border-width: 4px; }
        }
        .bw-box {
            animation: animBw 1.0s linear 1 forwards;
        }
    )");
    Panel pBw;
    pBw.addClass("bw-box");
    pBw.applyStyle(bwSheet);
    pBw.update(0.5f); // at 50% it should be 2.0px (from 0 to 4), NOT 2.5px (from 1 to 4)
    float midBw = pBw.style().box.normal.has_value() ? pBw.style().box.normal->borderWidth.value_or(0.0f) : 0.0f;
    TEST_ASSERT(std::abs(midBw - 2.0f) < 0.05f, "Element without base border animates border-width from 0 (at 50% is 2px, not 2.5px)");

    // A3. Color lerp from transparent: at t=0.5 with red target, RGB is pure red without darkening
    Panel pBg;
    pBg.addClass("bg-box");
    pBg.applyStyle(sheet);
    pBg.update(0.5f);
    BoxStyle bs = pBg.resolveBoxStyle(WidgetState::Normal);
    Color midCol = bs.backgroundColor.value_or(Color::Transparent());
    TEST_ASSERT(midCol.r > 240, "Midway background color red channel is fully vibrant (not darkened to 127)");
    TEST_ASSERT(midCol.a > 100 && midCol.a < 155, "Midway background alpha is ~127");

    std::cout << "  -> Passed TestKeyframeBaseRestorationAndTracks" << std::endl;
}

void TestViewportUnitsExtended() {
    std::cout << "[Test] Running TestViewportUnitsExtended..." << std::endl;

    // 1. Multi-resolution scaling across 1080p, 21:9 (3440x1440), 32:9 (5120x1440), 4K (3840x2160)
    UIContext ctx;
    ctx.setViewportSize(Dimensions{ 1920.0f, 1080.0f });
    StyleSheet& sheet = ctx.styleSheet();
    sheet.loadString(R"(
        :root {
            font-size: 16px;
        }
        .scaled-box {
            font-size: 2vh;
            padding: 1vmin;
            gap: 1vw;
            border-radius: 0.5vh;
            border-width: 0.2vw;
            margin: 1.5rem;
        }
    )");

    Panel* p = ctx.root()->add<Panel>("ScaledBox");
    p->addClass("scaled-box");
    p->applyStyle(sheet);

    // 1080p (1920x1080)
    ctx.setViewportSize(Dimensions{ 1920.0f, 1080.0f });
    float fs1080 = p->style().text.normal->fontSize.value_or(0.0f);
    TEST_ASSERT(std::abs(fs1080 - 21.6f) < 0.2f, "1080p: font-size 2vh is 21.6px");
    TEST_ASSERT(std::abs(p->layout().padding().left - 10.8f) < 0.2f, "1080p: padding 1vmin is 10.8px");
    TEST_ASSERT(std::abs(p->layout().gap() - 19.2f) < 0.2f, "1080p: gap 1vw is 19.2px");
    TEST_ASSERT(std::abs(p->style().box.normal->cornerRadius.value_or(0.0f) - 5.4f) < 0.2f, "1080p: border-radius 0.5vh is 5.4px");
    TEST_ASSERT(std::abs(p->style().box.normal->borderWidth.value_or(0.0f) - 3.84f) < 0.2f, "1080p: border-width 0.2vw is 3.84px");
    TEST_ASSERT(std::abs(p->layout().margin().left - 24.0f) < 0.2f, "1080p: margin 1.5rem (16px base) is 24px");

    // 21:9 Ultrawide (3440x1440)
    ctx.setViewportSize(Dimensions{ 3440.0f, 1440.0f });
    float fs21_9 = p->style().text.normal->fontSize.value_or(0.0f);
    TEST_ASSERT(std::abs(fs21_9 - 28.8f) < 0.2f, "21:9: font-size 2vh is 28.8px");
    TEST_ASSERT(std::abs(p->layout().padding().left - 14.4f) < 0.2f, "21:9: padding 1vmin is 14.4px (min is 1440)");
    TEST_ASSERT(std::abs(p->layout().gap() - 34.4f) < 0.2f, "21:9: gap 1vw is 34.4px");
    TEST_ASSERT(std::abs(p->style().box.normal->cornerRadius.value_or(0.0f) - 7.2f) < 0.2f, "21:9: border-radius 0.5vh is 7.2px");

    // 32:9 Super Ultrawide (5120x1440)
    ctx.setViewportSize(Dimensions{ 5120.0f, 1440.0f });
    float fs32_9 = p->style().text.normal->fontSize.value_or(0.0f);
    TEST_ASSERT(std::abs(fs32_9 - 28.8f) < 0.2f, "32:9: font-size 2vh is 28.8px (height unchanged from 21:9)");
    TEST_ASSERT(std::abs(p->layout().padding().left - 14.4f) < 0.2f, "32:9: padding 1vmin is 14.4px");
    TEST_ASSERT(std::abs(p->layout().gap() - 51.2f) < 0.2f, "32:9: gap 1vw is 51.2px (width scales with 5120)");
    TEST_ASSERT(std::abs(p->style().box.normal->cornerRadius.value_or(0.0f) - 7.2f) < 0.2f, "32:9: border-radius 0.5vh is 7.2px");

    // 4K UHD (3840x2160)
    ctx.setViewportSize(Dimensions{ 3840.0f, 2160.0f });
    float fs4k = p->style().text.normal->fontSize.value_or(0.0f);
    TEST_ASSERT(std::abs(fs4k - 43.2f) < 0.2f, "4K: font-size 2vh is 43.2px");
    TEST_ASSERT(std::abs(p->layout().padding().left - 21.6f) < 0.2f, "4K: padding 1vmin is 21.6px");
    TEST_ASSERT(std::abs(p->layout().gap() - 38.4f) < 0.2f, "4K: gap 1vw is 38.4px");
    TEST_ASSERT(std::abs(p->style().box.normal->cornerRadius.value_or(0.0f) - 10.8f) < 0.2f, "4K: border-radius 0.5vh is 10.8px");

    // 2. Font size clamping: min-font-size and max-font-size (B2)
    StyleSheet clampSheet;
    clampSheet.loadString(R"(
        .clamp-box {
            font-size: 2vh;
            min-font-size: 14px;
            max-font-size: 32px;
        }
    )");

    Panel pClamp;
    pClamp.addClass("clamp-box");

    // 1080p: 2vh = 21.6px (in [14, 32])
    UIContext ctx1080;
    ctx1080.setViewportSize(Dimensions{ 1920.0f, 1080.0f });
    pClamp.setContext(&ctx1080);
    pClamp.applyStyle(clampSheet);
    TEST_ASSERT(std::abs(pClamp.style().text.normal->fontSize.value_or(0.0f) - 21.6f) < 0.2f, "1080p: font size within clamp bounds (21.6px)");

    // 4K (2160p): 2vh = 43.2px -> clamped to 32px
    UIContext ctxClamp4k;
    ctxClamp4k.setViewportSize(Dimensions{ 3840.0f, 2160.0f });
    pClamp.setContext(&ctxClamp4k);
    pClamp.applyStyle(clampSheet);
    TEST_ASSERT(std::abs(pClamp.style().text.normal->fontSize.value_or(0.0f) - 32.0f) < 0.2f, "4K: font size 43.2px clamped to max-font-size 32px");

    // 600p: 2vh = 12px -> clamped to 14px
    UIContext ctxClamp600;
    ctxClamp600.setViewportSize(Dimensions{ 800.0f, 600.0f });
    pClamp.setContext(&ctxClamp600);
    pClamp.applyStyle(clampSheet);
    TEST_ASSERT(std::abs(pClamp.style().text.normal->fontSize.value_or(0.0f) - 14.0f) < 0.2f, "600p: font size 12.0px clamped to min-font-size 14px");

    // 3. Custom :root { font-size: 20px; } with rem units
    StyleSheet rootSheet;
    rootSheet.loadString(R"(
        :root {
            font-size: 20px;
        }
        .rem-box {
            font-size: 1.5rem;
            margin: 2rem;
        }
    )");
    Panel pRem;
    pRem.addClass("rem-box");
    pRem.applyStyle(rootSheet);
    TEST_ASSERT(std::abs(pRem.style().text.normal->fontSize.value_or(0.0f) - 30.0f) < 0.2f, "Custom root font-size 20px: 1.5rem is 30px");
    TEST_ASSERT(std::abs(pRem.layout().margin().left - 40.0f) < 0.2f, "Custom root font-size 20px: 2rem margin is 40px");

    // 4. Diagnostics for unknown units: 12vwx and 20bad (B3)
    StyleSheet typoSheet;
    typoSheet.loadString(R"(
        .typo-box {
            font-size: 12vwx;
            padding: 10px 20bad;
        }
    )", "my_styles.pss");

    bool foundVwx = false;
    bool foundBad = false;
    for (const auto& w : typoSheet.warnings()) {
        if (w.find("my_styles.pss:") != std::string::npos) {
            if (w.find("vwx") != std::string::npos) foundVwx = true;
            if (w.find("bad") != std::string::npos) foundBad = true;
        }
    }
    TEST_ASSERT(foundVwx, "Diagnostic warning with file:line:col produced for unknown unit 12vwx");
    TEST_ASSERT(foundBad, "Diagnostic warning with file:line:col produced for unknown unit 20bad");

    Panel pTypo;
    pTypo.addClass("typo-box");
    pTypo.applyStyle(typoSheet);
    TEST_ASSERT(!pTypo.style().text.normal.has_value() || !pTypo.style().text.normal->fontSize.has_value(), "Property with 12vwx was ignored (not parsed as 12)");
    TEST_ASSERT(pTypo.layout().padding().left == 0.0f, "Property with 20bad was ignored");

    // 5. Keyframes animating viewport units (font-size 2vh -> 4vh)
    StyleSheet kfVpSheet;
    kfVpSheet.loadString(R"(
        @keyframes animTextVp {
            from { font-size: 2vh; }
            to   { font-size: 4vh; }
        }
        .anim-text-vp {
            animation: animTextVp 1.0s linear 1 forwards;
        }
    )");
    Panel pAnimText;
    pAnimText.addClass("anim-text-vp");
    pAnimText.applyStyle(kfVpSheet);

    pAnimText.update(0.0f); // start: 2vh = 21.6px
    TEST_ASSERT(std::abs(pAnimText.style().text.normal->fontSize.value_or(0.0f) - 21.6f) < 0.5f, "Keyframe at t=0 evaluates 2vh = 21.6px");
    pAnimText.update(0.5f); // mid: midway to 4vh (43.2px) -> 32.4px
    TEST_ASSERT(std::abs(pAnimText.style().text.normal->fontSize.value_or(0.0f) - 32.4f) < 0.5f, "Keyframe at t=0.5 evaluates ~32.4px");
    pAnimText.update(0.5f); // end: 4vh = 43.2px
    TEST_ASSERT(std::abs(pAnimText.style().text.normal->fontSize.value_or(0.0f) - 43.2f) < 0.5f, "Keyframe at t=1.0 evaluates 4vh = 43.2px");

    // 6. Edge cases: zero viewport dimensions and fractional units
    UIContext zeroCtx;
    zeroCtx.setViewportSize(Dimensions{ 0.0f, 0.0f });
    Panel pZero;
    pZero.setContext(&zeroCtx);
    pZero.addClass("scaled-box");
    pZero.applyStyle(sheet);
    float zeroFs = pZero.style().text.normal->fontSize.value_or(-1.0f);
    TEST_ASSERT(!std::isnan(zeroFs) && zeroFs == 0.0f, "Zero viewport size resolves viewport units to 0.0f without crash or NaN");

    // 7. Benchmark: cost of viewport unit recalculation across 1000 elements on resolution change
    std::cout << "[Benchmark] Viewport Units Recalculation Benchmark (1000 elements)..." << std::endl;
    UIContext benchVpCtx;
    benchVpCtx.setViewportSize(Dimensions{ 1920.0f, 1080.0f });
    StyleSheet& bSheet = benchVpCtx.styleSheet();
    bSheet.loadString(R"(
        .vp-item {
            font-size: 2vh;
            padding: 1vmin;
            gap: 1vw;
            border-radius: 0.5vh;
            border-width: 0.1vw;
        }
    )");

    constexpr int kElements = 1000;
    for (int i = 0; i < kElements; ++i) {
        Panel* item = benchVpCtx.root()->add<Panel>("Item" + std::to_string(i));
        item->addClass("vp-item");
        item->applyStyle(bSheet);
    }

    auto bStart = std::chrono::high_resolution_clock::now();
    benchVpCtx.setViewportSize(Dimensions{ 3840.0f, 2160.0f });
    auto bEnd = std::chrono::high_resolution_clock::now();

    double elapsedUs = std::chrono::duration<double, std::micro>(bEnd - bStart).count();
    std::cout << "  * Recalculation time for 1000 elements: " << (elapsedUs / 1000.0) << " ms (" << elapsedUs << " us)" << std::endl;
    std::cout << "  * Cost per element: " << (elapsedUs / kElements) << " us" << std::endl;
    TEST_ASSERT(elapsedUs < 10000.0, "Viewport recalculation on 1000 elements under 10 ms");

    std::cout << "  -> Passed TestViewportUnitsExtended" << std::endl;
}

void TestBug1SimultaneousAnimationRemoval() {
    std::cout << "[Test] Running TestBug1SimultaneousAnimationRemoval..." << std::endl;

    StyleSheet sheet;
    sheet.loadString(R"(
        @keyframes animPulse {
            0%   { opacity: 1.0; }
            50%  { opacity: 0.2; }
            100% { opacity: 1.0; }
        }
        @keyframes animGlow {
            0%   { opacity: 1.0; }
            50%  { opacity: 0.5; }
            100% { opacity: 1.0; }
        }
        @keyframes animColor {
            0%   { color: #ffffff; }
            100% { color: #ff0000; }
        }
        @keyframes animScale {
            0%   { scale: 1.0; }
            50%  { scale: 2.0; }
            100% { scale: 1.0; }
        }
        @keyframes animPulseForwards {
            0%   { opacity: 1.0; }
            100% { opacity: 0.3; }
        }
        .multi-anim {
            animation: animPulse 1.0s linear infinite, animGlow 1.0s linear infinite, animColor 1.0s linear infinite;
        }
        .retained-anim {
            animation: animColor 1.0s linear infinite;
        }

        .three-anims {
            animation: animPulse 1.0s linear infinite, animScale 1.0s linear infinite, animGlow 1.0s linear infinite;
        }
        .retained-opacity {
            animation: animGlow 1.0s linear infinite;
        }

        .forwards-and-normal {
            animation: animPulseForwards 1.0s linear 1 forwards, animScale 1.0s linear 1;
        }
        .dummy-retained {
            animation: animColor 1.0s linear infinite;
        }

        .timer-two-anims {
            animation: animPulse 0.5s linear 1, animGlow 1.0s linear 1;
        }
    )");

    // Case 1: Two animations with common opacity removed simultaneously (retaining animColor)
    Panel p1;
    p1.setOpacity(1.0f);
    p1.addClass("multi-anim");
    p1.applyStyle(sheet);

    p1.update(0.5f);
    TEST_ASSERT(p1.opacity() < 0.6f, "Case 1: Opacity is animated at 50%");

    p1.removeClass("multi-anim");
    p1.addClass("retained-anim");
    p1.applyStyle(sheet);

    TEST_ASSERT(std::abs(p1.opacity() - 1.0f) < 0.01f, "Case 1: Opacity restored to base 1.0f when both pulse and glow removed simultaneously");

    // Case 2: Three animations: two removed (animPulse with opacity, animScale with scale), one retained (animGlow with opacity)
    Panel p2;
    p2.setOpacity(1.0f);
    p2.setRenderScale(Point{ 1.0f, 1.0f });
    p2.addClass("three-anims");
    p2.applyStyle(sheet);

    p2.update(0.5f);
    TEST_ASSERT(p2.renderScale().x > 1.5f, "Case 2: Scale is animated at 50%");
    TEST_ASSERT(p2.opacity() < 0.6f, "Case 2: Opacity is animated at 50%");

    p2.removeClass("three-anims");
    p2.addClass("retained-opacity");
    p2.applyStyle(sheet);

    // Scale must be restored to base 1.0, but opacity must NOT be restored (still running in animGlow)
    TEST_ASSERT(std::abs(p2.renderScale().x - 1.0f) < 0.01f, "Case 2: Scale restored to base 1.0");
    TEST_ASSERT(p2.opacity() < 0.6f, "Case 2: Opacity remains animated by retained animGlow");

    // Case 3: Removed animation with fill-mode: forwards does not restore its property, but allows other removed animation to restore
    Panel p3;
    p3.setOpacity(1.0f);
    p3.setRenderScale(Point{ 1.0f, 1.0f });
    p3.addClass("forwards-and-normal");
    p3.applyStyle(sheet);

    p3.update(1.0f); // finishes forwards animation
    TEST_ASSERT(std::abs(p3.opacity() - 0.3f) < 0.05f, "Case 3: Opacity reached forwards value 0.3");

    p3.removeClass("forwards-and-normal");
    p3.addClass("dummy-retained");
    p3.applyStyle(sheet);

    // animPulseForwards had fill-mode: forwards -> opacity remains 0.3
    // animScale had fill-mode: none -> scale restored to base 1.0
    TEST_ASSERT(std::abs(p3.opacity() - 0.3f) < 0.05f, "Case 3: Forwards opacity preserved on removal");
    TEST_ASSERT(std::abs(p3.renderScale().x - 1.0f) < 0.01f, "Case 3: Normal scale restored on removal");

    // Case 4: Timer-based finish in update(): animPulse (0.5s) finishes, but animGlow (1.0s) still runs
    Panel p4;
    p4.setOpacity(1.0f);
    p4.addClass("timer-two-anims");
    p4.applyStyle(sheet);

    p4.update(0.6f); // 0.6s: animPulse is finished, animGlow still running (total duration 1.0s)
    // animGlow is still actively running on opacity, so opacity must NOT be restored to base 1.0
    TEST_ASSERT(p4.opacity() < 0.95f, "Case 4: Property animated by still-running animGlow is not touched when animPulse finishes");

    std::cout << "  -> Passed TestBug1SimultaneousAnimationRemoval" << std::endl;
}



int main() {
    std::cout << "=== Running PerfUI Style System & Components Tests ===" << std::endl;

    TestThemePresetsAndTokens();
    TestStyleCascadeAndResolution();
    TestAnimationAndEasing();
    TestNewComponents();
    TestMarkupIntegration();
    TestTransitionsAndInterpolation();
    TestOutlineAndBorderImage();
    TestLinearGradients();
    TestSpringTransitions();
    TestViewportUnits();
    TestKeyframeAnimations();
    TestKeyframeBaseRestorationAndTracks();
    TestViewportUnitsExtended();
    TestBug1SimultaneousAnimationRemoval();
    TestKeyframeAnimationBenchmark();

    std::cout << "=== ALL STYLE SYSTEM & COMPONENT TESTS PASSED ===" << std::endl;
    return 0;
}
