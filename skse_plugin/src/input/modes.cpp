#include "modes.h"
#include "../logger/logger.h"
#include "../rendering/render_manager.h"
#include "../casts/casting_controller.h"

namespace SpellHotbar::Input {

    InputModeBase* InputModeBase::current_mode = InputModeCast::getSingleton();

    bool is_oblivion_mode()
    {
        return InputModeBase::current_mode == InputModeOblivion::getSingleton();
    }

    bool is_equip_mode()
    {
        return InputModeBase::current_mode == InputModeEquip::getSingleton();
    }

    int get_current_mode_index()
    {
        if (InputModeBase::current_mode == InputModeOblivion::getSingleton()) {
            return 2;
        }
        else if (InputModeBase::current_mode == InputModeEquip::getSingleton()) {
            return 1;
        }
        return 0;
    }

    void set_input_mode(int index)
    {
        if (index == 2) {
            InputModeBase::current_mode = InputModeOblivion::getSingleton();
        }
        else if (index == 1) {
            InputModeBase::current_mode = InputModeEquip::getSingleton();
        }
        else {
            InputModeBase::current_mode = InputModeCast::getSingleton();
        }
    }

	void InputModeCast::process_input(SlottedSkill& skill, RE::InputEvent*& addEvent, size_t& i, const KeyBind& bind, RE::INPUT_DEVICE& shoutKeyDev, uint8_t& shoutKey)
	{
        bool allowed = allowed_to_instantcast(skill.formID);
        bool can_start = casts::CastingController::can_start_new_cast();

        if (allowed_to_instantcast(skill.formID) && casts::CastingController::can_start_new_cast()) {
            if (skill.formID > 0) {
                auto form = RE::TESForm::LookupByID(skill.formID);

                if (skill.type == slot_type::spell) {
                    if (allowed_to_cast(skill.formID)) {
                        bool success = casts::CastingController::try_start_cast(form, bind, i, skill.hand);
                        SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.5, !success);
                    }
                    else {
                        SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.5, true);
                    }
                }
                else if (skill.type == slot_type::shout || skill.type == slot_type::lesser_power || skill.type == slot_type::power)
                {
                    bool can_start{ true };
                    if (skill.type == slot_type::shout) {
                        if (!allowed_to_cast(skill.formID)) can_start = false;
                    }

                    if (can_start) {
                        //Start a shout
                        if (!addEvent) { //do not accidentaly create another event

                            if (casts::CastingController::try_cast_power(form, bind, i, skill.hand)) {
                                addEvent = RE::ButtonEvent::Create(shoutKeyDev, "Shout", shoutKey, 1.0f, 0.0f); //default shout key
                            }
                        }
                    }
                    else {
                        SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.5, true);
                    }
                }
                else if (skill.type == slot_type::potion) {
                    bool success = casts::CastingController::try_start_cast(form, bind, i, skill.hand);
                    SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.5, !success);
                }
            }
            else {
                //slot not bound
                SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.25, true);
            }
        }
        else {
            //error highlight
            SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.5, true);
        }
    }
    InputModeCast* InputModeCast::getSingleton()
    {
        static InputModeCast instance;
        return &instance;
    }

    void InputModeEquip::process_input(SlottedSkill& skill, RE::InputEvent*& addEvent, size_t& i, const KeyBind& bind, RE::INPUT_DEVICE&, uint8_t&)
    {
        auto pc = RE::PlayerCharacter::GetSingleton();
        if (pc && allowed_to_instantcast(skill.formID)) {
            if (skill.formID > 0) {
                auto form = RE::TESForm::LookupByID(skill.formID);

                if (skill.type == slot_type::spell || skill.type == slot_type::lesser_power || skill.type == slot_type::power) {
                    RE::BGSEquipSlot* slot = nullptr;

                    switch (skill.hand) {
                    case hand_mode::auto_hand:
                    case hand_mode::left_hand:
                        slot = GameData::equip_slot_left_hand;
                        break;
                    case hand_mode::right_hand:
                        slot = GameData::equip_slot_right_hand;
                        break;
                    case hand_mode::dual_hand:
                        slot = GameData::equip_slot_both_hand;
                        break;
                    case hand_mode::voice:
                        slot = GameData::equip_slot_voice;
                        break;
                    }

                    RE::ActorEquipManager::GetSingleton()->EquipSpell(pc, form->As<RE::SpellItem>(), slot);
                    SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.25);
                }
                else if (skill.type == slot_type::shout)
                {
                    RE::ActorEquipManager::GetSingleton()->EquipShout(pc, form->As<RE::TESShout>());
                    SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.25);
                }
                else if (skill.type == slot_type::potion) {
                    if (casts::CastingController::can_start_new_cast()) {
                        bool success = casts::CastingController::try_start_cast(form, bind, i, skill.hand);
                        SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.5, !success);
                    }
                    else {
                        //Potion on cd (cast still running)
                        SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.25, true);
                    }
                }
            }
            else {
                //slot not bound
                SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.25, true);
            }
        }
        else {
            //error highlight
            SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.5, true);
        }
    }
    InputModeEquip* InputModeEquip::getSingleton()
    {
        static InputModeEquip instance;
        return &instance;
    }

    void InputModeOblivion::process_input(SlottedSkill& skill, RE::InputEvent*& addEvent, size_t& i, const KeyBind& bind, RE::INPUT_DEVICE& shoutKeDev, uint8_t& shoutKey)
    {
        if (i == Input::keybind_id::oblivion_cast || i == Input::keybind_id::oblivion_potion) {
            //logger::info("Start Regular Cast");
            InputModeCast::getSingleton()->process_input(skill, addEvent, i, bind, shoutKeDev, shoutKey);
        }
        else {
            auto pc = RE::PlayerCharacter::GetSingleton();
            if (pc) {
                if (skill.formID > 0) {
                    auto form = RE::TESForm::LookupByID(skill.formID);

                    if (skill.type == slot_type::spell) {
                        GameData::oblivion_bar.set_spell(skill);
                    }
                    else if (skill.type == slot_type::shout)
                    {
                        RE::ActorEquipManager::GetSingleton()->EquipShout(pc, form->As<RE::TESShout>());
                    }
                    else if (skill.type == slot_type::lesser_power || skill.type == slot_type::power) {
                        RE::ActorEquipManager::GetSingleton()->EquipSpell(pc, form->As<RE::SpellItem>(), GameData::equip_slot_voice);
                    }
                    else if (skill.type == slot_type::potion) {
                        GameData::oblivion_bar.set_potion(skill);
                    }
                }
                else {
                    //slot not bound
                    SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.25, true);
                }
            }
            else {
                //error highlight
                SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.5, true);
            }
        }
    }
    void InputModeOblivion::process_key_update(const KeyBind& bind, size_t i, float held_down_sec)
    {
        //Only called for main keys (0-11), no check needed
        constexpr float show_time = 1.0f;

        if (Bars::oblivion_bar_held_show_time_threshold > 0.0f) {
            if (Bars::oblivion_bar_press_show_timer > 0.0f) {
                Bars::oblivion_bar_press_show_timer = show_time;
            }
            else if (held_down_sec >= Bars::oblivion_bar_held_show_time_threshold) {
                Bars::oblivion_bar_press_show_timer = show_time;
            }
        }
    }

    InputModeOblivion* InputModeOblivion::getSingleton()
    {
        static InputModeOblivion instance;
        return &instance;
    }

    void InputModeVampireLord::process_input(SlottedSkill& skill, RE::InputEvent*& addEvent, size_t& i, const KeyBind& bind, RE::INPUT_DEVICE& shoutKeyDev, uint8_t& shoutKey)
    {
        auto pc = RE::PlayerCharacter::GetSingleton();
        if (pc && allowed_to_instantcast(skill.formID)) {
            if (skill.formID > 0) {
                auto form = RE::TESForm::LookupByID(skill.formID);

                if (skill.type == slot_type::spell) {

                    RE::BGSEquipSlot* equip_slot = GameData::equip_slot_right_hand;
                    if (skill.hand == left_hand) {
                        equip_slot = GameData::equip_slot_left_hand;
                    }

                    RE::ActorEquipManager::GetSingleton()->EquipSpell(pc, form->As<RE::SpellItem>(), equip_slot);
                    SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.25);

                } else if(skill.type == slot_type::lesser_power || skill.type == slot_type::power || skill.type == slot_type::shout) {
                    bool can_start{ true };
                    if (skill.type == slot_type::shout) {
                        if (!allowed_to_cast(skill.formID)) can_start = false;
                    }

                    if (can_start) {
                        //Start a shout
                        if (!addEvent) { //do not accidentaly create another event

                            if (casts::CastingController::try_cast_power(form, bind, i, skill.hand)) {
                                addEvent = RE::ButtonEvent::Create(shoutKeyDev, "Shout", shoutKey, 1.0f, 0.0f); //default shout key
                            }
                        }
                    }
                    else {
                        SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.5, true);
                    }
                }
                else if (skill.type == slot_type::potion) {
                    bool success = casts::CastingController::try_start_cast(form, bind, i, skill.hand);
                    SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.5, !success);
                }
            }
        }
        else {
            //error highlight
            SpellHotbar::RenderManager::highlight_skill_slot(static_cast<int>(i), 0.5, true);
        }
    }

    InputModeVampireLord* InputModeVampireLord::getSingleton()
    {
        static InputModeVampireLord instance;
        return &instance;
    }

    void InputModeBase::process_key_update(const KeyBind& bind, size_t i, float held_down_sec)
    {
        //do nothing
    }

}