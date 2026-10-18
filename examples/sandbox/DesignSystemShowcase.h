#pragma once

#include "PerfUI/PerfUI.h"
#include <memory>
#include <string>

namespace PerfUI::Examples {

class DesignSystemShowcase : public Panel {
public:
    explicit DesignSystemShowcase(std::string name = "DesignSystemShowcase")
        : Panel(std::move(name)) {
        buildUI();
    }

    void buildUI() {
        clearChildren();
        clipChildren(true);
        layout().direction(LayoutDirection::Vertical)
                .padding(16.0f)
                .gap(12.0f);

        // Header with Title and Theme Selector
        auto* header = add<Panel>("Header");
        header->layout().direction(LayoutDirection::Horizontal)
                       .alignment(Alignment::Center)
                       .justify(JustifyContent::SpaceBetween)
                       .height(36.0f);

        header->add<Label>("PerfUI Design System & Components", LabelVariant::Heading);

        auto* themeSelector = header->add<SegmentedControl>("ThemeSelector");
        themeSelector->addSegment("Nordic", "nordic")
                     .addSegment("Dark", "dark")
                     .addSegment("Minimal", "minimal")
                     .addSegment("Vanilla", "vanilla")
                     .addSegment("Sci-Fi", "scifi");

        themeSelector->onSelectionChanged([](size_t index, const std::string& val) {
            (void)index;
            if (val == "Nordic") Theme::ApplyPreset("Nordic Fantasy");
            else if (val == "Dark") Theme::ApplyPreset("Dark Fantasy");
            else if (val == "Minimal") Theme::ApplyPreset("Minimalist");
            else if (val == "Vanilla") Theme::ApplyPreset("Vanilla Skyrim");
            else if (val == "Sci-Fi") Theme::ApplyPreset("Futuristic");
        });

        add<Divider>(LayoutDirection::Horizontal)->thickness(1.0f);

        // Main content layout: Sidebar on left, Content on right
        auto* mainRow = add<Panel>("MainRow");
        mainRow->layout().direction(LayoutDirection::Horizontal)
                        .gap(16.0f)
                        .flex(1.0f);

        // Navigation Sidebar
        auto* sidebar = mainRow->add<Sidebar>("NavSidebar");
        sidebar->setTitle("Navigation")
               .addItem("mcm", "MCM Settings")
               .addItem("inventory", "Inventory Grid", "", "5")
               .addItem("quests", "Quest Tracker")
               .addItem("alerts", "Notifications");

        // Content Area Container
        auto* contentArea = mainRow->add<Panel>("ContentArea");
        contentArea->layout().direction(LayoutDirection::Vertical)
                            .gap(12.0f)
                            .flex(1.0f);

        // Content 1: MCM Settings Panel
        auto* mcmSection = contentArea->add<Accordion>("McmAccordion");
        mcmSection->exclusive(true);

        auto* secGeneral = mcmSection->addSection("General Controls", nullptr, true);
        secGeneral->content()->layout().direction(LayoutDirection::Vertical).gap(8.0f);
        secGeneral->content()->add<Label>("Keybindings & Toggles", LabelVariant::Body);

        auto* keybindRow = secGeneral->content()->add<Panel>("KeyRow");
        keybindRow->layout().direction(LayoutDirection::Horizontal).gap(12.0f).alignment(Alignment::Center);
        keybindRow->add<Label>("Cast Spell Hotkey:", LabelVariant::Caption);
        keybindRow->add<KeybindSelector>(0x52, "Cast Spell", "CastHotkey"); // 'R'
        keybindRow->add<Switch>(true);

        auto* radioRow = secGeneral->content()->add<RadioGroup>("QualityGroup");
        radioRow->addOption(1, "Performance Mode");
        radioRow->addOption(2, "Balanced Mode");
        radioRow->addOption(3, "Ultra Quality");
        radioRow->select(2);

        auto* secLog = mcmSection->addSection("Diagnostics & Log Output", nullptr, false);
        secLog->content()->layout().direction(LayoutDirection::Vertical).gap(6.0f);
        auto* logArea = secLog->content()->add<TextArea>("PerfUI Retained-mode tree loaded.\nDirectX 11 backend active.\nTheme cascades operational.", "LogArea");
        logArea->layout().height(90.0f);

        // Content 2: Item Slots Grid Row
        contentArea->add<Label>("Inventory Item Slots (Rarity & Equipped badges):", LabelVariant::Caption);
        auto* itemGrid = contentArea->add<Panel>("ItemGrid");
        itemGrid->layout().direction(LayoutDirection::Horizontal).gap(10.0f);

        auto* slot1 = itemGrid->add<ItemSlot>("Slot1");
        slot1->rarity(ItemRarity::Common).count(1);

        auto* slot2 = itemGrid->add<ItemSlot>("Slot2");
        slot2->rarity(ItemRarity::Superior).count(12).equipped(true);

        auto* slot3 = itemGrid->add<ItemSlot>("Slot3");
        slot3->rarity(ItemRarity::Epic).count(1);

        auto* slot4 = itemGrid->add<ItemSlot>("Slot4");
        slot4->rarity(ItemRarity::Legendary).count(3).equipped(true);

        // Content 3: Quest Tracker & Notification Banner Row
        auto* questNotifRow = contentArea->add<Panel>("QuestNotifRow");
        questNotifRow->layout().direction(LayoutDirection::Horizontal).gap(16.0f);

        auto* tracker = questNotifRow->add<QuestTracker>("ShowcaseQuest");
        tracker->setQuestTitle("The Horn of Jurgen Windcaller")
               .setQuestCategory("Main Quest")
               .addObjective("Retrieve the Horn from Ustengrav")
               .addObjective("Investigate the Tomb", true)
               .addObjective("Meet with Delphine (Optional)", false, 0, 0, true);

        auto* notif = questNotifRow->add<Notification>("ShowcaseAlert");
        notif->setTitle("Level Up Available")
             .setMessage("You have skill points ready to allocate.")
             .setLevel(NotificationLevel::Success)
             .setAction("Open Perks", [] {})
             .setDuration(0.0f); // persistent for demo
    }
};

} // namespace PerfUI::Examples
