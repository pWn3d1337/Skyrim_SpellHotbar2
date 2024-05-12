#include "spell_casteffect_csv_loader.h"
#include <rapidcsv.h>
#include "csv_loader.h"
#include "../logger/logger.h"
#include "game_data.h"

namespace SpellHotbar::SpellCastEffectCSVLoader {

    bool is_spell_effect_file(rapidcsv::Document& doc) {
        std::vector<std::string> columns = doc.GetColumnNames();

        // check required column names
        return csv::has_column(columns, "Key") &&
               csv::has_column(columns, "FormID_L") && csv::has_column(columns, "Plugin_L") &&
               csv::has_column(columns, "FormID_R") && csv::has_column(columns, "Plugin_R");
    }

    void load_spell_casteffects_from_csv(const std::string& path) {
        rapidcsv::Document doc(path, rapidcsv::LabelParams(0, -1), rapidcsv::SeparatorParams('\t'));

        // check the column names
        if (!is_spell_effect_file(doc)) {
            logger::warn("Could not parse '{}', skipping", path);
        } else {
            for (size_t i = 0; i < doc.GetRowCount(); i++) {
                try {
                    std::string key = doc.GetCell<std::string>("Key", i);

                    std::string str_form_l = doc.GetCell<std::string>("FormID_L", i);
                    uint32_t form_id_l = static_cast<uint32_t>(std::stoul(str_form_l, nullptr, 16));
                    std::string plugin_l = doc.GetCell<std::string>("Plugin_L", i);

                    std::string str_form_r = doc.GetCell<std::string>("FormID_R", i);
                    uint32_t form_id_r = static_cast<uint32_t>(std::stoul(str_form_r, nullptr, 16));
                    std::string plugin_r = doc.GetCell<std::string>("Plugin_R", i);


                    auto* form_l = GameData::get_form_from_file(form_id_l, plugin_l);
                    auto* form_r = GameData::get_form_from_file(form_id_r, plugin_r);

                    RE::BGSArtObject* left_art{nullptr};
                    RE::BGSArtObject* right_art{nullptr};

                    if (form_l && form_l->GetFormType() == RE::FormType::ArtObject) {
                        left_art = form_l->As<RE::BGSArtObject>();
                    }
                    if (form_r && form_r->GetFormType() == RE::FormType::ArtObject) {
                        right_art = form_r->As<RE::BGSArtObject>();
                    }
                    GameData::add_casteffect(key, left_art, right_art);

                } catch (const std::exception& e) {
                    std::string msg = e.what();
                    logger::error("Error Loading csv: {}", msg);
                }
            }
        }
    }

    void load_spell_casteffects(std::filesystem::path folder)
    {
        GameData::add_casteffect("No Effect", nullptr, nullptr);  // add null at index0
        csv::load_folder(folder, "spell casteffects", load_spell_casteffects_from_csv);
    }
}