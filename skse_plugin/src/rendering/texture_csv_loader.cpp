#include "texture_csv_loader.h"
#include "../logger/logger.h"
#include "rapidcsv.h"
#include "../game_data/game_data.h"
#include "render_manager.h"
#include "../game_data/csv_loader.h"
#include <unordered_set>

namespace SpellHotbar::TextureCSVLoader {

    namespace fs = std::filesystem;

    std::unordered_map<std::string, GameData::DefaultIconType> const default_icon_names = {
        {"UNKNOWN", GameData::DefaultIconType::UNKNOWN},
        {"BAR_EMPTY", GameData::DefaultIconType::BAR_EMPTY},
        {"BAR_OVERLAY", GameData::DefaultIconType::BAR_OVERLAY},
        {"BAR_HIGHLIGHT", GameData::DefaultIconType::BAR_HIGHLIGHT},
        {"UNBIND_SLOT", GameData::DefaultIconType::UNBIND_SLOT},
        {"LESSER_POWER", GameData::DefaultIconType::LESSER_POWER},
        {"GREATER_POWER", GameData::DefaultIconType::GREATER_POWER},
        {"DESTRUCTION_FIRE_NOVICE", GameData::DefaultIconType::DESTRUCTION_FIRE_NOVICE},
        {"DESTRUCTION_FIRE_APPRENTICE", GameData::DefaultIconType::DESTRUCTION_FIRE_APPRENTICE},
        {"DESTRUCTION_FIRE_ADEPT", GameData::DefaultIconType::DESTRUCTION_FIRE_ADEPT},
        {"DESTRUCTION_FIRE_EXPERT", GameData::DefaultIconType::DESTRUCTION_FIRE_EXPERT},
        {"DESTRUCTION_FIRE_MASTER", GameData::DefaultIconType::DESTRUCTION_FIRE_MASTER},
        {"DESTRUCTION_FROST_NOVICE", GameData::DefaultIconType::DESTRUCTION_FROST_NOVICE},
        {"DESTRUCTION_FROST_APPRENTICE", GameData::DefaultIconType::DESTRUCTION_FROST_APPRENTICE},
        {"DESTRUCTION_FROST_ADEPT", GameData::DefaultIconType::DESTRUCTION_FROST_ADEPT},
        {"DESTRUCTION_FROST_EXPERT", GameData::DefaultIconType::DESTRUCTION_FROST_EXPERT},
        {"DESTRUCTION_FROST_MASTER", GameData::DefaultIconType::DESTRUCTION_FROST_MASTER},
        {"DESTRUCTION_SHOCK_NOVICE", GameData::DefaultIconType::DESTRUCTION_SHOCK_NOVICE},
        {"DESTRUCTION_SHOCK_APPRENTICE", GameData::DefaultIconType::DESTRUCTION_SHOCK_APPRENTICE},
        {"DESTRUCTION_SHOCK_ADEPT", GameData::DefaultIconType::DESTRUCTION_SHOCK_ADEPT},
        {"DESTRUCTION_SHOCK_EXPERT", GameData::DefaultIconType::DESTRUCTION_SHOCK_EXPERT},
        {"DESTRUCTION_SHOCK_MASTER", GameData::DefaultIconType::DESTRUCTION_SHOCK_MASTER},
        {"DESTRUCTION_GENERIC_NOVICE", GameData::DefaultIconType::DESTRUCTION_GENERIC_NOVICE},
        {"DESTRUCTION_GENERIC_APPRENTICE", GameData::DefaultIconType::DESTRUCTION_GENERIC_APPRENTICE},
        {"DESTRUCTION_GENERIC_ADEPT", GameData::DefaultIconType::DESTRUCTION_GENERIC_ADEPT},
        {"DESTRUCTION_GENERIC_EXPERT", GameData::DefaultIconType::DESTRUCTION_GENERIC_EXPERT},
        {"DESTRUCTION_GENERIC_MASTER", GameData::DefaultIconType::DESTRUCTION_GENERIC_MASTER},
        {"ALTERATION_NOVICE", GameData::DefaultIconType::ALTERATION_NOVICE},
        {"ALTERATION_APPRENTICE", GameData::DefaultIconType::ALTERATION_APPRENTICE},
        {"ALTERATION_ADEPT", GameData::DefaultIconType::ALTERATION_ADEPT},
        {"ALTERATION_EXPERT", GameData::DefaultIconType::ALTERATION_EXPERT},
        {"ALTERATION_MASTER", GameData::DefaultIconType::ALTERATION_MASTER},
        {"RESTORATION_FRIENDLY_NOVICE", GameData::DefaultIconType::RESTORATION_FRIENDLY_NOVICE},
        {"RESTORATION_FRIENDLY_APPRENTICE", GameData::DefaultIconType::RESTORATION_FRIENDLY_APPRENTICE},
        {"RESTORATION_FRIENDLY_ADEPT", GameData::DefaultIconType::RESTORATION_FRIENDLY_ADEPT},
        {"RESTORATION_FRIENDLY_EXPERT", GameData::DefaultIconType::RESTORATION_FRIENDLY_EXPERT},
        {"RESTORATION_FRIENDLY_MASTER", GameData::DefaultIconType::RESTORATION_FRIENDLY_MASTER},
        {"RESTORATION_HOSTILE_NOVICE", GameData::DefaultIconType::RESTORATION_HOSTILE_NOVICE},
        {"RESTORATION_HOSTILE_APPRENTICE", GameData::DefaultIconType::RESTORATION_HOSTILE_APPRENTICE},
        {"RESTORATION_HOSTILE_ADEPT", GameData::DefaultIconType::RESTORATION_HOSTILE_ADEPT},
        {"RESTORATION_HOSTILE_EXPERT", GameData::DefaultIconType::RESTORATION_HOSTILE_EXPERT},
        {"RESTORATION_HOSTILE_MASTER", GameData::DefaultIconType::RESTORATION_HOSTILE_MASTER},
        {"ILLUSION_FRIENDLY_NOVICE", GameData::DefaultIconType::ILLUSION_FRIENDLY_NOVICE},
        {"ILLUSION_FRIENDLY_APPRENTICE", GameData::DefaultIconType::ILLUSION_FRIENDLY_APPRENTICE},
        {"ILLUSION_FRIENDLY_ADEPT", GameData::DefaultIconType::ILLUSION_FRIENDLY_ADEPT},
        {"ILLUSION_FRIENDLY_EXPERT", GameData::DefaultIconType::ILLUSION_FRIENDLY_EXPERT},
        {"ILLUSION_FRIENDLY_MASTER", GameData::DefaultIconType::ILLUSION_FRIENDLY_MASTER},
        {"ILLUSION_HOSTILE_NOVICE", GameData::DefaultIconType::ILLUSION_HOSTILE_NOVICE},
        {"ILLUSION_HOSTILE_APPRENTICE", GameData::DefaultIconType::ILLUSION_HOSTILE_APPRENTICE},
        {"ILLUSION_HOSTILE_ADEPT", GameData::DefaultIconType::ILLUSION_HOSTILE_ADEPT},
        {"ILLUSION_HOSTILE_EXPERT", GameData::DefaultIconType::ILLUSION_HOSTILE_EXPERT},
        {"ILLUSION_HOSTILE_MASTER", GameData::DefaultIconType::ILLUSION_HOSTILE_MASTER},
        {"CONJURATION_BOUND_WEAPON_NOVICE", GameData::DefaultIconType::CONJURATION_BOUND_WEAPON_NOVICE},
        {"CONJURATION_BOUND_WEAPON_APPRENTICE", GameData::DefaultIconType::CONJURATION_BOUND_WEAPON_APPRENTICE},
        {"CONJURATION_BOUND_WEAPON_ADEPT", GameData::DefaultIconType::CONJURATION_BOUND_WEAPON_ADEPT},
        {"CONJURATION_BOUND_WEAPON_EXPERT", GameData::DefaultIconType::CONJURATION_BOUND_WEAPON_EXPERT},
        {"CONJURATION_BOUND_WEAPON_MASTER", GameData::DefaultIconType::CONJURATION_BOUND_WEAPON_MASTER},
        {"CONJURATION_SUMMON_NOVICE", GameData::DefaultIconType::CONJURATION_SUMMON_NOVICE},
        {"CONJURATION_SUMMON_APPRENTICE", GameData::DefaultIconType::CONJURATION_SUMMON_APPRENTICE},
        {"CONJURATION_SUMMON_ADEPT", GameData::DefaultIconType::CONJURATION_SUMMON_ADEPT},
        {"CONJURATION_SUMMON_EXPERT", GameData::DefaultIconType::CONJURATION_SUMMON_EXPERT},
        {"CONJURATION_SUMMON_MASTER", GameData::DefaultIconType::CONJURATION_SUMMON_MASTER},
        {"SHOUT_GENERIC", GameData::DefaultIconType::SHOUT_GENERIC},
        {"SINGLE_CAST", GameData::DefaultIconType::SINGLE_CAST},
        {"DUAL_CAST", GameData::DefaultIconType::DUAL_CAST},
        {"SCROLL_OVERLAY", GameData::DefaultIconType::SCROLL_OVERLAY},
        {"NO_OVERLAY", GameData::DefaultIconType::NO_OVERLAY},
        {"GENERIC_POTION", GameData::DefaultIconType::GENERIC_POTION},
        {"GENERIC_POTION_SMALL", GameData::DefaultIconType::GENERIC_POTION_SMALL},
        {"GENERIC_POTION_LARGE", GameData::DefaultIconType::GENERIC_POTION_LARGE},
        {"GENERIC_POISON", GameData::DefaultIconType::GENERIC_POISON},
        {"GENERIC_POISON_SMALL", GameData::DefaultIconType::GENERIC_POISON_SMALL},
        {"GENERIC_POISON_LARGE", GameData::DefaultIconType::GENERIC_POISON_LARGE}
    };

    bool is_spell_icon_file(rapidcsv::Document& doc)
    { 
        std::vector<std::string> columns = doc.GetColumnNames();

        //check required column names
        return csv::has_column(columns, "FormID") && csv::has_column(columns, "Plugin") &&
               csv::has_column(columns, "u0") && csv::has_column(columns, "v0") && csv::has_column(columns, "u1") &&
               csv::has_column(columns, "v1");
    }

    bool is_cooldown_icon_file(rapidcsv::Document& doc) {
        std::vector<std::string> columns = doc.GetColumnNames();

        // check required column names
        return csv::has_column(columns, "Cooldown") && csv::has_column(columns, "u0") &&
               csv::has_column(columns, "v0") && csv::has_column(columns, "u1") && csv::has_column(columns, "v1");
    }

    bool is_spellproc_overlay_icon_file(rapidcsv::Document& doc) {
        std::vector<std::string> columns = doc.GetColumnNames();

        // check required column names
        return csv::has_column(columns, "SpellProc") && csv::has_column(columns, "u0") &&
            csv::has_column(columns, "v0") && csv::has_column(columns, "u1") && csv::has_column(columns, "v1");
    }

    
    bool is_default_icon_file(rapidcsv::Document& doc) {
        std::vector<std::string> columns = doc.GetColumnNames();

        // check required column names
        return csv::has_column(columns, "IconName") && csv::has_column(columns, "u0") &&
               csv::has_column(columns, "v0") && csv::has_column(columns, "u1") && csv::has_column(columns, "v1");
    }


	void load_texture_from_csv(const std::string & path, const std::string & img_path)
    {
        rapidcsv::Document doc(path, rapidcsv::LabelParams(0, -1), rapidcsv::SeparatorParams('\t'));

        // check the column names
        bool spell_icons = is_spell_icon_file(doc);
        bool default_icons = is_default_icon_file(doc); //also used for extra icons
        bool cooldown_icons = is_cooldown_icon_file(doc);
        bool spellproc_icons = is_spellproc_overlay_icon_file(doc);

        if (!spell_icons && !default_icons && !cooldown_icons && !spellproc_icons) {
            logger::warn("Could not parse '{}', skipping", path);
        }

        if (cooldown_icons) {
            size_t num_cds = doc.GetRowCount();
            RenderManager::init_cooldown_icons(num_cds);
        }
        else if (spellproc_icons) {
            size_t num_frames = doc.GetRowCount();
            RenderManager::init_spellproc_overlay_icons(num_frames);
        }

        TextureImage& main_tex = RenderManager::load_texture(img_path);
        const std::string filename = std::filesystem::path(img_path).filename().string();

        std::unordered_set<std::string> warned_plugins;

        for (size_t i = 0; i < doc.GetRowCount(); i++)
        {
            try {
                // used by all
                float u0 = doc.GetCell<float>("u0", i);
                float v0 = doc.GetCell<float>("v0", i);
                float u1 = doc.GetCell<float>("u1", i);
                float v1 = doc.GetCell<float>("v1", i);

                if (spell_icons) {
                    std::string str_form = doc.GetCell<std::string>("FormID", i);
                    uint32_t form_id = static_cast<uint32_t>(std::stoul(str_form, nullptr, 16));

                    std::string plugin = doc.GetCell<std::string>("Plugin", i);

                    if (RE::TESDataHandler::GetSingleton()->GetModIndex(plugin).has_value()) {
                        auto* form = GameData::get_form_from_file(form_id, plugin);

                        if (form != nullptr) {
                            RenderManager::add_spell_texture(main_tex, form->GetFormID(), ImVec2(u0, v0), ImVec2(u1, v1), filename);

                        }
                        else {
                            logger::warn("Skipping spell icon {} {}, because form is null", str_form, plugin);
                        }
                    }
                    else {
                        if (!warned_plugins.contains(plugin)) {
                            warned_plugins.emplace(plugin);
                            logger::warn("Skipping Plugin '{}', not loaded.", plugin);
                        }
                    }

                } else if (default_icons) {
                    std::string str_name = doc.GetCell<std::string>("IconName", i);   
                    if (default_icon_names.contains(str_name)) {
                        auto type = default_icon_names.at(str_name);
                        RenderManager::add_default_icon(main_tex, type, ImVec2(u0, v0), ImVec2(u1, v1), str_name);
                    } else {
                        RenderManager::add_extra_icon(main_tex, str_name, ImVec2(u0, v0), ImVec2(u1, v1), filename);
                    }
                } else if (cooldown_icons) {
                    RenderManager::add_cooldown_icon(main_tex, ImVec2(u0, v0), ImVec2(u1, v1));
                }
                else if (spellproc_icons){
                    RenderManager::add_spellproc_overlay_icon(main_tex, ImVec2(u0, v0), ImVec2(u1, v1));
                }
            } catch (const std::exception& e) {
                std::string msg = e.what();
                logger::error("Error Loading csv: {}", msg);
            }
        }

    }

    inline void _load_images(const std::string & str_path) {
        // find matching .png
        std::string png_path = str_path.substr(0, str_path.size() - 3) + "png";
        if (fs::exists(fs::path(png_path))) {
            logger::info("Loading icons: {}", png_path);

            load_texture_from_csv(str_path, png_path);

        }
        else {
            logger::warn("No matching image file for {}", str_path);
        }
    }

    void load_icons(std::filesystem::path folder)
    {
        try {
            //Do vanilla icons first, so mods can override
            for (const auto& entry : std::filesystem::directory_iterator(folder)) {
                if (entry.is_regular_file()) {
                    std::string str_path = entry.path().string();

                    if (str_path.ends_with(".csv") && str_path.starts_with("icons_vanilla")) {
                        _load_images(str_path);
                    }
                }
            }
            //non vanilla 
            for (const auto& entry : std::filesystem::directory_iterator(folder)) {
                if (entry.is_regular_file()) {
                    std::string str_path = entry.path().string();

                    if (str_path.ends_with(".csv") && !str_path.starts_with("icons_vanilla")) {
                        _load_images(str_path);
                    }
                }
            }
        } catch (const std::exception& e) {
            std::string msg = e.what();
            logger::error("Error loading resources: {}", msg);
        }
    }

}