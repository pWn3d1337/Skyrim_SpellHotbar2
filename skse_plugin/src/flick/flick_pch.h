#pragma once

// Precompiled prelude for every translation unit in the sh2_flick object library.
//
// These TUs do NOT use the plugin's PCH. The split is the boundary between the side that draws
// -- these files, through FUCK:: -- and the side that owns the state, reached through headers
// that carry no drawing (`rendering/ui_bridge.h`, `flick/flick_watch.h`, `flick/flick_windows.h`).
//
// Two rules follow, and both are enforced mechanically rather than by memory:
//
//   1. Inside a FLICK `Draw()`, only `FUCK::` calls. SH2 links its own ImGui with its own GImGui,
//      which is uninitialised on FLICK's render path; one `ImGui::TextEx` there is an access
//      violation on the first frame. The real imgui.h is included here for its types, so a stray
//      `ImGui::` call would compile; CMake greps src for one and fails the configure.
//   2. No FLICK TU includes `rendering/render_manager.h`; it `#error`s when SH2_FLICK_TU is
//      defined. The texture registry answers through ui_bridge.h.
//
// Order is load-bearing: CommonLib's REX/W32/BASE.h hard-errors if <Windows.h> was included ahead
// of it ("Please move any Windows API includes after CommonLib"), so SKSE comes first.
#define SH2_FLICK_TU 1

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <mutex>
#include <numbers>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <imgui.h>

#include "../../third_party/flick/FUCK_API.h"

using namespace std::literals;

namespace logger = SKSE::log;
