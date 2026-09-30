#include "InputHook.h"
#include "D3D11Hook.h"
#include <windowsx.h>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace PerfUI::Skyrim {

InputHook& InputHook::GetSingleton() {
    static InputHook s_instance;
    return s_instance;
}

bool InputHook::Install(HWND hWnd) {
    if (m_installed.load() || !hWnd) return false;

    m_hWnd = hWnd;
    m_originalWndProc = reinterpret_cast<WNDPROC>(
        ::SetWindowLongPtrW(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Hooked_WndProc))
    );

    auto* inputManager = RE::BSInputDeviceManager::GetSingleton();
    if (inputManager) {
        inputManager->AddEventSink(this);
    }

    m_installed.store(true);
    SKSE::log::info("InputHook installed successfully");
    return true;
}

void InputHook::Uninstall() {
    if (!m_installed.load()) return;

    SetCaptureInput(false);

    if (m_hWnd && m_originalWndProc) {
        ::SetWindowLongPtrW(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_originalWndProc));
        m_originalWndProc = nullptr;
    }

    auto* inputManager = RE::BSInputDeviceManager::GetSingleton();
    if (inputManager) {
        inputManager->RemoveEventSink(this);
    }

    m_installed.store(false);
}

void InputHook::SetCaptureInput(bool capture) {
    m_captureInput.store(capture);

    // Properly adjust Windows cursor display count
    if (capture) {
        while (::ShowCursor(TRUE) < 0);
    } else {
        while (::ShowCursor(FALSE) >= 0);
    }
}

void InputHook::PushInput(QueuedInput event) {
    std::lock_guard<std::mutex> lock(m_queueLock);
    m_inputQueue.push_back(event);
}

std::vector<InputHook::QueuedInput> InputHook::DrainInputQueue() {
    std::lock_guard<std::mutex> lock(m_queueLock);
    std::vector<QueuedInput> result;
    result.swap(m_inputQueue);
    return result;
}

LRESULT CALLBACK InputHook::Hooked_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto& hook = InputHook::GetSingleton();

    if (msg == WM_KEYDOWN) {
        if (wParam == kDefaultToggleKey) {
            D3D11Hook::GetSingleton().ToggleUI();
            return 0;
        }

        if (hook.m_captureInput.load() && wParam == VK_ESCAPE) {
            D3D11Hook::GetSingleton().SetUIVisible(false);
            return 0;
        }
    }

    if (hook.m_captureInput.load()) {
        ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

        switch (msg) {
        case WM_MOUSEMOVE: {
            float x = static_cast<float>(GET_X_LPARAM(lParam));
            float y = static_cast<float>(GET_Y_LPARAM(lParam));
            hook.PushInput({ QueuedInput::Type::MouseMove, 0, x, y });
            return 0;
        }
        case WM_LBUTTONDOWN: {
            float x = static_cast<float>(GET_X_LPARAM(lParam));
            float y = static_cast<float>(GET_Y_LPARAM(lParam));
            hook.PushInput({ QueuedInput::Type::MouseDown, 0, x, y });
            return 0;
        }
        case WM_LBUTTONUP: {
            float x = static_cast<float>(GET_X_LPARAM(lParam));
            float y = static_cast<float>(GET_Y_LPARAM(lParam));
            hook.PushInput({ QueuedInput::Type::MouseUp, 0, x, y });
            return 0;
        }
        case WM_MOUSEWHEEL: {
            short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            float delta = static_cast<float>(zDelta) / static_cast<float>(WHEEL_DELTA);
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ::ScreenToClient(hWnd, &pt);
            hook.PushInput({ QueuedInput::Type::MouseWheel, 0, static_cast<float>(pt.x), static_cast<float>(pt.y), delta });
            return 0;
        }
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            return 0; // Swallow from Skyrim
        }
    }

    return ::CallWindowProcW(hook.m_originalWndProc, hWnd, msg, wParam, lParam);
}

RE::BSEventNotifyControl InputHook::ProcessEvent(
    RE::InputEvent* const* a_event,
    RE::BSTEventSource<RE::InputEvent*>*
) {
    (void)a_event;
    // Always return kContinue to avoid breaking Skyrim's internal input event pipeline
    return RE::BSEventNotifyControl::kContinue;
}

} // namespace PerfUI::Skyrim
