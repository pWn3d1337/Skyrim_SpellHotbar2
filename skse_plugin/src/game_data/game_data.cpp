#include "game_data.h"
#include "../logger/logger.h"
#include "../bar/hotbars.h"
#include "../rendering/render_manager.h"
#include "../input/keybinds.h"
#include "../casts/casting_controller.h"
#include "spell_data_csv_loader.h"
#include "spell_casteffect_csv_loader.h"
#include "custom_transform_csv_loader.h"
#include "animation_data_csv_loader.h"
#include "keynames_csv_loader.h"

namespace SpellHotbar::GameData {

    constexpr std::string_view spell_data_root = ".\\data\\SKSE\\Plugins\\SpellHotbar\\spelldata\\";
    constexpr std::string_view spell_casteffects_root = ".\\data\\SKSE\\Plugins\\SpellHotbar\\effectdata\\";
    constexpr std::string_view custom_transformations_root = ".\\data\\SKSE\\Plugins\\SpellHotbar\\transformdata\\";
    constexpr std::string_view animation_data_root = ".\\data\\SKSE\\Plugins\\SpellHotbar\\animationdata\\";

    inline const std::string keynames_csv_path = ".\\data\\SKSE\\Plugins\\SpellHotbar\\keynames\\keynames.csv";

    RE::TESRace* vampire_lord_race = nullptr;
    RE::TESRace* werewolf_beast_race = nullptr;

    RE::TESGlobal* global_animation_type = nullptr;
    RE::TESGlobal* global_casting_source = nullptr;
    RE::TESGlobal* global_vampire_lord_equip_mode = nullptr;
    RE::TESGlobal* global_casting_timer = nullptr;
    RE::TESGlobal* global_casting_conc_spell = nullptr;

    RE::SpellItem* spellhotbar_castfx_spell = nullptr;
    RE::SpellItem* spellhotbar_unbind_slot = nullptr;
    RE::SpellItem* spellhotbar_toggle_dualcast = nullptr;

    RE::TESGlobal* global_spellhotbar_use_dual_casting = nullptr;

    RE::BGSEquipSlot* equip_slot_right_hand = nullptr;
    RE::BGSEquipSlot* equip_slot_left_hand = nullptr;
    RE::BGSEquipSlot* equip_slot_either_hand = nullptr;
    RE::BGSEquipSlot* equip_slot_both_hand = nullptr;
    RE::BGSEquipSlot* equip_slot_voice = nullptr;

    RE::BGSPerk* perk_alteration_dual_casting = nullptr;
    RE::BGSPerk* perk_conjuration_dual_casting = nullptr;
    RE::BGSPerk* perk_destruction_dual_casting = nullptr;
    RE::BGSPerk* perk_illusion_dual_casting = nullptr;
    RE::BGSPerk* perk_restoration_dual_casting = nullptr;

    std::unordered_map<RE::FormID, Spell_cast_data> spell_cast_info;
    std::vector<std::tuple<RE::BGSArtObject*, RE::BGSArtObject*, const std::string>> spell_casteffect_art;
    std::unordered_map<RE::FormID, Gametime_cooldown_value> gametime_cooldowns;

    std::unique_ptr<std::unordered_map<std::string, size_t>> spell_effects_key_indices{nullptr};

    std::unordered_map<RE::FormID, Transformation_data> custom_transformation_data;

    std::unordered_map<int, std::pair<std::string, std::string>> key_names;

    std::unordered_map<int, std::string> animation_names;

    template<typename T>
    void load_form_from_game(RE::FormID formId, const std::string & plugin, T** out_ptr, const std::string & name, RE::FormType type) {
        auto form = SpellHotbar::GameData::get_form_from_file(formId, plugin);

        if (form && form->GetFormType() == type) {
            *out_ptr = form->As<T>();
        }
        else {
            logger::error("Could not get '{}' from {}", name, plugin);
        }
    }

    void onDataLoad() 
    { 
        load_form_from_game(0x00283A, "Dawnguard.esm", &vampire_lord_race, "Vampire Lord Race", RE::FormType::Race);
        load_form_from_game(0xCDD84, "Skyrim.esm", &werewolf_beast_race, "Werewolf Beast Race", RE::FormType::Race);

        load_form_from_game(0x815, "SpellHotbar.esp", &global_animation_type, "SpellHotbar_SpellAnimationType", RE::FormType::Global);

        load_form_from_game(0x835, "SpellHotbar.esp", &global_casting_source, "SpellHotbar_CastingSource", RE::FormType::Global);

        load_form_from_game(0x820, "SpellHotbar.esp", &global_vampire_lord_equip_mode, "SpellHotbar_VampireLordUseEquipMode", RE::FormType::Global);

        load_form_from_game(0x816, "SpellHotbar.esp", &spellhotbar_castfx_spell, "SpellHotbar_CastFX_Spell", RE::FormType::Spell);

        load_form_from_game(0x810, "SpellHotbar.esp", &spellhotbar_unbind_slot, "SpellHotbar_unbind", RE::FormType::Spell);

        load_form_from_game(0x839, "SpellHotbar.esp", &spellhotbar_toggle_dualcast, "SpellHotbar_ToggleDualCasting", RE::FormType::Spell);
        load_form_from_game(0x83B, "SpellHotbar.esp", &global_spellhotbar_use_dual_casting, "SpellHotbar_UseDualCasting", RE::FormType::Global);

        load_form_from_game(0x838, "SpellHotbar.esp", &global_casting_timer, "SpellHotbar_Casttimer", RE::FormType::Global);
        load_form_from_game(0x834, "SpellHotbar.esp", &global_casting_conc_spell, "SpellHotbar_isCastingConcSpell", RE::FormType::Global);


        load_form_from_game(0x13F42, "Skyrim.esm", &equip_slot_right_hand, "RightHand", RE::FormType::EquipSlot);
        load_form_from_game(0x13F43, "Skyrim.esm", &equip_slot_left_hand, "LeftHand", RE::FormType::EquipSlot);
        load_form_from_game(0x13F44, "Skyrim.esm", &equip_slot_either_hand, "EitherHand", RE::FormType::EquipSlot);
        load_form_from_game(0x13F45, "Skyrim.esm", &equip_slot_both_hand, "BothHands", RE::FormType::EquipSlot);
        load_form_from_game(0x25BEE, "Skyrim.esm", &equip_slot_voice, "Voice", RE::FormType::EquipSlot);

        load_form_from_game(0x153CD, "Skyrim.esm", &perk_alteration_dual_casting, "AlterationDualCasting", RE::FormType::Perk);
        load_form_from_game(0x153CE, "Skyrim.esm", &perk_conjuration_dual_casting, "ConjurationDualCasting", RE::FormType::Perk);
        load_form_from_game(0x153CF, "Skyrim.esm", &perk_destruction_dual_casting, "DestructionDualCasting", RE::FormType::Perk);
        load_form_from_game(0x153D0, "Skyrim.esm", &perk_illusion_dual_casting, "IllusionDualCasting", RE::FormType::Perk);
        load_form_from_game(0x153D1, "Skyrim.esm", &perk_restoration_dual_casting, "RestorationDualCasting", RE::FormType::Perk);

        load_keynames_file();

        //need to wait for game data beeing available
        RenderManager::load_gamedata_dependant_resources();

        spell_effects_key_indices = std::make_unique<std::unordered_map<std::string, size_t>>();
        SpellCastEffectCSVLoader::load_spell_casteffects(std::filesystem::path(spell_casteffects_root));
        SpellDataCSVLoader::load_spell_data(std::filesystem::path(spell_data_root));
        AnimationDataCSVLoader::load_anim_data(std::filesystem::path(animation_data_root));

        spell_effects_key_indices = nullptr; //no longer need this

        //load animation data
        AnimationDataCSVLoader::load_anim_data(std::filesystem::path(animation_data_root));

        //load custom transformations
        CustomTransformCSVLoader::load_transformations(std::filesystem::path(custom_transformations_root));

        //vampire lord add unbind power
        auto vl_formlist = SpellHotbar::GameData::get_form_from_file(0x019AD9, "Dawnguard.esm");
        if (vl_formlist && spellhotbar_unbind_slot) {
            auto fl = vl_formlist->As<RE::BGSListForm>();
            fl->AddForm(spellhotbar_unbind_slot);
        }
        else {
            logger::error("Could not get Vampire Powers Formlist");
        }
    }

    void reload_data()
    { 
        logger::info("Reloading Spell Data...");
        spell_cast_info.clear();
        spell_casteffect_art.clear();
        animation_names.clear();

        spell_effects_key_indices = std::make_unique<std::unordered_map<std::string, size_t>>();
        SpellCastEffectCSVLoader::load_spell_casteffects(std::filesystem::path(spell_casteffects_root));
        SpellDataCSVLoader::load_spell_data(std::filesystem::path(spell_data_root));
        AnimationDataCSVLoader::load_anim_data(std::filesystem::path(animation_data_root));
        spell_effects_key_indices = nullptr;  // no longer need this
    }

    void load_keynames_file()
    {
        KeyNamesCSVLoader::load_keynames(keynames_csv_path);
    }

    int get_spell_keybind(int slot_index)
    {
        if (slot_index >= 0 && slot_index < SpellHotbar::Input::key_spells.size()) {
            return SpellHotbar::Input::key_spells.at(slot_index).get_dx_scancode();
        }
        return 0;
    }

    std::string get_keybind_text(int slot_index, key_modifier mod)
    {
        int keybind = get_spell_keybind(slot_index);

        std::string key_text;
        if (key_names.contains(keybind)) {
            key_text = key_names.at(keybind).first;

            switch (mod) {
                case SpellHotbar::key_modifier::ctrl:
                    key_text = key_names.at(Input::mod_ctrl.get_dx_scancode()).second + "-" + key_text;
                    break;
                case SpellHotbar::key_modifier::shift:
                    key_text = key_names.at(Input::mod_shift.get_dx_scancode()).second + "-" + key_text;
                    break;
                case SpellHotbar::key_modifier::alt:
                    key_text = key_names.at(Input::mod_alt.get_dx_scancode()).second + "-" + key_text;
                    break;
                default:
                    break;
            }

        } else {
            key_text = "??";
        }
        return key_text;
    }

    EquippedType getPlayerEquipmentType()
    { 
        auto pc = RE::PlayerCharacter::GetSingleton();
        RE::TESForm* right_hand = pc->GetEquippedObject(false);
        RE::TESForm* left_hand = pc->GetEquippedObject(true);

        if (right_hand == nullptr)
        {
            return EquippedType::FIST;
        }
        else if (right_hand->GetFormType() == RE::FormType::Weapon)
        {
            RE::TESObjectWEAP* weapon_right = right_hand->As<RE::TESObjectWEAP>();
            RE::WEAPON_TYPE type = weapon_right->GetWeaponType();
            switch (type)
            { 
            case RE::WEAPON_TYPE::kHandToHandMelee:
                    return EquippedType::FIST;
            case RE::WEAPON_TYPE::kTwoHandAxe:
            case RE::WEAPON_TYPE::kTwoHandSword:
                    return EquippedType::TWOHAND;
            case RE::WEAPON_TYPE::kStaff:
                    if (left_hand != nullptr && left_hand->GetFormType() == RE::FormType::Armor) {
                        return EquippedType::STAFF_SHIELD;
                    }
                    return EquippedType::SPELL;
            case RE::WEAPON_TYPE::kBow:
                    return EquippedType::BOW;
            case RE::WEAPON_TYPE::kCrossbow:
                    return EquippedType::CROSSBOW;
            case RE::WEAPON_TYPE::kOneHandAxe:
            case RE::WEAPON_TYPE::kOneHandDagger:
            case RE::WEAPON_TYPE::kOneHandMace:
            case RE::WEAPON_TYPE::kOneHandSword:
                //need to check left hand
                    if (left_hand == nullptr) return EquippedType::ONEHAND_EMPTY;
                    if (left_hand->GetFormType() == RE::FormType::Weapon) {
                        RE::TESObjectWEAP* weapon_left = left_hand->As<RE::TESObjectWEAP>();
                        if (weapon_left->IsMelee()) {
                            return EquippedType::DUAL_WIELD;
                        }
                        else
                        {
                            //If not empty, weapon record and no melee weapon -> treat as 1h + spell
                            return EquippedType::ONEHAND_SPELL;
                        } 
                    } else if (left_hand->GetFormType() == RE::FormType::Armor){
                        return EquippedType::ONEHAND_SHIELD;
                    } else if (left_hand->GetFormType() == RE::FormType::Spell) {
                        return EquippedType::ONEHAND_SPELL;
                    }
            }
        }
        else if (right_hand->GetFormType() == RE::FormType::Spell)
        {
            return EquippedType::SPELL;
        }
        return EquippedType::FIST;
    }

    bool isVampireLord()
    { 
        auto * pc = RE::PlayerCharacter::GetSingleton();
        if (!pc) return false;
        return (vampire_lord_race != nullptr) && (pc->GetRace() == vampire_lord_race);
    }
    bool isWerewolf() {
        auto* pc = RE::PlayerCharacter::GetSingleton();
        if (!pc) return false;
        return (werewolf_beast_race != nullptr) && (pc->GetRace() == werewolf_beast_race);
    }

    uint32_t isCustomTransform() {
        auto* pc = RE::PlayerCharacter::GetSingleton();
        if (!pc) return 0U;
        RE::FormID currentRace = pc->GetRace()->GetFormID();

        if (custom_transformation_data.contains(currentRace)) {
            auto &dat = custom_transformation_data.at(currentRace);
            return dat.bar_id;
        }
        return 0U;
    }

    custom_transform_spell_type SpellHotbar::GameData::getCustomTransformCasttype() {
        auto* pc = RE::PlayerCharacter::GetSingleton();
        if (!pc) return custom_transform_spell_type::regular;
        RE::FormID currentRace = pc->GetRace()->GetFormID();

        if (custom_transformation_data.contains(currentRace)) {
            auto& dat = custom_transformation_data.at(currentRace);
            return dat.casting_type;
        }
        return custom_transform_spell_type::regular;
    }

    bool hasFavMenuSlotBinding() {
        bool ret = isVampireLord() || isWerewolf();
        if (!ret && isCustomTransform()) {
            auto casttype = GameData::getCustomTransformCasttype();
            if (casttype == GameData::custom_transform_spell_type::fav_menu ||
                casttype == GameData::custom_transform_spell_type::fav_menu_switch) {
                ret = true;
            }
        }
        return ret;
    }

    std::string resolve_spellname(RE::FormID formID) {
        if (formID == 0) return "EMPTY";
        if (GameData::is_clear_spell(formID)) return "BLOCKED";

        auto form = RE::TESForm::LookupByID(formID);
        if (form == nullptr) {
            return "<INVALID>";
        } else {
            return form->GetName();
        }
    }

    std::tuple<bool, float> shouldShowHUDBar() {
        constexpr float fast_fade = 0.1f;
        constexpr float slow_fade = 0.5f;
        //credits to LamasTinyHUD
        auto* ui = RE::UI::GetSingleton(); 
        if (!ui || ui->GameIsPaused() || !ui->IsCursorHiddenWhenTopmost() || !ui->IsShowingMenus() || !ui->GetMenu<RE::HUDMenu>() || ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME))
        {
            return std::make_tuple(false, fast_fade);
        }

        //crashes on 1170
        /*if (const auto* control_map = RE::ControlMap::GetSingleton();
            !control_map || !control_map->IsMovementControlsEnabled() ||
            (control_map->contextPriorityStack.size() > 0 && control_map->contextPriorityStack.back() != RE::UserEvents::INPUT_CONTEXT_ID::kGameplay)) {
            return std::make_pair(false, fast_fade);
        }*/
        //not sure what exactly contextPriorityStack does anyway
        if (const auto* control_map = RE::ControlMap::GetSingleton();
            !control_map || !control_map->IsMovementControlsEnabled())
        { 
            return std::make_tuple(false, fast_fade);
        }

        if (Bars::disable_non_modifier_bar && !(Input::mod_ctrl.isDown() || Input::mod_shift.isDown() || Input::mod_alt.isDown())) {
            //Main bar not used -> hide
            return std::make_tuple(false, fast_fade);
        }

        auto* pc = RE::PlayerCharacter::GetSingleton();
        if (pc) {
            if (GameData::isVampireLord()) {
                switch (Bars::bar_show_setting_vampire_lord) {
                    case Bars::bar_show_mode::always:
                        return std::make_tuple(true, slow_fade);

                    case Bars::bar_show_mode::combat:
                        return std::make_tuple(pc->IsInCombat(), slow_fade);

                    default:
                        return std::make_tuple(false, slow_fade);
                }
            } else if (GameData::isWerewolf()) {
                switch (Bars::bar_show_setting_werewolf) {
                    case Bars::bar_show_mode::always:
                        return std::make_tuple(true, slow_fade);

                    case Bars::bar_show_mode::combat:
                        return std::make_tuple(pc->IsInCombat(), slow_fade);

                    default:
                        return std::make_tuple(false, slow_fade);
                }
            } else {
                if (Bars::bar_show_setting == Bars::bar_show_mode::never) return std::make_tuple(false, slow_fade);
                if (Bars::bar_show_setting == Bars::bar_show_mode::always) return std::make_tuple(true, slow_fade);

                bool in_combat = pc->IsInCombat();
                bool weapon_drawn = pc->AsActorState()->IsWeaponDrawn();

                switch (Bars::bar_show_setting) {
                    case Bars::bar_show_mode::combat:
                        return std::make_tuple(in_combat, slow_fade);

                    case Bars::bar_show_mode::drawn_weapon:
                        return std::make_tuple(weapon_drawn, slow_fade);

                    case Bars::bar_show_mode::combat_and_drawn:
                        return std::make_tuple(in_combat && weapon_drawn, slow_fade);

                    case Bars::bar_show_mode::combat_or_drawn:
                        return std::make_tuple(in_combat || weapon_drawn, slow_fade);
                }
            }
        }
        return std::make_tuple(false, fast_fade);
    }

    inline int get_spell_rank(int32_t minlevel) {
        int ret{0};
        if (minlevel >= 100) {
            ret = 4;
        } else if (minlevel >= 75) {
            ret = 3;
        } else if (minlevel >= 50) {
            ret = 2;
        } else if (minlevel >= 25) {
            ret = 1;
        }
        return ret;
    }

    DefaultIconType get_fallback_icon_type(RE::TESForm* form)
    { 
        DefaultIconType ret {DefaultIconType::UNKNOWN};

        if (form->GetFormType() == RE::FormType::Spell) {
            RE::SpellItem* spell = form->As<RE::SpellItem>();
            if (spell->GetSpellType() == RE::MagicSystem::SpellType::kLesserPower) {
            
                ret = DefaultIconType::LESSER_POWER;
            } else if (spell->GetSpellType() == RE::MagicSystem::SpellType::kPower) {
                
                ret = DefaultIconType::GREATER_POWER;
            } else if (spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                if (spell->effects.size() > 0U) {

                    for (RE::BSTArrayBase::size_type i = 0U; i < spell->effects.size() && ret == DefaultIconType::UNKNOWN; i++) {
                        //find first spell effect that has a magic school
                        RE::Effect* effect = spell->effects[i];
                        if (effect->baseEffect) {
                            RE::ActorValue school = effect->baseEffect->GetMagickSkill();
                            int rank = get_spell_rank(effect->baseEffect->GetMinimumSkillLevel());
                            RE::ActorValue dmg_type = effect->baseEffect->data.resistVariable;

                            switch (school) {
                                case RE::ActorValue::kDestruction:
                                    if (dmg_type == RE::ActorValue::kResistFire) {
                                        ret = DefaultIconType::DESTRUCTION_FIRE_NOVICE;
                                    } else if (dmg_type == RE::ActorValue::kResistFrost) {
                                        ret = DefaultIconType::DESTRUCTION_FROST_NOVICE;
                                    } else if (dmg_type == RE::ActorValue::kResistShock) {
                                        ret = DefaultIconType::DESTRUCTION_SHOCK_NOVICE;
                                    } else {
                                        ret = DefaultIconType::DESTRUCTION_GENERIC_NOVICE;
                                    }
                                    ret = DefaultIconType(static_cast<uint32_t>(ret) + rank);
                                    break;

                                case RE::ActorValue::kAlteration:
                                    ret = DefaultIconType(static_cast<uint32_t>(DefaultIconType::ALTERATION_NOVICE) +
                                                          rank);
                                    break;

                                case RE::ActorValue::kIllusion:
                                    if (effect->baseEffect->data.flags.all(
                                            RE::EffectSetting::EffectSettingData::Flag::kHostile)) {
                                        ret = DefaultIconType::ILLUSION_HOSTILE_NOVICE;
                                    } else {
                                        ret = DefaultIconType::ILLUSION_FRIENDLY_NOVICE;
                                    }
                                    ret = DefaultIconType(static_cast<uint32_t>(ret) + rank);
                                    break;

                                case RE::ActorValue::kRestoration:
                                    if (effect->baseEffect->data.flags.all(
                                            RE::EffectSetting::EffectSettingData::Flag::kHostile)) {
                                        ret = DefaultIconType::RESTORATION_HOSTILE_NOVICE;
                                    } else {
                                        ret = DefaultIconType::RESTORATION_FRIENDLY_NOVICE;
                                    }
                                    ret = DefaultIconType(static_cast<uint32_t>(ret) + rank);
                                    break;

                                case RE::ActorValue::kConjuration:
                                    auto archetype = effect->baseEffect->data.archetype;
                                    if (archetype == RE::EffectSetting::Archetype::kBoundWeapon) {
                                        ret = DefaultIconType::CONJURATION_BOUND_WEAPON_NOVICE;
                                    } else if (archetype == RE::EffectSetting::Archetype::kSummonCreature ||
                                               archetype == RE::EffectSetting::Archetype::kReanimate) {
                                        ret = DefaultIconType::CONJURATION_SUMMON_NOVICE;
                                    } else {
                                        ret = DefaultIconType::CONJURATION_SUMMON_NOVICE;
                                    }
                                    ret = DefaultIconType(static_cast<uint32_t>(ret) + rank);
                                    break;
                            }
                        }
                    }
                }
            }
        }
        else if (form->GetFormType() == RE::FormType::Shout) {
            //RE::TESShout* shout = form->As<RE::TESShout>();
            ret = DefaultIconType::SHOUT_GENERIC;
        }
        return ret;
    }

    void set_spell_cast_data(RE::FormID spell, Spell_cast_data&& data) {
        auto it = spell_cast_info.find(spell);
        if (it != spell_cast_info.end()) {
            it->second = std::move(data);
        }
        else {
            spell_cast_info.emplace(spell, std::move(data));
        }
    }

    void save_to_SKSE_save(SKSE::SerializationInterface* a_intfc)
    {
        //purge expired cooldowns before saving
        purge_expired_gametime_cooldowns();

        //write cooldowns size, also write 0 so its known there is nothing else to load
        uint32_t size = static_cast<uint32_t>(gametime_cooldowns.size()); //32bit will be enough...
        a_intfc->WriteRecordData(&size, sizeof(uint32_t));

        for (const auto& [id, cd] : gametime_cooldowns) {
            a_intfc->WriteRecordData(&id, sizeof(RE::FormID));     
            a_intfc->WriteRecordData(&cd.readytime, sizeof(float));    
            a_intfc->WriteRecordData(&cd.duration, sizeof(float));  
        }

    }

    void load_from_SKSE_save(SKSE::SerializationInterface* a_intfc) {
        uint32_t size;
        if (!a_intfc->ReadRecordData(&size, sizeof(uint32_t)))
        {
            logger::error("Error reading Cooldown map size!");
        } else {
            gametime_cooldowns.clear();
            auto cal = RE::Calendar::GetSingleton();

            for (uint32_t i = 0Ui32; i < size; i++) {
                RE::FormID id{0};
                float ready{0.0f};
                float dur{0.0f};

                if (!a_intfc->ReadRecordData(&id, sizeof(RE::FormID))) {
                    logger::error("Failed to read cooldown entry!");
                    break;
                }
                if (!a_intfc->ReadRecordData(&ready, sizeof(float))) {
                    logger::error("Failed to read cooldown entry!");
                    break;
                }
                if (!a_intfc->ReadRecordData(&dur, sizeof(float))) {
                    logger::error("Failed to read cooldown entry!");
                    break;
                }
                if (cal) {
                    if (cal->GetCurrentGameTime() < ready) {
                        gametime_cooldowns.emplace(id, Gametime_cooldown_value(ready, dur));
                    } else {
                        logger::warn("Expired cooldown was saved, skipping!");
                    }
                } else {
                    logger::error("Game Calendar is null");
                }
            }
        }
    }

    void add_gametime_cooldown(RE::FormID skill, float hours, bool update_existing)
    { 
        auto cal = RE::Calendar::GetSingleton();
        if (cal) {
            float curr_time = cal->GetCurrentGameTime();
            float gt_value = hours / 24.0f;

            if (gametime_cooldowns.contains(skill)) {
                auto cd = gametime_cooldowns.find(skill);
                if (update_existing || cd->second.is_expired(curr_time)) {
                    cd->second = Gametime_cooldown_value(curr_time + gt_value, gt_value);
                }
            }
            else {
                gametime_cooldowns.emplace(skill, Gametime_cooldown_value(curr_time + gt_value, gt_value));
            }
        }
    }

    void add_gametime_cooldown_with_timescale(RE::FormID skill, float days, bool update_existing) {

        auto cal = RE::Calendar::GetSingleton();
        if (cal) {
            float gt_value = days * cal->GetTimescale() * 24.0f;
            add_gametime_cooldown(skill, gt_value, update_existing);
        }
    }

    void purge_expired_gametime_cooldowns() {
        auto cal = RE::Calendar::GetSingleton();
        float gt = cal->GetCurrentGameTime();
        if (cal) {
            std::erase_if(gametime_cooldowns, [gt](const auto& elem)
                {
                    auto const& [key, value] = elem;
                    return value.readytime < gt;
                });
        } else {
            logger::warn("Could not purge cooldowns, calendar was null");
        }
    }

    std::tuple<float, float> get_gametime_cooldown(float curr_game_time, RE::FormID skill)
    {
        if (gametime_cooldowns.contains(skill)) {
            auto iter = gametime_cooldowns.find(skill);
            auto & cd_values = iter->second;

            if (cd_values.readytime <= curr_game_time) {
                //Cooldown is finished
                //logger::info("Removing CD for {} from map, finished!", skill);
                gametime_cooldowns.erase(iter);
            } else {
                float prog = cd_values.get_progress(curr_game_time);
                return std::make_tuple(prog, cd_values.duration);
            }
        }
        return std::make_tuple(0.0f, 0.0f);
    }

    Gametime_cooldown_value::Gametime_cooldown_value(float time, float dur) : readytime(time), duration(dur){};

    float Gametime_cooldown_value::get_progress(float curr_time) {
        if (readytime > curr_time) {
            return 1.0f - ((readytime - curr_time) / duration);
        }
        return 0.0f;
    }

    bool Gametime_cooldown_value::is_expired(float current_game_time)
    {
        return readytime <= 0.0f || current_game_time >= readytime;
    }

    Spell_cast_data::Spell_cast_data() : gcd(-1.0f), cooldown(-1.0f), casttime(-1.0f), animation(-1), animation2(-1), casteffectid{0} {}
    bool Spell_cast_data::is_empty()
    { 
        return animation <= 0 && animation2 <= 0 && gcd < 0.0f && cooldown < 0.0f && casttime < 0.0f && casteffectid == 0U;
    }

    void Spell_cast_data::fill_default_values_from_spell(const RE::SpellItem* spell)
    {       
        if (casttime < 0.0f) {
            casttime = spell->GetChargeTime();
        }
        if (spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
            //min casttime of 0.25f for actual spells
            casttime = std::max(0.25f, casttime);
        }

        if (gcd < 0.0f) {
            gcd = 0.0f;
        }
        if (cooldown < 0.0f) {
            //TODO check for power and set to 24h
            cooldown = 0.0f;
        }
        if (animation < 0) {
            animation = GameData::chose_default_anim_for_spell(spell, -1, false);
        }
        if (animation2 < 0) {
            animation2 = GameData::chose_default_anim_for_spell(spell, -1, true);
        }

    }

    void add_casteffect(const std::string& key, RE::BGSArtObject* left_art, RE::BGSArtObject* right_art) 
    {
        spell_casteffect_art.emplace_back(std::make_tuple(left_art, right_art, key));
        if (spell_effects_key_indices) {
            spell_effects_key_indices.get()->insert(std::make_pair(key, spell_casteffect_art.size()-1));
        } else {
            logger::error("Spell effect key index map is null during load!");
        }
    }

    size_t get_cast_effect_id(const std::string& key)
    {
        size_t ret{0};
        if (spell_effects_key_indices) {
            if (spell_effects_key_indices.get()->contains(key)) {
                ret = spell_effects_key_indices.get()->at(key);
            } else {
                logger::warn("Unknown Casteffect: {}", key);
            }
        } else {
            logger::error("Spell effect key index map is called at incorrect time!");
        }
        return ret;
    }

    void update_spell_casting_art_and_time(size_t art_index, uint32_t casttime, hand_mode hands) { 
        if (spellhotbar_castfx_spell)
        {
            if (hands == hand_mode::left_hand || hands == hand_mode::dual_hand) {
                spellhotbar_castfx_spell->effects[0]->effectItem.duration = casttime;
            }
            else {
                spellhotbar_castfx_spell->effects[0]->effectItem.duration = 0;
            }

            if (hands == hand_mode::right_hand || hands == hand_mode::dual_hand) {
                spellhotbar_castfx_spell->effects[1]->effectItem.duration = casttime;
            }
            else {
                spellhotbar_castfx_spell->effects[1]->effectItem.duration = 0;
            }
            if (art_index >= spell_casteffect_art.size()) {
                art_index = 0U;
            }

            const auto& effect = spell_casteffect_art.at(art_index);

            if (hands == hand_mode::left_hand || hands == hand_mode::dual_hand) {
                spellhotbar_castfx_spell->effects[0]->baseEffect->data.hitEffectArt = std::get<0>(effect);
            }
            else {
                spellhotbar_castfx_spell->effects[0]->baseEffect->data.hitEffectArt = nullptr;
            }

            if (hands == hand_mode::right_hand || hands == hand_mode::dual_hand) {
                spellhotbar_castfx_spell->effects[1]->baseEffect->data.hitEffectArt = std::get<1>(effect);
            }
            else {
                spellhotbar_castfx_spell->effects[1]->baseEffect->data.hitEffectArt = nullptr;
            }
        }
    }

    std::tuple<RE::FormID, slot_type, hand_mode> get_current_spell_info_in_slot(size_t index) {
        RE::FormID ret{ 0 };
        hand_mode reth{ hand_mode::auto_hand };
        slot_type rett{ slot_type::empty };
        if (index < max_bar_size) {
            uint32_t bar_id = Bars::getCurrentHotbar_ingame();
            if (Bars::hotbars.contains(bar_id)) {
                auto& bar = Bars::hotbars.at(bar_id);
                auto [id, type, hand, inherited] = bar.get_skill_in_bar_with_inheritance(static_cast<int>(index), Bars::get_current_modifier(), false);
                ret = id;
                reth = hand;
                rett = type;
            }
        }
        if (GameData::is_clear_spell(ret)) {
            ret = 0U;
            reth = hand_mode::voice;
            rett = slot_type::blocked;
        }
        return std::make_tuple(ret, rett, reth);
    }

     RE::FormID get_current_spell_in_slot(size_t index) {
         auto [id, type, hand] = get_current_spell_info_in_slot(index);
         return id;
     }



     bool is_clear_spell(RE::FormID spell)
     { 
         if (spellhotbar_unbind_slot)
         {
             return spell == spellhotbar_unbind_slot->formID;
         }
         return false;
     }

     bool is_toggle_dualcast_spell(RE::FormID spell)
     {
         if (spellhotbar_toggle_dualcast)
         {
             return spell == spellhotbar_toggle_dualcast->formID;
         }
         return false;
     }

     bool should_dual_cast()
     {
         bool state{ false };
         if (global_spellhotbar_use_dual_casting) {
             state = global_spellhotbar_use_dual_casting->value != 0.0f;
         }
         if (Input::mod_dual_cast.isValidBound() && Input::mod_dual_cast.isDown()) {
             state = !state;
         }
         return state;
     }

     void add_custom_tranformation(uint32_t bar, std::string name, RE::FormID race_id, custom_transform_spell_type cast_type)
     {
         custom_transformation_data.insert(std::make_pair(race_id, Transformation_data(bar, cast_type)));

         if (!Bars::hotbars.contains(bar)) {
             if (!Bars::bar_names.contains(bar)) {
                Bars::bar_names[bar] = name;
             }
             Bars::add_special_bar(bar);
         }
     }
     
     RE::MagicSystem::CastingSource get_cast_hand_from_equip() {
         auto type = getPlayerEquipmentType();
         RE::MagicSystem::CastingSource hand{ RE::MagicSystem::CastingSource::kOther }; //Right 
         switch (type) {
         case EquippedType::FIST:
         case EquippedType::ONEHAND_EMPTY:
         case EquippedType::ONEHAND_SPELL:
         case EquippedType::DUAL_WIELD:
         case EquippedType::TWOHAND:
         case EquippedType::CROSSBOW:
         case EquippedType::SPELL:
             hand = RE::MagicSystem::CastingSource::kLeftHand;
             break;
         case EquippedType::BOW:
         case EquippedType::ONEHAND_SHIELD:
         case EquippedType::STAFF_SHIELD:
         default:
             hand = RE::MagicSystem::CastingSource::kRightHand;
             break;
         }

         return hand;
     }
     bool player_can_dualcast_spell(RE::SpellItem* spell)
     {
         bool hasPerk{ false };
         auto pc = RE::PlayerCharacter::GetSingleton();
         if (spell && pc) {
             RE::ActorValue school = spell->GetAssociatedSkill();
         
             switch (school) {
             case RE::ActorValue::kAlteration:
                 hasPerk = pc->HasPerk(perk_alteration_dual_casting);
                 break;
             case RE::ActorValue::kConjuration:
                 hasPerk = pc->HasPerk(perk_conjuration_dual_casting);
                 break;
             case RE::ActorValue::kDestruction:
                 hasPerk = pc->HasPerk(perk_destruction_dual_casting);
                 break;
             case RE::ActorValue::kIllusion:
                 hasPerk = pc->HasPerk(perk_illusion_dual_casting);
                 break;
             case RE::ActorValue::kRestoration:
                 hasPerk = pc->HasPerk(perk_restoration_dual_casting);
                 break;
             default:
                 break;
             }
         }

         return hasPerk;
     }
     void set_animtype_global(int value)
     {
         if (global_animation_type) {
             global_animation_type->value = static_cast<float>(value);
         }
     }
     hand_mode set_weapon_dependent_casting_source(hand_mode hand)
     {
         hand_mode used_hand = hand;
         if (SpellHotbar::GameData::global_casting_source) {
             //Set casting source according to spell anim
             RE::MagicSystem::CastingSource castsource = RE::MagicSystem::CastingSource::kOther;

             switch (hand) {
             case hand_mode::auto_hand:
                 castsource = SpellHotbar::GameData::get_cast_hand_from_equip();
                 if (castsource == RE::MagicSystem::CastingSource::kLeftHand) {
                     used_hand = hand_mode::left_hand;
                 }
                 else if (castsource == RE::MagicSystem::CastingSource::kRightHand) {
                     used_hand = hand_mode::right_hand;
                 }
                 break;
             case hand_mode::right_hand:
                 castsource = RE::MagicSystem::CastingSource::kRightHand;
                 break;
             case hand_mode::left_hand:
             case hand_mode::dual_hand:
                 castsource = RE::MagicSystem::CastingSource::kLeftHand;
                 break;
             case hand_mode::voice:
                 castsource = RE::MagicSystem::CastingSource::kInstant;
                 break;
             default:
                 castsource = RE::MagicSystem::CastingSource::kOther;
             }

             SpellHotbar::GameData::global_casting_source->value = static_cast<float>(static_cast<int>(castsource));
         }
         return used_hand;
     }
     void reset_animation_vars()
     {
         if (global_animation_type) {
             global_animation_type->value = 0.0f;
         }
         if (global_casting_source) {
             global_casting_source->value = 0.0f;
         }
         if (global_casting_timer) {
             global_casting_timer->value = 0.0f;
         }
         if (global_casting_conc_spell) {
             global_casting_conc_spell->value = 0.0f;
         }
     }
     void start_cast_timer()
     {
         if (global_casting_timer) {
             global_casting_timer->value = 0.0f;
         }
     }
     void advance_cast_timer(float delta)
     {
         if (global_casting_timer) {
             global_casting_timer->value += delta;
         }
     }

     constexpr std::array<std::uint16_t, 2> cast_anims_aimed{ 1U, 10016U };
     constexpr std::array<std::uint16_t, 2> cast_anims_self{ 2U, 10017U };
     constexpr std::array<std::uint16_t, 2> cast_anims_aimed_conc{ 1001U, 11003U };
     constexpr std::array<std::uint16_t, 2> cast_anims_self_conc{ 1002U, 11004U };
     constexpr std::array<std::uint16_t, 2> cast_anims_ritual{ 10000U, 10016U };
     constexpr std::array<std::uint16_t, 2> cast_anims_ritual_conc{ 11001U, 11003U };
     constexpr std::array<std::uint16_t, 2> cast_anims_ritual_self{ 10000U, 10017U }; //same regular anim, but self cast fast anim

     inline bool is_ward_spell(const RE::SpellItem* spell) {
         return spell->effects.size() > 0 && spell->effects[0]->baseEffect && spell->effects[0]->baseEffect->HasKeywordID(0x1EA69);
     }

     float get_ritual_conc_anim_prerelease_time(int anim) {
         float pre_release_anim{ 0.0f };
         if (anim == cast_anims_ritual_conc[0]) {
             auto cam = RE::PlayerCamera::GetSingleton();
             if (cam && cam->IsInFirstPerson()) {
                 pre_release_anim = 1.5f;
             }
             else
             {
                 pre_release_anim = 1.0f;
             }
         }
         return pre_release_anim;
     }

     GameData::Spell_cast_data get_spell_data(const RE::TESForm* spell, bool fill_defaults)
     {
         GameData::Spell_cast_data data;
         RE::FormID id = spell->GetFormID();

         //TODO custom user data

         if (SpellHotbar::GameData::spell_cast_info.contains(id)) {
             data = SpellHotbar::GameData::spell_cast_info.at(id);
         }

         //Fill default values
         if (fill_defaults) {
             if (spell->GetFormType() == RE::FormType::Spell) {
                 data.fill_default_values_from_spell(spell->As<RE::SpellItem>());
             }
         }
         //TODO shouts

         return data;
     }

     void add_animation_data(const std::string& name, int anim_id)
     {
         auto it = animation_names.find(anim_id);
         if (it != animation_names.end()) {
             it->second = name;
         }
         else {
             animation_names.insert(std::make_pair(anim_id, name));
         }
     }

     uint16_t chose_default_anim_for_spell(const RE::TESForm* form, int anim, bool anim2) {
         uint16_t ret{ 0U };

         if (anim < 0) {
             size_t ind = anim2 ? 1U : 0U;
             if (form->GetFormType() == RE::FormType::Spell) {
                 const RE::SpellItem* spell = form->As<RE::SpellItem>();

                 bool self = spell->GetDelivery() == RE::MagicSystem::Delivery::kSelf;
                 if (spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration) {
                     if (spell->IsTwoHanded()) {
                         //Ritual Conc
                         ret = cast_anims_ritual_conc[ind]; // ritual self conc is not used or provided by vanilla game
                     }
                     else
                     {
                         if (is_ward_spell(spell)) {
                             //Ward anim, ward has no dual cast
                             ret = 1003U;
                         }
                         else {
                             //1H conc
                             ret = self ? cast_anims_self_conc[ind] : cast_anims_aimed_conc[ind];
                         }
                     }
                 }
                 else {
                     if (spell->IsTwoHanded()) {
                         //Ritual Cast
                         ret = self ? cast_anims_ritual_self[ind] : cast_anims_ritual[ind];
                     }
                     else
                     {
                         //Regular Cast
                         ret = self ? cast_anims_self[ind] : cast_anims_aimed[ind];
                     }
                 }

             }
         }
         else {
             ret = static_cast<uint16_t>(anim);
         }
         return ret;
     }
}
