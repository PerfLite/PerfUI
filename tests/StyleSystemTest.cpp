#include <iostream>
#include <cassert>
#include <cmath>
#include <string>
#include "PerfUI/PerfUI.h"
#include "PerfUI/Style/StyleSheet.h"
#include "../src/style/StyleTransition.h"
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

int main() {
    std::cout << "=== Running PerfUI Style System & Components Tests ===" << std::endl;

    TestThemePresetsAndTokens();
    TestStyleCascadeAndResolution();
    TestAnimationAndEasing();
    TestNewComponents();
    TestMarkupIntegration();
    TestTransitionsAndInterpolation();

    std::cout << "=== ALL STYLE SYSTEM & COMPONENT TESTS PASSED ===" << std::endl;
    return 0;
}
