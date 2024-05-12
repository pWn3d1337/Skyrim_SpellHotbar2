#include "custom_transform_csv_loader.h"
#include "csv_loader.h"
#include "game_data.h"

namespace SpellHotbar::CustomTransformCSVLoader {

        bool is_custom_transform_file(rapidcsv::Document& doc) {
        std::vector<std::string> columns = doc.GetColumnNames();

        // check required column names
        return csv::has_column(columns, "Name") && csv::has_column(columns, "RaceID") &&
               csv::has_column(columns, "Plugin") && csv::has_column(columns, "BarID") &&
               csv::has_column(columns, "Mode");
    }

    void load_custom_transformations_from_csv(const std::string& path) {
        rapidcsv::Document doc(path, rapidcsv::LabelParams(0, -1), rapidcsv::SeparatorParams('\t'));

        // check the column names
        if (!is_custom_transform_file(doc)) {
            logger::warn("Could not parse '{}', skipping", path);
        } else {
            for (size_t i = 0; i < doc.GetRowCount(); i++) {
                try {
                    std::string name = doc.GetCell<std::string>("Name", i);

                    std::string race_str = doc.GetCell<std::string>("RaceID", i);
                    uint32_t race_id = static_cast<uint32_t>(std::stoul(race_str, nullptr, 16));
                    std::string plugin = doc.GetCell<std::string>("Plugin", i);

                    uint32_t bar_id = static_cast<uint32_t>(doc.GetCell<long>("BarID", i));

                    uint8_t mode = static_cast<uint8_t>(doc.GetCell<int>("Mode", i));

                    auto race_form = GameData::get_form_from_file(race_id, plugin);
                    if (race_form && race_form->GetFormType() == RE::FormType::Race) {
                        GameData::add_custom_tranformation(bar_id, name, race_form->GetFormID(),
                                                           GameData::custom_transform_spell_type(std::clamp(mode, 0Ui8, 2Ui8)));

                        logger::info("Adding Custom Transformation {}, {}, {}, {}, {}", name, race_id, plugin, bar_id, mode);


                    } else {
                        logger::error("Could not load custom transform race {},{}, not a valid RACE form", race_id, plugin);
                    }

                } catch (const std::exception& e) {
                    std::string msg = e.what();
                    logger::error("Error Loading csv: {}", msg);
                }
            }
        }
    }

    void load_transformations(std::filesystem::path folder) {
        csv::load_folder(folder, "custom transformations", load_custom_transformations_from_csv);
    }

}