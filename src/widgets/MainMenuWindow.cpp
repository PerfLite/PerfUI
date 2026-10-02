#include "PerfUI/MainMenuWindow.h"
#include "PerfUI/UIContext.h"
#include "PerfUI/ConfigManager.h"
#include "PerfUI/Theme.h"
#include "PerfUI/ModalDialog.h"
#include "PerfUI/Toast.h"
#include <algorithm>
#include <cstdio>

namespace PerfUI {

MainMenuWindow::MainMenuWindow(std::string name)
    : Panel(std::move(name))
{
    setFocusable(true);
    initDefaultSaves();
    buildUI();
}

void MainMenuWindow::initDefaultSaves() {
    m_saveSlots = {
        { "Dovahkiin", 14, "Whiterun - Dragonsreach", "32h 45m", "17 Last Seed, 4E 201", 1 },
        { "Dovahkiin", 12, "Bleak Falls Barrow", "26h 10m", "14 Last Seed, 4E 201", 2 },
        { "Dovahkiin", 8, "Riverwood - Sleeping Giant Inn", "15h 20m", "10 Last Seed, 4E 201", 3 },
        { "Dovahkiin", 1, "Helgen Keep - Escape", "1h 05m", "17 Last Seed, 4E 201", 4 }
    };
}

void MainMenuWindow::buildUI() {
    // Fullscreen styling
    backgroundColor(Color(10, 14, 20, 245));
    borderWidth(0.0f);

    layout().direction(LayoutDirection::Vertical)
            .padding(28.0f)
            .gap(16.0f);

    // Main Body: Left Menu Column + Right Dynamic Preview
    auto* mainBody = add<Panel>("MainBody");
    mainBody->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    mainBody->layout().direction(LayoutDirection::Horizontal)
                      .gap(32.0f)
                      .flex(1.0f);

    // ==========================================
    // 1. LEFT NAVIGATION COLUMN (Width: 340px)
    // ==========================================
    auto* leftCol = mainBody->add<Panel>("LeftNavColumn");
    leftCol->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    leftCol->layout().width(340.0f)
                    .direction(LayoutDirection::Vertical)
                    .gap(14.0f);

    // Logo & Title Stack
    auto* titleStack = leftCol->add<Panel>("TitleStack");
    titleStack->backgroundColor(Color(16, 22, 32, 190))
              .borderColor(Color::BorderSubtle())
              .borderWidth(1.0f)
              .cornerRadius(8.0f);
    titleStack->layout().direction(LayoutDirection::Vertical)
                        .padding(18.0f)
                        .gap(4.0f);

    auto* franchiseText = titleStack->add<Text>("THE ELDER SCROLLS V");
    franchiseText->fontSize(11.0f).color(Color::TextSecondary()).bold(true);

    auto* gameTitle = titleStack->add<Text>("SKYRIM");
    gameTitle->fontSize(28.0f).color(Color::NordicGold()).bold(true);

    auto* frameworkBadge = titleStack->add<Text>("PERFUI RETAINED ENGINE");
    frameworkBadge->fontSize(10.5f).color(Color::TextAccent());

    // Navigation Buttons Stack
    auto* navList = leftCol->add<Panel>("NavButtonsStack");
    navList->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    navList->layout().direction(LayoutDirection::Vertical).gap(8.0f).flex(1.0f);

    auto createNavBtn = [navList](std::string label, std::string tooltip) -> Button* {
        auto* btn = navList->add<Button>(std::move(label));
        btn->layout().width(DimensionConstraint::Flex(1.0f)).padding(16.0f, 10.0f);
        btn->fontSize(13.5f);
        btn->normalColor(Color(20, 26, 36, 220), Color::BorderSubtle(), Color::TextPrimary());
        btn->hoverColor(Color(36, 48, 66, 250), Color::BorderFocus(), Color::NordicGold());
        btn->pressedColor(Color(45, 60, 82, 250), Color::BorderFocus(), Color::White());
        btn->cornerRadius(6.0f);
        if (!tooltip.empty()) {
            btn->tooltip(tooltip);
        }
        return btn;
    };

    m_btnContinue = createNavBtn("CONTINUE", "Resume your last adventure immediately");
    m_btnContinue->normalColor(Color(28, 36, 50, 240), Color::BorderFocus(), Color::NordicGold());
    m_btnContinue->onClick([this]() {
        switchRightPanel(0);
        if (m_onContinue) {
            m_onContinue();
        } else if (context()) {
            context()->showToast("Resuming Game", "Loading Dovahkiin at Whiterun Dragonsreach...", ToastType::Success);
            context()->playSound("UIJournalOpen");
        }
    });

    m_btnNewGame = createNavBtn("NEW GAME", "Start a new adventure from Helgen");
    m_btnNewGame->onClick([this]() {
        if (context()) {
            auto modal = std::make_shared<ModalDialog>(
                "Start New Adventure?",
                "Are you sure you want to begin a new game? Any unsaved progress will be lost.",
                "Begin New Game",
                "Cancel"
            );
            modal->onConfirm([this]() {
                if (m_onNewGame) {
                    m_onNewGame();
                } else if (context()) {
                    context()->showToast("New Game Started", "Welcome to Skyrim, Dragonborn!", ToastType::Success);
                    context()->playSound("UIJournalOpen");
                }
            });
            context()->showModal(modal);
        }
    });

    m_btnLoadGame = createNavBtn("LOAD GAME", "Browse and load previous saved games");
    m_btnLoadGame->onClick([this]() {
        switchRightPanel(1);
    });

    m_btnSettings = createNavBtn("SETTINGS / MCM", "Configure visual themes, hotkeys, and gameplay settings");
    m_btnSettings->onClick([this]() {
        switchRightPanel(2);
    });

    m_btnCredits = createNavBtn("CREDITS", "View PerfUI framework developers and architecture");
    m_btnCredits->onClick([this]() {
        switchRightPanel(3);
    });

    m_btnQuit = createNavBtn("QUIT TO DESKTOP", "Exit Skyrim and return to Windows desktop");
    m_btnQuit->hoverColor(Color(56, 26, 26, 250), Color(230, 80, 80, 255), Color(255, 120, 120, 255));
    m_btnQuit->onClick([this]() {
        if (context()) {
            auto modal = std::make_shared<ModalDialog>(
                "Exit Skyrim?",
                "Are you sure you want to quit to the Windows desktop?",
                "Quit to Desktop",
                "Cancel"
            );
            modal->onConfirm([this]() {
                if (m_onQuit) {
                    m_onQuit();
                } else if (context()) {
                    context()->showToast("Exiting Game", "Farewell, traveler of Skyrim...", ToastType::Info);
                }
            });
            context()->showModal(modal);
        }
    });

    // ==========================================
    // 2. RIGHT CONTENT AREA (Flex container)
    // ==========================================
    m_rightContainer = mainBody->add<Panel>("RightContentArea");
    m_rightContainer->backgroundColor(Color(16, 22, 32, 210))
                    .borderColor(Color::BorderSubtle())
                    .borderWidth(1.0f)
                    .cornerRadius(10.0f)
                    .shadow(true, Color(0, 0, 0, 190), 20.0f, { 0.0f, 6.0f });
    m_rightContainer->layout().flex(1.0f)
                            .padding(24.0f)
                            .direction(LayoutDirection::Vertical)
                            .gap(16.0f);

    // --- Sub-panel 0: Continue Preview Card ---
    m_continueCard = m_rightContainer->add<Panel>("ContinueCard");
    m_continueCard->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    m_continueCard->layout().direction(LayoutDirection::Vertical).gap(14.0f).flex(1.0f);

    auto* contTitle = m_continueCard->add<Text>("RESUME ADVENTURE");
    contTitle->color(Color::NordicGold()).fontSize(16.0f).bold(true);

    auto* heroCard = m_continueCard->add<Panel>("HeroCard");
    heroCard->backgroundColor(Color(22, 29, 42, 220))
            .borderColor(Color::BorderFocus())
            .borderWidth(1.5f)
            .cornerRadius(8.0f);
    heroCard->layout().padding(20.0f).direction(LayoutDirection::Vertical).gap(10.0f);

    m_continueCharText = heroCard->add<Text>("Dovahkiin - Level 14 Nord");
    m_continueCharText->fontSize(20.0f).color(Color::White()).bold(true);

    auto* divider = heroCard->add<Panel>("HeroDivider");
    divider->backgroundColor(Color::BorderSubtle()).borderWidth(0.0f);
    divider->layout().height(1.0f).width(DimensionConstraint::Flex(1.0f));

    m_continueLocText = heroCard->add<Text>("Location: Whiterun - Dragonsreach");
    m_continueLocText->fontSize(13.5f).color(Color::TextPrimary());

    m_continuePlaytimeText = heroCard->add<Text>("Playtime: 32 hours, 45 minutes");
    m_continuePlaytimeText->fontSize(13.0f).color(Color::TextSecondary());

    m_continueDateText = heroCard->add<Text>("Date: 17th of Last Seed, 4E 201 (Skyrim Time)");
    m_continueDateText->fontSize(12.5f).color(Color::TextAccent());

    auto* btnResumeHero = m_continueCard->add<Button>("[ Enter ] Resume Adventure");
    btnResumeHero->layout().padding(18.0f, 10.0f);
    btnResumeHero->fontSize(14.0f);
    btnResumeHero->normalColor(Color::BackgroundElevated(), Color::NordicGold(), Color::NordicGold());
    btnResumeHero->hoverColor(Color::BackgroundActive(), Color::NordicGold(), Color::White());
    btnResumeHero->onClick([this]() {
        if (m_onContinue) {
            m_onContinue();
        } else if (context()) {
            context()->showToast("Resuming Game", "Loading Dovahkiin at Dragonsreach...", ToastType::Success);
            context()->playSound("UIJournalOpen");
        }
    });

    // --- Sub-panel 1: Load Save Game List ---
    m_loadSaveScroll = m_rightContainer->add<ScrollView>("LoadSaveScroll");
    m_loadSaveScroll->layout().direction(LayoutDirection::Vertical).gap(10.0f).flex(1.0f);
    m_loadSaveScroll->setVisible(false);
    rebuildSaveSlotsList();

    // --- Sub-panel 2: SettingsView ---
    m_settingsView = m_rightContainer->add<SettingsView>("MainMenuSettings");
    m_settingsView->layout().flex(1.0f);
    m_settingsView->setVisible(false);

    auto& cfg = ConfigManager::GetSingleton().config();
    auto* appSec = m_settingsView->addSection("GRAPHICS & THEMES");
    appSec->addChoice(
        "Theme Palette",
        { "Nordic Gold", "Imperial Ruby", "Dawnguard Amber", "Winterhold Frost" },
        0,
        "Select active UI accent theme",
        [this](size_t idx, const std::string& choice) {
            (void)idx;
            ConfigManager::GetSingleton().config().themePreset = choice;
            Theme::ApplyPreset(choice);
            if (context()) context()->showToast("Theme Changed", choice, ToastType::Info);
            markLayoutDirty();
        }
    );

    auto* ctrlSec = m_settingsView->addSection("CONTROLS & KEYBINDINGS");
    ctrlSec->addKeybind(
        "Journal Hotkey",
        cfg.toggleHotkey,
        "Press key to rebind the in-game Quest Journal hotkey",
        [](uint32_t key) {
            ConfigManager::GetSingleton().config().toggleHotkey = key;
            ConfigManager::GetSingleton().saveToFile();
        }
    );

    // --- Sub-panel 3: Credits Card ---
    m_creditsCard = m_rightContainer->add<Panel>("CreditsCard");
    m_creditsCard->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    m_creditsCard->layout().direction(LayoutDirection::Vertical).gap(14.0f).flex(1.0f);
    m_creditsCard->setVisible(false);

    auto* credTitle = m_creditsCard->add<Text>("PERFUI FRAMEWORK CREDITS");
    credTitle->color(Color::NordicGold()).fontSize(16.0f).bold(true);

    auto addCredEntry = [this](std::string role, std::string desc) {
        auto* row = m_creditsCard->add<Panel>("CredRow");
        row->backgroundColor(Color(22, 28, 38, 180)).borderColor(Color::BorderSubtle()).borderWidth(1.0f).cornerRadius(6.0f);
        row->layout().padding(12.0f, 8.0f).direction(LayoutDirection::Vertical).gap(2.0f);
        auto* rText = row->add<Text>(role);
        rText->fontSize(13.0f).bold(true).color(Color::TextAccent());
        auto* dText = row->add<Text>(desc);
        dText->fontSize(12.0f).color(Color::TextPrimary());
    };

    addCredEntry("Core Framework Architecture", "Retained-mode C++20 UI Tree, Layout Engine & Visual System by PerfLite & Antigravity");
    addCredEntry("Direct3D 11 & SKSE Integration", "CommonLibSSE-NG, DXGI Present Hooks & Pipeline State Isolation");
    addCredEntry("Rendering Backend", "Invisible Dear ImGui Vector DrawList abstraction (Zero direct ImGui headers in Core)");
    addCredEntry("Audio & Engine Services", "RE::BSAudioManager TaskInterface bridge with safe thread dispatching");
    addCredEntry("Special Thanks", "Bethesda Game Studios, SKSE Team, and the Skyrim Modding Community");

    // ==========================================
    // 3. BOTTOM FOOTER BAR
    // ==========================================
    auto* footer = add<Panel>("Footer");
    footer->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    footer->layout().direction(LayoutDirection::Horizontal)
                    .justify(JustifyContent::SpaceBetween)
                    .alignment(Alignment::Center)
                    .padding(4.0f, 0.0f);

    auto* footerInfo = footer->add<Text>("Skyrim Special Edition (1.5.97 / AE) | SKSE64 2.0.20 | PerfUI Core v0.1.0");
    footerInfo->fontSize(11.5f).color(Color(100, 110, 125, 255));

    auto* footerKeys = footer->add<Text>("[Enter / LMB] Select   [ESC] Return to Game");
    footerKeys->fontSize(11.5f).color(Color::TextSecondary());
}

void MainMenuWindow::switchRightPanel(int mode) {
    m_currentRightMode = mode;
    if (context()) {
        context()->playSound("UIMenuBlade");
    }

    if (m_continueCard) m_continueCard->setVisible(mode == 0);
    if (m_loadSaveScroll) m_loadSaveScroll->setVisible(mode == 1);
    if (m_settingsView) m_settingsView->setVisible(mode == 2);
    if (m_creditsCard) m_creditsCard->setVisible(mode == 3);

    // Highlight current button
    auto updateBtn = [](Button* btn, bool active) {
        if (!btn) return;
        if (active) {
            btn->normalColor(Color(36, 48, 66, 250), Color::BorderFocus(), Color::NordicGold());
        } else {
            btn->normalColor(Color(20, 26, 36, 220), Color::BorderSubtle(), Color::TextPrimary());
        }
    };

    updateBtn(m_btnContinue, mode == 0);
    updateBtn(m_btnLoadGame, mode == 1);
    updateBtn(m_btnSettings, mode == 2);
    updateBtn(m_btnCredits, mode == 3);

    markLayoutDirty();
}

void MainMenuWindow::rebuildSaveSlotsList() {
    if (!m_loadSaveScroll) return;

    m_loadSaveScroll->clearChildren();

    auto* header = m_loadSaveScroll->add<Text>("SAVED ADVENTURES");
    header->color(Color::NordicGold()).fontSize(16.0f).bold(true);

    for (const auto& slot : m_saveSlots) {
        auto* card = m_loadSaveScroll->add<Panel>("SaveSlotCard");
        card->backgroundColor(Color(22, 29, 42, 220))
            .borderColor(Color::BorderSubtle())
            .borderWidth(1.0f)
            .cornerRadius(8.0f);
        card->layout().padding(14.0f).direction(LayoutDirection::Horizontal)
                      .justify(JustifyContent::SpaceBetween)
                      .alignment(Alignment::Center);

        auto* infoCol = card->add<Panel>("SlotInfo");
        infoCol->backgroundColor(Color::Transparent()).borderWidth(0.0f);
        infoCol->layout().direction(LayoutDirection::Vertical).gap(3.0f);

        char titleBuf[64];
        std::snprintf(titleBuf, sizeof(titleBuf), "%s - Level %d", slot.characterName.c_str(), slot.level);
        auto* titleText = infoCol->add<Text>(titleBuf);
        titleText->fontSize(14.0f).bold(true).color(Color::White());

        auto* locText = infoCol->add<Text>(slot.location + " (" + slot.playtime + ")");
        locText->fontSize(12.5f).color(Color::TextSecondary());

        auto* dateText = infoCol->add<Text>(slot.date);
        dateText->fontSize(11.5f).color(Color::TextAccent());

        int slotNum = slot.slotNumber;
        auto* loadBtn = card->add<Button>("LOAD");
        loadBtn->layout().padding(16.0f, 6.0f);
        loadBtn->fontSize(12.0f);
        loadBtn->normalColor(Color::BackgroundElevated(), Color::BorderFocus(), Color::NordicGold());
        loadBtn->onClick([this, slotNum, name = slot.characterName, loc = slot.location]() {
            if (m_onLoadSave) {
                m_onLoadSave(slotNum);
            } else if (context()) {
                context()->showToast("Loading Save Slot", name + " at " + loc, ToastType::Success);
                context()->playSound("UIJournalOpen");
            }
        });
    }

    markLayoutDirty();
}

void MainMenuWindow::setSaveSlots(std::vector<SaveGameSlot> slots) {
    m_saveSlots = std::move(slots);
    if (!m_saveSlots.empty()) {
        const auto& s = m_saveSlots[0];
        if (m_continueCharText) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%s - Level %d", s.characterName.c_str(), s.level);
            m_continueCharText->text(buf);
        }
        if (m_continueLocText) m_continueLocText->text("Location: " + s.location);
        if (m_continuePlaytimeText) m_continuePlaytimeText->text("Playtime: " + s.playtime);
        if (m_continueDateText) m_continueDateText->text("Date: " + s.date);
    }
    rebuildSaveSlotsList();
}

void MainMenuWindow::update(float deltaTime) {
    Panel::update(deltaTime);
}

void MainMenuWindow::measure(Dimensions availableSize) {
    layout().width(availableSize.width).height(availableSize.height);
    Panel::measure(availableSize);
}

void MainMenuWindow::arrange(const Rect& finalRect) {
    Panel::arrange(finalRect);
}

} // namespace PerfUI
