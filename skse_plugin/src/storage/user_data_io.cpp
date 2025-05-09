#include "user_data_io.h"
#include "../logger/logger.h"
#include "../input/keybinds.h"

#include <Shlobj.h>
#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/error/en.h>
#include "../bar/hotbars.h"
#include "../rendering/render_manager.h"
#include "../input/modes.h"

namespace SpellHotbar::Storage::IO {

    namespace rj = rapidjson;

    constexpr std::string_view user_dir_presets{ "My Games/Skyrim Special Edition/SpellHotbar/presets" };
    constexpr std::string_view user_dir_bars{ "My Games/Skyrim Special Edition/SpellHotbar/bars" };
    constexpr std::string_view user_dir_icon_edits{ "My Games/Skyrim Special Edition/SpellHotbar/icon_edits" };
    
    constexpr std::string_view preset_dir{ "Data/SKSE/Plugins/SpellHotbar/presets/" };
    constexpr std::string_view bars_dir{ "Data/SKSE/Plugins/SpellHotbar/bars/" };
    constexpr std::string_view icon_edits_dir{ "Data/SKSE/Plugins/SpellHotbar/icon_edits/" };

	std::filesystem::path IO::get_doc_dir()
	{
        std::filesystem::path ret;

        TCHAR mydoc_path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPath(NULL,
            CSIDL_MYDOCUMENTS,
            NULL,
            SHGFP_TYPE_CURRENT,
            mydoc_path)))
        {
            ret = std::filesystem::path(mydoc_path);
        }

		return ret;
	}

    std::filesystem::path get_preset_user_dir() {
        return get_doc_dir() / user_dir_presets;
    }

    std::filesystem::path get_bars_user_dir() {
        return get_doc_dir() / user_dir_bars;
    }

    std::filesystem::path get_icon_edits_user_dir() {
        return get_doc_dir() / user_dir_icon_edits;
    }

    inline static int get_int_or_default(rj::Document& d, std::string key, int def) {
        const char* str_key = key.c_str();
        if (d.HasMember(str_key)) {
            return d[str_key].GetInt();
        }
        else {
            return def;
        }
    }

    inline static float get_float_or_default(const rj::Document& d, std::string key, float def) {
        const char* str_key = key.c_str();
        if (d.HasMember(str_key)) {
            return d[str_key].GetFloat();
        }
        else {
            return def;
        }
    }

    inline static void load_global_from_json(const rj::Document& d, RE::TESGlobal* glob, const std::string & key, float def) {
        if (glob != nullptr) {
            glob->value = get_float_or_default(d, key, def);
        }
    }

    static bool load_profile_json_v2(rj::Document& d) {
        
        logger::trace("Loading profile from json...");

        for (int i = 0; i < static_cast<int>(max_bar_size); i++) {
            int keycode = get_int_or_default(d, "keybind." + std::to_string(i), -1);
            Input::rebind_key(i, keycode, false);
        }
        Input::rebind_key(Input::keybind_id::ui_next, get_int_or_default(d, "keybind.next", -1), false);
        Input::rebind_key(Input::keybind_id::ui_prev, get_int_or_default(d, "keybind.prev", -1), false);

        Input::rebind_key(Input::keybind_id::modifier_1, get_int_or_default(d, "keybind.modifier.1", -1), false);
        Input::rebind_key(Input::keybind_id::modifier_2, get_int_or_default(d, "keybind.modifier.2", -1), false);
        Input::rebind_key(Input::keybind_id::modifier_3, get_int_or_default(d, "keybind.modifier.3", -1), false);

        Input::rebind_key(Input::keybind_id::dual_casting_mod, get_int_or_default(d, "keybind.modifier.dual_cast", -1), false);
        Input::rebind_key(Input::keybind_id::show_bar_mod, get_int_or_default(d, "keybind.modifier.show_bar", -1), false);
        
        Input::rebind_key(Input::keybind_id::oblivion_cast, get_int_or_default(d, "keybind.oblivion_mode.cast", -1), false);
        Input::rebind_key(Input::keybind_id::oblivion_potion, get_int_or_default(d, "keybind.oblivion_mode.potion", -1), false);
        Input::rebind_key(Input::keybind_id::oblivion_show_bar_mod, get_int_or_default(d, "keybind.oblivion_mode.modifier_show_bar", -1), false);
        Input::rebind_key(Input::keybind_id::open_advanced_bind_menu, get_int_or_default(d, "keybind.open_advanced_bind_menu", -1), false);

        Bars::disable_non_modifier_bar = get_int_or_default(d, "settings.disable_non_mod_bar", 0) != 0;

        int barsize = get_int_or_default(d, "settings.number_of_slots", static_cast<int>(max_bar_size));
        Bars::barsize = static_cast<uint8_t>(std::clamp(barsize, 1, static_cast<int>(max_bar_size)));

        Bars::slot_scale = get_float_or_default(d, "settings.slot_scale", 1.0f);

        Bars::offset_x = RenderManager::scale_to_resolution(get_float_or_default(d, "settings.offset_x", 0.0f));
        Bars::offset_y = RenderManager::scale_to_resolution(get_float_or_default(d, "settings.offset_y", 0.0f));

        Bars::slot_spacing = get_float_or_default(d, "settings.slot_spacing", 8.0f);

        Bars::bar_anchor_point = Bars::anchor_point(std::clamp(get_int_or_default(d, "settings.bar_anchor_point", 0), 0, static_cast<int>(Bars::anchor_point::CENTER)));

        Bars::text_show_setting = Bars::text_show_mode(std::clamp(get_int_or_default(d, "settings.text_show_setting", 1), 0, static_cast<int>(Bars::text_show_mode::always)));

        Bars::bar_show_setting = Bars::bar_show_mode(std::clamp(get_int_or_default(d, "settings.hud_show_mode", static_cast<int>(Bars::bar_show_mode::combat_or_drawn)), 0, 5));
        Bars::bar_show_setting_vampire_lord = Bars::bar_show_mode(std::clamp(get_int_or_default(d, "settings.hud_show_mode_vampire_lord", static_cast<int>(Bars::bar_show_mode::combat)), 0, 2));
        Bars::bar_show_setting_werewolf = Bars::bar_show_mode(std::clamp(get_int_or_default(d, "settings.hud_show_mode_werewolf", static_cast<int>(Bars::bar_show_mode::never)), 0, 2));

        Bars::use_default_bar_when_sheathed = get_int_or_default(d, "settings.use_default_bar_when_sheathed", 0) != 0;
        Bars::disable_menu_rendering = get_int_or_default(d, "settings.disable_menu_rendering", 0) != 0;
        Bars::disable_menu_binding = get_int_or_default(d, "settings.disable_menu_binding", 0) != 0;
        Bars::disable_non_modifier_bar = get_int_or_default(d, "settings.disable_non_mod_bar", 0) != 0;
        Bars::set_use_keybind_icons(get_int_or_default(d, "settings.use_key_icons", 0) != 0);

        GameData::potion_gcd = get_float_or_default(d, "settings.potion_gcd", 1.0f);
        bool individual_shout_cds = get_int_or_default(d, "settings.individual_shout_cooldowns", 0) != 0;
        if (individual_shout_cds != GameData::individual_shout_cooldowns) {
            GameData::toggle_individual_shout_cooldowns;
        }

        //oblivion mode
        Bars::oblivion_slot_scale = get_float_or_default(d, "settings.oblivion_bar.slot_scale", 1.0f);
        Bars::oblivion_offset_x = RenderManager::scale_to_resolution(get_float_or_default(d, "settings.oblivion_bar.offset_x", 0.0f));
        Bars::oblivion_offset_y = RenderManager::scale_to_resolution(get_float_or_default(d, "settings.oblivion_bar.offset_y", 0.0f));
        Bars::oblivion_slot_spacing = get_float_or_default(d, "settings.oblivion_bar.slot_spacing", 8.0f);
        Bars::oblivion_bar_anchor_point = Bars::anchor_point(std::clamp(get_int_or_default(d, "settings.oblivion_bar.bar_anchor_point", static_cast<int>(Bars::anchor_point::LEFT)), 0, static_cast<int>(Bars::anchor_point::CENTER)));
        Bars::oblivion_bar_show_power = get_int_or_default(d, "settings.oblivion_bar.show_power", 1) != 0;
        Bars::oblivion_bar_show_setting = Bars::bar_show_mode(std::clamp(get_int_or_default(d, "settings.oblivion_bar.show_mode", static_cast<int>(Bars::bar_show_mode::combat_or_drawn)), 0, 5));
        
        Bars::oblivion_bar_held_show_time_threshold = std::clamp(get_float_or_default(d, "settings.oblivion_bar.held_show_time", 0.25f), 0.0f, 5.0f);
        Bars::oblivion_bar_vertical = get_int_or_default(d, "settings.oblivion_bar.vertical", 0) != 0;

        Input::set_input_mode(std::clamp(get_int_or_default(d, "settings.input_mode", 0), 0, 2));
        Bars::bar_row_len = std::clamp(static_cast<uint8_t>(get_int_or_default(d, "settings.bar_row_ren", static_cast<int>(max_bar_size))), 1Ui8, static_cast<uint8_t>(max_bar_size));
        Bars::layout = Bars::bar_layout(std::clamp(get_int_or_default(d, "settings.bar_layout", 0), 0, 2));
        Bars::bar_circle_radius = std::max(get_float_or_default(d, "settings.bar_circle_radius", 2.2f), 0.1f);
        Bars::bar_cross_distance = std::clamp(get_float_or_default(d, "settings.bar_cross_distance", 0.0f), 0.0f, 1.0f);

        //battlemage perks
        load_global_from_json(d, GameData::global_spellhotbar_perks_override, "battlemage.overrideperks", 0.0f);
        load_global_from_json(d, GameData::global_spellhotbar_perks_timed_block_window, "battlemage.timedblockwindow", 0.5f);
        load_global_from_json(d, GameData::global_spellhotbar_perks_block_trigger_chance, "battlemage.blockprocchance", 1.0f);
        load_global_from_json(d, GameData::global_spellhotbar_perks_power_attack_trigger_chance, "battlemage.powerattackprocchance", 0.5f);
        load_global_from_json(d, GameData::global_spellhotbar_perks_sneak_attack_trigger_chance, "battlemage.sneakattackprocchance", 1.0f);
        load_global_from_json(d, GameData::global_spellhotbar_perks_crit_trigger_chance, "battlemage.critprocchance", 1.0f);
        load_global_from_json(d, GameData::global_spellhotbar_perks_proc_cooldown, "battlemage.proccooldown", 10.0f);
        load_global_from_json(d, GameData::global_spellhotbar_perks_require_halfcostperk, "battlemage.requirehalfcostperk", 0.0f);

        //Bar Enabled & inherit state:
        const std::vector<uint32_t> bars = {
            Bars::MAIN_BAR_SNEAK,
            Bars::MELEE_BAR,
            Bars::MELEE_BAR_SNEAK,
            Bars::ONE_HAND_SHIELD_BAR,
            Bars::ONE_HAND_SHIELD_BAR_SNEAK,
            Bars::ONE_HAND_SPELL_BAR,
            Bars::ONE_HAND_SPELL_BAR_SNEAK,
            Bars::DUAL_WIELD_BAR,
            Bars::DUAL_WIELD_BAR_SNEAK,
            Bars::TWO_HANDED_BAR,
            Bars::TWO_HANDED_BAR_SNEAK,
            Bars::RANGED_BAR,
            Bars::RANGED_BAR_SNEAK,
            Bars::MAGIC_BAR,
            Bars::MAGIC_BAR_SNEAK
        };
        for (uint32_t bar_id : bars) {
            bool enabled = get_int_or_default(d, "bar." + std::to_string(bar_id) + ".enabled", 1) != 0;
            int inherit = get_int_or_default(d, "bar." + std::to_string(bar_id) + ".inherit", 0);
                
            if (SpellHotbar::Bars::hotbars.contains(bar_id)) {
                auto& bar = SpellHotbar::Bars::hotbars.at(bar_id);
                bar.set_enabled(enabled);
                bar.set_inherit_mode(inherit);
            }
        }
        return true;
    }

    static bool load_preset_json(const std::filesystem::path& path) {
    
        logger::trace("Loading Profile from {}", path.string());
        std::ifstream ifs(path);

        if (!ifs.is_open()) {
            logger::error("Could not open '{}' for reading.", path.string());
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

            int version = 0;
            if (!d.HasMember("version")) {
                version = 2; //assume v2 for old configs
                //logger::error("Could not read version from {}", path.string());
                //return false;
            }
            else {
                version = d["version"].GetInt();
            }
            if (version == 2) {
                return load_profile_json_v2(d);
            }
            else {
                logger::error("Unknown bars json version: {}", version);
                return false;
            }

        }
        catch (const std::exception& e) {
            std::string msg(e.what());
            logger::error("Error during parsing: {}", msg);
            return false;
        }

        return false;
    }

    bool load_preset(const std::string& profile_name, bool include_user_dir)
    {
        std::filesystem::path user_dir_path = get_preset_user_dir() / profile_name;
        std::filesystem::path mod_dir_path = std::filesystem::path(preset_dir) / profile_name;

        if (include_user_dir && std::filesystem::exists(user_dir_path)) {
            return load_preset_json(user_dir_path);
        }
        else if (std::filesystem::exists(mod_dir_path)) {
            return load_preset_json(mod_dir_path);
        }

        return false;
    }

    inline static void add_int(rj::Document& d, const std::string& key, int value) {
        rj::Value name(key.c_str(), d.GetAllocator());
        d.AddMember(name, value, d.GetAllocator());
    }

    inline static void add_float(rj::Document& d, const std::string& key, float value) {
        rj::Value name(key.c_str(), d.GetAllocator());
        d.AddMember(name, value, d.GetAllocator());
    }

    inline static void save_global_to_json(rj::Document& d, RE::TESGlobal* glob, const std::string& key) {
        if (glob != nullptr) {
            add_float(d, key, glob->value);
        }
    }

    static bool save_preset_v2(rj::Document& d) {
        
        for (int i = 0; i < static_cast<int>(max_bar_size); i++) {

            add_int(d, "keybind." + std::to_string(i), Input::get_keybind(i));
        }
        add_int(d, "keybind.next", Input::get_keybind(Input::keybind_id::ui_next));
        add_int(d, "keybind.prev", Input::get_keybind(Input::keybind_id::ui_prev));
        add_int(d, "keybind.modifier.1", Input::get_keybind(Input::keybind_id::modifier_1));
        add_int(d, "keybind.modifier.2", Input::get_keybind(Input::keybind_id::modifier_2));
        add_int(d, "keybind.modifier.3", Input::get_keybind(Input::keybind_id::modifier_3));
        add_int(d, "keybind.modifier.dual_cast", Input::get_keybind(Input::keybind_id::dual_casting_mod));
        add_int(d, "keybind.modifier.show_bar", Input::get_keybind(Input::keybind_id::show_bar_mod));
        add_int(d, "keybind.oblivion_mode.cast", Input::get_keybind(Input::keybind_id::oblivion_cast));
        add_int(d, "keybind.oblivion_mode.potion", Input::get_keybind(Input::keybind_id::oblivion_potion));
        add_int(d, "keybind.oblivion_mode.modifier_show_bar", Input::get_keybind(Input::keybind_id::oblivion_show_bar_mod));
        add_int(d, "keybind.open_advanced_bind_menu", Input::get_keybind(Input::keybind_id::open_advanced_bind_menu));

        add_int(d, "settings.disable_non_mod_bar", Bars::disable_non_modifier_bar ? 1 : 0);
        add_int(d, "settings.number_of_slots", static_cast<int>(Bars::barsize));

        add_float(d, "settings.slot_scale", Bars::slot_scale);
        add_float(d, "settings.offset_x", RenderManager::scale_from_resolution(Bars::offset_x));
        add_float(d, "settings.offset_y", RenderManager::scale_from_resolution(Bars::offset_y));
        add_float(d, "settings.slot_spacing", Bars::slot_spacing);
        add_int(d, "settings.bar_anchor_point", static_cast<int>(Bars::bar_anchor_point));
        add_int(d, "settings.text_show_setting", static_cast<int>(Bars::text_show_setting));

        add_int(d, "settings.hud_show_mode", static_cast<int>(Bars::bar_show_setting));
        add_int(d, "settings.hud_show_mode_vampire_lord", static_cast<int>(Bars::bar_show_setting_vampire_lord));
        add_int(d, "settings.hud_show_mode_werewolf", static_cast<int>(Bars::bar_show_setting_werewolf));

        add_int(d, "settings.use_default_bar_when_sheathed", Bars::use_default_bar_when_sheathed ? 1 : 0);
        add_int(d, "settings.disable_menu_rendering", Bars::disable_menu_rendering ? 1 : 0);
        add_int(d, "settings.disable_menu_binding", Bars::disable_menu_binding ? 1 : 0);
        add_int(d, "settings.disable_non_mod_bar", Bars::disable_non_modifier_bar ? 1 : 0);
        add_int(d, "settings.use_key_icons", Bars::get_use_keybind_icons() ? 1 : 0);

        add_float(d, "settings.potion_gcd", GameData::potion_gcd);
        add_int(d, "settings.individual_shout_cooldowns", GameData::individual_shout_cooldowns ? 1 : 0);

        add_float(d, "settings.oblivion_bar.slot_scale", Bars::oblivion_slot_scale);
        add_float(d, "settings.oblivion_bar.offset_x", RenderManager::scale_from_resolution(Bars::oblivion_offset_x));
        add_float(d, "settings.oblivion_bar.offset_y", RenderManager::scale_from_resolution(Bars::oblivion_offset_y));
        add_float(d, "settings.oblivion_bar.slot_spacing", Bars::oblivion_slot_spacing);
        add_int(d, "settings.oblivion_bar.bar_anchor_point", static_cast<int>(Bars::oblivion_bar_anchor_point));
        add_int(d, "settings.oblivion_bar.show_power", Bars::oblivion_bar_show_power ? 1 : 0);
        add_int(d, "settings.oblivion_bar.show_mode", static_cast<int>(Bars::oblivion_bar_show_setting));
        add_float(d, "settings.oblivion_bar.held_show_time", Bars::oblivion_bar_held_show_time_threshold);
        add_int(d, "settings.oblivion_bar.vertical", Bars::oblivion_bar_vertical ? 1 : 0);

        add_int(d, "settings.input_mode", Input::get_current_mode_index());
        add_int(d, "settings.bar_row_ren", Bars::bar_row_len);
        add_int(d, "settings.bar_layout", static_cast<int>(Bars::layout));
        add_float(d, "settings.bar_circle_radius", Bars::bar_circle_radius);
        add_float(d, "settings.bar_cross_distance", Bars::bar_cross_distance);

        save_global_to_json(d, GameData::global_spellhotbar_perks_override, "battlemage.overrideperks");
        save_global_to_json(d, GameData::global_spellhotbar_perks_timed_block_window, "battlemage.timedblockwindow");
        save_global_to_json(d, GameData::global_spellhotbar_perks_block_trigger_chance, "battlemage.blockprocchance");
        save_global_to_json(d, GameData::global_spellhotbar_perks_power_attack_trigger_chance, "battlemage.powerattackprocchance");
        save_global_to_json(d, GameData::global_spellhotbar_perks_sneak_attack_trigger_chance, "battlemage.sneakattackprocchance");
        save_global_to_json(d, GameData::global_spellhotbar_perks_crit_trigger_chance, "battlemage.critprocchance");
        save_global_to_json(d, GameData::global_spellhotbar_perks_proc_cooldown, "battlemage.proccooldown");
        save_global_to_json(d, GameData::global_spellhotbar_perks_require_halfcostperk, "battlemage.requirehalfcostperk");

        //Bar Enabled & inherit state:
        const std::vector<uint32_t> bars = {
            Bars::MAIN_BAR_SNEAK,
            Bars::MELEE_BAR,
            Bars::MELEE_BAR_SNEAK,
            Bars::ONE_HAND_SHIELD_BAR,
            Bars::ONE_HAND_SHIELD_BAR_SNEAK,
            Bars::ONE_HAND_SPELL_BAR,
            Bars::ONE_HAND_SPELL_BAR_SNEAK,
            Bars::DUAL_WIELD_BAR,
            Bars::DUAL_WIELD_BAR_SNEAK,
            Bars::TWO_HANDED_BAR,
            Bars::TWO_HANDED_BAR_SNEAK,
            Bars::RANGED_BAR,
            Bars::RANGED_BAR_SNEAK,
            Bars::MAGIC_BAR,
            Bars::MAGIC_BAR_SNEAK
        };
        for (uint32_t bar_id : bars) {
            bool enabled { true };
            int inherit { 0 };
            if (SpellHotbar::Bars::hotbars.contains(bar_id)) {
                auto& bar = SpellHotbar::Bars::hotbars.at(bar_id);
                enabled = bar.is_enabled();
                inherit = bar.get_inherit_mode();
            }
            add_int(d, "bar." + std::to_string(bar_id) + ".enabled", enabled ? 1 : 0);
            add_int(d, "bar." + std::to_string(bar_id) + ".inherit", inherit);
        }
        return true;
    }

    bool save_preset(const std::string& profile_name) {
        std::string filename;
        if (profile_name.ends_with(".json")) {
            filename = profile_name;
        }
        else {
            filename = profile_name + ".json";
        }

        std::filesystem::path out_path = get_preset_user_dir() / filename;

        std::filesystem::create_directories(out_path.parent_path());

        rj::Document d;
        d.SetObject();
        d.AddMember("version", preset_save_version, d.GetAllocator());

        std::ofstream ofs(out_path, std::ofstream::out);
        if (ofs.is_open()) {
            rj::OStreamWrapper osw(ofs);

            save_preset_v2(d);

            rj::PrettyWriter<rj::OStreamWrapper> writer(osw);
            writer.SetIndent(' ', 4);
            d.Accept(writer);
            return true;
        }
        else {
            logger::error("Could not open '{}' for writing", out_path.string());
            return false;
        }
    }

    static void add_json_files(std::vector<std::string>& vec, std::filesystem::path& folder) {
        if (std::filesystem::exists(folder) && std::filesystem::is_directory(folder)) {
            for (auto& p : std::filesystem::directory_iterator(folder))
            {
                if (p.path().extension() == ".json")
                {
                    vec.push_back(p.path().filename().string());
                }
            }
        }
    }

    std::vector<std::string> get_config_presets()
    {
        std::vector<std::string> ret;
        ret.emplace_back("<cancel>");

        std::filesystem::path user_dir = get_preset_user_dir();
        std::filesystem::path mod_dir{ preset_dir };

        add_json_files(ret, user_dir);
        add_json_files(ret, mod_dir);

        return ret;
    }

    std::vector<std::string> get_bar_presets()
    {
        std::vector<std::string> ret;
        ret.emplace_back("<cancel>");

        std::filesystem::path user_dir = get_bars_user_dir();
        std::filesystem::path mod_dir{ bars_dir };

        add_json_files(ret, user_dir);
        add_json_files(ret, mod_dir);

        return ret;
    }

    std::vector<std::string> get_icon_edits_presets()
    {
        std::vector<std::string> ret;
        ret.emplace_back("<cancel>");

        std::filesystem::path user_dir = get_icon_edits_user_dir();
        std::filesystem::path mod_dir{ icon_edits_dir };

        add_json_files(ret, user_dir);
        add_json_files(ret, mod_dir);

        return ret;
    }

}