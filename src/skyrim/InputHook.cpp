#include "InputHook.h"
#include "D3D11Hook.h"
#include "PerfUI/ConfigManager.h"
#include "PerfUI/UIContext.h"
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
        ::SetWindowLongPtrA(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Hooked_WndProc))
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
        ::SetWindowLongPtrA(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_originalWndProc));
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
        ::SetCursor(::LoadCursorA(nullptr, IDC_ARROW));
    } else {
        while (::ShowCursor(FALSE) >= 0);
    }

    SKSE::GetTaskInterface()->AddTask([capture]() {
        auto* controlMap = RE::ControlMap::GetSingleton();
        if (controlMap) {
            using UEFlag = RE::UserEvents::USER_EVENT_FLAG;
            controlMap->ToggleControls(UEFlag::kAll, !capture);
        }
    });
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

    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
        auto* uiCtx = D3D11Hook::GetSingleton().GetContext();
        bool capturingKeybind = uiCtx && uiCtx->isCapturingKeybind();

        if (!capturingKeybind) {
            // First check user registered hotkeys via UIContext
            if (uiCtx && uiCtx->triggerHotkey(static_cast<uint32_t>(wParam))) {
                return 0;
            }

            if (wParam == VK_F10) {
                D3D11Hook::GetSingleton().ToggleMainMenu();
                return 0;
            }

            // F11 / toggleHotkey disabled as requested by user
            (void)wParam;

            if (hook.m_captureInput.load() && wParam == VK_ESCAPE) {
                D3D11Hook::GetSingleton().SetUIVisible(false);
                hook.SetCaptureInput(false);
                return 0;
            }
        }
    }

    if (hook.m_captureInput.load()) {
        // Force Windows hardware arrow cursor
        if (msg == WM_SETCURSOR) {
            if (LOWORD(lParam) == HTCLIENT) {
                while (::ShowCursor(TRUE) < 0);
                ::SetCursor(::LoadCursorA(nullptr, IDC_ARROW));
                return TRUE;
            }
        }

        bool isMouseMsg = (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) || msg == WM_SETCURSOR;
        if (!isMouseMsg) {
            auto* imguiCtx = D3D11Hook::GetSingleton().GetImGuiContext();
            if (imguiCtx) {
                auto* prevCtx = ImGui::GetCurrentContext();
                ImGui::SetCurrentContext(imguiCtx);
                ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
                if (prevCtx && prevCtx != imguiCtx) {
                    ImGui::SetCurrentContext(prevCtx);
                }
            } else {
                ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
            }
        }

        switch (msg) {
        case WM_MOUSEMOVE: {
            float x = static_cast<float>(GET_X_LPARAM(lParam));
            float y = static_cast<float>(GET_Y_LPARAM(lParam));
            hook.m_lastMousePos = { static_cast<LONG>(x), static_cast<LONG>(y) };
            hook.PushInput({ QueuedInput::Type::MouseMove, 0, x, y });

            auto* mc = RE::MenuCursor::GetSingleton();
            if (mc) {
                mc->cursorPosX = x;
                mc->cursorPosY = y;
            }
            return 0;
        }
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_XBUTTONDBLCLK: {
            while (::ShowCursor(TRUE) < 0);
            ::SetCursor(::LoadCursorA(nullptr, IDC_ARROW));
            return 0; // Handled via DirectInput in ProcessEvent
        }
        case WM_MOUSEWHEEL: {
            short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            float delta = static_cast<float>(zDelta) / static_cast<float>(WHEEL_DELTA);
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ::ScreenToClient(hWnd, &pt);
            hook.PushInput({ QueuedInput::Type::MouseWheel, 0, static_cast<float>(pt.x), static_cast<float>(pt.y), delta });
            return 0;
        }
        case WM_CHAR: {
            hook.PushInput({ QueuedInput::Type::Char, 0, 0.0f, 0.0f, 0.0f, static_cast<uint32_t>(wParam), 0 });
            return 0; // Swallow from Skyrim
        }
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: {
            hook.PushInput({ QueuedInput::Type::KeyDown, 0, 0.0f, 0.0f, 0.0f, 0, static_cast<int>(wParam) });
            return 0; // Swallow from Skyrim
        }
        case WM_KEYUP:
        case WM_SYSKEYUP:
        case WM_DEADCHAR:
            return 0; // Swallow from Skyrim
        }
    }

    return ::CallWindowProcA(hook.m_originalWndProc, hWnd, msg, wParam, lParam);
}

RE::BSEventNotifyControl InputHook::ProcessEvent(
    RE::InputEvent* const* a_event,
    RE::BSTEventSource<RE::InputEvent*>*
) {
    if (!m_captureInput.load() || !a_event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    // Walk the linked list of input events from Skyrim's DirectInput polling
    for (auto* event = *a_event; event; event = event->next) {
        // Mouse buttons: device == kMouse, type == kButton
        if (event->GetDevice() == RE::INPUT_DEVICE::kMouse &&
            event->GetEventType() == RE::INPUT_EVENT_TYPE::kButton)
        {
            auto* btnEvent = event->AsButtonEvent();
            if (!btnEvent) continue;

            uint32_t keyCode = btnEvent->GetIDCode();

            // kLeftButton = 0, kRightButton = 1, kMiddleButton = 2
            // kWheelUp = 8, kWheelDown = 9
            static bool s_mouseButtonDown[3] = { false, false, false };

            if (keyCode <= 2) {
                while (::ShowCursor(TRUE) < 0);
                ::SetCursor(::LoadCursorA(nullptr, IDC_ARROW));

                POINT pt;
                if (m_hWnd && ::GetCursorPos(&pt) && ::ScreenToClient(m_hWnd, &pt)) {
                    m_lastMousePos = pt;
                }

                bool isPressed = btnEvent->IsPressed();
                if (isPressed && !s_mouseButtonDown[keyCode]) {
                    s_mouseButtonDown[keyCode] = true;
                    PushInput({ QueuedInput::Type::MouseDown, static_cast<int>(keyCode), static_cast<float>(m_lastMousePos.x), static_cast<float>(m_lastMousePos.y) });
                } else if (!isPressed && s_mouseButtonDown[keyCode]) {
                    s_mouseButtonDown[keyCode] = false;
                    PushInput({ QueuedInput::Type::MouseUp, static_cast<int>(keyCode), static_cast<float>(m_lastMousePos.x), static_cast<float>(m_lastMousePos.y) });
                }
            } else if (keyCode == 8) {
                if (btnEvent->IsDown()) {
                    PushInput({ QueuedInput::Type::MouseWheel, 0, static_cast<float>(m_lastMousePos.x), static_cast<float>(m_lastMousePos.y), 1.0f });
                }
            } else if (keyCode == 9) {
                if (btnEvent->IsDown()) {
                    PushInput({ QueuedInput::Type::MouseWheel, 0, static_cast<float>(m_lastMousePos.x), static_cast<float>(m_lastMousePos.y), -1.0f });
                }
            }
        }

        // DirectInput keyboard events
        if (event->GetDevice() == RE::INPUT_DEVICE::kKeyboard &&
            event->GetEventType() == RE::INPUT_EVENT_TYPE::kButton)
        {
            auto* btnEvent = event->AsButtonEvent();
            if (!btnEvent) continue;

            uint32_t scanCode = btnEvent->GetIDCode();
            if (btnEvent->IsDown()) {
                if (scanCode == 1) { // ESC
                    PushInput({ QueuedInput::Type::KeyDown, 0, 0, 0, 0, 0, VK_ESCAPE });
                } else if (scanCode == 28) { // Enter
                    PushInput({ QueuedInput::Type::KeyDown, 0, 0, 0, 0, 0, VK_RETURN });
                }
            }
        }
    }

    // Consume input completely while modal UI is open so vanilla Skyrim never processes ghost clicks or resets cursor
    return RE::BSEventNotifyControl::kStop;
}

} // namespace PerfUI::Skyrim
