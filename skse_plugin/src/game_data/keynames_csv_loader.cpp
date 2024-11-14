#include "keynames_csv_loader.h"
#include "game_data.h"
#include "csv_loader.h"

namespace SpellHotbar::KeyNamesCSVLoader {

bool is_keynames_file(rapidcsv::Document& doc) {
    std::vector<std::string> columns = doc.GetColumnNames();

    // check required column names
    return csv::has_column(columns, "Code") && csv::has_column(columns, "Text") && csv::has_column(columns, "Short");
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

                GameData::key_names.emplace(code, std::make_pair(text, text_short));
            }
            catch (const std::exception& e) {
                std::string msg = e.what();
                logger::error("Error Loading csv: {}", msg);
            }
        }
    }
}
}
