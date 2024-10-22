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
#include "spell_cast_data.h"
#include "../input/modes.h"

#include <random>

namespace SpellHotbar::GameData {

    constexpr std::string_view spell_data_root = ".\\data\\SKSE\\Plugins\\SpellHotbar\\spelldata\\";
    constexpr std::string_view spell_casteffects_root = ".\\data\\SKSE\\Plugins\\SpellHotbar\\effectdata\\";
    constexpr std::string_view custom_transformations_root = ".\\data\\SKSE\\Plugins\\SpellHotbar\\transformdata\\";
    constexpr std::string_view animation_data_root = ".\\data\\SKSE\\Plugins\\SpellHotbar\\animationdata\\";

    inline const std::string keynames_csv_path = ".\\data\\SKSE\\Plugins\\SpellHotbar\\keynames\\keynames.csv";

    std::random_device _random_device;
    std::mt19937 random_engine(_random_device());
    std::uniform_real_distribution<float> random_dist(0.0f, 1.0f);

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
    RE::EffectSetting* spellhotbar_spellproc_cd = nullptr;
    RE::SpellItem* spellhotbar_apply_spellproc_cd = nullptr;

    RE::TESGlobal* global_spellhotbar_use_dual_casting = nullptr;

    RE::TESGlobal* global_spellhotbar_perks_override = nullptr;
    RE::TESGlobal* global_spellhotbar_perks_timed_block_window = nullptr;
    RE::TESGlobal* global_spellhotbar_perks_block_trigger_chance = nullptr;
    RE::TESGlobal* global_spellhotbar_perks_power_attack_trigger_chance = nullptr;
    RE::TESGlobal* global_spellhotbar_perks_sneak_attack_trigger_chance = nullptr;
    RE::TESGlobal* global_spellhotbar_perks_crit_trigger_chance = nullptr;
    RE::TESGlobal* global_spellhotbar_perks_proc_cooldown = nullptr;
    RE::TESGlobal* global_spellhotbar_perks_require_halfcostperk = nullptr;

    RE::BGSPerk* spellhotbar_perk_cast_on_power_attack = nullptr;
    RE::BGSPerk* spellhotbar_perk_cast_on_sneak_attack = nullptr;
    RE::BGSPerk* spellhotbar_perk_cast_on_crit = nullptr;
    RE::BGSPerk* spellhotbar_perk_cast_on_block = nullptr;
    RE::BGSPerk* spellhotbar_perk_cast_on_concentration = nullptr;
    RE::BGSPerk* spellhotbar_perk_expert = nullptr;
    RE::BGSPerk* spellhotbar_perk_master = nullptr;

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

    RE::BGSSoundDescriptorForm* sound_ITMPotionUse = nullptr;
    RE::BGSSoundDescriptorForm* sound_NPCHumanEatSoup = nullptr;

    RE::SpellItem* werewolf_change_power = nullptr;

    std::unordered_map<RE::FormID, Spell_cast_data> spell_cast_info;
    std::unordered_map<RE::FormID, User_custom_spelldata> user_spell_cast_info;

    std::unordered_map<RE::FormID, User_custom_entry> user_custom_entry_info;

    std::vector<std::tuple<RE::BGSArtObject*, RE::BGSArtObject*, const std::string>> spell_casteffect_art;
    std::unordered_map<RE::FormID, Gametime_cooldown_value> gametime_cooldowns;

    std::unique_ptr<std::unordered_map<std::string, size_t>> spell_effects_key_indices{nullptr};

    std::unordered_map<RE::FormID, Transformation_data> custom_transformation_data;

    std::unordered_map<int, std::pair<std::string, std::string>> key_names;

    std::unordered_map<int, std::string> animation_names;

    /*
    * Spells that have a magiceffect as cooldown
    */
    std::unordered_map<RE::FormID, RE::FormID> spell_cd_magiceffect_tracking;

    float potion_gcd { 1.0f };
    float block_timer { 0.0f };

    Bars::OblivionBar oblivion_bar;

    constexpr ImU32 col_OneHanded {IM_COL32(255,132,0,255)};
    constexpr ImU32 col_TwoHanded {IM_COL32(255,120,0,255)};
    constexpr ImU32 col_Archery {IM_COL32(128,255,255,255)};
    constexpr ImU32 col_Block {IM_COL32(255,116,0,255)};
    constexpr ImU32 col_Smithing {IM_COL32(255,128,0,255)};
    constexpr ImU32 col_HeavyArmor {IM_COL32(255,124,0,255)};
    constexpr ImU32 col_LightArmor {IM_COL32(120,255,0,255)};
    constexpr ImU32 col_Pickpocket {IM_COL32(116,255,0,255)};
    constexpr ImU32 col_Lockpicking {IM_COL32(112,255,0,255)};
    constexpr ImU32 col_Sneak {IM_COL32(90,255,0,255)};
    constexpr ImU32 col_Alchemy {IM_COL32(95,255,95,255)};
    constexpr ImU32 col_Speech {IM_COL32(135,255,0,255)};
    constexpr ImU32 col_Alteration {IM_COL32(0,200,255,255)};
    constexpr ImU32 col_Conjuration {IM_COL32(120,160,255,255)};
    constexpr ImU32 col_Destruction {IM_COL32(255,90,70,255)};
    constexpr ImU32 col_Illusion {IM_COL32(0,200,140,255)};
    constexpr ImU32 col_Restoration {IM_COL32(255,255,160,255)};
    constexpr ImU32 col_Enchanting {IM_COL32(0,255,200,255)};

    std::unordered_map<RE::ActorValue, ImU32> potion_color_mapping =
    {
        //{RE::ActorValue::kAggression, IM_COL32_WHITE},
        //{RE::ActorValue::kConfidence, IM_COL32_WHITE},
        //{RE::ActorValue::kEnergy, IM_COL32_WHITE},
        //{RE::ActorValue::kMorality, IM_COL32_WHITE},
        //{RE::ActorValue::kMood, IM_COL32_WHITE},
        //{RE::ActorValue::kAssistance, IM_COL32_WHITE},
        {RE::ActorValue::kOneHanded, col_OneHanded},
        {RE::ActorValue::kTwoHanded, col_TwoHanded},
        {RE::ActorValue::kArchery, col_Archery},
        {RE::ActorValue::kBlock, col_Block},
        {RE::ActorValue::kSmithing, col_Smithing},
        {RE::ActorValue::kHeavyArmor, col_HeavyArmor},
        {RE::ActorValue::kLightArmor, col_LightArmor},
        {RE::ActorValue::kPickpocket, col_Pickpocket},
        {RE::ActorValue::kLockpicking, col_Lockpicking},
        {RE::ActorValue::kSneak, col_Sneak},
        {RE::ActorValue::kAlchemy, col_Alchemy},
        {RE::ActorValue::kSpeech, col_Speech},
        {RE::ActorValue::kAlteration, col_Alteration},
        {RE::ActorValue::kConjuration, col_Conjuration},
        {RE::ActorValue::kDestruction, col_Destruction},
        {RE::ActorValue::kIllusion, col_Illusion},
        {RE::ActorValue::kRestoration, col_Restoration},
        {RE::ActorValue::kEnchanting, col_Enchanting},
        {RE::ActorValue::kHealth, IM_COL32(255,0,0,255)},
        {RE::ActorValue::kMagicka, IM_COL32(0,0,255,255)},
        {RE::ActorValue::kStamina, IM_COL32(0,255,0,255)},
        {RE::ActorValue::kHealRate, IM_COL32(255,50,50,255)},
        {RE::ActorValue::kMagickaRate, IM_COL32(50,50,255,255)},
        {RE::ActorValue::kStaminaRate, IM_COL32(50,255,50,255)},
        {RE::ActorValue::kSpeedMult, IM_COL32(60,255,80,255)},
        //{RE::ActorValue::kInventoryWeight, IM_COL32_WHITE},
        {RE::ActorValue::kCarryWeight, IM_COL32(0,230,0,255)},
        {RE::ActorValue::kCriticalChance, IM_COL32(230,230,230,255)},
        //{RE::ActorValue::kMeleeDamage, IM_COL32_WHITE},
        //{RE::ActorValue::kUnarmedDamage, IM_COL32_WHITE},
        //{RE::ActorValue::kMass, IM_COL32_WHITE},
        //{RE::ActorValue::kVoicePoints, IM_COL32_WHITE},
        //{RE::ActorValue::kVoiceRate, IM_COL32_WHITE},
        {RE::ActorValue::kDamageResist, IM_COL32(200,200,200,255)},
        {RE::ActorValue::kPoisonResist, IM_COL32(0,200,0,255)},
        {RE::ActorValue::kResistFire, IM_COL32(200,0,0,255)},
        {RE::ActorValue::kResistShock, IM_COL32(200,200,0,255)},
        {RE::ActorValue::kResistFrost, IM_COL32(0,0,200,255)},
        {RE::ActorValue::kResistMagic, IM_COL32(0,100,200,255)},
        {RE::ActorValue::kResistDisease, IM_COL32(50,200,50,255)},
        //{RE::ActorValue::kPerceptionCondition, IM_COL32_WHITE},
        //{RE::ActorValue::kEnduranceCondition, IM_COL32_WHITE},
        //{RE::ActorValue::kLeftAttackCondition, IM_COL32_WHITE},
        //{RE::ActorValue::kRightAttackCondition, IM_COL32_WHITE},
        //{RE::ActorValue::kLeftMobilityCondition, IM_COL32_WHITE},
        //{RE::ActorValue::kRightMobilityCondition, IM_COL32_WHITE},
        //{RE::ActorValue::kBrainCondition, IM_COL32_WHITE},
        {RE::ActorValue::kParalysis, IM_COL32(80,255,80,255)},
        {RE::ActorValue::kInvisibility, IM_COL32(255,130,255,255)},
        {RE::ActorValue::kNightEye, IM_COL32(100,150,255,255)},
        //{RE::ActorValue::kDetectLifeRange, IM_COL32_WHITE},
        {RE::ActorValue::kWaterBreathing, IM_COL32(30,60,255,255)},
        {RE::ActorValue::kWaterWalking, IM_COL32(0,120,255,255)},
        //{RE::ActorValue::kIgnoreCrippledLimbs, IM_COL32_WHITE},
        //{RE::ActorValue::kFame, IM_COL32_WHITE},
        //{RE::ActorValue::kInfamy, IM_COL32_WHITE},
        //{RE::ActorValue::kJumpingBonus, IM_COL32_WHITE},
        //{RE::ActorValue::kWardPower, IM_COL32_WHITE},
        //{RE::ActorValue::kRightItemCharge, IM_COL32_WHITE},
        //{RE::ActorValue::kArmorPerks, IM_COL32_WHITE},
        //{RE::ActorValue::kShieldPerks, IM_COL32_WHITE},
        //{RE::ActorValue::kWardDeflection, IM_COL32_WHITE},
        /*{RE::ActorValue::kVariable01, IM_COL32_WHITE},
        {RE::ActorValue::kVariable02, IM_COL32_WHITE},
        {RE::ActorValue::kVariable03, IM_COL32_WHITE},
        {RE::ActorValue::kVariable04, IM_COL32_WHITE},
        {RE::ActorValue::kVariable05, IM_COL32_WHITE},
        {RE::ActorValue::kVariable06, IM_COL32_WHITE},
        {RE::ActorValue::kVariable07, IM_COL32_WHITE},
        {RE::ActorValue::kVariable08, IM_COL32_WHITE},
        {RE::ActorValue::kVariable09, IM_COL32_WHITE},
        {RE::ActorValue::kVariable10, IM_COL32_WHITE},*/
        //{RE::ActorValue::kBowSpeedBonus, IM_COL32_WHITE},
        //{RE::ActorValue::kFavorActive, IM_COL32_WHITE},
        //{RE::ActorValue::kFavorsPerDay, IM_COL32_WHITE},
        //{RE::ActorValue::kFavorsPerDayTimer, IM_COL32_WHITE},
        //{RE::ActorValue::kLeftItemCharge, IM_COL32_WHITE},
        //{RE::ActorValue::kAbsorbChance, IM_COL32_WHITE},
        //{RE::ActorValue::kBlindness, IM_COL32_WHITE},
        //{RE::ActorValue::kWeaponSpeedMult, IM_COL32_WHITE},
        //{RE::ActorValue::kShoutRecoveryMult, IM_COL32_WHITE},
        //{RE::ActorValue::kBowStaggerBonus, IM_COL32_WHITE},
        //{RE::ActorValue::kTelekinesis, IM_COL32_WHITE},
        //{RE::ActorValue::kFavorPointsBonus, IM_COL32_WHITE},
        //{RE::ActorValue::kLastBribedIntimidated, IM_COL32_WHITE},
        //{RE::ActorValue::kLastFlattered, IM_COL32_WHITE},
        //{RE::ActorValue::kMovementNoiseMult, IM_COL32_WHITE},
        //{RE::ActorValue::kBypassVendorStolenCheck, IM_COL32_WHITE},
        //{RE::ActorValue::kBypassVendorKeywordCheck, IM_COL32_WHITE},
        //{RE::ActorValue::kWaitingForPlayer, IM_COL32_WHITE},
        {RE::ActorValue::kOneHandedModifier, col_OneHanded},
        {RE::ActorValue::kTwoHandedModifier, col_TwoHanded},
        {RE::ActorValue::kMarksmanModifier, col_Archery},
        {RE::ActorValue::kBlockModifier, col_Block},
        {RE::ActorValue::kSmithingModifier, col_Smithing},
        {RE::ActorValue::kHeavyArmorModifier, col_HeavyArmor},
        {RE::ActorValue::kLightArmorModifier, col_LightArmor},
        {RE::ActorValue::kPickpocketModifier, col_Pickpocket},
        {RE::ActorValue::kLockpickingModifier, col_Lockpicking},
        {RE::ActorValue::kSneakingModifier, col_Sneak},
        {RE::ActorValue::kAlchemyModifier, col_Alchemy},
        {RE::ActorValue::kSpeechcraftModifier, col_Speech},
        {RE::ActorValue::kAlterationModifier, col_Alteration},
        {RE::ActorValue::kConjurationModifier, col_Conjuration},
        {RE::ActorValue::kDestructionModifier, col_Destruction},
        {RE::ActorValue::kIllusionModifier, col_Illusion},
        {RE::ActorValue::kRestorationModifier, col_Restoration},
        {RE::ActorValue::kEnchantingModifier, col_Enchanting},
        /*{RE::ActorValue::kOneHandedSkillAdvance, IM_COL32_WHITE},
        {RE::ActorValue::kTwoHandedSkillAdvance, IM_COL32_WHITE},
        {RE::ActorValue::kMarksmanSkillAdvance, IM_COL32_WHITE},
        {RE::ActorValue::kBlockSkillAdvance, IM_COL32_WHITE},
        {RE::ActorValue::kSmithingSkillAdvance, IM_COL32_WHITE},
        {RE::ActorValue::kHeavyArmorSkillAdvance, IM_COL32_WHITE},
        {RE::ActorValue::kLightArmorSkillAdvance, IM_COL32_WHITE},
        {RE::ActorValue::kPickpocketSkillAdvance, IM_COL32_WHITE},
        {RE::ActorValue::kLockpickingSkillAdvance, IM_COL32_WHITE},
        {RE::ActorValue::kSneakingSkillAdvance, IM_COL32_WHITE},
        {RE::ActorValue::kAlchemySkillAdvance, IM_COL32_WHITE},
        {RE::ActorValue::kSpeechcraftSkillAdvance, IM_COL32_WHITE},
        {RE::ActorValue::kAlterationSkillAdvance, IM_COL32_WHITE},
        {RE::ActorValue::kConjurationSkillAdvance, IM_COL32_WHITE},
        {RE::ActorValue::kDestructionSkillAdvance, IM_COL32_WHITE},
        {RE::ActorValue::kIllusionSkillAdvance, IM_COL32_WHITE},
        {RE::ActorValue::kRestorationSkillAdvance, IM_COL32_WHITE},
        {RE::ActorValue::kEnchantingSkillAdvance, IM_COL32_WHITE},*/
        //{RE::ActorValue::kLeftWeaponSpeedMultiply, IM_COL32_WHITE},
        //{RE::ActorValue::kDragonSouls, IM_COL32_WHITE},
        {RE::ActorValue::kCombatHealthRegenMultiply, IM_COL32(255,30,30,255) },
        {RE::ActorValue::kOneHandedPowerModifier, col_OneHanded},
        {RE::ActorValue::kTwoHandedPowerModifier, col_TwoHanded},
        {RE::ActorValue::kMarksmanPowerModifier, col_Archery},
        {RE::ActorValue::kBlockPowerModifier, col_Block},
        {RE::ActorValue::kSmithingPowerModifier, col_Smithing},
        {RE::ActorValue::kHeavyArmorPowerModifier, col_HeavyArmor},
        {RE::ActorValue::kLightArmorPowerModifier, col_LightArmor},
        {RE::ActorValue::kPickpocketPowerModifier, col_Pickpocket},
        {RE::ActorValue::kLockpickingPowerModifier, col_Lockpicking},
        {RE::ActorValue::kSneakingPowerModifier, col_Sneak},
        {RE::ActorValue::kAlchemyPowerModifier, col_Alchemy},
        {RE::ActorValue::kSpeechcraftPowerModifier, col_Speech},
        {RE::ActorValue::kAlterationPowerModifier, col_Alteration},
        {RE::ActorValue::kConjurationPowerModifier, col_Conjuration},
        {RE::ActorValue::kDestructionPowerModifier, col_Destruction},
        {RE::ActorValue::kIllusionPowerModifier, col_Illusion},
        {RE::ActorValue::kRestorationPowerModifier, col_Restoration},
        {RE::ActorValue::kEnchantingPowerModifier, col_Enchanting},
        //{RE::ActorValue::kDragonRend, IM_COL32_WHITE},
        //{RE::ActorValue::kAttackDamageMult, IM_COL32_WHITE},
        {RE::ActorValue::kHealRateMult, IM_COL32(240,40,40,255) },
        {RE::ActorValue::kMagickaRateMult, IM_COL32(40,40,240,255) },
        {RE::ActorValue::kStaminaRateMult, IM_COL32(40,240,40,255) },
        //{RE::ActorValue::kWerewolfPerks, IM_COL32_WHITE},
        //{RE::ActorValue::kVampirePerks, IM_COL32_WHITE},
        //{RE::ActorValue::kGrabActorOffset, IM_COL32_WHITE},
        //{RE::ActorValue::kGrabbed, IM_COL32_WHITE},
        //{RE::ActorValue::kDEPRECATED05, IM_COL32_WHITE},
        {RE::ActorValue::kReflectDamage, IM_COL32_WHITE}
    };

    //Mod support:

    //Sacrosanct
    RE::TESGlobal* sacrosanct_VampireSpells_Vanilla_Power_Global_CanLamaesPyre = nullptr;
    RE::FormID sacrosanct_lamaes_pyre_id = 0U;

    //Growl
    RE::EffectSetting* growl_beast_form_cd_magic_effect = nullptr;
    
    template<typename T>
    void load_form_from_game(RE::FormID formId, const std::string_view & plugin, T** out_ptr, const std::string_view & name, RE::FormType type) {
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
        load_form_from_game(0x83E, "SpellHotbar.esp", &spellhotbar_spellproc_cd, "SpellHotbar_SpellProcCD", RE::FormType::MagicEffect);
        load_form_from_game(0x83F, "SpellHotbar.esp", &spellhotbar_apply_spellproc_cd, "SpellHotbar_ApplySpellProcCD", RE::FormType::Spell);

        load_form_from_game(0x840, "SpellHotbar.esp", &global_spellhotbar_perks_override, "SpellHotbar_BattleMage_OverridePerks", RE::FormType::Global);
        load_form_from_game(0x841, "SpellHotbar.esp", &global_spellhotbar_perks_timed_block_window, "SpellHotbar_BattleMage_TimedBlockWindow", RE::FormType::Global);
        load_form_from_game(0x842, "SpellHotbar.esp", &global_spellhotbar_perks_block_trigger_chance, "SpellHotbar_BattleMage_BlockProcChance", RE::FormType::Global);
        load_form_from_game(0x843, "SpellHotbar.esp", &global_spellhotbar_perks_power_attack_trigger_chance, "SpellHotbar_BattleMage_PowerAttackProcChance", RE::FormType::Global);
        load_form_from_game(0x844, "SpellHotbar.esp", &global_spellhotbar_perks_sneak_attack_trigger_chance, "SpellHotbar_BattleMage_SneakAttackProcChance", RE::FormType::Global);
        load_form_from_game(0x845, "SpellHotbar.esp", &global_spellhotbar_perks_crit_trigger_chance, "SpellHotbar_BattleMage_CritProcChance", RE::FormType::Global);
        load_form_from_game(0x84F, "SpellHotbar.esp", &global_spellhotbar_perks_proc_cooldown, "SpellHotbar_BattleMage_ProcCooldown", RE::FormType::Global);
        load_form_from_game(0x850, "SpellHotbar.esp", &global_spellhotbar_perks_require_halfcostperk, "SpellHotbar_BattleMage_RequireHalfCostPerk", RE::FormType::Global);

        load_form_from_game(0x83D, "SpellHotbar.esp", &spellhotbar_perk_cast_on_power_attack, "SpellHotbar_BattleMage_PerkProcPowerAttack", RE::FormType::Perk);
        load_form_from_game(0x846, "SpellHotbar.esp", &spellhotbar_perk_cast_on_sneak_attack, "SpellHotbar_BattleMage_PerkProcSneakAttack", RE::FormType::Perk);
        load_form_from_game(0x847, "SpellHotbar.esp", &spellhotbar_perk_cast_on_crit, "SpellHotbar_BattleMage_PerkProcCrit", RE::FormType::Perk);
        load_form_from_game(0x848, "SpellHotbar.esp", &spellhotbar_perk_cast_on_block, "SpellHotbar_BattleMage_PerkProcBlock", RE::FormType::Perk);
        load_form_from_game(0x849, "SpellHotbar.esp", &spellhotbar_perk_cast_on_concentration, "SpellHotbar_BattleMage_PerkProcConcentrationCast", RE::FormType::Perk);
        load_form_from_game(0x84A, "SpellHotbar.esp", &spellhotbar_perk_expert, "SpellHotbar_BattleMage_PerkExpert", RE::FormType::Perk);
        load_form_from_game(0x84B, "SpellHotbar.esp", &spellhotbar_perk_master, "SpellHotbar_BattleMage_PerkMaster", RE::FormType::Perk);


        load_form_from_game(0x13F42, "Skyrim.esm", &equip_slot_right_hand, "RightHand", RE::FormType::EquipSlot);
        load_form_from_game(0x13F43, "Skyrim.esm", &equip_slot_left_hand, "LeftHand", RE::FormType::EquipSlot);
        load_form_from_game(0x13F44, "Skyrim.esm", &equip_slot_either_hand, "EitherHand", RE::FormType::EquipSlot);
        load_form_from_game(0x13F45, "Skyrim.esm", &equip_slot_both_hand, "BothHands", RE::FormType::EquipSlot);
        load_form_from_game(0x25BEE, "Skyrim.esm", &equip_slot_voice, "Voice", RE::FormType::EquipSlot);

        //now loaded from csv for mod compatibility
        //load_form_from_game(0x153CD, "Skyrim.esm", &perk_alteration_dual_casting, "AlterationDualCasting", RE::FormType::Perk);
        //load_form_from_game(0x153CE, "Skyrim.esm", &perk_conjuration_dual_casting, "ConjurationDualCasting", RE::FormType::Perk);
        //load_form_from_game(0x153CF, "Skyrim.esm", &perk_destruction_dual_casting, "DestructionDualCasting", RE::FormType::Perk);
        //load_form_from_game(0x153D0, "Skyrim.esm", &perk_illusion_dual_casting, "IllusionDualCasting", RE::FormType::Perk);
        //load_form_from_game(0x153D1, "Skyrim.esm", &perk_restoration_dual_casting, "RestorationDualCasting", RE::FormType::Perk);

        load_form_from_game(0x10E2EA, "Skyrim.esm", &sound_NPCHumanEatSoup, "NPCHumanEatSoup", RE::FormType::SoundRecord);
        load_form_from_game(0xB6435, "Skyrim.esm", &sound_ITMPotionUse, "ITMPostionUse", RE::FormType::SoundRecord);

        load_form_from_game(0x092C48, "Skyrim.esm", &werewolf_change_power, "Beast Form", RE::FormType::Spell);

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


        //mod Support:
        //Sacrosanct Lamae's pyre: can only cast when global is true
        constexpr std::string_view sacrosanct_esp_name = "Sacrosanct - Vampires of Skyrim.esp";
        if (RE::TESDataHandler::GetSingleton()->GetModIndex(sacrosanct_esp_name).has_value()) {
            logger::info("Loading Sacrosanct compatibility...");
            load_form_from_game(0x05EB00, sacrosanct_esp_name, &sacrosanct_VampireSpells_Vanilla_Power_Global_CanLamaesPyre, "SCS_VampireSpells_Vanilla_Power_Global_CanLamaesPyre", RE::FormType::Global);
            auto form_sacrosanct_lamaes_pyre = SpellHotbar::GameData::get_form_from_file(0x05EAFD, sacrosanct_esp_name);
            if (form_sacrosanct_lamaes_pyre) {
                sacrosanct_lamaes_pyre_id = form_sacrosanct_lamaes_pyre->GetFormID();
            }
        }

        //Growl werewolf cd tracking
        constexpr std::string_view growl_esp_name = "Growl - Werebeasts of Skyrim.esp";
        if (RE::TESDataHandler::GetSingleton()->GetModIndex(growl_esp_name).has_value()) {
            logger::info("Loading Growl compatibility...");
            load_form_from_game(0x13A7D6, growl_esp_name, &growl_beast_form_cd_magic_effect, "HRI_Lycan_Effect_BeastFormCooldown", RE::FormType::MagicEffect);
        }
    }

    void reload_data()
    { 
        logger::info("Reloading Spell Data...");
        spell_cast_info.clear();
        spell_cd_magiceffect_tracking.clear();
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
        else if (slot_index == SpellHotbar::Input::keybind_id::oblivion_cast) {
            return SpellHotbar::Input::key_oblivion_cast.get_dx_scancode();
        }
        else if (slot_index == SpellHotbar::Input::keybind_id::oblivion_potion) {
            return SpellHotbar::Input::key_oblivion_potion.get_dx_scancode();
        }
        else if (slot_index == SpellHotbar::Input::keybind_id::dummy_key_vanilla_shout) {

            return Input::get_shout_key_dxcode();
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
                    key_text = key_names.at(Input::mod_1.get_dx_scancode()).second + "-" + key_text;
                    break;
                case SpellHotbar::key_modifier::shift:
                    key_text = key_names.at(Input::mod_2.get_dx_scancode()).second + "-" + key_text;
                    break;
                case SpellHotbar::key_modifier::alt:
                    key_text = key_names.at(Input::mod_3.get_dx_scancode()).second + "-" + key_text;
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
                    /*if (left_hand != nullptr && left_hand->GetFormType() == RE::FormType::Armor) {
                        return EquippedType::STAFF_SHIELD;
                    }
                    return EquippedType::SPELL;*/
                return EquippedType::STAFF_SHIELD;
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

        if (Bars::disable_non_modifier_bar && !(Input::mod_1.isDown() || Input::mod_2.isDown() || Input::mod_3.isDown())) {
            //Main bar not used -> hide
            return std::make_tuple(false, fast_fade);
        }

        if (Input::mod_show_bar.isValidBound() && Input::mod_show_bar.isDown()) {
            return std::make_tuple(true, fast_fade);
        }

        if (Bars::oblivion_bar_press_show_timer > 0.0f) {
            return std::make_tuple(true, fast_fade);
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

    std::tuple<bool, float> shouldShowOblivionHUDBar() {
        constexpr float fast_fade = 0.1f;
        constexpr float slow_fade = 0.5f;

        if(!Input::is_oblivion_mode())
        {
            return std::make_tuple(false, fast_fade);
        }

        auto* ui = RE::UI::GetSingleton();
        if (!ui || ui->GameIsPaused() || !ui->IsCursorHiddenWhenTopmost() || !ui->IsShowingMenus() || !ui->GetMenu<RE::HUDMenu>() || ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME))
        {
            return std::make_tuple(false, fast_fade);
        }
        if (const auto* control_map = RE::ControlMap::GetSingleton();
            !control_map || !control_map->IsMovementControlsEnabled())
        {
            return std::make_tuple(false, fast_fade);
        }

        if (Input::mod_oblivion_show_bar.isValidBound() && Input::mod_oblivion_show_bar.isDown()) {
            return std::make_tuple(true, fast_fade);
        }

        auto* pc = RE::PlayerCharacter::GetSingleton();
        if (pc) {

            if (Bars::oblivion_bar_show_setting == Bars::bar_show_mode::never) return std::make_tuple(false, slow_fade);
            if (Bars::oblivion_bar_show_setting == Bars::bar_show_mode::always) return std::make_tuple(true, slow_fade);

            bool in_combat = pc->IsInCombat();
            bool weapon_drawn = pc->AsActorState()->IsWeaponDrawn();

            switch (Bars::oblivion_bar_show_setting) {
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
        if (form->GetFormType() == RE::FormType::Spell || form->GetFormType() == RE::FormType::Scroll) {
            RE::SpellItem* spell = form->As<RE::SpellItem>();
            if (spell->GetSpellType() == RE::MagicSystem::SpellType::kLesserPower) {
            
                ret = DefaultIconType::LESSER_POWER;
            } else if (spell->GetSpellType() == RE::MagicSystem::SpellType::kPower) {
                
                ret = DefaultIconType::GREATER_POWER;
            } else if (spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell || spell->GetSpellType() == RE::MagicSystem::SpellType::kScroll) {
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
        else if (form->GetFormType() == RE::FormType::AlchemyItem) {

            auto alch = form->As<RE::AlchemyItem>();
            auto gold_Val = alch->GetGoldValue();
            if (alch->IsPoison()) {
                if (gold_Val >= 300) {
                    ret = DefaultIconType::GENERIC_POISON_LARGE;
                }
                else if (gold_Val >= 100) {
                    ret = DefaultIconType::GENERIC_POISON;
                }
                else {
                    ret = DefaultIconType::GENERIC_POISON_SMALL;
                }
            }
            else {
                if (gold_Val >= 300) {
                    ret = DefaultIconType::GENERIC_POTION_LARGE;
                }
                else if (gold_Val >= 100) {
                    ret = DefaultIconType::GENERIC_POTION;
                }
                else {
                    ret = DefaultIconType::GENERIC_POTION_SMALL;
                }
            }
        }
        return ret;
    }

    void set_spell_cast_data(RE::FormID spell, Spell_cast_data&& data) {
        spell_cast_info.insert_or_assign(spell, std::move(data));
    }

    void set_spell_cooldown_effect(RE::FormID spell, RE::FormID cd_effect)
    {
        spell_cd_magiceffect_tracking.insert_or_assign(spell, cd_effect);
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

    void load_user_spell_data_from_SKSE_save(SKSE::SerializationInterface* a_intfc, uint32_t version)
    {
        user_spell_cast_info.clear();
        uint16_t size{ 0Ui16 };
        if (a_intfc->ReadRecordData(&size, sizeof(uint16_t))) {
            for (uint16_t i = 0Ui16; i < size; i++) {
                auto data = User_custom_spelldata::deserialize(a_intfc, version);
                user_spell_cast_info.insert_or_assign(data.m_form_id, data);
            }
        }
        else {
            logger::error("Error loading custom spelldata from SKSE save");
        }
    }

    void save_user_spell_data_to_SKSE_save(SKSE::SerializationInterface* a_intfc)
    {
        uint16_t size = static_cast<uint16_t>(std::min(user_spell_cast_info.size(), static_cast<size_t>(std::numeric_limits<uint16_t>::max())));
        a_intfc->WriteRecordData(&size, sizeof(uint16_t));
        for (const auto& [k, v] : user_spell_cast_info) {
            v.serialize(a_intfc);
        }
    }

    void load_user_entry_info_from_SKSE_save(SKSE::SerializationInterface* a_intfc, uint32_t version)
    {
        user_custom_entry_info.clear();
        uint16_t size{ 0Ui16 };
        if (a_intfc->ReadRecordData(&size, sizeof(uint16_t))) {
            for (uint16_t i = 0Ui16; i < size; i++) {
                auto data = User_custom_entry::deserialize(a_intfc, version);
                user_custom_entry_info.insert_or_assign(data.m_form_id, data);
            }
        }
        else {
            logger::error("Error loading custom icon info from SKSE save");
        }
    }

    void save_user_entry_info_to_SKSE_save(SKSE::SerializationInterface* a_intfc)
    {
        uint16_t size = static_cast<uint16_t>(std::min(user_custom_entry_info.size(), static_cast<size_t>(std::numeric_limits<uint16_t>::max())));
        a_intfc->WriteRecordData(&size, sizeof(uint16_t));
        for (const auto& [k, v] : user_custom_entry_info) {
            v.serialize(a_intfc);
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

    bool is_skill_on_cd(RE::FormID skill)
    {
        bool ret{ false };
        if (is_on_binary_cd(skill)) {
            ret = true;
        } else if (get_special_cd(skill) > 0.0f) {
            ret = true;
        }
        else {
            RE::Calendar* cal = RE::Calendar::GetSingleton();
            if (cal) {
                float game_time = cal->GetCurrentGameTime();

                if (gametime_cooldowns.contains(skill)) {
                    auto& cd_info = gametime_cooldowns.at(skill);
                    ret = !cd_info.is_expired(game_time);
                }
            }
        }

        return ret;
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
        if (!key.empty()) {
            if (spell_effects_key_indices) {
                if (spell_effects_key_indices.get()->contains(key)) {
                    ret = spell_effects_key_indices.get()->at(key);
                }
                else {
                    logger::warn("Unknown Casteffect: {}", key);
                }
            }
            else {
                logger::error("Spell effect key index map is called at incorrect time!");
            }
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

    SlottedSkill get_current_spell_info_in_slot(size_t index) {
        SlottedSkill ret;
        if (index < max_bar_size) {
            uint32_t bar_id = Bars::getCurrentHotbar_ingame();
            if (Bars::hotbars.contains(bar_id)) {
                auto& bar = Bars::hotbars.at(bar_id);
                auto [skill, inherited] = bar.get_skill_in_bar_with_inheritance(static_cast<int>(index), Bars::get_current_modifier(), false);
                ret = skill;
            }
        } else if (index == Input::keybind_id::oblivion_cast) {
            ret = oblivion_bar.get_slotted_spell();
        }
        else if (index == Input::keybind_id::oblivion_potion) {
            ret = oblivion_bar.get_slotted_potion();
        }

        if (GameData::is_clear_spell(ret.formID)) {
            ret.formID = 0U;
            ret.hand = hand_mode::voice;
            ret.type = slot_type::blocked;
        }
        return ret;
    }

     RE::FormID get_current_spell_in_slot(size_t index) {
         auto skill = get_current_spell_info_in_slot(index);
         return skill.formID;
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
     inline bool _check_perk(RE::PlayerCharacter* pc, RE::BGSPerk* perk) {
         return perk != nullptr && pc->HasPerk(perk);
     }

     bool player_can_dualcast_spell(RE::SpellItem* spell)
     {
         bool hasPerk{ false };
         auto pc = RE::PlayerCharacter::GetSingleton();
         if (spell && pc) {
             RE::ActorValue school = spell->GetAssociatedSkill();
         
             switch (school) {
             case RE::ActorValue::kAlteration:
                 hasPerk = _check_perk(pc, perk_alteration_dual_casting);
                 break;
             case RE::ActorValue::kConjuration:
                 hasPerk = _check_perk(pc, perk_conjuration_dual_casting);
                 break;
             case RE::ActorValue::kDestruction:
                 hasPerk = _check_perk(pc, perk_destruction_dual_casting);
                 break;
             case RE::ActorValue::kIllusion:
                 hasPerk = _check_perk(pc, perk_illusion_dual_casting);;
                 break;
             case RE::ActorValue::kRestoration:
                 hasPerk = _check_perk(pc, perk_restoration_dual_casting);
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
     hand_mode set_weapon_dependent_casting_source(hand_mode hand, bool dual_cast)
     {
         hand_mode used_hand = hand;
         if (SpellHotbar::GameData::global_casting_source) {
             //Set casting source according to spell anim
             RE::MagicSystem::CastingSource castsource = RE::MagicSystem::CastingSource::kOther;

             switch (hand) {
             case hand_mode::auto_hand:
                 castsource = SpellHotbar::GameData::get_cast_hand_from_equip();
                 if (castsource == RE::MagicSystem::CastingSource::kLeftHand || dual_cast) {
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

     GameData::Spell_cast_data get_spell_data(const RE::TESForm* spell, bool fill_defaults, bool include_custom_user_data)
     {
         GameData::Spell_cast_data data;
         RE::FormID id = spell->GetFormID();

         if (spell_cast_info.contains(id)) {
             data = spell_cast_info.at(id);
         }

         //fill in custom user data
         if (include_custom_user_data) {
             if (user_spell_cast_info.contains(id)) {
                 auto& user_data = user_spell_cast_info.at(id);
                 data.fill_and_override_from_non_default_values(user_data.m_spell_data);
             }
         }

         //Fill default values
         if (fill_defaults) {
             if (spell->GetFormType() == RE::FormType::Spell || spell->GetFormType() == RE::FormType::Scroll) {
                 data.fill_default_values_from_spell(spell->As<RE::SpellItem>());
             }
             else if (spell->GetFormType() == RE::FormType::Shout) {
                 data.fill_default_values_from_shout(spell->As<RE::TESShout>());
             }
         }

         return data;
     }

     void add_animation_data(const std::string& name, int anim_id)
     {
         animation_names.insert_or_assign(anim_id, name);
     }

     bool form_has_special_icon(RE::TESForm* form)
     {
         return form == spellhotbar_toggle_dualcast || form == spellhotbar_unbind_slot;
     }

     int count_item_in_inv(RE::FormID form)
     {
         int count{ 0 };
         auto pc = RE::PlayerCharacter::GetSingleton();
         if (pc) {
             auto refs = pc->GetInventoryCounts([form](const RE::TESBoundObject& object) { return object.formID == form; });
             for (auto& [k, v] : refs) {
                 count = v;
             }
         }
         return count;
     }

     bool is_on_binary_cd(RE::FormID skill)
     {
         bool can_cast{ true };
         if (sacrosanct_lamaes_pyre_id != 0 && sacrosanct_lamaes_pyre_id == skill) {
             can_cast = sacrosanct_VampireSpells_Vanilla_Power_Global_CanLamaesPyre->value != 0.0f;
         }

         return !can_cast;
     }

     inline float get_magic_effect_cooldown(RE::FormID effect_id) {
         float cd{ 0.0f };
         auto pc = RE::PlayerCharacter::GetSingleton();
         auto magic_target = pc->AsMagicTarget();
         if (magic_target) {
             auto effect_list = magic_target->GetActiveEffectList();

             if (effect_list) {
                 bool found{ false };
                 for (auto it = effect_list->begin(); it != effect_list->end() && !found; ++it) {

                     auto effect = *it;
                     if (effect->effect->baseEffect->GetFormID() == effect_id) {
                         found = true;
                         float max_dur = effect->duration;
                         float elapsed = effect->elapsedSeconds;

                         if (elapsed < max_dur)
                         {
                             cd = elapsed / max_dur;
                         }
                     }
                 }
             }
         }
         return cd;
     }

     float get_special_cd(RE::FormID formID)
     {
         float cd{ 0.0f };
         //Wereworlf CD if Growl is loaded
         if (formID == werewolf_change_power->GetFormID() && growl_beast_form_cd_magic_effect != nullptr) {
             cd = get_magic_effect_cooldown(growl_beast_form_cd_magic_effect->GetFormID());
         }

         if (spell_cd_magiceffect_tracking.contains(formID)) {
             cd = get_magic_effect_cooldown(spell_cd_magiceffect_tracking.at(formID));
         }

         return cd;
     }

     bool player_has_trigger_perk(RE::BGSPerk* perk)
     {
         bool ret{ false };
         if (global_spellhotbar_perks_override && global_spellhotbar_perks_override->value != 0.0f) {
             ret = true;
         }
         else {
             auto pc = RE::PlayerCharacter::GetSingleton();
             if (pc) {
                 ret = pc->HasPerk(perk);
             }
         }
         return ret;
     }

     bool calc_random_proc(RE::TESGlobal* chance)
     {
         bool proc{ false };
         if (chance) {
             if (chance->value >= 1.0f) {
                 proc = true;
             }
             else if (chance->value <= 0.0f) {
                 proc = false;
             }
             else {
                 proc = random_dist(random_engine) < chance->value;
             }
         }

         return proc;
     }

     uint16_t chose_default_anim_for_spell(const RE::TESForm* form, int anim, bool anim2) {
         return Spell_cast_data::chose_default_anim_for_spell(form, anim, anim2);
     }
}
