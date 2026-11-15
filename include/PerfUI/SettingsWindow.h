#pragma once

#include "Panel.h"
#include "Text.h"
#include "Button.h"
#include "CategoryList.h"
#include "SettingsView.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace PerfUI {

class PERFUI_API SettingsWindow : public Panel {
public:
    using CloseCallback = std::function<void()>;

    explicit SettingsWindow(std::string name = "SettingsWindow");
    ~SettingsWindow() override = default;

    void update(float deltaTime) override;
    void measure(Dimensions availableSize) override;
    void arrange(const Rect& finalRect) override;

    void selectCategory(const std::string& id);
    SettingsView* getOrCreateCategory(std::string id, std::string title, std::string icon = "", std::string badge = "");

    void resetPosition();
    void close();
    void setOnClose(CloseCallback callback) { m_onClose = std::move(callback); }
    const Point& customPosition() const { return m_customPosition; }

    // Hotkey and mouse dragging helpers
    bool onKeyDown(int keyCode) override;
    bool onPointerDown(const Point& localPoint) override;
    void onPointerMove(const Point& localPoint) override;
    bool onPointerUp(const Point& localPoint) override;

private:
    void buildUI();
    void buildBuiltinCategories();

    // Header elements
    Text* m_titleText{ nullptr };
    Button* m_btnClose{ nullptr };

    // Body elements
    Panel* m_bodyContainer{ nullptr };
    CategoryList* m_categoriesList{ nullptr };
    Panel* m_contentContainer{ nullptr };

    // Footer elements
    Panel* m_footerContainer{ nullptr };
    Button* m_btnSave{ nullptr };
    Button* m_btnCenter{ nullptr };
    Button* m_btnReset{ nullptr };

    // Registered category views
    std::unordered_map<std::string, SettingsView*> m_categoryViews;
    std::string m_activeCategoryId{ "toasts" };

    CloseCallback m_onClose;
    Point m_customPosition{ -1.0f, -1.0f };
    Point m_dragStartMouse{ 0.0f, 0.0f };
    Point m_dragStartPos{ 0.0f, 0.0f };
    bool m_dragging{ false };
};

} // namespace PerfUI
