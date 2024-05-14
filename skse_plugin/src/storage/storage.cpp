#include "storage.h"
#include "../logger/logger.h"
#include "../rendering/render_manager.h"
#include "../bar/hotbars.h"
#include "../bar/hotbar.h"

namespace SpellHotbar::Storage {

    void SaveCallback(SKSE::SerializationInterface* a_intfc)
    {
        logger::trace("Saving to SKSE save...");
        /// main hotbars data
        if (!a_intfc->OpenRecord('HOTB', Storage::save_format))
        {
            logger::error("Could not store main hotbar settings!");
        } else {
            a_intfc->WriteRecordData(&Bars::barsize, sizeof(uint8_t));
            a_intfc->WriteRecordData(&Bars::disable_non_modifier_bar, sizeof(bool));

            a_intfc->WriteRecordData(&Bars::slot_scale, sizeof(float));
            a_intfc->WriteRecordData(&Bars::offset_x, sizeof(float));
            a_intfc->WriteRecordData(&Bars::offset_y, sizeof(float));

            uint8_t spacing = static_cast<uint8_t>(Bars::slot_spacing);
            a_intfc->WriteRecordData(&spacing, sizeof(uint8_t));

            uint8_t text_show = static_cast<uint8_t>(Bars::text_show_setting);
            a_intfc->WriteRecordData(&text_show, sizeof(uint8_t));

            uint8_t bar_show = static_cast<uint8_t>(Bars::bar_show_setting);
            a_intfc->WriteRecordData(&bar_show, sizeof(uint8_t));

            uint8_t bar_show_vl = static_cast<uint8_t>(Bars::bar_show_setting_vampire_lord);
            a_intfc->WriteRecordData(&bar_show_vl, sizeof(uint8_t));

            uint8_t bar_show_ww = static_cast<uint8_t>(Bars::bar_show_setting_werewolf);
            a_intfc->WriteRecordData(&bar_show_ww, sizeof(uint8_t));

            a_intfc->WriteRecordData(&Bars::use_default_bar_when_sheathed, sizeof(bool));
            a_intfc->WriteRecordData(&Bars::disable_menu_rendering, sizeof(bool));
        }

        for (const auto& [k, v]: SpellHotbar::Bars::hotbars)
        {
            v.serialize(a_intfc, k);
        }

        // Save GameData values
        if (!a_intfc->OpenRecord('GDAT', Storage::save_format)) {
            logger::error("Could not store game_data values!");
        } else {
            GameData::save_to_SKSE_save(a_intfc);
        }

        // Save custom spelldata if present
        if (!GameData::user_spell_cast_info.empty()) {
            if (!a_intfc->OpenRecord('SDAT', Storage::save_format)) {
                logger::error("Could not store user spell data values!");
            }
            else {
                GameData::save_user_spell_data_to_SKSE_save(a_intfc);
            }
        }

    }

    void LoadCallback(SKSE::SerializationInterface* a_intfc)
    {
        logger::trace("Loading from SKSE save...");

        //clear all bars
        SpellHotbar::Bars::clear_bars();

        uint32_t type{0};
        uint32_t version{0};
        uint32_t length{0};
        while (a_intfc->GetNextRecordInfo(type, version, length)) {
            if (type == 'HOTB')
            {
                logger::trace("Reading 'HOTB' data from save...");
                if (length != ((sizeof(bool) * 3) + (sizeof(uint8_t) * 6) + (sizeof(float)* 3) )) {
                    logger::error("Invalid Record data length for 'HOTB'");
                }
                else
                {
                    if (!a_intfc->ReadRecordData(&Bars::barsize, sizeof(uint8_t))) {
                        logger::error("Failed to read bar_size!");
                        break;
                    }
                    if (!a_intfc->ReadRecordData(&Bars::disable_non_modifier_bar, sizeof(bool))) {
                        logger::error("Failed to read disable_non_modifier_bar!");
                        break;
                    }
                    if (!a_intfc->ReadRecordData(&Bars::slot_scale, sizeof(float))) {
                        logger::error("Failed to read slot_scale!");
                        break;
                    }
                    if (!a_intfc->ReadRecordData(&Bars::offset_x, sizeof(float))) {
                        logger::error("Failed to read offset_x!");
                        break;
                    }
                    if (!a_intfc->ReadRecordData(&Bars::offset_y, sizeof(float))) {
                        logger::error("Failed to read offset_y!");
                        break;
                    }
                    uint8_t spacing{0};
                    if (!a_intfc->ReadRecordData(&spacing, sizeof(uint8_t))) {
                        logger::error("Failed to read slot_spacing!");
                        break;
                    } else {
                        Bars::slot_spacing = static_cast<int>(spacing);
                    }

                    uint8_t text_show{0};
                    if (!a_intfc->ReadRecordData(&text_show, sizeof(uint8_t))) {
                        logger::error("Failed to read text_show_setting!");
                        break;
                    } else {
                        Bars::text_show_setting = Bars::text_show_mode(std::clamp(text_show, 0Ui8, 2Ui8));
                    }

                    uint8_t bar_show{0};
                    if (!a_intfc->ReadRecordData(&bar_show, sizeof(uint8_t))) {
                        logger::error("Failed to read bar_show_setting!");
                        break;
                    } else {
                        Bars::bar_show_setting = Bars::bar_show_mode(std::clamp(bar_show, 0Ui8, 5Ui8));
                    }

                    uint8_t bar_show_vl{0};
                    if (!a_intfc->ReadRecordData(&bar_show_vl, sizeof(uint8_t))) {
                        logger::error("Failed to read bar_show_setting vampire_lord!");
                        break;
                    } else {
                        Bars::bar_show_setting_vampire_lord = Bars::bar_show_mode(std::clamp(bar_show_vl, 0Ui8, 2Ui8));
                    }

                    uint8_t bar_show_ww{0};
                    if (!a_intfc->ReadRecordData(&bar_show_ww, sizeof(uint8_t))) {
                        logger::error("Failed to read bar_show_setting werewolf!");
                        break;
                    } else {
                        Bars::bar_show_setting_werewolf = Bars::bar_show_mode(std::clamp(bar_show_ww, 0Ui8, 2Ui8));
                    }

                    if (!a_intfc->ReadRecordData(&Bars::use_default_bar_when_sheathed, sizeof(bool))) {
                        logger::error("Failed to read use_default_bar_when_sheathed!");
                        break;
                    }

                    if (!a_intfc->ReadRecordData(&Bars::disable_menu_rendering, sizeof(bool))) {
                        logger::error("Failed to read disable_menu_rendering!");
                        break;
                    }
                }
            }
            else if (type =='GDAT')
            {
                logger::trace("Reading 'GDAT' data from save...");
                GameData::load_from_SKSE_save(a_intfc);
            }
            else if (type == 'SDAT') {
                logger::trace("Reading 'SDAT' data from save...");
                GameData::load_user_spell_data_from_SKSE_save(a_intfc, version);
            }
            else if (SpellHotbar::Bars::hotbars.contains(type))
            {
                SpellHotbar::Bars::hotbars.at(type).deserialize(a_intfc, type, version, length);
            }
            else
            {
                logger::warn("Unknown Record Type: {}", type);
            }

        }
    }

    bool slotSpell_internal(RE::FormID form, size_t index, uint32_t bar_id)
    {
        
        if (Bars::hotbars.contains(bar_id)) {
            try {
                auto& bar = Bars::hotbars.at(bar_id);

                bar.slot_spell(index, form, Bars::get_current_modifier());
                RenderManager::highlight_skill_slot(static_cast<int>(index));
                return true;
            } catch (std::exception& e) {
                std::string msg = e.what();
                logger::error("C++ Exception: {}", msg);
                return false;
            }
        }
        return false;
    }

    bool slotSpell(RE::FormID form, size_t index, menu_slot_type slot_type)
    {
        uint32_t bar;
        switch (slot_type) {
            case SpellHotbar::Storage::menu_slot_type::vampire_lord:
                bar = Bars::VAMPIRE_LORD_BAR;
                break;
            case SpellHotbar::Storage::menu_slot_type::werewolf:
                bar = Bars::WEREWOLF_BAR;
                break;
            case SpellHotbar::Storage::menu_slot_type::custom_favmenu:
                bar = GameData::isCustomTransform();
                break;
            case SpellHotbar::Storage::menu_slot_type::magic_menu:
            default:
                bar = Bars::menu_bar_id;
                break;
        }

        return slotSpell_internal(form, index, bar);
    }

}