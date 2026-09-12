#include "flick_images.h"

namespace SpellHotbar::FlickImages {
    namespace {
        std::unordered_map<std::string, FUCK::Image*>& image_store()
        {
            static auto* store = new std::unordered_map<std::string, FUCK::Image*>();
            return *store;
        }
    }

    FUCK::Image* image_for(const std::string& a_sh2_path)
    {
        auto& store = image_store();
        if (auto it = store.find(a_sh2_path); it != store.end()) {
            return it->second;
        }
        // ".\data\SKSE\...\x.dds" -> "Data/SKSE/.../x.png": FLICK opens paths relative to the
        // game root, and only the PNG sibling.
        std::string p = a_sh2_path;
        if (p.rfind(".\\", 0) == 0 || p.rfind("./", 0) == 0) {
            p.erase(0, 2);
        }
        std::replace(p.begin(), p.end(), '\\', '/');
        if (const auto dot = p.rfind('.'); dot != std::string::npos) {
            p.erase(dot);
        }
        p += ".png";

        auto* img = new FUCK::Image(p.c_str());
        if (!img->IsLoaded()) {
            logger::warn("SH2 FLICK: could not load '{}' (for '{}'); run scripts/dds_to_png.py", p, a_sh2_path);
            delete img;
            img = nullptr;
        }
        store.emplace(a_sh2_path, img);
        return img;
    }

    FUCK::Image* glyph(const char* a_stem)
    {
        return image_for(std::string("Data/SKSE/Plugins/SpellHotbar/images/dock/") + a_stem + ".png");
    }

    ImVec4 unpack(unsigned a_col)
    {
        return ImVec4(((a_col >> 0) & 0xFF) / 255.0f, ((a_col >> 8) & 0xFF) / 255.0f,
                      ((a_col >> 16) & 0xFF) / 255.0f, ((a_col >> 24) & 0xFF) / 255.0f);
    }
}
