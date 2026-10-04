#pragma once

#include "Panel.h"
#include "Text.h"
#include "Button.h"
#include "ScrollView.h"
#include "SettingsView.h"
#include <string>
#include <vector>
#include <functional>

namespace PerfUI {

struct SaveGameSlot {
    std::string characterName{ "Dovahkiin" };
    int level{ 14 };
    std::string location{ "Whiterun - Dragonsreach" };
    std::string playtime{ "32h 45m" };
    std::string date{ "17 Last Seed, 4E 201" };
    int slotNumber{ 1 };
};

class PERFUI_API MainMenuWindow : public Panel {
public:
    explicit MainMenuWindow(std::string name = "MainMenuWindow");
    ~MainMenuWindow() override = default;

    void update(float deltaTime) override;
    void measure(Dimensions availableSize) override;
    void arrange(const Rect& finalRect) override;

    // Callbacks for game actions
    void onContinueGame(std::function<void()> cb) { m_onContinue = std::move(cb); }
    void onNewGame(std::function<void()> cb) { m_onNewGame = std::move(cb); }
    void onLoadSave(std::function<void(int slot)> cb) { m_onLoadSave = std::move(cb); }
    void onQuitToDesktop(std::function<void()> cb) { m_onQuit = std::move(cb); }

    void setSaveSlots(std::vector<SaveGameSlot> slots);
    void switchRightPanel(int mode); // 0 = Preview Last Save, 1 = Load Save List, 2 = Settings, 3 = Credits

private:
    void initDefaultSaves();
    void buildUI();
    void rebuildSaveSlotsList();

    std::vector<SaveGameSlot> m_saveSlots;
    int m_currentRightMode{ 0 };

    // Navigation buttons
    Button* m_btnContinue{ nullptr };
    Button* m_btnNewGame{ nullptr };
    Button* m_btnLoadGame{ nullptr };
    Button* m_btnSettings{ nullptr };
    Button* m_btnCredits{ nullptr };
    Button* m_btnQuit{ nullptr };

    // Right Content Area
    Panel* m_rightContainer{ nullptr };
    Panel* m_continueCard{ nullptr };
    ScrollView* m_loadSaveScroll{ nullptr };
    SettingsView* m_settingsView{ nullptr };
    Panel* m_creditsCard{ nullptr };

    // Continue Card texts
    Text* m_continueCharText{ nullptr };
    Text* m_continueLocText{ nullptr };
    Text* m_continuePlaytimeText{ nullptr };
    Text* m_continueDateText{ nullptr };

    // Callbacks
    std::function<void()> m_onContinue;
    std::function<void()> m_onNewGame;
    std::function<void(int)> m_onLoadSave;
    std::function<void()> m_onQuit;
};

} // namespace PerfUI
