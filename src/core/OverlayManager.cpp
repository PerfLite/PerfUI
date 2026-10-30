#include "PerfUI/OverlayManager.h"
#include "PerfUI/Diagnostics.h"
#include "ModuleLog.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace PerfUI {
namespace {

struct FaultInfo {
    unsigned long code{ 0 };
    void* address{ nullptr };
};

#if defined(_WIN32)
// MSVC forbids __try in a function that needs C++ object unwinding (C2712),
// so the guarded call is isolated in this object-free frame. Access violations
// raised by overlay code reach the __except handler; C++ exceptions are caught
// one layer up, inside InvokeOverlayCallback.
int RecordFault(_EXCEPTION_POINTERS* info, FaultInfo* fault) {
    if (info && info->ExceptionRecord) {
        fault->code = info->ExceptionRecord->ExceptionCode;
        fault->address = info->ExceptionRecord->ExceptionAddress;
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

bool InvokeSehGuarded(void (*fn)(void*), void* state, FaultInfo* fault) {
    __try {
        fn(state);
        return true;
    } __except (RecordFault(GetExceptionInformation(), fault)) {
        return false;
    }
}
#else
bool InvokeSehGuarded(void (*fn)(void*), void* state, FaultInfo* fault) {
    (void)fault;
    fn(state);
    return true;
}
#endif

struct OverlayInvocation {
    const OverlayCallback* callback{ nullptr };
    OverlayContext* context{ nullptr };
    bool completed{ false };
    std::string error;
};

void InvokeOverlayCallback(void* state) {
    auto* invocation = static_cast<OverlayInvocation*>(state);
    try {
        (*invocation->callback)(*invocation->context);
        invocation->completed = true;
    } catch (const std::exception& ex) {
        invocation->error = ex.what();
    } catch (...) {
        invocation->error = "unknown C++ exception";
    }
}

std::string DescribeFault(const FaultInfo& fault) {
    char buffer[320]{};
    const char* moduleName = nullptr;
    size_t offset = 0;
#if defined(_WIN32)
    HMODULE module = nullptr;
    const auto address = reinterpret_cast<LPCSTR>(fault.address);
    if (address &&
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           address, &module) &&
        module) {
        char modulePath[MAX_PATH]{};
        if (GetModuleFileNameA(module, modulePath, MAX_PATH) > 0) {
            const char* slash = std::strrchr(modulePath, '\\');
            moduleName = slash ? slash + 1 : modulePath;
            offset = reinterpret_cast<uintptr_t>(fault.address) - reinterpret_cast<uintptr_t>(module);
        }
    }
#endif
    if (moduleName) {
        std::snprintf(buffer, sizeof(buffer), "native fault 0x%08lX at %s+0x%zX", fault.code, moduleName, offset);
    } else {
        std::snprintf(buffer, sizeof(buffer), "native fault 0x%08lX at %p", fault.code, fault.address);
    }
    return buffer;
}

// Best-effort fault report: hardware faults in mod code are otherwise
// invisible without a debugger.
void LogOverlayFault(const std::string& overlayName, const std::string& message) {
    const std::string line = "[PerfUI] overlay '" + overlayName + "' disabled: " + message + "\n";
    detail::AppendLogNextToModule(reinterpret_cast<const void*>(&LogOverlayFault), "PerfUI_faults.log", line);
}

} // namespace

OverlayId OverlayManager::registerOverlay(const char* name, OverlayCallback cb, int zOrder, bool alwaysVisible) {
    if (!cb) return 0;

    std::lock_guard<std::mutex> lock(m_mutex);
    OverlayId id = m_nextId++;
    OverlayEntry entry;
    entry.id = id;
    entry.name = name ? name : "UnnamedOverlay";
    entry.callback = std::move(cb);
    entry.zOrder = zOrder;
    entry.visible = true;
    entry.alwaysVisible = alwaysVisible;
    entry.enabled = true;

    m_overlays.push_back(std::move(entry));
    sortOverlaysLocked();
    return id;
}

void OverlayManager::unregisterOverlay(OverlayId id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_overlays.erase(
        std::remove_if(m_overlays.begin(), m_overlays.end(), [id](const OverlayEntry& e) {
            return e.id == id;
        }),
        m_overlays.end()
    );
}

void OverlayManager::setOverlayVisible(OverlayId id, bool visible) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& e : m_overlays) {
        if (e.id == id) {
            e.visible = visible;
            break;
        }
    }
}

bool OverlayManager::isOverlayVisible(OverlayId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& e : m_overlays) {
        if (e.id == id) {
            return e.visible;
        }
    }
    return false;
}

bool OverlayManager::isOverlayEnabled(OverlayId id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& e : m_overlays) {
        if (e.id == id) {
            return e.enabled;
        }
    }
    return false;
}

void OverlayManager::setOverlayEnabled(OverlayId id, bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& e : m_overlays) {
        if (e.id == id) {
            e.enabled = enabled;
            break;
        }
    }
}

bool OverlayManager::hasVisibleOverlays() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& e : m_overlays) {
        if (e.enabled && e.visible) {
            return true;
        }
    }
    return false;
}

void OverlayManager::renderOverlays(OverlayContext& ctx, bool isUIVisible) {
    std::vector<OverlayEntry> copy;
    ErrorCallback errorCallback;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        copy = m_overlays;
        errorCallback = m_errorCallback;
    }

    Diagnostics::GetSingleton().beginOverlayFrame();

    for (const auto& e : copy) {
        if (!e.enabled || !e.visible) continue;
        if (!e.alwaysVisible && !isUIVisible) continue;

        const auto overlayStart = std::chrono::steady_clock::now();
        OverlayInvocation invocation{ &e.callback, &ctx };
        FaultInfo fault;
        const bool survived = InvokeSehGuarded(&InvokeOverlayCallback, &invocation, &fault);
        Diagnostics::GetSingleton().recordOverlayTime(
            e.name.c_str(),
            std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - overlayStart).count());

        std::string error;
        if (!survived) {
            error = DescribeFault(fault);
        } else if (!invocation.completed) {
            error = invocation.error.empty() ? "unknown C++ exception" : invocation.error;
        }
        if (error.empty()) continue;

        setOverlayEnabled(e.id, false);
        LogOverlayFault(e.name, error);
        if (errorCallback) {
            errorCallback(e.name, error);
        }
    }

    Diagnostics::GetSingleton().endOverlayFrame();
}

void OverlayManager::setErrorCallback(ErrorCallback cb) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_errorCallback = std::move(cb);
}

size_t OverlayManager::overlayCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_overlays.size();
}

void OverlayManager::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_overlays.clear();
}

void OverlayManager::sortOverlaysLocked() {
    std::stable_sort(m_overlays.begin(), m_overlays.end(), [](const OverlayEntry& a, const OverlayEntry& b) {
        return a.zOrder < b.zOrder;
    });
}

} // namespace PerfUI
