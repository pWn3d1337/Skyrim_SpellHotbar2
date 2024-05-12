#pragma once
#include <rapidcsv.h>
#include "../logger/logger.h"
#include <functional>

namespace SpellHotbar::csv {
    inline bool has_column(const std::vector<std::string>& vec, std::string name) {
        return std::any_of(vec.begin(), vec.end(), [&name](const auto& str) { return str == name; });
    };

    inline void load_folder(std::filesystem::path folder, const std::string_view & type, std::function<void(std::string)> parse_func) {
        try {
            for (const auto& entry : std::filesystem::directory_iterator(folder)) {
                if (entry.is_regular_file()) {
                    std::string str_path = entry.path().string();

                    if (str_path.ends_with(".csv")) {
                        logger::info("Loading {}: {}", type, str_path);
                        parse_func(str_path);
                    }
                }
            }
        } catch (const std::exception& e) {
            std::string msg = e.what();
            logger::error("Error loading {}: {}", type, msg);
        }
    }
}