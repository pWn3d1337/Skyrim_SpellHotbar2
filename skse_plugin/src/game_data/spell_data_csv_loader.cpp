#include "spell_data_csv_loader.h"
#include "../logger/logger.h"
#include "../game_data/game_data.h"
#include <regex>
#include "csv_loader.h"
#include "../rendering/texture_csv_loader.h"
#include <unordered_set>

namespace SpellHotbar::SpellDataCSVLoader {

    constexpr float hours_to_days = 1.0f / (24.0f);
    constexpr float minutes_to_days = 1.0f / (24.0f * 60.0f);
    constexpr float seconds_to_days = 1.0f / (24.0f * 60.0f * 60.0f);

    // will match int and floats with optionally s,m or h at the end
    const std::regex re_time_str("(-?\\d+(\\.\\d*)?)([smh]?)");

    /**
    * Parses a time string in the form of 30s, 1.5h, 12.245m into a float representing days. (value used for gametime)
    * Timescale is NOT factored in yet.
    */
    float parse_time(const std::string& time_str)
    {
        std::string input_str = time_str;
        //some locales write , instead of .
        std::replace(input_str.begin(), input_str.end(), ',', '.');

        std::smatch m;
        std::regex_match(input_str, m, re_time_str);
        float value{0.0f};
        float factor = seconds_to_days;

        if (m.size() >= 2) {
            std::string fstr = m[1];
            value = std::stof(fstr);

            if (m.size() > 2) {
                std::string dur = m[m.size()-1];
                if (dur == "m") {
                    factor = minutes_to_days;
                } else if (dur == "h") {
                    factor = hours_to_days;
                }
            }
        } else {
            logger::error("Invalid time string: {}", input_str);
        }
        if (value > 0.0f) {
            return value * factor;   
        } else {
            //negative values or 0 are all no cooldown
            return -1.0f;
        }
    }

    bool is_spell_data_file(rapidcsv::Document& doc) {
        std::vector<std::string> columns = doc.GetColumnNames();

        // check required column names
        return csv::has_column(columns, "FormID") && csv::has_column(columns, "Plugin") && csv::has_column(columns, "Casteffect") &&
               csv::has_column(columns, "GCD") && csv::has_column(columns, "Cooldown") && csv::has_column(columns, "Casttime") &&
               csv::has_column(columns, "Animation") && csv::has_column(columns, "Animation2");
    }  

    void load_spell_data_from_csv(const std::string& path)
    {
        rapidcsv::Document doc(path, rapidcsv::LabelParams(0, -1), rapidcsv::SeparatorParams('\t'));

        // check the column names
        if (!is_spell_data_file(doc)) {
            logger::warn("Could not parse '{}', skipping", path);
        } else {
        
            bool has_symbol = csv::has_column(doc.GetColumnNames(), "Symbol");

            std::unordered_set<std::string> warned_plugins;

            for (size_t i = 0; i < doc.GetRowCount(); i++) {
                try {

                    std::string str_form = doc.GetCell<std::string>("FormID", i);
                    uint32_t form_id = static_cast<uint32_t>(std::stoul(str_form, nullptr, 16));

                    std::string plugin = doc.GetCell<std::string>("Plugin", i);

                    if (RE::TESDataHandler::GetSingleton()->GetModIndex(plugin).has_value()) {

                        auto* form = GameData::get_form_from_file(form_id, plugin);

                        if (form != nullptr) {
                            RE::FormID actual_form_id = form->GetFormID();
                            GameData::Spell_cast_data data;
                            data.gcd = doc.GetCell<float>("GCD", i);
                            std::string time_str = doc.GetCell<std::string>("Cooldown", i);
                            RE::FormID cooldown_effect = 0U;
                            if (time_str.starts_with("0x")) {
                                data.cooldown = 0.0f;
                                cooldown_effect = static_cast<uint32_t>(std::stoul(time_str, nullptr, 16));
                                auto* cd_form = GameData::get_form_from_file(cooldown_effect, plugin);
                                if (cd_form) {
                                    cooldown_effect = cd_form->GetFormID();
                                }
                                else
                                {
                                    logger::error("Could not Load {:8x} from {}", cooldown_effect, plugin);
                                    cooldown_effect = 0;
                                }
                            }
                            else {
                                data.cooldown = parse_time(time_str);
                            }
                            data.casttime = doc.GetCell<float>("Casttime", i);

                            int anim_var = std::min(doc.GetCell<int>("Animation", i), static_cast<int>(std::numeric_limits<uint16_t>::max()));
                            int anim_var2 = std::min(doc.GetCell<int>("Animation2", i), static_cast<int>(std::numeric_limits<uint16_t>::max()));

                            data.animation = GameData::chose_default_anim_for_spell(form, anim_var, false);
                            data.animation2 = GameData::chose_default_anim_for_spell(form, anim_var2, true);

                            std::string cast_effect = doc.GetCell<std::string>("Casteffect", i);
                            data.casteffectid = static_cast<uint16_t>(GameData::get_cast_effect_id(cast_effect));

                            if (has_symbol) {
                                std::string symbol = doc.GetCell<std::string>("Symbol", i);
                                if (!symbol.empty()) {
                                    if (TextureCSVLoader::default_icon_names.contains(symbol)) {
                                        data.overlay_icon = TextureCSVLoader::default_icon_names.at(symbol);
                                    }
                                    else {
                                        logger::warn("Unknown Overlay Symbol: '{}'", symbol);
                                    }
                                }
                            }

                            // skip saving default spell data
                            if (!data.is_empty()) {

                                GameData::set_spell_cast_data(actual_form_id, std::move(data));
                            }
                            if (cooldown_effect > 0U) {
                                GameData::set_spell_cooldown_effect(actual_form_id, cooldown_effect);
                            }
                        }
                        else {
                            logger::warn("Skipping spell data {} {}, because form is null", str_form, plugin);
                        }
                    }
                    else {
                        if (!warned_plugins.contains(plugin)) {
                            warned_plugins.emplace(plugin);
                            logger::warn("Skipping Plugin '{}', not loaded.", plugin);
                        }
                    }

                } catch (const std::exception& e) {
                    std::string msg = e.what();
                    logger::error("Error Loading csv: {}", msg);
                }
            }
        }
    }

    inline void _load_perk_from_game(const rapidcsv::Document & doc, const std::string & school, RE::BGSPerk** out_perk) {
        try {
            std::string str_form = doc.GetCell<std::string>("FormID", school);
            uint32_t form_id = static_cast<uint32_t>(std::stoul(str_form, nullptr, 16));

            std::string plugin = doc.GetCell<std::string>("Plugin", school);

            auto* form = GameData::get_form_from_file(form_id, plugin);
            if (form && form->GetFormType() == RE::FormType::Perk) {
                *out_perk = form->As<RE::BGSPerk>();
            } else {
                logger::error("Dual cast perk for '{}' is null!", school);
            }
        }
        catch (const std::exception& e) {
            logger::error("Could not load perk data for {}: {}", school, e.what());
        }
    }

    void load_spell_data(std::filesystem::path folder)
    {
        try {
            //load vanilla first so mods can override
            for (const auto& entry : std::filesystem::directory_iterator(folder)) {
                if (entry.is_regular_file()) {
                    std::string str_path = entry.path().string();

                    if (str_path.ends_with(".csv") && str_path.starts_with("spells_vanilla")) {
                        logger::info("Loading spelldata: {}", str_path);
                        load_spell_data_from_csv(str_path);
                    }
                }
            }
            //load modded
            for (const auto& entry : std::filesystem::directory_iterator(folder)) {
                if (entry.is_regular_file()) {
                    std::string str_path = entry.path().string();

                    if (str_path.ends_with(".csv") && !str_path.starts_with("spells_vanilla")) {
                        logger::info("Loading spelldata: {}", str_path);
                        load_spell_data_from_csv(str_path);
                    }
                }
            }
        }
        catch (const std::exception& e) {
            std::string msg = e.what();
            logger::error("Error loading spell data: {}", msg);
        }

        //load perk data
        logger::info("Loading dual cast perk data...");
        const std::string perk_data_file = ".\\data\\SKSE\\Plugins\\SpellHotbar\\perkdata\\dual_cast_perks.csv";
        rapidcsv::Document doc(perk_data_file, rapidcsv::LabelParams(0, 0), rapidcsv::SeparatorParams('\t'));

        std::vector<std::string> columns = doc.GetColumnNames();
        std::vector<std::string> rows = doc.GetRowNames();

        if (csv::has_column(columns, "FormID") && csv::has_column(columns, "Plugin") &&
            csv::has_column(rows, "Alteration") && csv::has_column(rows, "Conjuration") && csv::has_column(rows, "Destruction") &&
            csv::has_column(rows, "Illusion") && csv::has_column(rows, "Restoration")) {
           
            _load_perk_from_game(doc, "Alteration", &GameData::perk_alteration_dual_casting);
            _load_perk_from_game(doc, "Conjuration", &GameData::perk_conjuration_dual_casting);
            _load_perk_from_game(doc, "Destruction", &GameData::perk_destruction_dual_casting);
            _load_perk_from_game(doc, "Illusion", &GameData::perk_illusion_dual_casting);
            _load_perk_from_game(doc, "Restoration", &GameData::perk_restoration_dual_casting);
        }
        else {
            logger::error("Dual cast perk config file invalid: {}", perk_data_file);
        }


    }
}