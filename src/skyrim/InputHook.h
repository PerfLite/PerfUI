#pragma once

#include "Pch.h"
#include <mutex>
#include <vector>

namespace PerfUI::Skyrim {

class InputHook : public RE::BSTEventSink<RE::InputEvent*> {
public:
    struct QueuedInput {
        enum class Type {
            MouseMove,
            MouseDown,
            MouseUp,
            MouseWheel,
            Char,
            KeyDown
        } type;
        int button{ 0 };
        float x{ 0.0f };
        float y{ 0.0f };
        float wheelDelta{ 0.0f };
        uint32_t charCode{ 0 };
        int keyCode{ 0 };
    };

    static InputHook& GetSingleton();

    bool Install(HWND hWnd);
    void Uninstall();

    void SetCaptureInput(bool capture);
    bool IsCaptureInput() const { return m_captureInput.load(); }

    void PushInput(QueuedInput event);
    std::vector<QueuedInput> DrainInputQueue();

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

    std::mutex m_queueLock;
    std::vector<QueuedInput> m_inputQueue;
    POINT m_lastMousePos{ 0, 0 };

    static constexpr uint32_t kDefaultToggleKey = VK_F11;
};

} // namespace PerfUI::Skyrim
