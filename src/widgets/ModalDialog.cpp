#include "PerfUI/ModalDialog.h"
#include "PerfUI/UIContext.h"
#include "PerfUI/UIRenderBackend.h"

namespace PerfUI {

ModalDialog::ModalDialog(
    std::string title,
    std::string message,
    std::string confirmText,
    std::string cancelText
)
    : Panel("ModalDialog")
    , m_title(std::move(title))
    , m_message(std::move(message))
    , m_confirmText(std::move(confirmText))
    , m_cancelText(std::move(cancelText))
{
    setFocusable(true);
    backgroundColor(Color::Transparent());
    borderWidth(0.0f);

    // Full viewport centering layout
    layout().direction(LayoutDirection::Vertical)
            .alignment(Alignment::Center)
            .justify(JustifyContent::Center);

    // Center card
    m_card = add<Panel>("DialogCard");
    m_card->backgroundColor(Color(18, 23, 31, 250));
    m_card->borderColor(Color::NordicGold());
    m_card->borderWidth(1.5f);
    m_card->cornerRadius(10.0f);
    m_card->shadow(true, Color(0, 0, 0, 220), 24.0f, { 0.0f, 8.0f });
    m_card->layout()
          .width(440.0f)
          .direction(LayoutDirection::Vertical)
          .padding(20.0f)
          .gap(12.0f);

    // Title
    m_titleText = m_card->add<Text>(m_title);
    m_titleText->color(Color::TextAccent()).fontSize(16.0f).bold(true);

    // Divider
    auto* divider = m_card->add<Panel>("Divider");
    divider->backgroundColor(Color::BorderSubtle()).borderWidth(0.0f);
    divider->layout().height(1.0f).width(DimensionConstraint::Flex(1.0f));

    // Message
    m_messageText = m_card->add<Text>(m_message);
    m_messageText->wrap(true);
    m_messageText->color(Color(215, 220, 230, 255)).fontSize(13.0f);

    // Button Row
    auto* btnRow = m_card->add<Panel>("ButtonRow");
    btnRow->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    btnRow->layout()
           .direction(LayoutDirection::Horizontal)
           .justify(JustifyContent::End)
           .gap(12.0f)
           .margin(0.0f, 8.0f, 0.0f, 0.0f);

    m_cancelBtn = btnRow->add<Button>(m_cancelText);
    m_cancelBtn->layout().padding(16.0f, 8.0f);
    m_cancelBtn->fontSize(12.0f);
    m_cancelBtn->onClick([this]() {
        if (m_onCancel) m_onCancel();
        close();
    });

    m_confirmBtn = btnRow->add<Button>(m_confirmText);
    m_confirmBtn->layout().padding(16.0f, 8.0f);
    m_confirmBtn->fontSize(12.0f);
    m_confirmBtn->normalColor(Color(45, 55, 75, 230), Color::NordicGold(), Color::White());
    m_confirmBtn->hoverColor(Color(65, 80, 110, 255), Color::NordicGold(), Color::White());
    m_confirmBtn->onClick([this]() {
        if (m_onConfirm) m_onConfirm();
        close();
    });

    open();
}

void ModalDialog::open() {
    m_isOpen = true;
    m_scrimAlpha.setTarget(1.0f);
}

void ModalDialog::close() {
    if (!m_isOpen) return;
    m_isOpen = false;
    m_scrimAlpha.setTarget(0.0f);
}

void ModalDialog::update(float deltaTime) {
    m_scrimAlpha.update(deltaTime);
    Panel::update(deltaTime);
}

void ModalDialog::measure(Dimensions availableSize) {
    if (m_card) {
        float cardW = (std::min)(availableSize.width * 0.90f, 480.0f);
        cardW = (std::max)(cardW, 280.0f);
        m_card->layout().width(cardW);
        m_card->measure(availableSize);
    }
    setDesiredSize(availableSize);
}

void ModalDialog::arrange(const Rect& finalRect) {
    setBounds(finalRect);
    clearLayoutDirty();

    if (m_card) {
        float cardW = (std::min)(finalRect.width * 0.90f, 480.0f);
        cardW = (std::max)(cardW, 280.0f);
        m_card->layout().width(cardW);
        m_card->measure(Dimensions{ cardW, finalRect.height });

        float cardH = m_card->desiredSize().height;
        float cardX = finalRect.x + (std::max)(0.0f, (finalRect.width - cardW) * 0.5f);
        float cardY = finalRect.y + (std::max)(0.0f, (finalRect.height - cardH) * 0.5f);

        m_card->arrange(Rect{ cardX, cardY, cardW, cardH });
    }
}

void ModalDialog::render(UIRenderBackend& backend) {
    float a = m_scrimAlpha.value();
    if (a <= 0.01f && !m_isOpen) return;

    // Draw full-screen scrim
    backend.drawRect(m_bounds, Color(0, 0, 0, static_cast<uint8_t>(175.0f * a)));

    // Render card
    Panel::render(backend);
}

bool ModalDialog::onPointerDown(const Point& localPoint) {
    // If clicked outside card, close as cancel
    if (m_card && !m_card->bounds().contains(localPoint)) {
        if (m_onCancel) m_onCancel();
        close();
        return true;
    }
    return Panel::onPointerDown(localPoint);
}

bool ModalDialog::onKeyDown(int keyCode) {
    if (keyCode == 0x1B) { // VK_ESCAPE
        if (m_onCancel) m_onCancel();
        close();
        return true;
    }
    if (keyCode == 0x0D) { // VK_RETURN
        if (m_onConfirm) m_onConfirm();
        close();
        return true;
    }
    return false;
}

} // namespace PerfUI
