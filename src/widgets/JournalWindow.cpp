#include "PerfUI/JournalWindow.h"
#include "PerfUI/UIContext.h"
#include "PerfUI/ConfigManager.h"
#include "PerfUI/Theme.h"
#include "PerfUI/ModalDialog.h"
#include "PerfUI/Toast.h"
#include <algorithm>
#include <cstdio>

namespace PerfUI {

JournalWindow::JournalWindow(std::string name)
    : Panel(std::move(name))
{
    setFocusable(true);
    if (!ConfigManager::GetSingleton().loadFromFile()) {
        ConfigManager::GetSingleton().saveToFile();
    }
    Theme::ApplyPreset(ConfigManager::GetSingleton().config().themePreset);
    m_customPosition = Point{ ConfigManager::GetSingleton().config().windowPosX, ConfigManager::GetSingleton().config().windowPosY };
    initQuestData();
    buildUI();
}

void JournalWindow::initQuestData() {
    m_activeQuests = {
        {
            "The Way of the Voice",
            "MAIN QUEST",
            "High Hrothgar",
            "I have been summoned by the Greybeards to their monastery at High Hrothgar on the slopes of the Throat of the World. They wish to speak to me about being Dragonborn.",
            true,
            0x000242BA,
            {
                { "Speak to the Greybeards", true, 10 },
                { "Demonstrate your 'Unrelenting Force' Shout", true, 20 },
                { "Speak to Arngeir", false, 30 },
                { "Learn the word of power from Einarth", false, 40 }
            }
        },
        {
            "Bleak Falls Barrow",
            "MAIN QUEST",
            "Whiterun Hold",
            "Farengar Secret-Fire, the court wizard in Dragonsreach, has asked me to retrieve a Dragonstone from the ancient Nordic ruins of Bleak Falls Barrow.",
            false,
            0x0003554A,
            {
                { "Retrieve the Dragonstone", true, 10 },
                { "Deliver the Dragonstone to Farengar", false, 20 }
            }
        },
        {
            "Dragon Rising",
            "MAIN QUEST",
            "Western Watchtower",
            "A dragon has attacked the Western Watchtower outside Whiterun. Jarl Balgruuf has dispatched Irileth and a squad of Whiterun Guards to investigate.",
            false,
            0x0002610C,
            {
                { "Meet Irileth near the Western Watchtower", true, 10 },
                { "Slay the dragon Mirmulnir", true, 20 },
                { "Investigate the dragon", true, 30 },
                { "Report back to Jarl Balgruuf", false, 40 }
            }
        },
        {
            "The Golden Claw",
            "SIDE QUEST",
            "Riverwood",
            "Lucan Valerius of the Riverwood Trader had a valuable golden claw stolen by bandits who fled to Bleak Falls Barrow. He wants me to recover it.",
            false,
            0x00039646,
            {
                { "Find the secret of Bleak Falls Barrow", true, 10 },
                { "Find the Golden Claw", true, 20 },
                { "Bring the claw to Lucan", false, 30 }
            }
        }
    };

    m_completedQuests = {
        {
            "Unbound",
            "MAIN QUEST",
            "Helgen",
            "A dragon attacked Helgen as I was about to be executed. I managed to escape the burning town through the keep.",
            false,
            0x0001B6BA,
            {
                { "Escape Helgen", true, 10 },
                { "Flee into the keep", true, 20 },
                { "Make your way out through the caverns", true, 30 }
            }
        },
        {
            "Before the Storm",
            "MAIN QUEST",
            "Riverwood",
            "I arrived in Riverwood and warned the townsfolk about the dragon attack on Helgen.",
            false,
            0x0001B6BD,
            {
                { "Talk to Gerdur or Alvor", true, 10 },
                { "Travel to Whiterun to warn the Jarl", true, 20 }
            }
        }
    };

    m_playerStats.name = "Dovahkiin";
    m_playerStats.level = 14;
    m_playerStats.gold = 3450;
    m_playerStats.health = 220.0f;
    m_playerStats.maxHealth = 220.0f;
    m_playerStats.magicka = 150.0f;
    m_playerStats.maxMagicka = 150.0f;
    m_playerStats.stamina = 180.0f;
    m_playerStats.maxStamina = 180.0f;
    m_playerStats.activeQuestsCount = static_cast<int>(m_activeQuests.size());
    m_playerStats.completedQuestsCount = static_cast<int>(m_completedQuests.size());
}

void JournalWindow::buildUI() {
    const auto& cfg = ConfigManager::GetSingleton().config();

    // Window Styling
    Color bg = Color::BackgroundBase();
    bg.a = static_cast<uint8_t>(cfg.windowOpacity * 255.0f);
    backgroundColor(bg);
    borderColor(Color::BorderFocus());
    borderWidth(2.0f);
    cornerRadius(12.0f);
    shadow(true, Color(0, 0, 0, 190), 28.0f, { 0.0f, 8.0f });

    layout()
        .direction(LayoutDirection::Vertical)
        .padding(18.0f)
        .gap(12.0f);

    // 1. Top Header Row: Title on Left, TabBar in Center, Profiler on Right
    auto* header = add<Panel>("Header");
    header->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    header->layout()
           .direction(LayoutDirection::Horizontal)
           .justify(JustifyContent::SpaceBetween)
           .alignment(Alignment::Center);

    auto* titleStack = header->add<Panel>("TitleStack");
    titleStack->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    titleStack->layout().direction(LayoutDirection::Vertical).gap(2.0f);

    auto* titleText = titleStack->add<Text>("PERFUI");
    titleText->color(Color::NordicGold()).fontSize(18.0f).bold(true);

    auto* subTitleText = titleStack->add<Text>("Skyrim SE/AE Framework");
    subTitleText->color(Color::TextSecondary()).fontSize(11.0f);

    // Tab Bar in header
    m_tabBar = header->add<TabBar>("MainTabBar");
    m_tabBar->addTab("ACTIVE QUESTS")
            .addTab("COMPLETED")
            .addTab("CHARACTER STATS")
            .addTab("SETTINGS");
    m_tabBar->tooltip("Switch views: Active Quests, Completed Quests, Character Stats, or Settings");
    m_tabBar->onTabChanged([this](size_t index) {
        switchView(index);
    });

    // Real-Time Performance Profiler Badge (Phase 12)
    auto* badge = header->add<Panel>("PerfBadge");
    badge->backgroundColor(Color(20, 26, 36, 220))
         .borderColor(Color::BorderSubtle())
         .borderWidth(1.0f)
         .cornerRadius(6.0f);
    badge->layout().padding(8.0f, 4.0f);
    badge->tooltip("Real-time GPU and CPU execution time in microseconds");
    badge->setVisible(cfg.showProfiler);
    m_profilerText = badge->add<Text>("Layout: 0.01ms | Render: 0.04ms");
    m_profilerText->color(Color::TextSuccess()).fontSize(11.0f).bold(true);

    // 2. Horizontal Divider
    auto* divider = add<Panel>("Divider");
    divider->backgroundColor(Color::BorderSubtle()).borderWidth(0.0f);
    divider->layout().height(1.0f);

    // -------------------------------------------------------------
    // 3. View 0 & 1: Quests View Panel (Sidebar + Details)
    // -------------------------------------------------------------
    m_questViewPanel = add<Panel>("QuestViewPanel");
    m_questViewPanel->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    m_questViewPanel->layout()
                     .flex(1.0f)
                     .direction(LayoutDirection::Horizontal)
                     .gap(16.0f);

    // 3A. Left Sidebar (Quest List)
    auto* sidebar = m_questViewPanel->add<Panel>("Sidebar");
    sidebar->backgroundColor(Color(18, 22, 29, 180))
           .borderColor(Color::BorderSubtle())
           .borderWidth(1.0f)
           .cornerRadius(8.0f);
    sidebar->layout()
           .width(260.0f)
           .direction(LayoutDirection::Vertical)
           .padding(10.0f)
           .gap(8.0f);

    auto* listHeader = sidebar->add<Text>("QUEST LIST");
    listHeader->color(Color::TextAccent()).fontSize(13.0f).bold(true);

    // Category Filter ComboBox (Phase 5 / Overlay test)
    m_categoryFilter = sidebar->add<ComboBox>(std::vector<std::string>{
        "All Quests",
        "Main Quest",
        "College of Winterhold",
        "Thieves Guild",
        "Dark Brotherhood",
        "The Companions",
        "Daedric Quest",
        "Side Quest",
        "Civil War",
        "Dawnguard",
        "Dragonborn",
        "Miscellaneous"
    });
    m_categoryFilter->layout().width(DimensionConstraint::Flex(1.0f));
    m_categoryFilter->tooltip("Filter quests by storyline category");
    m_categoryFilter->onSelectionChanged([this](size_t index, const std::string& text) {
        (void)index;
        m_selectedCategoryFilter = text;
        rebuildQuestList();
        updateDetailsPanel();
    });

    // Search Bar (TextInput)
    auto* searchRow = sidebar->add<Panel>("SearchRow");
    searchRow->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    searchRow->layout()
             .direction(LayoutDirection::Horizontal)
             .gap(4.0f)
             .width(DimensionConstraint::Flex(1.0f));

    m_searchInput = searchRow->add<TextInput>("Search quests...");
    m_searchInput->layout().flex(1.0f).height(28.0f);
    m_searchInput->fontSize(12.0f);
    m_searchInput->tooltip("Search quests by title or description in real-time");
    m_searchInput->onTextChanged([this](const std::string& query) {
        m_searchQuery = query;
        rebuildQuestList();
        updateDetailsPanel();
    });

    auto* clearBtn = searchRow->add<Button>("x");
    clearBtn->layout().width(24.0f).height(28.0f).padding(0.0f);
    clearBtn->fontSize(11.0f);
    clearBtn->tooltip("Clear search query");
    clearBtn->onClick([this]() {
        if (m_searchInput) {
            m_searchInput->clear();
        }
    });

    m_questScrollView = sidebar->add<ScrollView>("QuestScrollView");
    m_questScrollView->layout().flex(1.0f).gap(6.0f);

    // 3B. Right Details Panel
    auto* details = m_questViewPanel->add<Panel>("DetailsPanel");
    details->backgroundColor(Color::BackgroundElevated())
           .borderColor(Color::BorderStrong())
           .borderWidth(1.0f)
           .cornerRadius(8.0f);
    details->layout()
           .flex(1.0f)
           .direction(LayoutDirection::Vertical)
           .padding(16.0f)
           .gap(10.0f);

    // Tags Row
    auto* tagRow = details->add<Panel>("TagRow");
    tagRow->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    tagRow->layout().direction(LayoutDirection::Horizontal).gap(8.0f);

    auto* catBadge = tagRow->add<Panel>("CatBadge");
    catBadge->backgroundColor(Color(45, 38, 20, 230))
            .borderColor(Color::NordicGold())
            .borderWidth(1.0f)
            .cornerRadius(4.0f);
    catBadge->layout().padding(8.0f, 3.0f);
    m_questCategoryText = catBadge->add<Text>("MAIN QUEST");
    m_questCategoryText->color(Color::TextAccent()).fontSize(11.0f).bold(true);

    auto* locBadge = tagRow->add<Panel>("LocBadge");
    locBadge->backgroundColor(Color(25, 32, 42, 230))
            .borderColor(Color::BorderSubtle())
            .borderWidth(1.0f)
            .cornerRadius(4.0f);
    locBadge->layout().padding(8.0f, 3.0f);
    m_questLocationText = locBadge->add<Text>("Skyrim");
    m_questLocationText->color(Color::TextSecondary()).fontSize(11.0f);

    // Title
    m_questTitleText = details->add<Text>("The Way of the Voice");
    m_questTitleText->color(Color::TextPrimary()).fontSize(18.0f).bold(true);

    // Progress Bar
    m_progressBar = details->add<ProgressBar>(0.5f);
    m_progressBar->height(10.0f).cornerRadius(4.0f).fillColor(Color::NordicGold());

    // Inner Divider
    auto* innerDivider = details->add<Panel>("InnerDivider");
    innerDivider->backgroundColor(Color::BorderSubtle()).borderWidth(0.0f);
    innerDivider->layout().height(1.0f);

    // Description (Word wrapped)
    m_questDescText = details->add<Text>("Description placeholder...");
    m_questDescText->wrap(true);
    m_questDescText->color(Color(200, 205, 215, 255)).fontSize(13.0f);

    // Objectives Section
    auto* objHeader = details->add<Text>("OBJECTIVES:");
    objHeader->color(Color::TextAccent()).fontSize(12.0f).bold(true);

    m_objectivesContainer = details->add<Panel>("ObjectivesContainer");
    m_objectivesContainer->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    m_objectivesContainer->layout().direction(LayoutDirection::Vertical).gap(6.0f);

    // Action Buttons at bottom
    auto* actionRow = details->add<Panel>("ActionRow");
    actionRow->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    actionRow->layout()
             .direction(LayoutDirection::Horizontal)
             .justify(JustifyContent::End)
             .gap(10.0f)
             .margin(0.0f, 8.0f, 0.0f, 0.0f);

    m_setActiveBtn = actionRow->add<Button>("Track Quest");
    m_setActiveBtn->layout().padding(14.0f, 6.0f);
    m_setActiveBtn->fontSize(12.0f);
    m_setActiveBtn->tooltip("Toggle quest tracking marker on compass and world map");
    m_setActiveBtn->normalColor(Color(35, 55, 35, 230), Color::TextSuccess(), Color::White());
    m_setActiveBtn->hoverColor(Color(45, 75, 45, 255), Color::TextSuccess(), Color::White());

    // -------------------------------------------------------------
    // 4. View 2: Character Stats View Panel
    // -------------------------------------------------------------
    m_statsViewPanel = add<Panel>("StatsViewPanel");
    m_statsViewPanel->backgroundColor(Color::BackgroundElevated())
                    .borderColor(Color::BorderStrong())
                    .borderWidth(1.0f)
                    .cornerRadius(8.0f);
    m_statsViewPanel->layout()
                    .flex(1.0f)
                    .direction(LayoutDirection::Vertical)
                    .padding(20.0f)
                    .gap(16.0f);
    m_statsViewPanel->setVisible(false);

    auto* heroHeaderRow = m_statsViewPanel->add<Panel>("HeroHeaderRow");
    heroHeaderRow->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    heroHeaderRow->layout().direction(LayoutDirection::Horizontal).justify(JustifyContent::SpaceBetween).alignment(Alignment::Center);

    m_statsHeroNameText = heroHeaderRow->add<Text>("Dovahkiin");
    m_statsHeroNameText->fontSize(22.0f).bold(true).color(Color::NordicGold());
    m_statsHeroNameText->tooltip("Player Character Name");

    auto* metaRow = heroHeaderRow->add<Panel>("MetaRow");
    metaRow->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    metaRow->layout().direction(LayoutDirection::Horizontal).gap(14.0f);

    m_statsHeroLevelText = metaRow->add<Text>("Level: 14");
    m_statsHeroLevelText->fontSize(14.0f).color(Color::TextPrimary()).bold(true);
    m_statsHeroLevelText->tooltip("Player progression level");

    m_statsHeroGoldText = metaRow->add<Text>("Gold: 3450");
    m_statsHeroGoldText->fontSize(14.0f).color(Color::TextAccent()).bold(true);
    m_statsHeroGoldText->tooltip("Total Septims held in player inventory");

    auto* statsDivider = m_statsViewPanel->add<Panel>("StatsDivider");
    statsDivider->backgroundColor(Color::BorderSubtle()).borderWidth(0.0f);
    statsDivider->layout().height(1.0f);

    // Vitals Section (Health, Magicka, Stamina)
    auto* vitalsContainer = m_statsViewPanel->add<Panel>("VitalsContainer");
    vitalsContainer->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    vitalsContainer->layout()
                   .direction(LayoutDirection::Vertical)
                   .gap(10.0f)
                   .width(DimensionConstraint::Flex(1.0f));

    auto* hRow = vitalsContainer->add<Panel>("HRow");
    hRow->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    hRow->layout().direction(LayoutDirection::Horizontal).gap(10.0f).width(DimensionConstraint::Flex(1.0f));
    auto* hLabel = hRow->add<Text>("HEALTH");
    hLabel->layout().width(80.0f);
    hLabel->fontSize(12.0f).color(Color(230, 90, 90, 255)).bold(true);
    m_healthBar = hRow->add<ProgressBar>(1.0f);
    m_healthBar->layout().flex(1.0f);
    m_healthBar->height(12.0f).cornerRadius(4.0f).fillColor(Color(190, 45, 45, 255));
    m_healthBar->tooltip("Player Health Points (HP)");

    auto* mRow = vitalsContainer->add<Panel>("MRow");
    mRow->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    mRow->layout().direction(LayoutDirection::Horizontal).gap(10.0f).width(DimensionConstraint::Flex(1.0f));
    auto* mLabel = mRow->add<Text>("MAGICKA");
    mLabel->layout().width(80.0f);
    mLabel->fontSize(12.0f).color(Color(90, 150, 240, 255)).bold(true);
    m_magickaBar = mRow->add<ProgressBar>(1.0f);
    m_magickaBar->layout().flex(1.0f);
    m_magickaBar->height(12.0f).cornerRadius(4.0f).fillColor(Color(45, 110, 210, 255));
    m_magickaBar->tooltip("Player Magicka Points (MP)");

    auto* sRow = vitalsContainer->add<Panel>("SRow");
    sRow->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    sRow->layout().direction(LayoutDirection::Horizontal).gap(10.0f).width(DimensionConstraint::Flex(1.0f));
    auto* sLabel = sRow->add<Text>("STAMINA");
    sLabel->layout().width(80.0f);
    sLabel->fontSize(12.0f).color(Color(90, 215, 120, 255)).bold(true);
    m_staminaBar = sRow->add<ProgressBar>(1.0f);
    m_staminaBar->layout().flex(1.0f);
    m_staminaBar->height(12.0f).cornerRadius(4.0f).fillColor(Color(45, 175, 75, 255));
    m_staminaBar->tooltip("Player Stamina Points (SP)");

    // Quest Statistics Card
    auto* statsSummary = m_statsViewPanel->add<Panel>("StatsSummary");
    statsSummary->backgroundColor(Color(16, 20, 28, 200))
                .borderColor(Color::BorderSubtle())
                .borderWidth(1.0f)
                .cornerRadius(6.0f);
    statsSummary->layout().padding(14.0f).gap(8.0f).direction(LayoutDirection::Vertical);

    auto* questRecordTitle = statsSummary->add<Text>("ADVENTURE RECORD");
    questRecordTitle->fontSize(13.0f).bold(true).color(Color::TextAccent());

    m_statsActiveCountText = statsSummary->add<Text>("Active Quests: 4");
    m_statsActiveCountText->fontSize(13.0f).color(Color::TextPrimary());

    m_statsCompletedCountText = statsSummary->add<Text>("Completed Quests: 2");
    m_statsCompletedCountText->fontSize(13.0f).color(Color::TextSuccess());

    // 4. View 3: Settings View Panel (MCM on C++)
    m_settingsView = add<SettingsView>("SettingsView");
    m_settingsView->layout().flex(1.0f);
    m_settingsView->setVisible(false);
    buildSettingsView();

    // 5. Footer Hint & Slider Controls
    auto* footer = add<Panel>("Footer");
    footer->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    footer->layout().direction(LayoutDirection::Horizontal).justify(JustifyContent::SpaceBetween).alignment(Alignment::Center);

    auto* hintText = footer->add<Text>("[F11] Journal   [F10] Menu   [F8] Modder HUD   [ESC] Close");
    hintText->color(Color(110, 120, 135, 255)).fontSize(11.5f);

    auto* sliderContainer = footer->add<Panel>("SliderContainer");
    sliderContainer->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    sliderContainer->layout().direction(LayoutDirection::Horizontal).gap(8.0f).alignment(Alignment::Center);

    auto* sliderLabel = sliderContainer->add<Text>("Opacity:");
    sliderLabel->fontSize(11.0f).color(Color::TextSecondary());

    m_opacitySlider = sliderContainer->add<Slider>(cfg.windowOpacity, 0.35f, 1.0f, "");
    m_opacitySlider->layout().width(100.0f).height(20.0f);
    m_opacitySlider->tooltip("Adjust window background transparency (35% to 100%)");
    m_opacitySlider->onValueChanged([this](float val) {
        ConfigManager::GetSingleton().config().windowOpacity = val;
        Color bg = Color::BackgroundBase();
        bg.a = static_cast<uint8_t>(val * 255.0f);
        backgroundColor(bg);
    });

    rebuildQuestList();
    updateDetailsPanel();
    updateStatsView();
}

void JournalWindow::switchView(size_t tabIndex) {
    m_currentViewIndex = tabIndex;
    if (m_context) {
        m_context->playSound("UIMenuBlade");
    }

    if (m_currentViewIndex == 0) {
        // Active Quests
        m_questViewPanel->setVisible(true);
        m_statsViewPanel->setVisible(false);
        if (m_settingsView) m_settingsView->setVisible(false);
        rebuildQuestList();
        updateDetailsPanel();
    } else if (m_currentViewIndex == 1) {
        // Completed Quests
        m_questViewPanel->setVisible(true);
        m_statsViewPanel->setVisible(false);
        if (m_settingsView) m_settingsView->setVisible(false);
        rebuildQuestList();
        updateDetailsPanel();
    } else if (m_currentViewIndex == 2) {
        // Stats View
        m_questViewPanel->setVisible(false);
        m_statsViewPanel->setVisible(true);
        if (m_settingsView) m_settingsView->setVisible(false);
        updateStatsView();
    } else if (m_currentViewIndex == 3) {
        // Settings View (MCM on C++)
        m_questViewPanel->setVisible(false);
        m_statsViewPanel->setVisible(false);
        if (m_settingsView) m_settingsView->setVisible(true);
    }

    markLayoutDirty();
}

void JournalWindow::buildSettingsView() {
    if (!m_settingsView) return;
    m_settingsView->clearSections();

    auto& cfg = ConfigManager::GetSingleton().config();

    // Section 1: Appearance & Theme
    auto* appSec = m_settingsView->addSection("APPEARANCE & THEME");

    appSec->addSlider(
        "Window Background Opacity",
        0.35f, 1.0f, cfg.windowOpacity,
        "Adjust transparency of the main window background (35% to 100%)",
        [this](float val) {
            ConfigManager::GetSingleton().config().windowOpacity = val;
            Color bg = Color::BackgroundBase();
            bg.a = static_cast<uint8_t>(val * 255.0f);
            backgroundColor(bg);
            if (m_opacitySlider) {
                m_opacitySlider->value(val);
            }
        },
        "%.2f"
    );

    std::vector<std::string> themePresets = {
        "Nordic Gold",
        "Imperial Ruby",
        "Dawnguard Amber",
        "Winterhold Frost"
    };
    size_t currentThemeIdx = 0;
    for (size_t i = 0; i < themePresets.size(); ++i) {
        if (themePresets[i] == cfg.themePreset) {
            currentThemeIdx = i;
            break;
        }
    }

    appSec->addChoice(
        "Color Palette / Theme Preset",
        themePresets,
        currentThemeIdx,
        "Select the primary accent color for borders, highlights, and tabs",
        [this](size_t idx, const std::string& choice) {
            (void)idx;
            ConfigManager::GetSingleton().config().themePreset = choice;
            Theme::ApplyPreset(choice);
            borderColor(Theme::Current().colors.borderFocus);
            if (m_context) {
                m_context->showToast("Theme Applied", choice, ToastType::Info);
                m_context->playSound("UIMenuOK");
            }
            markLayoutDirty();
        }
    );

    appSec->addToggle(
        "Show Real-Time Telemetry Profiler",
        cfg.showProfiler,
        "Displays microsecond layout and render performance in the header bar",
        [this](bool enabled) {
            ConfigManager::GetSingleton().config().showProfiler = enabled;
            if (m_profilerText && m_profilerText->parent()) {
                m_profilerText->parent()->setVisible(enabled);
            }
            if (m_context) {
                m_context->playSound("UIMenuOK");
            }
        }
    );

    // Section 2: Gameplay & Behavior
    auto* gameSec = m_settingsView->addSection("GAMEPLAY & BEHAVIOR");

    gameSec->addToggle(
        "Pause Game World While Open",
        cfg.pauseGameWhenOpen,
        "Freezes Skyrim game world physics and time when the journal is displayed",
        [this](bool enabled) {
            ConfigManager::GetSingleton().config().pauseGameWhenOpen = enabled;
            if (m_context) {
                m_context->playSound("UIMenuOK");
            }
        }
    );

    gameSec->addToggle(
        "Auto-Track Newly Discovered Quests",
        cfg.autoTrackNewQuests,
        "Automatically enable compass tracking markers for newly received quests",
        [this](bool enabled) {
            ConfigManager::GetSingleton().config().autoTrackNewQuests = enabled;
            if (m_context) {
                m_context->playSound("UIMenuOK");
            }
        }
    );

    // Section 3: Controls & Shortcuts
    auto* ctrlSec = m_settingsView->addSection("CONTROLS & SHORTCUTS");

    ctrlSec->addKeybind(
        "Menu Toggle Hotkey",
        cfg.toggleHotkey,
        "Click and press any keyboard key to rebind the menu activation key",
        [this](uint32_t newKey) {
            ConfigManager::GetSingleton().config().toggleHotkey = newKey;
            ConfigManager::GetSingleton().saveToFile();
            if (m_context) {
                m_context->showToast("Hotkey Saved", "Toggle hotkey updated", ToastType::Success);
            }
        }
    );

    // Section 4: Storage & Configuration
    auto* storSec = m_settingsView->addSection("PRESETS & CONFIG STORAGE");

    storSec->addButton(
        "Save Settings to Disk",
        "Save Config",
        "Writes current settings to SKSE/Plugins/PerfUI.json",
        [this]() {
            ConfigManager::GetSingleton().saveToFile();
            if (m_context) {
                m_context->showToast("Settings Saved", "Preferences written to config file", ToastType::Success);
                m_context->playSound("UIMenuOK");
            }
        }
    );

    storSec->addButton(
        "Window Position",
        "Center Window",
        "Resets window coordinates back to the center of the display",
        [this]() {
            resetPosition();
            if (m_context) {
                m_context->showToast("Position Reset", "Window position centered", ToastType::Info);
                m_context->playSound("UIMenuOK");
            }
        }
    );

    storSec->addButton(
        "Reset to Factory Defaults",
        "Reset Defaults",
        "Restores default visual and gameplay settings",
        [this]() {
            if (m_context) {
                auto modal = std::make_shared<ModalDialog>(
                    "Reset Settings?",
                    "Are you sure you want to restore all settings to default values? Any custom preferences will be lost.",
                    "Reset Defaults",
                    "Cancel"
                );
                modal->onConfirm([this]() {
                    auto& c = ConfigManager::GetSingleton().config();
                    c.windowOpacity = 0.95f;
                    c.themePreset = "Nordic Gold";
                    c.showProfiler = true;
                    c.pauseGameWhenOpen = false;
                    c.autoTrackNewQuests = false;
                    c.windowPosX = -1.0f;
                    c.windowPosY = -1.0f;
                    c.toggleHotkey = 0x7A; // VK_F11
                    resetPosition();
                    Theme::ApplyPreset("Nordic Gold");
                    borderColor(Theme::Current().colors.borderFocus);
                    Color bg = Color::BackgroundBase();
                    bg.a = static_cast<uint8_t>(0.95f * 255.0f);
                    backgroundColor(bg);
                    if (m_opacitySlider) m_opacitySlider->value(0.95f);
                    if (m_profilerText && m_profilerText->parent()) m_profilerText->parent()->setVisible(true);
                    ConfigManager::GetSingleton().saveToFile();
                    buildSettingsView();
                    if (m_context) {
                        m_context->showToast("Defaults Restored", "Settings have been reset to factory defaults", ToastType::Info);
                    }
                });
                m_context->showModal(modal);
            }
        }
    );
}

static std::string ToUpperStr(std::string_view s) {
    std::string result;
    result.reserve(s.size());
    for (unsigned char c : s) {
        result.push_back(static_cast<char>(std::toupper(c)));
    }
    return result;
}

void JournalWindow::rebuildQuestList() {
    if (!m_questScrollView) return;

    m_questScrollView->clearChildren();
    m_questButtons.clear();
    m_filteredIndices.clear();

    const auto& quests = (m_currentViewIndex == 1) ? m_completedQuests : m_activeQuests;
    size_t selectedIdx = (m_currentViewIndex == 1) ? m_selectedCompletedIndex : m_selectedActiveIndex;

    for (size_t i = 0; i < quests.size(); ++i) {
        const auto& q = quests[i];

        // Apply category filter (if not "All Quests")
        if (m_selectedCategoryFilter != "All Quests" && !m_selectedCategoryFilter.empty()) {
            std::string qUpper = ToUpperStr(q.category);
            std::string fUpper = ToUpperStr(m_selectedCategoryFilter);
            if (qUpper.find(fUpper) == std::string::npos && fUpper.find(qUpper) == std::string::npos) {
                continue;
            }
        }

        // Apply live search query filter
        if (!m_searchQuery.empty()) {
            std::string sUpper = ToUpperStr(m_searchQuery);
            std::string titleUpper = ToUpperStr(q.title);
            if (titleUpper.find(sUpper) == std::string::npos) {
                std::string descUpper = ToUpperStr(q.description);
                if (descUpper.find(sUpper) == std::string::npos) {
                    continue;
                }
            }
        }

        m_filteredIndices.push_back(i);

        std::string label = (q.active ? "[*] " : "    ") + q.title;

        auto* btn = m_questScrollView->add<Button>(std::move(label));
        btn->layout().width(DimensionConstraint::Flex(1.0f)).padding(10.0f, 8.0f);
        btn->fontSize(13.0f);
        btn->tooltip("Click to view objectives | Right-Click for options");

        if (i == selectedIdx) {
            btn->normalColor(Color(45, 58, 78, 250), Color::BorderFocus(), Color::White());
        } else {
            btn->normalColor(Color(24, 30, 40, 220), Color::BorderSubtle(), Color::TextPrimary());
        }

        btn->hoverColor(Color(36, 46, 62, 240), Color::BorderFocus(), Color::White());
        btn->pressedColor(Color::BackgroundActive(), Color::BorderFocus(), Color::White());

        btn->onClick([this, i]() {
            selectQuest(i);
        });

        btn->onContextMenu([this, i](const Point& localPoint) {
            (void)localPoint;
            openQuestContextMenu(i);
            return true;
        });

        m_questButtons.push_back(btn);
    }

    if (m_filteredIndices.empty()) {
        auto* emptyNotice = m_questScrollView->add<Text>("No quests match filter");
        emptyNotice->color(Color(140, 150, 165, 180)).fontSize(12.0f);
        emptyNotice->layout().padding(10.0f, 12.0f);
    }

    markLayoutDirty();
}

void JournalWindow::setQuests(std::vector<QuestEntry> activeQuests, std::vector<QuestEntry> completedQuests) {
    if (!activeQuests.empty()) {
        m_activeQuests = std::move(activeQuests);
    }
    if (!completedQuests.empty()) {
        m_completedQuests = std::move(completedQuests);
    }

    m_playerStats.activeQuestsCount = static_cast<int>(m_activeQuests.size());
    m_playerStats.completedQuestsCount = static_cast<int>(m_completedQuests.size());

    rebuildQuestList();
    updateDetailsPanel();
    updateStatsView();
}

void JournalWindow::setPlayerStats(const PlayerStats& stats) {
    m_playerStats = stats;
    updateStatsView();
}

void JournalWindow::selectQuest(size_t index) {
    const auto& quests = (m_currentViewIndex == 1) ? m_completedQuests : m_activeQuests;
    if (index >= quests.size()) return;

    if (m_currentViewIndex == 1) {
        m_selectedCompletedIndex = index;
    } else {
        m_selectedActiveIndex = index;
    }

    // Highlight selected button
    for (size_t k = 0; k < m_questButtons.size(); ++k) {
        if (k < m_filteredIndices.size() && m_filteredIndices[k] == index) {
            m_questButtons[k]->normalColor(Color(45, 58, 78, 250), Color::BorderFocus(), Color::White());
        } else {
            m_questButtons[k]->normalColor(Color(24, 30, 40, 220), Color::BorderSubtle(), Color::TextPrimary());
        }
    }

    updateDetailsPanel();
}

void JournalWindow::openQuestContextMenu(size_t questIdx) {
    if (!context()) return;

    auto& curQuests = (m_currentViewIndex == 1) ? m_completedQuests : m_activeQuests;
    if (questIdx >= curQuests.size()) return;

    auto& quest = curQuests[questIdx];
    Point mousePos = context()->lastMousePos();

    std::vector<ContextMenuItem> items;

    if (m_currentViewIndex == 0) { // Active quests
        if (quest.active) {
            items.push_back(ContextMenuItem::Action("Untrack Quest", [this, questIdx]() {
                auto& q = m_activeQuests[questIdx];
                q.active = false;
                if (m_onTrackQuest) m_onTrackQuest(q.formId, false);
                rebuildQuestList();
                updateDetailsPanel();
                if (context()) context()->showToast("Quest Untracked", q.title, ToastType::Info);
            }));
        } else {
            items.push_back(ContextMenuItem::Action("Track on Compass", [this, questIdx]() {
                auto& q = m_activeQuests[questIdx];
                q.active = true;
                if (m_onTrackQuest) m_onTrackQuest(q.formId, true);
                rebuildQuestList();
                updateDetailsPanel();
                if (context()) context()->showToast("Quest Tracked", q.title, ToastType::Success);
            }));
        }

        items.push_back(ContextMenuItem::Action("Complete Next Objective", [this, questIdx]() {
            auto& q = m_activeQuests[questIdx];
            bool found = false;
            for (auto& obj : q.objectives) {
                if (!obj.completed) {
                    obj.completed = true;
                    found = true;
                    if (context()) context()->showToast("Objective Completed", obj.text, ToastType::Success);
                    break;
                }
            }
            if (!found && context()) {
                context()->showToast("Objectives", "All objectives already completed", ToastType::Info);
            }
            rebuildQuestList();
            updateDetailsPanel();
        }));
    }

    items.push_back(ContextMenuItem::Separator());

    items.push_back(ContextMenuItem::Action("Select & View Details", [this, questIdx]() {
        selectQuest(questIdx);
    }));

    std::string titleToCopy = quest.title;
    items.push_back(ContextMenuItem::Action("Copy Quest Title", [this, titleToCopy]() {
        if (context()) {
            context()->showToast("Copied to Clipboard", titleToCopy, ToastType::Info);
        }
    }));

    context()->showContextMenu(mousePos, std::move(items));
}

void JournalWindow::updateDetailsPanel() {
    auto& quests = (m_currentViewIndex == 1) ? m_completedQuests : m_activeQuests;
    size_t selectedIdx = (m_currentViewIndex == 1) ? m_selectedCompletedIndex : m_selectedActiveIndex;

    if (m_filteredIndices.empty()) {
        if (m_questCategoryText) m_questCategoryText->text("---");
        if (m_questLocationText) m_questLocationText->text("---");
        if (m_questTitleText) m_questTitleText->text("No Quests Found");
        if (m_questDescText) m_questDescText->text("No quests match current category or search criteria.");
        if (m_progressBar) m_progressBar->progress(0.0f);
        if (m_objectivesContainer) m_objectivesContainer->clearChildren();
        if (m_setActiveBtn) m_setActiveBtn->setVisible(false);
        return;
    }

    bool foundInFiltered = false;
    for (size_t idx : m_filteredIndices) {
        if (idx == selectedIdx) {
            foundInFiltered = true;
            break;
        }
    }
    if (!foundInFiltered) {
        selectedIdx = m_filteredIndices[0];
        if (m_currentViewIndex == 1) {
            m_selectedCompletedIndex = selectedIdx;
        } else {
            m_selectedActiveIndex = selectedIdx;
        }
    }

    if (selectedIdx >= quests.size()) return;

    auto& quest = quests[selectedIdx];
    if (m_questCategoryText) m_questCategoryText->text(quest.category);
    if (m_questLocationText) m_questLocationText->text(quest.location);
    if (m_questTitleText) m_questTitleText->text(quest.title);
    if (m_questDescText) m_questDescText->text(quest.description);

    int completedCount = 0;
    for (const auto& obj : quest.objectives) {
        if (obj.completed) completedCount++;
    }

    float ratio = quest.objectives.empty() ? 1.0f : (static_cast<float>(completedCount) / static_cast<float>(quest.objectives.size()));
    if (m_progressBar) {
        m_progressBar->progress(ratio);
    }

    // Update Track Quest button
    if (m_setActiveBtn) {
        if (m_currentViewIndex == 1) {
            // Completed quest
            m_setActiveBtn->setVisible(false);
        } else {
            m_setActiveBtn->setVisible(true);
            if (quest.active) {
                m_setActiveBtn->label("Untrack Quest");
                m_setActiveBtn->normalColor(Color(55, 35, 35, 230), Color(255, 120, 120, 255), Color::White());
            } else {
                m_setActiveBtn->label("Track Quest (Active)");
                m_setActiveBtn->normalColor(Color(35, 55, 35, 230), Color::TextSuccess(), Color::White());
            }

            m_setActiveBtn->onClick([this]() {
                if (m_selectedActiveIndex < m_activeQuests.size()) {
                    auto& curQuest = m_activeQuests[m_selectedActiveIndex];
                    curQuest.active = !curQuest.active;
                    if (m_onTrackQuest && curQuest.formId != 0) {
                        m_onTrackQuest(curQuest.formId, curQuest.active);
                    }
                    if (m_context) {
                        m_context->showToast(
                            curQuest.active ? "Quest Tracked" : "Quest Untracked",
                            curQuest.title,
                            curQuest.active ? ToastType::Success : ToastType::Info
                        );
                        m_context->playSound("UIQuestUpdate");
                    }
                    rebuildQuestList();
                    updateDetailsPanel();
                }
            });
        }
    }

    // Dynamic checkboxes for objectives
    if (m_objectivesContainer) {
        m_objectivesContainer->clearChildren();
        m_objectiveCheckboxes.clear();

        for (size_t i = 0; i < quest.objectives.size(); ++i) {
            const auto& obj = quest.objectives[i];
            auto* cb = m_objectivesContainer->add<Checkbox>(obj.text);
            cb->boxSize(16.0f).fontSize(13.0f);
            cb->checked(obj.completed);
            cb->labelColor(obj.completed ? Color::TextSuccess() : Color(200, 205, 215, 255));

            cb->onToggle([this, cb, i](bool checked) {
                auto& curQuests = (m_currentViewIndex == 1) ? m_completedQuests : m_activeQuests;
                size_t idx = (m_currentViewIndex == 1) ? m_selectedCompletedIndex : m_selectedActiveIndex;
                if (idx < curQuests.size() && i < curQuests[idx].objectives.size()) {
                    curQuests[idx].objectives[i].completed = checked;
                    cb->labelColor(checked ? Color::TextSuccess() : Color(200, 205, 215, 255));

                    int comp = 0;
                    for (const auto& o : curQuests[idx].objectives) {
                        if (o.completed) comp++;
                    }
                    float newRatio = static_cast<float>(comp) / static_cast<float>(curQuests[idx].objectives.size());
                    if (m_progressBar) {
                        m_progressBar->progress(newRatio);
                    }
                }
            });

            m_objectiveCheckboxes.push_back(cb);
        }
    }

    markLayoutDirty();
}

void JournalWindow::updateStatsView() {
    if (m_statsHeroNameText) m_statsHeroNameText->text(m_playerStats.name);
    if (m_statsHeroLevelText) m_statsHeroLevelText->text("Level: " + std::to_string(m_playerStats.level));
    if (m_statsHeroGoldText) m_statsHeroGoldText->text("Gold: " + std::to_string(m_playerStats.gold));

    if (m_healthBar && m_playerStats.maxHealth > 0.0f) {
        m_healthBar->progress(m_playerStats.health / m_playerStats.maxHealth);
    }
    if (m_magickaBar && m_playerStats.maxMagicka > 0.0f) {
        m_magickaBar->progress(m_playerStats.magicka / m_playerStats.maxMagicka);
    }
    if (m_staminaBar && m_playerStats.maxStamina > 0.0f) {
        m_staminaBar->progress(m_playerStats.stamina / m_playerStats.maxStamina);
    }

    if (m_statsActiveCountText) {
        m_statsActiveCountText->text("Active Quests: " + std::to_string(m_activeQuests.size()));
    }
    if (m_statsCompletedCountText) {
        m_statsCompletedCountText->text("Completed Quests: " + std::to_string(m_completedQuests.size()));
    }

    markLayoutDirty();
}

void JournalWindow::update(float deltaTime) {
    Panel::update(deltaTime);

    m_profilerTimer += deltaTime;
    if (m_profilerTimer >= 0.20f && m_profilerText && context()) {
        m_profilerTimer = 0.0f;
        const auto& metrics = context()->metrics();

        char buf[64];
        std::snprintf(buf, sizeof(buf), "Layout: %.0f\xc2\xb5s | Render: %.0f\xc2\xb5s | %u Items",
                      metrics.layoutTimeUs, metrics.renderTimeUs, metrics.elementCount);
        m_profilerText->text(buf);
    }
}

void JournalWindow::measure(Dimensions availableSize) {
    float targetW = (std::min)(availableSize.width * 0.90f, 960.0f);
    float targetH = (std::min)(availableSize.height * 0.88f, 580.0f);
    targetW = (std::max)(targetW, 600.0f);
    targetH = (std::max)(targetH, 380.0f);

    layout().width(targetW).height(targetH);
    Panel::measure(availableSize);
}

void JournalWindow::arrange(const Rect& finalRect) {
    float w = layout().width().value;
    float h = layout().height().value;
    float x = 0.0f;
    float y = 0.0f;

    if (m_customPosition.x >= 0.0f && m_customPosition.y >= 0.0f) {
        float maxX = (std::max)(0.0f, finalRect.width - w);
        float maxY = (std::max)(0.0f, finalRect.height - h);
        x = std::clamp(m_customPosition.x, 0.0f, maxX);
        y = std::clamp(m_customPosition.y, 0.0f, maxY);
    } else {
        x = (std::max)(0.0f, (finalRect.width - w) * 0.5f);
        y = (std::max)(0.0f, (finalRect.height - h) * 0.5f);
        m_customPosition = Point{ x, y };
    }

    Panel::arrange(Rect{ x, y, w, h });
}

bool JournalWindow::onPointerDown(const Point& localPoint) {
    if (localPoint.y >= 0.0f && localPoint.y <= m_headerHeight) {
        m_isDragging = true;
        m_dragStartMouse = context() ? context()->lastMousePos() : Point{ bounds().x + localPoint.x, bounds().y + localPoint.y };
        m_dragStartPos = Point{ bounds().x, bounds().y };
        return true;
    }
    return Panel::onPointerDown(localPoint);
}

void JournalWindow::onPointerMove(const Point& localPoint) {
    Panel::onPointerMove(localPoint);

    if (m_isDragging && context()) {
        Point currentMouse = context()->lastMousePos();
        Point delta = currentMouse - m_dragStartMouse;
        float newX = m_dragStartPos.x + delta.x;
        float newY = m_dragStartPos.y + delta.y;

        float maxW = context()->viewportSize().width - m_bounds.width;
        float maxH = context()->viewportSize().height - m_bounds.height;
        m_customPosition.x = std::clamp(newX, 0.0f, (std::max)(0.0f, maxW));
        m_customPosition.y = std::clamp(newY, 0.0f, (std::max)(0.0f, maxH));

        markLayoutDirty();
    }
}

bool JournalWindow::onPointerUp(const Point& localPoint) {
    if (m_isDragging) {
        m_isDragging = false;
        auto& cfg = ConfigManager::GetSingleton().config();
        cfg.windowPosX = m_customPosition.x;
        cfg.windowPosY = m_customPosition.y;
        ConfigManager::GetSingleton().saveToFile();
        return true;
    }
    return Panel::onPointerUp(localPoint);
}

void JournalWindow::setCustomPosition(const Point& pos) {
    m_customPosition = pos;
    auto& cfg = ConfigManager::GetSingleton().config();
    cfg.windowPosX = m_customPosition.x;
    cfg.windowPosY = m_customPosition.y;
    ConfigManager::GetSingleton().saveToFile();
    markLayoutDirty();
}

void JournalWindow::resetPosition() {
    m_customPosition = Point{ -1.0f, -1.0f };
    auto& cfg = ConfigManager::GetSingleton().config();
    cfg.windowPosX = -1.0f;
    cfg.windowPosY = -1.0f;
    ConfigManager::GetSingleton().saveToFile();
    markLayoutDirty();
}

} // namespace PerfUI
