#pragma once
// https://github.com/SkyrimScripting/SKSE_Template_Logging/tree/main

#include <spdlog/sinks/basic_file_sink.h>
namespace logger = SKSE::log;

namespace SpellHotbar {
    void SetupLogger();
}
