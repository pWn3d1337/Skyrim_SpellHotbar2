#pragma once

#include <string>
#include <string_view>

namespace SpellHotbar::Mcp {
    inline bool valid_preset_name(std::string_view name)
    {
        if (name.empty() || name.size() > 48) {
            return false;
        }
        if (name == "<cancel>" || name.starts_with("<")) {
            return false;
        }
        if (name.find("..") != std::string_view::npos) {
            return false;
        }
        return name.find_first_of("\\/:*?\"<>|") == std::string_view::npos;
    }

    inline bool is_listed_preset(std::string_view name)
    {
        return !name.empty() && !name.starts_with("<");
    }

    inline std::string with_json_extension(std::string name)
    {
        if (!name.ends_with(".json")) {
            name += ".json";
        }
        return name;
    }
}
