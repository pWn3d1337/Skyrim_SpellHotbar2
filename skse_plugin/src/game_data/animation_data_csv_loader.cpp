#include "animation_data_csv_loader.h"
#include "csv_loader.h"
#include "game_data.h"

namespace SpellHotbar::AnimationDataCSVLoader {
    bool is_animation_data_file(rapidcsv::Document& doc) {
        std::vector<std::string> columns = doc.GetColumnNames();

        // check required column names
        return csv::has_column(columns, "Name") && csv::has_column(columns, "Anim");
    }

    void load_animation_data_from_csv(const std::string& path) {
        rapidcsv::Document doc(path, rapidcsv::LabelParams(0, -1), rapidcsv::SeparatorParams('\t'));

        // check the column names
        if (!is_animation_data_file(doc)) {
            logger::warn("Could not parse '{}', skipping", path);
        }
        else {
            for (size_t i = 0; i < doc.GetRowCount(); i++) {
                try {

                    std::string name = doc.GetCell<std::string>("Name", i);
                    uint16_t animation = static_cast<uint16_t>(doc.GetCell<int>("Anim", i));

                    GameData::add_animation_data(name, animation);
                }
                catch (const std::exception& e) {
                    std::string msg = e.what();
                    logger::error("Error Loading csv: {}", msg);
                }
            }
        }
    }

    void load_anim_data(std::filesystem::path folder)
    {
        GameData::add_animation_data("Auto", -1);
        GameData::add_animation_data("Skyrim Shout", 0);
        csv::load_folder(folder, "animation data", load_animation_data_from_csv);
    }
}
