#pragma once

#include "Pch.h"

namespace PerfUI::Skyrim {

class InputHook : public RE::BSTEventSink<RE::InputEvent*> {
public:
    static InputHook& GetSingleton();

    bool Install(HWND hWnd);
    void Uninstall();

    void SetCaptureInput(bool capture);
    bool IsCaptureInput() const { return m_captureInput.load(); }

    RE::BSEventNotifyControl ProcessEvent(
        RE::InputEvent* const* a_event,
        RE::BSTEventSource<RE::InputEvent*>* a_eventSource
    ) override;

private:
    InputHook() = default;
    ~InputHook() = default;

    static LRESULT CALLBACK Hooked_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND m_hWnd{ nullptr };
    WNDPROC m_originalWndProc{ nullptr };
    std::atomic<bool> m_installed{ false };
    std::atomic<bool> m_captureInput{ false };

    // F11 = 0x7A, Escape = 0x1B
    static constexpr uint32_t kDefaultToggleKey = VK_F11;
};

} // namespace PerfUI::Skyrim
