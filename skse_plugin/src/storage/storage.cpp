#include "storage.h"
#include "../logger/logger.h"
#include "../rendering/render_manager.h"
#include "../bar/hotbars.h"
#include "../bar/hotbar.h"
#include "../input/keybinds.h"
#include "../input/modes.h"

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

            float offset_x_out = RenderManager::scale_from_resolution(Bars::offset_x);
            a_intfc->WriteRecordData(&offset_x_out, sizeof(float));

            float offset_y_out = RenderManager::scale_from_resolution(Bars::offset_y);
            a_intfc->WriteRecordData(&offset_y_out, sizeof(float));

            float spacing_out = RenderManager::scale_from_resolution(Bars::slot_spacing);
            a_intfc->WriteRecordData(&spacing_out, sizeof(float));

            uint8_t anchor = static_cast<uint8_t>(Bars::bar_anchor_point);
            a_intfc->WriteRecordData(&anchor, sizeof(uint8_t));

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
            a_intfc->WriteRecordData(&Bars::disable_non_modifier_bar, sizeof(bool));

            a_intfc->WriteRecordData(&GameData::potion_gcd, sizeof(float));

            //oblivion bar
            a_intfc->WriteRecordData(&Bars::oblivion_slot_scale, sizeof(float));

            float oblivion_offset_x_out = RenderManager::scale_from_resolution(Bars::oblivion_offset_x);
            a_intfc->WriteRecordData(&oblivion_offset_x_out, sizeof(float));

            float oblivion_offset_y_out = RenderManager::scale_from_resolution(Bars::oblivion_offset_y);
            a_intfc->WriteRecordData(&oblivion_offset_y_out, sizeof(float));

            float oblivion_spacing_out = RenderManager::scale_from_resolution(Bars::oblivion_slot_spacing);
            a_intfc->WriteRecordData(&oblivion_spacing_out, sizeof(float));

            uint8_t oblivion_anchor = static_cast<uint8_t>(Bars::oblivion_bar_anchor_point);
            a_intfc->WriteRecordData(&oblivion_anchor, sizeof(uint8_t));

            a_intfc->WriteRecordData(&Bars::oblivion_bar_show_power, sizeof(bool));

            uint8_t input_mode = static_cast<uint8_t>(Input::get_current_mode_index());
            a_intfc->WriteRecordData(&input_mode, sizeof(uint8_t));

            a_intfc->WriteRecordData(&Bars::bar_row_len, sizeof(uint8_t));

            uint8_t bar_layout = static_cast<uint8_t>(Bars::layout);
            a_intfc->WriteRecordData(&bar_layout, sizeof(uint8_t));

            a_intfc->WriteRecordData(&Bars::bar_circle_radius, sizeof(float));

            //write keybinds, make saves compatible when new binds are added
            uint8_t num_keybinds = static_cast<uint8_t>(Input::keybind_id::num_keys);
            a_intfc->WriteRecordData(&num_keybinds, sizeof(uint8_t));

            for (uint8_t i = 0U; i < num_keybinds; i++) {
                int16_t key = static_cast<int16_t>(Input::get_keybind(i)); //key can be -1 and >255, we need 2bytes
                a_intfc->WriteRecordData(&key, sizeof(int16_t));
            }

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

    template <typename T>
    inline bool read_clamped(SKSE::SerializationInterface* a_intfc, const std::string & name, T & target, T min, T max) {
        T read_value{};
        if (!a_intfc->ReadRecordData(&read_value, sizeof(T))) {
            logger::error("Failed to read '{}'!", name);
            return false;
        }
        else {
            target = std::clamp(read_value, min, max);
        }
        return true;
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
                //HOTB is now variable length
                logger::trace("Reading 'HOTB' data from save...");
                //if (length != ((sizeof(bool) * 3) + (sizeof(uint8_t) * 6) + (sizeof(float)* 3) )) {
                //    logger::error("Invalid Record data length for 'HOTB'");
                //}
                //else
                //{
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
                float read_offset_x{ 0.0f };
                if (!a_intfc->ReadRecordData(&read_offset_x, sizeof(float))) {
                    logger::error("Failed to read offset_x!");
                    break;
                }
                else {
                    Bars::offset_x = RenderManager::scale_to_resolution(read_offset_x);
                }
                float read_offset_y{ 0.0f };
                if (!a_intfc->ReadRecordData(&read_offset_y, sizeof(float))) {
                    logger::error("Failed to read offset_y!");
                    break;
                }
                else {
                    Bars::offset_y = RenderManager::scale_to_resolution(read_offset_y);
                }
                float read_spacing{0};
                if (!a_intfc->ReadRecordData(&read_spacing, sizeof(float))) {
                    logger::error("Failed to read slot_spacing!");
                    break;
                } else {
                    Bars::slot_spacing = RenderManager::scale_to_resolution(std::max(0.0f, read_spacing));
                }

                uint8_t anchor{0};
                if (!a_intfc->ReadRecordData(&anchor, sizeof(uint8_t))) {
                    logger::error("Failed to read bar anchor point!");
                    break;
                }
                else {
                    Bars::bar_anchor_point = Bars::anchor_point(std::clamp(anchor, 0Ui8, static_cast<uint8_t>(Bars::anchor_point::CENTER)));
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

                if (!a_intfc->ReadRecordData(&Bars::disable_non_modifier_bar, sizeof(bool))) {
                    logger::error("Failed to read disable_non_modifier_bar!");
                    break;
                }

                if (!a_intfc->ReadRecordData(&GameData::potion_gcd, sizeof(float))) {
                    logger::error("Failed to read potion gcd!");
                    break;
                }

                // oblivion bar setting
                if (!a_intfc->ReadRecordData(&Bars::oblivion_slot_scale, sizeof(float))) {
                    logger::error("Failed to read oblivion bar slot_scale!");
                    break;
                }
                float read_oblivion_offset_x{ 0.0f };
                if (!a_intfc->ReadRecordData(&read_oblivion_offset_x, sizeof(float))) {
                    logger::error("Failed to read oblivion bar offset_x!");
                    break;
                }
                else {
                    Bars::oblivion_offset_x = RenderManager::scale_to_resolution(read_oblivion_offset_x);
                }
                float read_oblivion_offset_y{ 0.0f };
                if (!a_intfc->ReadRecordData(&read_oblivion_offset_y, sizeof(float))) {
                    logger::error("Failed to read oblivion bar offset_y!");
                    break;
                }
                else {
                    Bars::oblivion_offset_y = RenderManager::scale_to_resolution(read_oblivion_offset_y);
                }
                float read_oblivion_spacing{ 0 };
                if (!a_intfc->ReadRecordData(&read_oblivion_spacing, sizeof(float))) {
                    logger::error("Failed to read oblivion bar slot_spacing!");
                    break;
                }
                else {
                    Bars::oblivion_slot_spacing = RenderManager::scale_to_resolution(std::max(0.0f, read_oblivion_spacing));
                }

                uint8_t oblivion_anchor{ 0 };
                if (!a_intfc->ReadRecordData(&oblivion_anchor, sizeof(uint8_t))) {
                    logger::error("Failed to read bar oblivion bar anchor point!");
                    break;
                }
                else {
                    Bars::oblivion_bar_anchor_point = Bars::anchor_point(std::clamp(oblivion_anchor, 0Ui8, static_cast<uint8_t>(Bars::anchor_point::CENTER)));
                }
                if (!a_intfc->ReadRecordData(&Bars::oblivion_bar_show_power, sizeof(bool))) {
                    logger::error("Failed to read oblivion_bar_show_power!");
                    break;
                }

                uint8_t input_mode{ 0 };
                if (!a_intfc->ReadRecordData(&input_mode, sizeof(uint8_t))) {
                    logger::error("Failed to read input mode!");
                    break;
                }
                else {
                    SpellHotbar::Input::set_input_mode(static_cast<int>(input_mode));
                }

                uint8_t read_row_len{ 0 };
                if (!a_intfc->ReadRecordData(&read_row_len, sizeof(uint8_t))) {
                    logger::error("Failed to read bar_row_len!");
                    break;
                }
                else {
                    Bars::bar_row_len = std::clamp(read_row_len, 1Ui8, static_cast<uint8_t>(max_bar_size));
                }

                uint8_t read_bar_layout{ 0 };
                if (!a_intfc->ReadRecordData(&read_bar_layout, sizeof(uint8_t))) {
                    logger::error("Failed to read bar_layout!");
                    break;
                }
                else {
                    Bars::layout = Bars::bar_layout(std::clamp(read_bar_layout, 0Ui8, 2Ui8));
                }

                float read_bar_circle_radius{ 0 };
                if (!a_intfc->ReadRecordData(&read_bar_circle_radius, sizeof(float))) {
                    logger::error("Failed to read bar_circle_radius!");
                    break;
                }
                else {
                    Bars::bar_circle_radius = std::clamp(read_bar_circle_radius, 0.1f, 10.0f);
                }


                //read num keybinds, make saves compatible when new binds are added
                uint8_t num_keybinds{ 0U };
                a_intfc->ReadRecordData(&num_keybinds, sizeof(uint8_t));

                for (uint8_t i = 0U; i < num_keybinds; i++) {
                    int16_t key{ -1 };
                    a_intfc->ReadRecordData(&key, sizeof(int16_t));
                    Input::rebind_key(i, key, false);
                }
                //assign keys with -1 that might have not been saved due older version
                while (num_keybinds < static_cast<uint8_t>(Input::keybind_id::num_keys)) {
                    Input::rebind_key(num_keybinds, -1, false);
                    num_keybinds++;
                }
                //}
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