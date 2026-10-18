#pragma once

#include "PerfUI/PerfUI.h"
#include <memory>
#include <string>
#include <vector>

namespace PerfUI::Examples {

class DesignSystemShowcase : public Panel {
public:
    explicit DesignSystemShowcase(std::string name = "DesignSystemShowcase")
        : Panel(std::move(name)) {
        buildUI();
    }

    void switchTab(const std::string& tabId) {
        if (m_mcmView) m_mcmView->setVisible(tabId == "mcm");
        if (m_inventoryView) m_inventoryView->setVisible(tabId == "inventory");
        if (m_questsView) m_questsView->setVisible(tabId == "quests");
        if (m_alertsView) m_alertsView->setVisible(tabId == "alerts");
        markLayoutDirty();
    }

private:
    Panel* m_mcmView{ nullptr };
    Panel* m_inventoryView{ nullptr };
    Panel* m_questsView{ nullptr };
    Panel* m_alertsView{ nullptr };
    TextArea* m_logArea{ nullptr };

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
               .addItem("inventory", "Inventory Grid", "", "8")
               .addItem("quests", "Quest Tracker")
               .addItem("alerts", "Notifications");

        sidebar->onItemSelected([this](const std::string& id) {
            switchTab(id);
        });

        // Content Area Container
        auto* contentArea = mainRow->add<Panel>("ContentArea");
        contentArea->layout().direction(LayoutDirection::Vertical)
                            .gap(12.0f)
                            .flex(1.0f);

        // -------------------------------------------------------------
        // TAB 1: MCM Settings View
        // -------------------------------------------------------------
        m_mcmView = contentArea->add<Panel>("McmView");
        m_mcmView->layout().direction(LayoutDirection::Vertical).gap(10.0f).flex(1.0f);

        m_mcmView->add<Label>("Mod Configuration Menu (MCM)", LabelVariant::Heading);
        m_mcmView->add<Label>("Hierarchical collapsible mod settings with interactive controls:", LabelVariant::Caption);

        auto* mcmAccordion = m_mcmView->add<Accordion>("McmAccordion");
        mcmAccordion->exclusive(true);

        // Section 1: Gameplay Controls
        auto* secGameplay = mcmAccordion->addSection("Gameplay & Keybindings", nullptr, true);
        secGameplay->content()->layout().direction(LayoutDirection::Vertical).gap(8.0f).padding(8.0f);

        auto* keyRow = secGameplay->content()->add<Panel>("KeyRow");
        keyRow->layout().direction(LayoutDirection::Horizontal).gap(12.0f).alignment(Alignment::Center);
        keyRow->add<Label>("Cast Spell Hotkey:", LabelVariant::Body);
        auto* keySelector = keyRow->add<KeybindSelector>(0x52, "Cast Spell", "CastHotkey"); // 'R'
        keySelector->onKeyChanged([this](uint32_t key) {
            if (m_logArea) {
                m_logArea->append("\nHotkey remapped to: " + KeybindSelector::KeyCodeToString(key));
            }
        });

        auto* switchRow1 = secGameplay->content()->add<Panel>("SwitchRow1");
        switchRow1->layout().direction(LayoutDirection::Horizontal).gap(10.0f).alignment(Alignment::Center);
        auto* sw1 = switchRow1->add<Switch>(true);
        switchRow1->add<Label>("Hardcore Needs & Survival Mode", LabelVariant::Body);
        sw1->onToggle([this](bool checked) {
            if (m_logArea) {
                m_logArea->append(checked ? "\nSurvival Mode: ENABLED" : "\nSurvival Mode: DISABLED");
            }
        });

        auto* switchRow2 = secGameplay->content()->add<Panel>("SwitchRow2");
        switchRow2->layout().direction(LayoutDirection::Horizontal).gap(10.0f).alignment(Alignment::Center);
        switchRow2->add<Switch>(false);
        switchRow2->add<Label>("Combat Overhaul Dynamic AI", LabelVariant::Body);

        auto* radioRow = secGameplay->content()->add<RadioGroup>("CombatPacing");
        radioRow->addOption(1, "Casual (1.0x incoming damage)");
        radioRow->addOption(2, "Master (2.5x incoming damage)");
        radioRow->addOption(3, "Legendary (4.0x incoming damage)");
        radioRow->select(2);

        // Section 2: Diagnostics & Logs
        auto* secLog = mcmAccordion->addSection("Diagnostics & Event Log", nullptr, false);
        secLog->content()->layout().direction(LayoutDirection::Vertical).gap(8.0f).padding(8.0f);

        m_logArea = secLog->content()->add<TextArea>(
            "PerfUI Retained-Mode UI Tree initialized.\n"
            "DirectX 11 ImGui rendering backend operational.\n"
            "Design system token cascades active.\n"
            "Ready for user interaction.",
            "LogArea"
        );
        m_logArea->layout().height(100.0f);

        auto* addLogBtn = secLog->content()->add<Button>("Add Test Log Entry");
        addLogBtn->onClick([this]() {
            if (m_logArea) {
                m_logArea->append("\nUser triggered diagnostic event at runtime.");
            }
        });

        // -------------------------------------------------------------
        // TAB 2: Inventory Grid View
        // -------------------------------------------------------------
        m_inventoryView = contentArea->add<Panel>("InventoryView");
        m_inventoryView->layout().direction(LayoutDirection::Vertical).gap(10.0f).flex(1.0f);
        m_inventoryView->setVisible(false);

        m_inventoryView->add<Label>("Inventory Equipment & Item Slots", LabelVariant::Heading);
        m_inventoryView->add<Label>("Item slots with rarity color accents (Common, Superior, Epic, Legendary), stack badges, and equipped indicators:", LabelVariant::Caption);

        // Searchable weapon/armor list
        auto* sList = m_inventoryView->add<SearchableList>("Filter weapons and items...", "InvSearch");
        sList->layout().height(110.0f);
        sList->addItem(1, "Daedric Warhammer (Legendary)");
        sList->addItem(2, "Ebony Bow (Epic)");
        sList->addItem(3, "Dragonplate Helmet (Legendary)");
        sList->addItem(4, "Elven Dagger (Superior)");
        sList->addItem(5, "Health Potion x15");
        sList->addItem(6, "Lockpick x99");

        m_inventoryView->add<Label>("Quick Slots Grid (Click slot to toggle equipped):", LabelVariant::Body);

        auto* gridRow1 = m_inventoryView->add<Panel>("GridRow1");
        gridRow1->layout().direction(LayoutDirection::Horizontal).gap(10.0f);

        auto createSlot = [](Panel* parent, std::string name, ItemRarity rarity, int count, bool eq) {
            auto* s = parent->add<ItemSlot>(std::move(name));
            s->layout().width(56.0f).height(56.0f);
            s->rarity(rarity).count(count).equipped(eq);
            s->onClick([s]() {
                s->equipped(!s->isEquipped());
            });
            return s;
        };

        createSlot(gridRow1, "Slot1", ItemRarity::Legendary, 1, true);
        createSlot(gridRow1, "Slot2", ItemRarity::Epic, 1, false);
        createSlot(gridRow1, "Slot3", ItemRarity::Superior, 15, false);
        createSlot(gridRow1, "Slot4", ItemRarity::Common, 99, false);

        auto* gridRow2 = m_inventoryView->add<Panel>("GridRow2");
        gridRow2->layout().direction(LayoutDirection::Horizontal).gap(10.0f);

        createSlot(gridRow2, "Slot5", ItemRarity::Legendary, 4, true);
        createSlot(gridRow2, "Slot6", ItemRarity::Epic, 3, false);
        createSlot(gridRow2, "Slot7", ItemRarity::Superior, 8, false);
        createSlot(gridRow2, "Slot8", ItemRarity::Fine, 42, false);

        // -------------------------------------------------------------
        // TAB 3: Quest Tracker View
        // -------------------------------------------------------------
        m_questsView = contentArea->add<Panel>("QuestsView");
        m_questsView->layout().direction(LayoutDirection::Vertical).gap(12.0f).flex(1.0f);
        m_questsView->setVisible(false);

        m_questsView->add<Label>("Quest Log & HUD Objectives", LabelVariant::Heading);
        m_questsView->add<Label>("Click any objective to toggle completion or progress:", LabelVariant::Caption);

        auto* questRow = m_questsView->add<Panel>("QuestRow");
        questRow->layout().direction(LayoutDirection::Horizontal).gap(16.0f);

        // Quest 1
        auto* q1 = questRow->add<QuestTracker>("Quest1");
        q1->layout().flex(1.0f);
        q1->setQuestTitle("The Horn of Jurgen Windcaller")
           .setQuestCategory("Main Quest")
           .addObjective("Retrieve the Horn from Ustengrav", false)
           .addObjective("Investigate the Tomb", true)
           .addObjective("Meet with Delphine at Riverwood", false, 0, 0, true);

        q1->onObjectiveClicked([q1](size_t idx) {
            // Toggle objective status on click
            if (idx == 0) q1->setObjectiveCompleted(0, true);
            else if (idx == 1) q1->setObjectiveCompleted(1, false);
            else if (idx == 2) q1->setObjectiveCompleted(2, true);
        });

        // Quest 2
        auto* q2 = questRow->add<QuestTracker>("Quest2");
        q2->layout().flex(1.0f);
        q2->setQuestTitle("The Black Star")
           .setQuestCategory("Daedric Quest")
           .addObjective("Find Azura's Shrine", true)
           .addObjective("Collect Soul Gems", false, 3, 5)
           .addObjective("Cleanse the Star (Optional)", false, 0, 0, true);

        q2->onObjectiveClicked([q2](size_t idx) {
            if (idx == 1) {
                // Increment soul gem counter
                static int s_gems = 3;
                s_gems = (s_gems % 5) + 1;
                q2->setObjectiveProgress(1, s_gems, 5);
            }
        });

        // -------------------------------------------------------------
        // TAB 4: Notifications View
        // -------------------------------------------------------------
        m_alertsView = contentArea->add<Panel>("AlertsView");
        m_alertsView->layout().direction(LayoutDirection::Vertical).gap(10.0f).flex(1.0f);
        m_alertsView->setVisible(false);

        m_alertsView->add<Label>("Notification System", LabelVariant::Heading);
        m_alertsView->add<Label>("Toast alerts and banners with timer progress and action callbacks:", LabelVariant::Caption);

        auto* notif1 = m_alertsView->add<Notification>("NotifSuccess");
        notif1->setTitle("Level Up Available")
              .setMessage("You have unallocated skill points. Open the perk constellation menu.")
              .setLevel(NotificationLevel::Success)
              .setAction("Perks", [] {})
              .setDuration(0.0f);

        auto* notif2 = m_alertsView->add<Notification>("NotifWarning");
        notif2->setTitle("Overencumbered")
              .setMessage("You are carrying too much to run. Drop items or use a draught of strength.")
              .setLevel(NotificationLevel::Warning)
              .setAction("Inventory", [] {})
              .setDuration(0.0f);

        auto* notif3 = m_alertsView->add<Notification>("NotifError");
        notif3->setTitle("Spell Failed: Fireball")
              .setMessage("Insufficient Magicka (Cost: 120, Available: 45).")
              .setLevel(NotificationLevel::Error)
              .setDuration(0.0f);

        auto* notif4 = m_alertsView->add<Notification>("NotifInfo");
        notif4->setTitle("Autosave Completed")
              .setMessage("Game progress saved to QuickSave slot.")
              .setLevel(NotificationLevel::Info)
              .setDuration(0.0f);
    }
};

} // namespace PerfUI::Examples
