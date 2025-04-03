#include "eventlistener.h"
#include "../logger/logger.h"
#include "../game_data/game_data.h"
#include "../casts/casting_controller.h"
#include "../casts/spell_proc.h"

namespace SpellHotbar::events {

    EventListener* EventListener::GetSingleton() {
        static EventListener instance;
        return &instance;
    }

    inline bool is_shout_variation(RE::TESShout* shout, RE::SpellItem* spell, RE::TESShout::VariationID variation) {
        return shout->variations[variation].spell == spell;
    }

    RE::BSEventNotifyControl EventListener::ProcessEvent(const RE::TESSpellCastEvent* event,
                                                         RE::BSTEventSource<RE::TESSpellCastEvent>*) {
        if (event) {
            if (event->object && (event->object->IsPlayerRef() || event->object->IsPlayer()))
            {
                auto form = RE::TESForm::LookupByID(event->spell);
                if (form) {                
                    if (form->GetFormType() == RE::FormType::Spell) {
                        RE::SpellItem* spell = form->As<RE::SpellItem>();
                        if (spell) {
                            if (spell->GetSpellType() == RE::MagicSystem::SpellType::kPower)
                            {
                                GameData::add_gametime_cooldown(event->spell, 24.0, false);
                                casts::CastingController::try_finish_power_cast(event->spell);
                            }
                            else if (spell->GetSpellType() == RE::MagicSystem::SpellType::kLesserPower)
                            {
                                RE::FormID id = spell->GetFormID();
                                if (SpellHotbar::GameData::spell_cast_info.contains(id)) {
                                    const auto& data = SpellHotbar::GameData::spell_cast_info.at(id);
                                    if (data.cooldown > 0.0f) {
                                        SpellHotbar::GameData::add_gametime_cooldown_with_timescale(id, data.cooldown, true);
                                    }
                                }
                                casts::CastingController::try_finish_power_cast(event->spell);
                            }
                            else if (GameData::individual_shout_cooldowns) {
                                auto pc = RE::PlayerCharacter::GetSingleton();
                                if (pc != nullptr) {
                                    auto& dat = pc->GetActorRuntimeData();
                                    if (dat.selectedPower != nullptr && dat.selectedPower->GetFormType() == RE::FormType::Shout) {

                                        //Check if the current casted spell is part of the shout
                                        RE::TESShout* shout = dat.selectedPower->As<RE::TESShout>();
                                        if (shout != nullptr) {
                                            float recovery_time = -1.0f;
                                            if (shout->variations[RE::TESShout::VariationID::kOne].spell == spell) {
                                                recovery_time = shout->variations[RE::TESShout::VariationID::kOne].recoveryTime;
                                            }
                                            else if (shout->variations[RE::TESShout::VariationID::kTwo].spell == spell) {
                                                recovery_time = shout->variations[RE::TESShout::VariationID::kTwo].recoveryTime;
                                            }
                                            else if (shout->variations[RE::TESShout::VariationID::kThree].spell == spell) {
                                                recovery_time = shout->variations[RE::TESShout::VariationID::kThree].recoveryTime;
                                            }
                                            if (recovery_time > 0.0f) {
                                                float shout_recovery_mult = pc->AsActorValueOwner()->GetActorValue(RE::ActorValue::kShoutRecoveryMult);
                                                shout_recovery_mult = std::max(0.0f, shout_recovery_mult); //clamp negative values to 0

                                                float cooldown_s = recovery_time * shout_recovery_mult;
                                                if (cooldown_s > 0.0f) {
                                                    constexpr float seconds_to_days = 1.0f / (60.0f * 60.0f * 24.0f);

                                                    SpellHotbar::GameData::add_gametime_cooldown_with_timescale(shout->GetFormID(), cooldown_s * seconds_to_days, true);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    /*RE::BSEventNotifyControl EventListener::ProcessEvent(const RE::BSAnimationGraphEvent* event, RE::BSTEventSource<RE::BSAnimationGraphEvent>*)
    {
        if (event) {
            logger::info("AnimationGraphEvent: {}, {}, {}", std::string(event->tag), event->holder->GetFormID(), std::string(event->payload));
        
        }

        return RE::BSEventNotifyControl::kContinue;
    }*/

    RE::BSEventNotifyControl EventListener::ProcessEvent(const SKSE::ActionEvent* event, RE::BSTEventSource<SKSE::ActionEvent>*)
    {
        if (event->type == SKSE::ActionEvent::Type::kVoiceFire) {
            if (event->sourceForm && event->sourceForm->GetFormType() == RE::FormType::Shout) {
                RE::TESShout* shout = event->sourceForm->As<RE::TESShout>();
                if (shout && (shout->formFlags & RE::TESShout::RecordFlags::kTreatSpellsAsPowers))
                {
                    GameData::add_gametime_cooldown(shout->GetFormID(), 24.0, false);
                }
                casts::CastingController::try_finish_shout_cast(event->sourceForm->GetFormID());
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl EventListener::ProcessEvent(const RE::TESHitEvent* event, RE::BSTEventSource<RE::TESHitEvent>*)
    {
        if (event->cause){
            if (event->cause->IsPlayerRef()) {
                if (event->target && event->target->GetFormType() == RE::FormType::ActorCharacter && !event->target->IsDead() && !event->target->IsDisabled() && !event->target->IsDeleted()) {
                    //if (event->target) {
                    //    logger::info("Target: {:08x}, FormType: {}", event->target->GetFormID(), static_cast<int>(event->target->GetFormType()));
                    //}
                    //logger::info("Weapon: {:08x}", event->source);
                    //logger::info("Flags: {:8b}", static_cast<int>(event->flags.get()));

                    if (event->source != 0) {
                        auto form = RE::TESForm::LookupByID(event->source);
                        if (form) {
                            if (form->GetFormType() == RE::FormType::Weapon) {
                                auto weap = form->As<RE::TESObjectWEAP>();
                                if (weap) {

                                    //logger::info("Flags: {:08b}", event->flags.underlying());
                                    switch (weap->GetWeaponType()) {
                                    case RE::WEAPON_TYPE::kOneHandAxe:
                                    case RE::WEAPON_TYPE::kOneHandMace:
                                    case RE::WEAPON_TYPE::kOneHandSword:
                                    case RE::WEAPON_TYPE::kOneHandDagger:
                                    case RE::WEAPON_TYPE::kTwoHandAxe:
                                    case RE::WEAPON_TYPE::kTwoHandSword:
                                    case RE::WEAPON_TYPE::kHandToHandMelee:

                                        if (event->flags.all(RE::TESHitEvent::Flag::kPowerAttack)) {
                                            if (GameData::player_has_trigger_perk(GameData::spellhotbar_perk_cast_on_power_attack) &&
                                                GameData::calc_random_proc(GameData::global_spellhotbar_perks_power_attack_trigger_chance)) {
                                                SpellHotbar::casts::SpellProc::trigger_spellproc();
                                            }
                                            return RE::BSEventNotifyControl::kContinue;
                                        }
                                    case RE::WEAPON_TYPE::kBow:
                                    case RE::WEAPON_TYPE::kCrossbow:
                                    default:
                                        if (event->flags.all(RE::TESHitEvent::Flag::kSneakAttack)) {
                                            if (GameData::player_has_trigger_perk(GameData::spellhotbar_perk_cast_on_sneak_attack) &&
                                                GameData::calc_random_proc(GameData::global_spellhotbar_perks_sneak_attack_trigger_chance)) {
                                                SpellHotbar::casts::SpellProc::trigger_spellproc();
                                            }
                                            return RE::BSEventNotifyControl::kContinue;
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    //logger::info("Flags: {:08b}", event->flags.underlying());
                }
            }
            else if (event->target && event->target->IsPlayerRef()) {
                if (event->flags.all(RE::TESHitEvent::Flag::kHitBlocked) && GameData::player_has_trigger_perk(GameData::spellhotbar_perk_cast_on_block)) {

                    bool timing_ok{ true };
                    if (GameData::global_spellhotbar_perks_timed_block_window && GameData::global_spellhotbar_perks_timed_block_window->value > 0.0f) {
                        timing_ok = GameData::block_timer <= GameData::global_spellhotbar_perks_timed_block_window->value;
                    }

                    if (timing_ok && GameData::calc_random_proc(GameData::global_spellhotbar_perks_block_trigger_chance)) {
                        SpellHotbar::casts::SpellProc::trigger_spellproc();
                    }

                    return RE::BSEventNotifyControl::kContinue;
                }
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl EventListener::ProcessEvent(const RE::CriticalHit::Event* event, RE::BSTEventSource<RE::CriticalHit::Event>*)
    {
        if (event->aggressor && event->aggressor->IsPlayerRef()) {
            if (GameData::player_has_trigger_perk(GameData::spellhotbar_perk_cast_on_crit) &&
                GameData::calc_random_proc(GameData::global_spellhotbar_perks_crit_trigger_chance)) {
                SpellHotbar::casts::SpellProc::trigger_spellproc();
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl EventListener::ProcessEvent(const RE::TESEquipEvent* event, RE::BSTEventSource<RE::TESEquipEvent>*)
    {
        if (GameData::individual_shout_cooldowns) {
            
            //only do for player
            if (event != nullptr && event->actor != nullptr && (event->actor->IsPlayer() || event->actor->IsPlayerRef())) {
                auto pc = RE::PlayerCharacter::GetSingleton();
                auto form = RE::TESForm::LookupByID(event->baseObject);
                if (pc != nullptr && form != nullptr) {
                    bool is_shout = form->GetFormType() == RE::FormType::Shout;
                    if (is_shout) {
                        if (event->equipped) {
                            //equipping a shout -> lookup cooldown
                            GameData::apply_cd_for_shout(form->GetFormID());
                        }
                        else {
                            //unequip shout -> reset cd
                            GameData::reset_shout_cd();
                        }
                    }
                }
            }

        }
        return RE::BSEventNotifyControl::kContinue;
    }

    //RE::BSEventNotifyControl EventListener::ProcessEvent(const RE::TESPlayerBowShotEvent* event, RE::BSTEventSource<RE::TESPlayerBowShotEvent>*)
    //{
        //logger::info("Bow Shot Power: {}", event->shotPower);
        //if (event->shotPower >= 1.0f) {
            //TODO check perk
        //    SpellHotbar::casts::SpellProc::trigger_spellproc();
        //}
    //    return RE::BSEventNotifyControl::kContinue;
    //}

}