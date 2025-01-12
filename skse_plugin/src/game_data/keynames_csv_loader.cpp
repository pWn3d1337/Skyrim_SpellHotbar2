#include "keynames_csv_loader.h"
#include "game_data.h"
#include "csv_loader.h"
#include "rendering/render_manager.h"

namespace SpellHotbar::KeyNamesCSVLoader {

bool is_keynames_file(rapidcsv::Document& doc) {
    std::vector<std::string> columns = doc.GetColumnNames();

    // check required column names
    return csv::has_column(columns, "Code") && csv::has_column(columns, "Text") && csv::has_column(columns, "Short") && csv::has_column(columns, "IconPath");
}

void load_keynames(const std::string & keynames_csv)
{
    auto keynames_path = std::filesystem::path(keynames_csv);
    if (!std::filesystem::exists(keynames_path)) {
        logger::error("Missing '{}', this is an installation error!", keynames_csv);
        return;
    }

    rapidcsv::Document doc(keynames_csv, rapidcsv::LabelParams(0, -1), rapidcsv::SeparatorParams('\t'));

    // check the column names
    if (!is_keynames_file(doc)) {
        logger::error("Could not load '{}'!", keynames_csv);
    }
    else {
        for (size_t i = 0; i < doc.GetRowCount(); i++) {
            try {

                int code = doc.GetCell<int>("Code", i);
                std::string text = doc.GetCell<std::string>("Text", i);
                std::string text_short = doc.GetCell<std::string>("Short", i);

                std::string icon_path = doc.GetCell<std::string>("IconPath", i);

                int tex_idx = RenderManager::load_texture_return_index("./data/" + icon_path);

                GameData::key_names.emplace(code, GameData::Key_Data(text_short, text, tex_idx));
            }
            catch (const std::exception& e) {
                std::string msg = e.what();
                logger::error("Error Loading csv: {}", msg);
            }
        }
    }
}
}
