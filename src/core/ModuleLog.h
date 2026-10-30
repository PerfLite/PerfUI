#pragma once

#include <string>

namespace PerfUI::detail {

// Appends one line to a log file placed next to the module that owns
// moduleAddress, and mirrors it to the debugger output. Best effort: any
// failure (no module, unwritable folder) is silently ignored.
void AppendLogNextToModule(const void* moduleAddress, const char* fileName, const std::string& line);

} // namespace PerfUI::detail
