#pragma once

// This file is required.

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

// After CommonLib: REX/W32 refuses a Windows.h that came first. FUCK_API.h's inline
// Connect() needs GetModuleHandleW and GetProcAddress.
#include <Windows.h>

#include <atomic>
#include <filesystem>

using namespace std::literals;

// Dear ImGui's types only -- ImVec2, ImU32, IM_COL32 -- for the texture registry and the packed
// colours the display lists carry. Nothing on this side of the plugin draws: FLICK does, in its
// own context, from the FUCK:: calls under src/flick. An ImGui:: call anywhere in the plugin
// would compile against the statically linked copy and read its uninitialised context; CMake
// greps the whole tree for one and fails the configure.
#include <imgui.h>
