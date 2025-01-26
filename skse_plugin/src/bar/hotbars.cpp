#include "hotbars.h"
#include "../game_data/game_data.h"
#include "../logger/logger.h"
#include "../input/input.h"
#include "../input/keybinds.h"
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/error/en.h>

namespace SpellHotbar::Bars {

    namespace rj = rapidjson;

    std::unordered_map<uint32_t, Hotbar> hotbars;
    std::vector<uint32_t> bar_cycle; //used for rotating bars in menu
    bool disable_non_modifier_bar;
    uint8_t barsize;

    float slot_scale;
    float slot_spacing; 
    float offset_x;
    float offset_y;

    float oblivion_slot_scale;
    float oblivion_slot_spacing;
    float oblivion_offset_x;
    float oblivion_offset_y;
    anchor_point oblivion_bar_anchor_point = anchor_point::LEFT;
    bool oblivion_bar_show_power = true;
    bar_show_mode oblivion_bar_show_setting;
    float oblivion_bar_held_show_time_threshold = 0.25f;
    bool oblivion_bar_vertical = false;

    text_show_mode text_show_setting;
    bar_show_mode bar_show_setting;
    bar_show_mode bar_show_setting_vampire_lord;
    bar_show_mode bar_show_setting_werewolf;
    bool use_default_bar_when_sheathed;
    bool disable_menu_rendering;
    bool disable_menu_binding;
    anchor_point bar_anchor_point = anchor_point::BOTTOM;

    uint8_t bar_row_len = 12U;
    bar_layout layout = bar_layout::BARS;
    float bar_circle_radius = 2.2f;
    float bar_cross_distance = 0.0f;

    bool _use_keybind_icons{ true };

    //non-persistent
    uint32_t menu_bar_id;
    float oblivion_bar_press_show_timer = 0.0f;

    void add_bars(uint32_t name, std::optional<uint32_t> parent = std::nullopt)
    {
        hotbars.insert({name, Hotbar(bar_names.at(name), Bars::barsize)});
        hotbars.insert({name+1, Hotbar(bar_names.at(name+1), Bars::barsize)});
        
        if (parent.has_value()) {
            uint32_t p = parent.value();
            if (hotbars.contains(p)) {
                hotbars.at(name).set_parent(p);
            }
            //auto parent_bar = std::make_shared<Hotbar>(hotbars.at(p));
            //hotbars.at(name).set_parent(parent_bar);

            //auto parent_bar_sneak = std::make_shared<Hotbar>(hotbars.at(p+1));
            hotbars.at(name+1).set_parent(p+1);
        } else {
            //no parent, main bar will be parent of sneak bar
            auto main_bar = std::make_shared<Hotbar>(hotbars.at(name));
            main_bar->set_enabled(true); //Main bar must be enabled
            hotbars.at(name + 1).set_parent(name);
        }

        bar_cycle.push_back(name);
        bar_cycle.push_back(name + 1);
    }

    void add_special_bar(uint32_t name, std::optional<uint32_t> parent) {
        hotbars.insert({name, Hotbar(bar_names.at(name), Bars::barsize)});

        if (parent.has_value()) {
            uint32_t p = parent.value();
            if (hotbars.contains(p)) {
                hotbars.at(name).set_parent(p);
            }
        } 
    }

    int get_num_rows(int bar_size, int row_len)
    {
        return static_cast<int>(std::ceil(static_cast<float>(bar_size) / static_cast<float>(row_len)));
    }

    std::vector<std::pair<uint32_t, std::string>> get_list_of_bars()
    {
        std::vector<std::pair<uint32_t, std::string>> ret;
        for (uint32_t id : bar_cycle) {
            const auto & bar = hotbars.at(id);
            if (bar.is_enabled()) {
                ret.push_back(std::make_pair(id, bar.get_name()));
            }
        }
        return ret;
    }

    bool use_keybind_icons()
    {
        return _use_keybind_icons && GameData::key_icons_available;
    }

    bool toggle_use_keyind_icons()
    {
        if (GameData::key_icons_available) {
            return _use_keybind_icons = !_use_keybind_icons;
        }
        else
        {
            return false;
        }
    }

    bool get_use_keybind_icons()
    {
        return _use_keybind_icons;
    }

    void set_use_keybind_icons(bool value)
    {
        _use_keybind_icons = value;
    }

    uint32_t get_enabled_hotbar(uint32_t barID)
    { 
        if (hotbars.contains(barID)) {
            auto& bar = hotbars.at(barID);
            if (bar.is_enabled()) {
                return barID;
            } else {
                return get_enabled_hotbar(bar.get_parent());
            }
        }
        return MAIN_BAR;
    }

    uint32_t getCurrentHotbar_ingame()
    {
        if (GameData::isVampireLord()) return VAMPIRE_LORD_BAR;
        if (GameData::isWerewolf()) return WEREWOLF_BAR;
        uint32_t custom_bar = GameData::isCustomTransform();
        if (custom_bar > 0) {
            return custom_bar;
        }



        uint32_t ret = MAIN_BAR;

        auto pc = RE::PlayerCharacter::GetSingleton();
        if (pc) {
            if (!(Bars::use_default_bar_when_sheathed && !pc->AsActorState()->IsWeaponDrawn())) {
                GameData::EquippedType equip_type = GameData::getPlayerEquipmentType();
                switch (equip_type) {
                    case GameData::EquippedType::DUAL_WIELD:
                    case GameData::EquippedType::ONEHAND_EMPTY:
                        ret = DUAL_WIELD_BAR;
                        break;
                    case GameData::EquippedType::ONEHAND_SHIELD:
                        ret = ONE_HAND_SHIELD_BAR;
                        break;
                    case GameData::EquippedType::ONEHAND_SPELL:
                        ret = ONE_HAND_SPELL_BAR;
                        break;
                    case GameData::EquippedType::TWOHAND:
                        ret = TWO_HANDED_BAR;
                        break;
                    case GameData::EquippedType::SPELL:
                    case GameData::EquippedType::STAFF_SHIELD:
                        ret = MAGIC_BAR;
                        break;
                    case GameData::EquippedType::BOW:
                    case GameData::EquippedType::CROSSBOW:
                        ret = RANGED_BAR;
                        break;
                }
                // FISTS use default bar
            }

            if (pc->IsSneaking()) {
                ret += 1;  // All sneak bars have ID of non_sneak +1
            }
        }
        return get_enabled_hotbar(ret);
    }

    void update_oblivion_bar_press_show_timer(float delta)
    {
        if (oblivion_bar_press_show_timer > 0.0f) {
            oblivion_bar_press_show_timer -= delta;
            if (oblivion_bar_press_show_timer < 0.0f) {
                oblivion_bar_press_show_timer = 0.0f;
            }
        }
    }

    void SpellHotbar::Bars::init()
    {
        //will be loaeded from skse save in storage.cpp
        barsize = max_bar_size;
        disable_non_modifier_bar = false;

        slot_scale = 1.0f;
        offset_x = 0.0f;
        offset_y = 0.0f;
        slot_spacing = 8.0f;
        text_show_setting = text_show_mode::fade;

        bar_show_setting = bar_show_mode::combat_or_drawn;
        bar_show_setting_vampire_lord = bar_show_mode::combat;
        bar_show_setting_werewolf = bar_show_mode::never;

        use_default_bar_when_sheathed = false;
        disable_menu_rendering = false;
        disable_menu_binding = false;

        add_bars(MAIN_BAR);
        add_bars(MELEE_BAR, MAIN_BAR);
        add_bars(ONE_HAND_SHIELD_BAR, MELEE_BAR);
        add_bars(ONE_HAND_SPELL_BAR, MELEE_BAR);
        add_bars(DUAL_WIELD_BAR, MELEE_BAR);
        add_bars(TWO_HANDED_BAR, MELEE_BAR);
        add_bars(RANGED_BAR, MAIN_BAR);
        add_bars(MAGIC_BAR, MAIN_BAR);

        add_special_bar(VAMPIRE_LORD_BAR);
        add_special_bar(WEREWOLF_BAR);

        menu_bar_id = MAIN_BAR;
        for (size_t i = 0U; i < bar_cycle.size(); i++) {
            std::string name = hotbars.at(bar_cycle.at(i)).get_name();
            //logger::trace("BarCycle({}, {}): {}", i, name, bar_cycle.at(i));
        }
    }

    inline int wrap_index(int i, int size)
    {
        return (i + size) % size;
    }

    uint32_t cycleMenuBar(uint32_t current_id, bool backwards) {
        auto bar_it = std::find(bar_cycle.begin(), bar_cycle.end(), current_id);
        if (bar_it != bar_cycle.end()) {
            int index = static_cast<int>(bar_it - bar_cycle.begin());
            //logger::trace("Bar Index: {}", index);
            bool found = false;
            while (!found) {
                index = wrap_index(backwards ? index - 1 : index + 1, static_cast<int>(bar_cycle.size()));
                current_id = bar_cycle.at(index);
                found = hotbars.at(current_id).is_enabled() || (current_id == MAIN_BAR); //mainbar check is safety for infinity loop, but should not be disableable
            }
            //logger::trace("End Index: {}", index);
        }
        return current_id;
    }
    uint32_t getPreviousMenuBar(uint32_t current_id) { return cycleMenuBar(current_id, true); }
    uint32_t getNextMenuBar(uint32_t current_id) { return cycleMenuBar(current_id, false); }

    key_modifier get_current_modifier() 
    {
        key_modifier keymod {key_modifier::none};

        if (Input::mod_1.isValidBound() && Input::mod_1.isDown())
        {
            keymod = key_modifier::ctrl;
        }
        else if (Input::mod_2.isValidBound() && Input::mod_2.isDown())
        {
            keymod = key_modifier::shift;
        }
        else if (Input::mod_3.isValidBound() && Input::mod_3.isDown())
        {
            keymod = key_modifier::alt;
        }
        return keymod;
    }

    bool save_bars_to_json(std::string path) {
        constexpr int save_format{1};

        std::filesystem::path file_path(path);
        std::filesystem::create_directories(file_path.parent_path());

        rj::Document d;
        d.SetObject();
        // rj::Value version_tag(rj::kObjectType);
        // version_tag.AddMember("version", save_format, d.GetAllocator());
        d.AddMember("version", save_format, d.GetAllocator());

        std::ofstream ofs(path, std::ofstream::out);
        if (ofs.is_open()) {
            rj::OStreamWrapper osw(ofs);

            rj::Value bars_node(rj::kArrayType);

            for (auto& [key, bar] : hotbars)
            {
                bar.to_json(d, key, bars_node);
            }

            d.AddMember("bars", bars_node, d.GetAllocator());

            rj::PrettyWriter<rj::OStreamWrapper> writer(osw);
            writer.SetIndent(' ', 4);
            d.Accept(writer);
            return true;
        } else {
            logger::error("Could not open '{}' for writing", path);
            return false;
        }
    }

    void clear_bars() {
        for (auto& [key, bar] : hotbars) {
            bar.clear();
        }
    }

    void load_subbar_from_json(Hotbar& bar, rj::Value & bar_object, const char* tag, key_modifier mod) {
        if (bar_object.HasMember(tag)) {
            auto subbar_arr = bar_object[tag].GetArray();

            auto& subbar = bar.get_sub_bar(mod);
            for (auto& entry_object : subbar_arr) {
                if (entry_object.HasMember("index") && entry_object.HasMember("form") &&
                    entry_object.HasMember("file")) {

                    size_t index = static_cast<size_t>(entry_object["index"].GetInt());
                    if (index < subbar.m_slotted_skills.size()) {
                        uint32_t form_id = static_cast<uint32_t>(entry_object["form"].GetInt64());
                        std::string file = entry_object["file"].GetString();

                        auto form = GameData::get_form_from_file(form_id, file);
                        if (form) {
                            subbar.m_slotted_skills[index] = form->GetFormID();

                        } else {
                            logger::warn("Skipping entry, could not find '{}' in '{}'", form_id, file);
                        }
                    } else {
                        logger::warn("Skipping entry, index too high: {}", index);
                    }
                } else {
                    logger::warn("Skipping entry due missing members");
                }
            }
        }
    }

    bool load_bars_from_json_v1(rj::Document& d) {
        if (!d.HasMember("bars")) {
            logger::error("Could not read bars from json");
            return false;
        } else {
            if (!d["bars"].IsArray()) {
                logger::error("Could not read bars array from json");
            }
        }

        clear_bars();

        for (auto& bar_object : d["bars"].GetArray()) {
            if (bar_object.HasMember("id")) {
                uint32_t key = static_cast<uint32_t>(bar_object["id"].GetInt64());

                if (hotbars.contains(key)) {
                    auto & bar = hotbars.at(key);

                    load_subbar_from_json(bar, bar_object, "none", key_modifier::none);
                    load_subbar_from_json(bar, bar_object, "ctrl", key_modifier::ctrl);
                    load_subbar_from_json(bar, bar_object, "shift", key_modifier::shift);
                    load_subbar_from_json(bar, bar_object, "alt", key_modifier::alt);

                } else {
                    logger::warn("Skipping bar with unknown id: {}", key);
                }
            } else {
                logger::warn("Skipping entries, missing bar id");
            }
        }
        return true;
    }

    bool load_bars_from_json(std::string path)
    {
        logger::info("Loading Bars from {}", path);
        std::ifstream ifs(path);

        if (!ifs.is_open()) {
            logger::error("Could not open '{}' for reading.", path);
            return false;
        }
        try {
            rj::IStreamWrapper isw(ifs);
            rj::Document d;
            d.ParseStream(isw);

            if (d.HasParseError()) {
                logger::error("JSON parse error (offset {}): {}", d.GetErrorOffset(), rj::GetParseError_En(d.GetParseError()));
                return false;
            }
        
            if (!d.HasMember("version")) {
                logger::error("Could not read version from {}", path);
                return false;
            }
            int version = d["version"].GetInt();
            if (version == 1) {
                return load_bars_from_json_v1(d);
            } else {
                logger::error("Unknown bars json version: {}", version); 
                return false;
            }
           
        } catch (const std::exception& e) {
            std::string msg(e.what());
            logger::error("Error during parsing: {}", msg);
            return false;
        }

        return false;
    }
}
