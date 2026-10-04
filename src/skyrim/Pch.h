#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <atomic>
#include <chrono>

#include "PerfUI/PerfUI.h"
