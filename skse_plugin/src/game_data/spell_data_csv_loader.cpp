#include "spell_data_csv_loader.h"
#include "../logger/logger.h"
#include "../game_data/game_data.h"
#include <regex>
#include "csv_loader.h"

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
        
            for (size_t i = 0; i < doc.GetRowCount(); i++) {
                try {

                    std::string str_form = doc.GetCell<std::string>("FormID", i);
                    uint32_t form_id = static_cast<uint32_t>(std::stoul(str_form, nullptr, 16));

                    std::string plugin = doc.GetCell<std::string>("Plugin", i);

                    auto* form = GameData::get_form_from_file(form_id, plugin);

                    if (form != nullptr) {
                        RE::FormID actual_form_id = form->GetFormID();
                        GameData::Spell_cast_data data;
                        data.gcd = doc.GetCell<float>("GCD", i);
                        std::string time_str = doc.GetCell<std::string>("Cooldown", i);
                        data.cooldown = parse_time(time_str);
                        data.casttime = doc.GetCell<float>("Casttime", i);

                        int anim_var = std::min(doc.GetCell<int>("Animation", i), static_cast<int>(std::numeric_limits<uint16_t>::max()));
                        int anim_var2 = std::min(doc.GetCell<int>("Animation2", i), static_cast<int>(std::numeric_limits<uint16_t>::max()));

                        data.animation = GameData::chose_default_anim_for_spell(form, anim_var, false);
                        data.animation2 = GameData::chose_default_anim_for_spell(form, anim_var2, true);

                        std::string cast_effect = doc.GetCell<std::string>("Casteffect", i);
                        data.casteffectid = static_cast<uint16_t>(GameData::get_cast_effect_id(cast_effect));

                        // skip saving default spell data
                        if (!data.is_empty()) {

                            GameData::set_spell_cast_data(actual_form_id, std::move(data));
                        }
                    } else {
                        logger::warn("Skipping spell data {} {}, because form is null", str_form, plugin);
                    }

                } catch (const std::exception& e) {
                    std::string msg = e.what();
                    logger::error("Error Loading csv: {}", msg);
                }
            }
        }
    }

    void load_spell_data(std::filesystem::path folder)
    {
        try {
            for (const auto& entry : std::filesystem::directory_iterator(folder)) {
                if (entry.is_regular_file()) {
                    std::string str_path = entry.path().string();

                    if (str_path.ends_with(".csv")) {
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
    }
}