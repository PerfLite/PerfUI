#include "ModuleLog.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <fstream>

namespace PerfUI::detail {

void AppendLogNextToModule(const void* moduleAddress, const char* fileName, const std::string& line) {
#if defined(_WIN32)
    OutputDebugStringA(line.c_str());

    HMODULE module = nullptr;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            static_cast<LPCSTR>(moduleAddress), &module) ||
        !module) {
        return;
    }

    char modulePath[MAX_PATH]{};
    if (GetModuleFileNameA(module, modulePath, MAX_PATH) == 0) {
        return;
    }

    std::string logPath(modulePath);
    const auto slash = logPath.find_last_of("\\/");
    if (slash != std::string::npos) {
        logPath.resize(slash + 1);
    }
    logPath += fileName;

    std::ofstream file(logPath, std::ios::app);
    if (file) {
        file << line;
    }
#else
    (void)moduleAddress;
    (void)fileName;
    (void)line;
#endif
}

} // namespace PerfUI::detail
