#include "InputHook.h"
#include "D3D11Hook.h"
#include "PerfUI/ConfigManager.h"
#include "PerfUI/UIContext.h"
#include <windowsx.h>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace PerfUI::Skyrim {

namespace {

using ProcessMessage_t = RE::UI_MESSAGE_RESULTS(RE::CursorMenu*, RE::UIMessage&);
static REL::Relocation<ProcessMessage_t> s_originalCursorProcessMessage;

using AdvanceMovie_t = void(RE::CursorMenu*, float, std::uint32_t);
static REL::Relocation<AdvanceMovie_t> s_originalCursorAdvanceMovie;

static RE::UI_MESSAGE_RESULTS Hooked_CursorMenu_ProcessMessage(RE::CursorMenu* a_this, RE::UIMessage& a_message) {
    if (a_message.type == RE::UI_MESSAGE_TYPE::kHide) {
        if (InputHook::GetSingleton().IsCaptureInput() || D3D11Hook::GetSingleton().HasVisibleRootWindows()) {
            return RE::UI_MESSAGE_RESULTS::kIgnore;
        }
    }
    return s_originalCursorProcessMessage(a_this, a_message);
}

static void Hooked_CursorMenu_AdvanceMovie(RE::CursorMenu* a_this, float a_interval, std::uint32_t a_currentTime) {
    if (a_this && a_this->uiMovie) {
        bool shouldSuppressVanillaCursor = D3D11Hook::GetSingleton().IsCursorNeeded();
        if (shouldSuppressVanillaCursor) {
            if (a_this->uiMovie->GetVisible()) {
                a_this->uiMovie->SetVisible(false);
            }
        } else {
            if (!a_this->uiMovie->GetVisible()) {
                a_this->uiMovie->SetVisible(true);
            }
        }
    }
    s_originalCursorAdvanceMovie(a_this, a_interval, a_currentTime);
}

static void InstallCursorMenuHooks() {
    static std::atomic<bool> s_installed{ false };
    if (s_installed.exchange(true)) return;

    REL::Relocation<std::uintptr_t> vTable(RE::VTABLE_CursorMenu[0]);
    s_originalCursorProcessMessage = vTable.write_vfunc(0x4, &Hooked_CursorMenu_ProcessMessage);
    s_originalCursorAdvanceMovie = vTable.write_vfunc(0x5, &Hooked_CursorMenu_AdvanceMovie);
    SKSE::log::info("InputHook: Installed CursorMenu ProcessMessage (0x4) and AdvanceMovie (0x5) hooks");
}

} // namespace

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

    EnsureInputSink();
    InstallCursorMenuHooks();

    m_installed.store(true);
    SKSE::log::info("InputHook installed successfully");
    return true;
}

void InputHook::EnsureInputSink() {
    auto* inputManager = RE::BSInputDeviceManager::GetSingleton();
    if (!inputManager) return;

    if (!m_sinkRegistered.load()) {
        inputManager->AddEventSink(this);
        m_sinkRegistered.store(true);
        SKSE::log::info("InputHook: Registered BSInputDeviceManager event sink");
    }

    // Promote to the very front of the sink array so InputHook intercepts events
    // BEFORE PlayerControls / LookHandler / MovementHandler
    RE::BSSpinLockGuard locker(inputManager->lock);
    auto it = std::find(inputManager->sinks.begin(), inputManager->sinks.end(),
                        static_cast<RE::BSTEventSink<RE::InputEvent*>*>(this));
    if (it != inputManager->sinks.end() && it != inputManager->sinks.begin()) {
        std::rotate(inputManager->sinks.begin(), it, it + 1);
        SKSE::log::info("InputHook: Promoted to front of input sink list (index 0)");
    }
}

void InputHook::Uninstall() {
    if (!m_installed.load()) return;

    SetCaptureInput(false);

    if (m_hWnd && m_originalWndProc) {
        ::SetWindowLongPtrA(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_originalWndProc));
        m_originalWndProc = nullptr;
    }

    if (m_sinkRegistered.load()) {
        auto* inputManager = RE::BSInputDeviceManager::GetSingleton();
        if (inputManager) {
            inputManager->RemoveEventSink(this);
        }
        m_sinkRegistered.store(false);
    }

    m_installed.store(false);
}

void InputHook::SetCaptureInput(bool capture) {
    bool wasCapturing = m_captureInput.exchange(capture);
    if (capture) {
        EnsureInputSink();
    } else {
        m_mouseButtonDown[0] = false;
        m_mouseButtonDown[1] = false;
        m_mouseButtonDown[2] = false;
    }

    if (capture != wasCapturing) {
        auto updateSkyrimCursor = [capture] {
            auto* mc = RE::MenuCursor::GetSingleton();
            auto* ui = RE::UI::GetSingleton();
            auto* queue = RE::UIMessageQueue::GetSingleton();

            if (capture) {
                if (mc && mc->showCursorCount <= 0) {
                    mc->SetCursorVisibility(true);
                }
                if (ui && queue && !ui->IsMenuOpen(RE::CursorMenu::MENU_NAME)) {
                    queue->AddMessage(RE::CursorMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);
                }
            } else {
                if (!D3D11Hook::GetSingleton().IsCursorNeeded()) {
                    if (mc && mc->showCursorCount > 0) {
                        mc->SetCursorVisibility(false);
                    }
                    if (ui && queue && ui->IsMenuOpen(RE::CursorMenu::MENU_NAME)) {
                        queue->AddMessage(RE::CursorMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
                    }
                }
            }
        };
        if (auto* tasks = SKSE::GetTaskInterface()) {
            tasks->AddTask(updateSkyrimCursor);
        } else {
            updateSkyrimCursor();
        }
    }

    while (::ShowCursor(FALSE) >= 0);
    ::SetCursor(nullptr);
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

    // Always keep Windows hardware cursor hidden - Skyrim renders its own native game cursor
    if (msg == WM_SETCURSOR && LOWORD(lParam) == HTCLIENT) {
        while (::ShowCursor(FALSE) >= 0);
        ::SetCursor(nullptr);
        return TRUE;
    }

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
        }
    }

    if (hook.m_captureInput.load()) {
        hook.EnsureInputSink();

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
            POINT pt;
            if (!::GetCursorPos(&pt) || !::ScreenToClient(hWnd, &pt)) {
                pt.x = GET_X_LPARAM(lParam);
                pt.y = GET_Y_LPARAM(lParam);
            }
            hook.m_lastMousePos = pt;
            hook.PushInput({ QueuedInput::Type::MouseMove, 0, static_cast<float>(pt.x), static_cast<float>(pt.y) });

            auto* mc = RE::MenuCursor::GetSingleton();
            if (mc) {
                mc->cursorPosX = static_cast<float>(pt.x);
                mc->cursorPosY = static_cast<float>(pt.y);
            }
            return 0;
        }
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK: {
            int btn = (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) ? 0 :
                      ((msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) ? 1 : 2);

            POINT pt;
            if (!::GetCursorPos(&pt) || !::ScreenToClient(hWnd, &pt)) {
                pt.x = GET_X_LPARAM(lParam);
                pt.y = GET_Y_LPARAM(lParam);
            }
            hook.m_lastMousePos = pt;

            auto* mc = RE::MenuCursor::GetSingleton();
            if (mc) {
                mc->cursorPosX = static_cast<float>(pt.x);
                mc->cursorPosY = static_cast<float>(pt.y);
            }

            if (!hook.m_mouseButtonDown[btn].exchange(true)) {
                hook.PushInput({ QueuedInput::Type::MouseDown, btn, static_cast<float>(pt.x), static_cast<float>(pt.y) });
            }
            return 0;
        }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP: {
            int btn = (msg == WM_LBUTTONUP) ? 0 : (msg == WM_RBUTTONUP ? 1 : 2);

            POINT pt;
            if (!::GetCursorPos(&pt) || !::ScreenToClient(hWnd, &pt)) {
                pt.x = GET_X_LPARAM(lParam);
                pt.y = GET_Y_LPARAM(lParam);
            }
            hook.m_lastMousePos = pt;

            auto* mc = RE::MenuCursor::GetSingleton();
            if (mc) {
                mc->cursorPosX = static_cast<float>(pt.x);
                mc->cursorPosY = static_cast<float>(pt.y);
            }

            if (hook.m_mouseButtonDown[btn].exchange(false)) {
                hook.PushInput({ QueuedInput::Type::MouseUp, btn, static_cast<float>(pt.x), static_cast<float>(pt.y) });
            }
            return 0;
        }
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_XBUTTONDBLCLK: {
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
        case WM_CHAR: {
            hook.PushInput({ QueuedInput::Type::Char, 0, 0.0f, 0.0f, 0.0f, static_cast<uint32_t>(wParam), 0 });
            return 0; // Swallow from Skyrim
        }
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: {
            hook.PushInput({ QueuedInput::Type::KeyDown, 0, 0.0f, 0.0f, 0.0f, 0, static_cast<int>(wParam) });
            if (!hook.m_captureKeyboard.load()) {
                break; // In overlay mode, allow keys to pass to Skyrim!
            }
            return 0; // Swallow from Skyrim
        }
        case WM_KEYUP:
        case WM_SYSKEYUP:
        case WM_DEADCHAR:
            if (!hook.m_captureKeyboard.load()) {
                break; // In overlay mode, allow keys to pass to Skyrim!
            }
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
            if (keyCode <= 2) {
                POINT pt;
                if (m_hWnd && ::GetCursorPos(&pt) && ::ScreenToClient(m_hWnd, &pt)) {
                    m_lastMousePos = pt;
                }

                auto* mc = RE::MenuCursor::GetSingleton();
                if (mc) {
                    mc->cursorPosX = static_cast<float>(m_lastMousePos.x);
                    mc->cursorPosY = static_cast<float>(m_lastMousePos.y);
                }

                bool isPressed = btnEvent->IsPressed();
                if (isPressed) {
                    if (!m_mouseButtonDown[keyCode].exchange(true)) {
                        PushInput({ QueuedInput::Type::MouseDown, static_cast<int>(keyCode), static_cast<float>(m_lastMousePos.x), static_cast<float>(m_lastMousePos.y) });
                    }
                } else {
                    if (m_mouseButtonDown[keyCode].exchange(false)) {
                        PushInput({ QueuedInput::Type::MouseUp, static_cast<int>(keyCode), static_cast<float>(m_lastMousePos.x), static_cast<float>(m_lastMousePos.y) });
                    }
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

        // Keyboard keys: device == kKeyboard, type == kButton
        if (event->GetDevice() == RE::INPUT_DEVICE::kKeyboard &&
            event->GetEventType() == RE::INPUT_EVENT_TYPE::kButton)
        {
            auto* btnEvent = event->AsButtonEvent();
            if (btnEvent && btnEvent->IsDown()) {
                uint32_t scanCode = btnEvent->GetIDCode();
                int vk = 0;
                switch (scanCode) {
                case 0xC8: vk = VK_UP; break;
                case 0xD0: vk = VK_DOWN; break;
                case 0xCB: vk = VK_LEFT; break;
                case 0xCD: vk = VK_RIGHT; break;
                case 0x1C: vk = VK_RETURN; break;
                case 0x9C: vk = VK_RETURN; break; // Numpad Enter
                case 0x39: vk = VK_SPACE; break;
                case 0x01: vk = VK_ESCAPE; break;
                case 0x0F: vk = VK_TAB; break;
                case 0x12: vk = 'E'; break;
                case 0x11: vk = 'W'; break;
                case 0x1F: vk = 'S'; break;
                case 0x1E: vk = 'A'; break;
                case 0x20: vk = 'D'; break;
                default:
                    vk = static_cast<int>(::MapVirtualKeyA(scanCode & 0xFF, MAPVK_VSC_TO_VK));
                    break;
                }
                if (vk > 0) {
                    PushInput({ QueuedInput::Type::KeyDown, 0, 0.0f, 0.0f, 0.0f, 0, vk });
                }
            }
        }
    }

    if (m_captureKeyboard.load()) {
        // Modal: clear user events across the linked list so even if any downstream sink
        // accidentally receives the event, no gameplay action triggers in the world.
        for (auto* event = *a_event; event; event = event->next) {
            if (auto* idEvent = event->AsIDEvent()) {
                idEvent->userEvent = RE::BSFixedString{};
            }
        }
        return RE::BSEventNotifyControl::kStop;
    }

    // Overlay mode: the list is shared by every device and a sink cannot drop one event without
    // dropping the whole list, so non-keyboard events are disarmed in place instead. An empty
    // user event maps to no handler in the game, while real keys keep flowing to the map, the
    // pause menu and everything else the player expects to work under a corner overlay.
    for (auto* event = *a_event; event; event = event->next) {
        if (event->GetDevice() == RE::INPUT_DEVICE::kKeyboard) continue;

        if (auto* idEvent = event->AsIDEvent()) {
            idEvent->userEvent = RE::BSFixedString{};
        }
    }

    return RE::BSEventNotifyControl::kContinue;
}

} // namespace PerfUI::Skyrim
