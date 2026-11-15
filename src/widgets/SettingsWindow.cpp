#include "PerfUI/SettingsWindow.h"
#include "PerfUI/UIContext.h"
#include "PerfUI/Theme.h"
#include "PerfUI/ModalDialog.h"
#include "PerfUI/ConfigManager.h"
#include "PerfUI/Localization.h"
#include <algorithm>

namespace PerfUI {

static inline std::string Tr(std::string_view key, std::string_view fallback) {
    return LocalizationService::instance().translate(key, fallback);
}

SettingsWindow::SettingsWindow(std::string name)
    : Panel(std::move(name)) {
    ConfigManager::GetSingleton().loadFromFile();

    const auto& cfg = ConfigManager::GetSingleton().config();
    if (cfg.windowPosX >= 0.0f && cfg.windowPosY >= 0.0f) {
        m_customPosition = Point{ cfg.windowPosX, cfg.windowPosY };
    } else {
        m_customPosition = Point{ -1.0f, -1.0f };
    }

    // Window base layout
    layout().width(960.0f)
            .height(640.0f)
            .direction(LayoutDirection::Vertical)
            .padding(0.0f);

    setAbsolute(true);
    layout().clearAbsoluteAnchors();

    buildUI();
    buildBuiltinCategories();
    selectCategory("toasts");
}

void SettingsWindow::resetPosition() {
    float vpW = 1920.0f;
    float vpH = 1080.0f;
    if (context()) {
        vpW = context()->viewportSize().width;
        vpH = context()->viewportSize().height;
    }
    float w = layout().width().value > 0.0f ? layout().width().value : 960.0f;
    float h = layout().height().value > 0.0f ? layout().height().value : 640.0f;
    if (vpW < w + 20.0f) w = (std::max)(600.0f, vpW - 20.0f);
    if (vpH < h + 20.0f) h = (std::max)(400.0f, vpH - 20.0f);

    m_customPosition.x = (std::max)(0.0f, (vpW - w) * 0.5f);
    m_customPosition.y = (std::max)(0.0f, (vpH - h) * 0.5f);
    m_bounds = Rect{ m_customPosition.x, m_customPosition.y, w, h };
    layout().clearAbsoluteAnchors();

    auto& cfg = ConfigManager::GetSingleton().config();
    cfg.windowPosX = -1.0f;
    cfg.windowPosY = -1.0f;
    ConfigManager::GetSingleton().saveToFile();

    markLayoutDirty();
}

void SettingsWindow::close() {
    setVisible(false);
    if (context()) {
        context()->playSound("UIMenuCancel");
    }
    if (m_onClose) {
        m_onClose();
    }
}

bool SettingsWindow::onKeyDown(int keyCode) {
    if (keyCode == 0x1B) { // VK_ESCAPE
        close();
        return true;
    }
    return Panel::onKeyDown(keyCode);
}

bool SettingsWindow::onPointerDown(const Point& localPoint) {
    // Top header dragging area (height 48px)
    if (localPoint.y >= 0.0f && localPoint.y <= 48.0f) {
        m_dragging = true;
        m_dragStartMouse = context() ? context()->lastMousePos() : Point{ m_bounds.x + localPoint.x, m_bounds.y + localPoint.y };
        m_dragStartPos = Point{ m_bounds.x, m_bounds.y };
        return true;
    }
    return Panel::onPointerDown(localPoint);
}

void SettingsWindow::onPointerMove(const Point& localPoint) {
    Panel::onPointerMove(localPoint);

    if (m_dragging && context()) {
        Point currentMouse = context()->lastMousePos();
        Point delta = currentMouse - m_dragStartMouse;
        float newX = m_dragStartPos.x + delta.x;
        float newY = m_dragStartPos.y + delta.y;

        float vpW = 1920.0f;
        float vpH = 1080.0f;
        if (context()) {
            vpW = context()->viewportSize().width;
            vpH = context()->viewportSize().height;
        }
        if (vpW <= 0.0f) vpW = 1920.0f;
        if (vpH <= 0.0f) vpH = 1080.0f;
        float maxW = (std::max)(0.0f, vpW - m_bounds.width);
        float maxH = (std::max)(0.0f, vpH - m_bounds.height);

        m_customPosition.x = std::clamp(newX, 0.0f, maxW);
        m_customPosition.y = std::clamp(newY, 0.0f, maxH);

        m_bounds.x = m_customPosition.x;
        m_bounds.y = m_customPosition.y;

        markLayoutDirty();
    }
}

bool SettingsWindow::onPointerUp(const Point& localPoint) {
    if (m_dragging) {
        m_dragging = false;
        auto& cfg = ConfigManager::GetSingleton().config();
        cfg.windowPosX = m_customPosition.x;
        cfg.windowPosY = m_customPosition.y;
        ConfigManager::GetSingleton().saveToFile();
        return true;
    }
    return Panel::onPointerUp(localPoint);
}

void SettingsWindow::buildUI() {
    const auto& theme = Theme::Current();

    backgroundColor(theme.colors.surfaceBase);
    borderColor(theme.colors.borderFocus);
    borderWidth(1.5f);
    cornerRadius(theme.metrics.radiusMedium > 0.0f ? theme.metrics.radiusMedium : 10.0f);
    shadow(true);
    backdropBlur(true, 1.5f);
    liquidGlass(true);

    // =========================================================================
    // 1. Header Bar
    // =========================================================================
    auto* header = add<Panel>("HeaderBar");
    header->backgroundColor(Color::Transparent());
    header->borderColor(theme.colors.borderSubtle);
    header->borderWidth(0.0f);
    header->cornerRadius(0.0f);
    header->layout().direction(LayoutDirection::Horizontal)
                    .alignment(Alignment::Center)
                    .justify(JustifyContent::SpaceBetween)
                    .padding(18.0f, 10.0f)
                    .height(44.0f)
                    .width(DimensionConstraint::Flex(1.0f));

    auto* titleLeft = header->add<Panel>("TitleLeft");
    titleLeft->backgroundColor(Color::Transparent());
    titleLeft->layout().direction(LayoutDirection::Horizontal).alignment(Alignment::Center).gap(10.0f);

    m_titleText = titleLeft->add<Text>(Tr("UI_SETTINGS_TITLE", "PERFUI MOD SETTINGS & CONFIGURATION"));
    m_titleText->color(theme.colors.textAccent).fontSize(14.0f).bold(true);

    m_btnClose = header->add<Button>("  X  ", "BtnCloseSettings");
    m_btnClose->layout().padding(10.0f, 4.0f);
    m_btnClose->fontSize(12.0f);
    m_btnClose->onClick([this]() {
        close();
    });

    // Divider between Header and Body
    auto* topDiv = add<Panel>("TopDivider");
    topDiv->backgroundColor(theme.colors.borderSubtle);
    topDiv->layout().height(1.0f).width(DimensionConstraint::Flex(1.0f));

    // =========================================================================
    // 2. Body Container (Left: Categories, Right: SettingsView)
    // =========================================================================
    m_bodyContainer = add<Panel>("BodyContainer");
    m_bodyContainer->backgroundColor(Color::Transparent());
    m_bodyContainer->layout().direction(LayoutDirection::Horizontal)
                             .flex(1.0f)
                             .width(DimensionConstraint::Flex(1.0f));

    // Left Sidebar: Categories
    auto* sidebar = m_bodyContainer->add<Panel>("SidebarPanel");
    sidebar->backgroundColor(Color::Transparent());
    sidebar->borderColor(theme.colors.borderSubtle);
    sidebar->borderWidth(0.0f);
    sidebar->layout().width(240.0f)
                     .direction(LayoutDirection::Vertical)
                     .padding(10.0f, 12.0f)
                     .gap(8.0f);

    auto* catHeader = sidebar->add<Text>(Tr("UI_CATEGORIES", "CATEGORIES"));
    catHeader->color(theme.colors.textSecondary).fontSize(11.0f).bold(true);

    m_categoriesList = sidebar->add<CategoryList>("SettingsCategories");
    m_categoriesList->layout().width(DimensionConstraint::Flex(1.0f));
    m_categoriesList->itemHeight(38.0f);
    m_categoriesList->onCategoryChanged([this](const std::string& id) {
        selectCategory(id);
    });

    // Vertical Divider
    auto* vdiv = m_bodyContainer->add<Panel>("VDiv");
    vdiv->backgroundColor(theme.colors.borderSubtle);
    vdiv->layout().width(1.0f).height(DimensionConstraint::Flex(1.0f));

    // Right Content Area: Settings View
    m_contentContainer = m_bodyContainer->add<Panel>("ContentContainer");
    m_contentContainer->backgroundColor(Color::Transparent());
    m_contentContainer->layout().direction(LayoutDirection::Vertical)
                                .flex(1.0f)
                                .width(DimensionConstraint::Flex(1.0f))
                                .padding(0.0f);

    // Divider between Body and Footer
    auto* botDiv = add<Panel>("BottomDivider");
    botDiv->backgroundColor(theme.colors.borderSubtle);
    botDiv->layout().height(1.0f).width(DimensionConstraint::Flex(1.0f));

    // =========================================================================
    // 3. Footer Bar
    // =========================================================================
    m_footerContainer = add<Panel>("FooterBar");
    m_footerContainer->backgroundColor(Color::Transparent());
    m_footerContainer->borderColor(theme.colors.borderSubtle);
    m_footerContainer->borderWidth(0.0f);
    m_footerContainer->layout().direction(LayoutDirection::Horizontal)
                               .alignment(Alignment::Center)
                               .justify(JustifyContent::SpaceBetween)
                               .padding(16.0f, 10.0f)
                               .height(48.0f)
                               .width(DimensionConstraint::Flex(1.0f));

    auto* leftFoot = m_footerContainer->add<Panel>("LeftFoot");
    leftFoot->backgroundColor(Color::Transparent());
    leftFoot->layout().direction(LayoutDirection::Horizontal).alignment(Alignment::Center).gap(10.0f);

    m_btnReset = leftFoot->add<Button>(Tr("UI_BTN_RESET_DEFAULTS", "Reset Defaults"), "BtnResetDefaults");
    m_btnReset->layout().padding(12.0f, 6.0f);
    m_btnReset->fontSize(12.0f);
    m_btnReset->onClick([this]() {
        if (context()) {
            auto modal = std::make_shared<ModalDialog>(
                Tr("UI_MODAL_RESET_TITLE", "Reset Settings?"),
                Tr("UI_MODAL_RESET_MSG", "Are you sure you want to restore all settings to default values? Any custom preferences will be lost."),
                Tr("UI_MODAL_RESET_CONFIRM", "Reset Defaults"),
                Tr("UI_MODAL_CANCEL", "Cancel")
            );
            modal->onConfirm([this]() {
                auto& c = ConfigManager::GetSingleton().config();
                c.windowOpacity = 0.95f;
                c.themePreset = "Liquid Glass";
                c.language = "auto";
                c.showProfiler = true;
                c.pauseGameWhenOpen = false;
                c.autoTrackNewQuests = false;
                c.settingsHotkey = 0x68; // VK_NUMPAD8
                Theme::ApplyPreset("Glass Frost");
                backdropBlur(true, 1.5f);
                const auto& theme = Theme::Current();
                backgroundColor(theme.colors.surfaceBase);
                borderColor(theme.colors.borderFocus);
                if (m_titleText) m_titleText->color(theme.colors.textAccent);
                if (context()) {
                    context()->showToast(Tr("UI_TOAST_DEFAULTS_RESTORED", "Defaults Restored"), Tr("UI_TOAST_DEFAULTS_RESTORED_DESC", "All settings reset to factory defaults"), ToastType::Info);
                    context()->playSound("UIMenuOK");
                }
                resetPosition();
            });
            context()->showModal(modal);
        }
    });

    m_btnCenter = leftFoot->add<Button>(Tr("UI_BTN_CENTER_WINDOW", "Center Window"), "BtnCenterWindow");
    m_btnCenter->layout().padding(12.0f, 6.0f);
    m_btnCenter->fontSize(12.0f);
    m_btnCenter->onClick([this]() {
        resetPosition();
        if (context()) {
            context()->showToast(Tr("UI_TOAST_CENTERED", "Window Centered"), Tr("UI_TOAST_CENTERED_DESC", "Coordinates reset to display center"), ToastType::Info);
            context()->playSound("UIMenuOK");
        }
    });

    auto* rightFoot = m_footerContainer->add<Panel>("RightFoot");
    rightFoot->backgroundColor(Color::Transparent());
    rightFoot->layout().direction(LayoutDirection::Horizontal).alignment(Alignment::Center).gap(10.0f);

    m_btnSave = rightFoot->add<Button>(Tr("UI_BTN_SAVE_SETTINGS", "Save Settings"), "BtnSaveSettings");
    m_btnSave->layout().padding(16.0f, 6.0f);
    m_btnSave->fontSize(12.0f);
    m_btnSave->normalColor(theme.colors.borderFocus, theme.colors.borderFocus, theme.colors.surfaceBase);
    m_btnSave->onClick([this]() {
        ConfigManager::GetSingleton().saveToFile();
        if (context()) {
            context()->showToast(Tr("UI_TOAST_SAVED", "Settings Saved"), Tr("UI_TOAST_SAVED_DESC", "Preferences written to SKSE/Plugins/PerfUI.json"), ToastType::Success);
            context()->playSound("UIMenuOK");
        }
    });
}

SettingsView* SettingsWindow::getOrCreateCategory(std::string id, std::string title, std::string icon, std::string badge) {
    auto it = m_categoryViews.find(id);
    if (it != m_categoryViews.end()) {
        return it->second;
    }

    if (m_categoriesList) {
        m_categoriesList->addCategory(id, std::move(title), std::move(icon), std::move(badge));
    }

    auto* view = m_contentContainer->add<SettingsView>("SettingsView_" + id);
    view->layout().flex(1.0f).width(DimensionConstraint::Flex(1.0f));
    view->setVisible(false);

    m_categoryViews[id] = view;
    return view;
}

void SettingsWindow::selectCategory(const std::string& id) {
    m_activeCategoryId = id;
    for (auto& [catId, view] : m_categoryViews) {
        if (view) {
            view->setVisible(catId == id);
        }
    }
    if (m_categoriesList) {
        m_categoriesList->selectCategory(id);
    }
    markLayoutDirty();
}

void SettingsWindow::buildBuiltinCategories() {
    auto& cfg = ConfigManager::GetSingleton().config();

    // =========================================================================
    // Category 1: Toasts & Alerts (Tests Toasts & Notifications thoroughly!)
    // =========================================================================
    auto* toastView = getOrCreateCategory("toasts", Tr("UI_TAB_TOASTS", "Toasts & Alerts"), "bell", "HUD");
    {
        auto* testSec = toastView->addSection(Tr("UI_SEC_TOAST_TESTING", "INTERACTIVE TOAST TESTING"));
        testSec->addButton(
            Tr("UI_BTN_TOAST_SUCCESS_LABEL", "Trigger Success Toast"),
            Tr("UI_BTN_TOAST_SUCCESS", "Show Success Toast"),
            Tr("UI_BTN_TOAST_SUCCESS_TIP", "Displays a green success toast in the HUD"),
            [this]() {
                if (context()) {
                    context()->showToast(Tr("UI_TOAST_SUCCESS_DEMO_TITLE", "Quest Completed!"), Tr("UI_TOAST_SUCCESS_DEMO_DESC", "Bleak Falls Barrow cleared (+500 Gold)"), ToastType::Success, 3.5f);
                    context()->playSound("UIMenuOK");
                }
            }
        );

        testSec->addButton(
            Tr("UI_BTN_TOAST_WARNING_LABEL", "Trigger Warning Toast"),
            Tr("UI_BTN_TOAST_WARNING", "Show Warning Toast"),
            Tr("UI_BTN_TOAST_WARNING_TIP", "Displays an amber warning toast in the HUD"),
            [this]() {
                if (context()) {
                    context()->showToast(Tr("UI_TOAST_WARNING_DEMO_TITLE", "Inventory Full"), Tr("UI_TOAST_WARNING_DEMO_DESC", "You are carrying too much weight to run"), ToastType::Warning, 3.5f);
                    context()->playSound("UIMenuOK");
                }
            }
        );

        testSec->addButton(
            Tr("UI_BTN_TOAST_ERROR_LABEL", "Trigger Error Toast"),
            Tr("UI_BTN_TOAST_ERROR", "Show Error Toast"),
            Tr("UI_BTN_TOAST_ERROR_TIP", "Displays a red error toast in the HUD"),
            [this]() {
                if (context()) {
                    context()->showToast(Tr("UI_TOAST_ERROR_DEMO_TITLE", "Spell Failed"), Tr("UI_TOAST_ERROR_DEMO_DESC", "Not enough Magicka to cast Fireball"), ToastType::Error, 3.5f);
                    context()->playSound("UIMenuCancel");
                }
            }
        );

        testSec->addButton(
            Tr("UI_BTN_TOAST_INFO_LABEL", "Trigger Info Toast"),
            Tr("UI_BTN_TOAST_INFO", "Show Info Toast"),
            Tr("UI_BTN_TOAST_INFO_TIP", "Displays a standard info toast in the HUD"),
            [this]() {
                if (context()) {
                    context()->showToast(Tr("UI_TOAST_INFO_DEMO_TITLE", "Location Discovered"), Tr("UI_TOAST_INFO_DEMO_DESC", "Riverwood - Sleeping Giant Inn"), ToastType::Info, 3.5f);
                    context()->playSound("UIMenuOK");
                }
            }
        );

        auto* cfgSec = toastView->addSection(Tr("UI_SEC_HUD_TOAST_CONFIG", "HUD TOAST CONFIGURATION"));
        cfgSec->addToggle(
            Tr("UI_TOGGLE_HUD_TOASTS", "Enable HUD Toasts"),
            ConfigManager::GetSingleton().config().enableToasts,
            Tr("UI_TOGGLE_HUD_TOASTS_TIP", "Show floating toast alerts on in-game events"),
            [this](bool val) {
                auto& cfg = ConfigManager::GetSingleton().config();
                cfg.enableToasts = val;
                ConfigManager::GetSingleton().saveToFile();
                if (context()) context()->playSound("UIMenuOK");
            }
        );

        cfgSec->addSlider(
            Tr("UI_SLIDER_TOAST_DURATION", "Toast Screen Duration"),
            1.0f, 10.0f, 3.5f,
            Tr("UI_SLIDER_TOAST_DURATION_TIP", "How long toasts stay visible before fading out"),
            [this](float val) {
                (void)val;
            },
            "%.1f s"
        );

        std::vector<std::string> positions = {
            Tr("UI_CORNER_BOTTOM_RIGHT", "Bottom Right"),
            Tr("UI_CORNER_TOP_RIGHT", "Top Right"),
            Tr("UI_CORNER_BOTTOM_LEFT", "Bottom Left"),
            Tr("UI_CORNER_TOP_LEFT", "Top Left")
        };
        cfgSec->addChoice(
            Tr("UI_CHOICE_TOAST_CORNER", "Toast Screen Corner"),
            positions,
            0,
            Tr("UI_CHOICE_TOAST_CORNER_TIP", "Corner of the display where toasts stack"),
            [this](size_t idx, const std::string& choice) {
                (void)idx;
                (void)choice;
                if (context()) context()->playSound("UIMenuOK");
            }
        );
    }

    // =========================================================================
    // Category 2: PerfUI Core Settings
    // =========================================================================
    auto* coreView = getOrCreateCategory("core", Tr("UI_TAB_CORE", "PerfUI Core"), "core", "v1.0");
    {
        auto* appSec = coreView->addSection(Tr("UI_SEC_APPEARANCE", "APPEARANCE & THEMES"));

        std::vector<std::string> langChoices = {
            Tr("UI_LANG_AUTO", "Auto (Game Language)"),
            Tr("UI_LANG_RU", "Русский (RU)"),
            Tr("UI_LANG_EN", "English (EN)")
        };
        size_t curLangIdx = 0;
        if (cfg.language == "ru") curLangIdx = 1;
        else if (cfg.language == "en") curLangIdx = 2;

        appSec->addChoice(
            Tr("UI_CHOICE_LANGUAGE", "Interface Language"),
            langChoices,
            curLangIdx,
            Tr("UI_CHOICE_LANGUAGE_TIP", "Display language for UI text, menus, notifications, and tooltips"),
            [this](size_t idx, const std::string& choice) {
                auto& c = ConfigManager::GetSingleton().config();
                std::string newLang = "auto";
                if (idx == 1) newLang = "ru";
                else if (idx == 2) newLang = "en";

                c.language = newLang;
                if (newLang == "auto") {
                    LocalizationService::instance().setLanguage(LocalizationService::instance().detectSystemLanguage());
                } else {
                    LocalizationService::instance().setLanguage(newLang);
                }

                if (context()) {
                    context()->showToast(Tr("UI_TOAST_LANG_CHANGED", "Language Changed"), choice, ToastType::Info);
                    context()->playSound("UIMenuOK");
                }
                markLayoutDirty();
            }
        );

        appSec->addSlider(
            Tr("UI_SLIDER_WINDOW_OPACITY", "Window Background Opacity"),
            0.35f, 1.0f, cfg.windowOpacity,
            Tr("UI_SLIDER_WINDOW_OPACITY_TIP", "Adjust transparency of modal and retained window backgrounds"),
            [this](float val) {
                ConfigManager::GetSingleton().config().windowOpacity = val;
                const auto& theme = Theme::Current();
                Color bg = theme.colors.surfaceBase;
                bg.a = static_cast<uint8_t>(val * 255.0f);
                backgroundColor(bg);
            },
            "%.2f"
        );

        std::vector<std::string> themes = Theme::GetAvailableThemes();
        size_t curIdx = 0;
        for (size_t i = 0; i < themes.size(); ++i) {
            if (themes[i] == cfg.themePreset) {
                curIdx = i;
                break;
            }
        }

        appSec->addChoice(
            Tr("UI_CHOICE_THEME", "Color Palette / Theme Preset"),
            themes,
            curIdx,
            Tr("UI_CHOICE_THEME_TIP", "Active visual theme for borders, tabs, highlights, and buttons"),
            [this](size_t idx, const std::string& choice) {
                (void)idx;
                ConfigManager::GetSingleton().config().themePreset = choice;
                Theme::ApplyPreset(choice);
                const auto& theme = Theme::Current();
                backgroundColor(theme.colors.surfaceBase);
                borderColor(theme.colors.borderFocus);
                bool isGlassTheme = (choice == "Liquid Glass" || choice == "Glass Frost" || choice.find("Glass") != std::string::npos);
                liquidGlass(isGlassTheme);
                backdropBlur(isGlassTheme, 1.5f);
                if (m_titleText) m_titleText->color(theme.colors.textAccent);
                if (context()) {
                    context()->showToast(Tr("UI_TOAST_THEME_APPLIED", "Theme Applied"), choice, ToastType::Info);
                    context()->playSound("UIMenuOK");
                }
                markLayoutDirty();
            }
        );

        appSec->addToggle(
            Tr("UI_TOGGLE_PROFILER", "Show Telemetry Profiler (F7)"),
            cfg.showProfiler,
            Tr("UI_TOGGLE_PROFILER_TIP", "Real-time FPS and microsecond layout profiling in diagnostics"),
            [this](bool val) {
                ConfigManager::GetSingleton().config().showProfiler = val;
                if (context()) context()->playSound("UIMenuOK");
            }
        );

        auto* keySec = coreView->addSection(Tr("UI_SEC_CONTROLS", "CONTROLS & KEYBINDINGS"));
        keySec->addKeybind(
            Tr("UI_KEYBIND_SETTINGS_HOTKEY", "Settings Menu Hotkey"),
            cfg.settingsHotkey,
            Tr("UI_KEYBIND_SETTINGS_HOTKEY_TIP", "Keyboard key used to open and close this settings window"),
            [this](uint32_t newKey) {
                ConfigManager::GetSingleton().config().settingsHotkey = newKey;
                if (context()) {
                    context()->registerHotkey("perfui.settings", newKey, [this]() {
                        setVisible(!isVisible());
                    });
                    context()->showToast(Tr("UI_TOAST_HOTKEY_REBOUND", "Hotkey Rebound"), Tr("UI_TOAST_HOTKEY_REBOUND_DESC", "New shortcut assigned to settings menu"), ToastType::Info);
                }
            }
        );

        keySec->addToggle(
            Tr("UI_TOGGLE_REPLACE_CURSOR", "Replace Vanilla Game Cursor"),
            cfg.replaceVanillaCursor,
            Tr("UI_TOGGLE_REPLACE_CURSOR_TIP", "Replaces Skyrim vanilla cursor with high-DPI styled blade cursor"),
            [this](bool val) {
                ConfigManager::GetSingleton().config().replaceVanillaCursor = val;
                if (context()) context()->playSound("UIMenuOK");
            }
        );
    }

    // =========================================================================
    // Category 3: Graphics & Blur
    // =========================================================================
    auto* gfxView = getOrCreateCategory("display", Tr("UI_TAB_GRAPHICS", "Graphics & Blur"), "eye");
    {
        auto* blurSec = gfxView->addSection(Tr("UI_SEC_D3D11_POSTFX", "DIRECTX 11 HARDWARE POST-PROCESSING"));
        blurSec->addToggle(
            Tr("UI_TOGGLE_KAWASE_BLUR", "Enable Background Kawase Blur"),
            backdropBlur(),
            Tr("UI_TOGGLE_KAWASE_BLUR_TIP", "Cinematically blurs the 3D game scene behind windows"),
            [this](bool val) {
                backdropBlur(val, blurRadius() > 0.0f ? blurRadius() : 1.5f);
                if (context()) {
                    context()->showToast(Tr("UI_TOAST_KAWASE_BLUR", "Kawase Blur"), val ? Tr("UI_TOAST_BLUR_ENABLED", "Backdrop blur enabled") : Tr("UI_TOAST_BLUR_DISABLED", "Backdrop blur disabled"), ToastType::Info);
                    context()->playSound("UIMenuOK");
                }
            }
        );

        blurSec->addSlider(
            Tr("UI_SLIDER_BLUR_RADIUS", "Blur Radius & Sampling Strength"),
            0.5f, 4.0f, blurRadius() > 0.0f ? blurRadius() : 1.5f,
            Tr("UI_SLIDER_BLUR_RADIUS_TIP", "Controls the softness of the frosted glass blur effect"),
            [this](float val) {
                backdropBlur(backdropBlur(), val);
            },
            "%.1f px"
        );
    }
}

void SettingsWindow::update(float deltaTime) {
    Panel::update(deltaTime);
}

void SettingsWindow::measure(Dimensions availableSize) {
    float vpW = availableSize.width;
    float vpH = availableSize.height;
    if (context()) {
        if (vpW <= 0.0f) vpW = context()->viewportSize().width;
        if (vpH <= 0.0f) vpH = context()->viewportSize().height;
    }
    if (vpW <= 0.0f) vpW = 1920.0f;
    if (vpH <= 0.0f) vpH = 1080.0f;

    float w = 960.0f;
    float h = 640.0f;
    if (vpW < w + 20.0f) w = (std::max)(600.0f, vpW - 20.0f);
    if (vpH < h + 20.0f) h = (std::max)(400.0f, vpH - 20.0f);
    layout().width(w).height(h);

    Panel::measure(availableSize);
}

void SettingsWindow::arrange(const Rect& finalRect) {
    float vpW = 1920.0f;
    float vpH = 1080.0f;
    if (context()) {
        vpW = context()->viewportSize().width;
        vpH = context()->viewportSize().height;
    }
    if (vpW <= 0.0f) vpW = (finalRect.width > 960.0f ? finalRect.width : 1920.0f);
    if (vpH <= 0.0f) vpH = (finalRect.height > 640.0f ? finalRect.height : 1080.0f);

    float w = 960.0f;
    float h = 640.0f;

    if (vpW < w + 20.0f) w = (std::max)(600.0f, vpW - 20.0f);
    if (vpH < h + 20.0f) h = (std::max)(400.0f, vpH - 20.0f);
    layout().width(w).height(h);

    float maxX = (std::max)(0.0f, vpW - w);
    float maxY = (std::max)(0.0f, vpH - h);

    if (m_customPosition.x < 0.0f || m_customPosition.y < 0.0f) {
        m_customPosition.x = (std::max)(0.0f, (vpW - w) * 0.5f);
        m_customPosition.y = (std::max)(0.0f, (vpH - h) * 0.5f);
    } else {
        m_customPosition.x = std::clamp(m_customPosition.x, 0.0f, maxX);
        m_customPosition.y = std::clamp(m_customPosition.y, 0.0f, maxY);
    }

    Rect windowRect{ m_customPosition.x, m_customPosition.y, w, h };
    m_bounds = windowRect;
    Panel::arrange(windowRect);
}

} // namespace PerfUI
