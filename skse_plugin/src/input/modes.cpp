#include "modes.h"
#include "../logger/logger.h"
#include "../rendering/render_manager.h"
#include "../casts/casting_controller.h"

namespace SpellHotbar::Input {

    InputModeBase* InputModeBase::current_mode = InputModeCast::getSingleton();



	void InputModeCast::process_input(SlottedSkill& skill, RE::InputEvent*& addEvent, size_t& i, const KeyBind& bind, uint32_t& shoutkey)
	{
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
                                addEvent = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kKeyboard, "Shout", shoutkey, 1.0f, 0.0f); //default shout key
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

    void InputModeEquip::process_input(SlottedSkill& skill, RE::InputEvent*& addEvent, size_t& i, const KeyBind& bind, uint32_t& shoutkey)
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
                    if (casts::CastingController::can_start_new_cast) {
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

    void InputModeOblivion::process_input(SlottedSkill& skill, RE::InputEvent*& addEvent, size_t& i, const KeyBind& bind, uint32_t& shoutkey)
    {
        auto pc = RE::PlayerCharacter::GetSingleton();
        if (pc) {
            if (skill.formID > 0) {
                auto form = RE::TESForm::LookupByID(skill.formID);

                if (skill.type == slot_type::spell) {
                 
                }
                else if (skill.type == slot_type::shout || skill.type == slot_type::lesser_power || skill.type == slot_type::power)
                {

                }
                else if (skill.type == slot_type::potion) {
                  
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
    InputModeOblivion* InputModeOblivion::getSingleton()
    {
        static InputModeOblivion instance;
        return &instance;
    }
}