#include "InputHook.h"
#include "D3D11Hook.h"
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
    // Show cursor when UI is captured
    ::ShowCursor(capture);
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

        auto* uiContext = D3D11Hook::GetSingleton().GetContext();
        if (uiContext) {
            switch (msg) {
            case WM_MOUSEMOVE: {
                float x = static_cast<float>(LOWORD(lParam));
                float y = static_cast<float>(HIWORD(lParam));
                uiContext->onMouseMove({ x, y });
                return 0; // Swallow from Skyrim camera
            }
            case WM_LBUTTONDOWN: {
                float x = static_cast<float>(LOWORD(lParam));
                float y = static_cast<float>(HIWORD(lParam));
                uiContext->onMouseDown(0, { x, y });
                return 0; // Swallow
            }
            case WM_LBUTTONUP: {
                float x = static_cast<float>(LOWORD(lParam));
                float y = static_cast<float>(HIWORD(lParam));
                uiContext->onMouseUp(0, { x, y });
                return 0; // Swallow
            }
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEWHEEL:
                return 0; // Swallow all mouse events while UI is active
            }
        }
    }

    return ::CallWindowProcW(hook.m_originalWndProc, hWnd, msg, wParam, lParam);
}

RE::BSEventNotifyControl InputHook::ProcessEvent(
    RE::InputEvent* const* a_event,
    RE::BSTEventSource<RE::InputEvent*>*
) {
    if (!a_event || !m_captureInput.load()) {
        return RE::BSEventNotifyControl::kContinue;
    }

    // While UI is open, block game controls (jumping, shouting, attacking)
    return RE::BSEventNotifyControl::kStop;
}

} // namespace PerfUI::Skyrim
