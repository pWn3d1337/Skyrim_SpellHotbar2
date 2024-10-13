#pragma once
#include "../bar/hotbar.h"
#include "../logger/logger.h"
#include "spell_cast_data.h"
#include "user_custom_spelldata.h"
#include "../bar/oblivion_bar.h"

namespace SpellHotbar::GameData {

    class User_custom_spelldata; //forward declaration

    enum class EquippedType
    {
        FIST,
        ONEHAND_EMPTY,
        ONEHAND_SHIELD,
        ONEHAND_SPELL,
        DUAL_WIELD,
        TWOHAND,
        BOW,
        SPELL,
        CROSSBOW,
        STAFF_SHIELD
    };

    enum class DefaultIconType : uint32_t {
        UNKNOWN = 0U,
        BAR_EMPTY,
        BAR_OVERLAY,
        BAR_HIGHLIGHT,
        UNBIND_SLOT,
        LESSER_POWER,
        GREATER_POWER,
        DESTRUCTION_FIRE_NOVICE,
        DESTRUCTION_FIRE_APPRENTICE,
        DESTRUCTION_FIRE_ADEPT,
        DESTRUCTION_FIRE_EXPERT,
        DESTRUCTION_FIRE_MASTER,
        DESTRUCTION_FROST_NOVICE,
        DESTRUCTION_FROST_APPRENTICE,
        DESTRUCTION_FROST_ADEPT,
        DESTRUCTION_FROST_EXPERT,
        DESTRUCTION_FROST_MASTER,
        DESTRUCTION_SHOCK_NOVICE,
        DESTRUCTION_SHOCK_APPRENTICE,
        DESTRUCTION_SHOCK_ADEPT,
        DESTRUCTION_SHOCK_EXPERT,
        DESTRUCTION_SHOCK_MASTER,
        DESTRUCTION_GENERIC_NOVICE,
        DESTRUCTION_GENERIC_APPRENTICE,
        DESTRUCTION_GENERIC_ADEPT,
        DESTRUCTION_GENERIC_EXPERT,
        DESTRUCTION_GENERIC_MASTER,
        ALTERATION_NOVICE,
        ALTERATION_APPRENTICE,
        ALTERATION_ADEPT,
        ALTERATION_EXPERT,
        ALTERATION_MASTER,
        RESTORATION_FRIENDLY_NOVICE,
        RESTORATION_FRIENDLY_APPRENTICE,
        RESTORATION_FRIENDLY_ADEPT,
        RESTORATION_FRIENDLY_EXPERT,
        RESTORATION_FRIENDLY_MASTER,
        RESTORATION_HOSTILE_NOVICE,
        RESTORATION_HOSTILE_APPRENTICE,
        RESTORATION_HOSTILE_ADEPT,
        RESTORATION_HOSTILE_EXPERT,
        RESTORATION_HOSTILE_MASTER,
        ILLUSION_FRIENDLY_NOVICE,
        ILLUSION_FRIENDLY_APPRENTICE,
        ILLUSION_FRIENDLY_ADEPT,
        ILLUSION_FRIENDLY_EXPERT,
        ILLUSION_FRIENDLY_MASTER,
        ILLUSION_HOSTILE_NOVICE,
        ILLUSION_HOSTILE_APPRENTICE,
        ILLUSION_HOSTILE_ADEPT,
        ILLUSION_HOSTILE_EXPERT,
        ILLUSION_HOSTILE_MASTER,
        CONJURATION_BOUND_WEAPON_NOVICE,
        CONJURATION_BOUND_WEAPON_APPRENTICE,
        CONJURATION_BOUND_WEAPON_ADEPT,
        CONJURATION_BOUND_WEAPON_EXPERT,
        CONJURATION_BOUND_WEAPON_MASTER,
        CONJURATION_SUMMON_NOVICE,
        CONJURATION_SUMMON_APPRENTICE,
        CONJURATION_SUMMON_ADEPT,
        CONJURATION_SUMMON_EXPERT,
        CONJURATION_SUMMON_MASTER,
        SHOUT_GENERIC,
        SINGLE_CAST,
        DUAL_CAST,
        SCROLL_OVERLAY,
        NO_OVERLAY,
        GENERIC_POTION,
        GENERIC_POTION_SMALL,
        GENERIC_POTION_LARGE,
        GENERIC_POISON,
        GENERIC_POISON_SMALL,
        GENERIC_POISON_LARGE,
        GENERIC_FOOD,
        GENERIC_FOOD_SOUP,
        GENERIC_FOOD_DRINK
    };

    struct Gametime_cooldown_value {
        float readytime; //Gametime when ready again
        float duration; //total cooldown duration

        Gametime_cooldown_value(float readytime, float duration);
        float get_progress(float current_game_time);
        bool is_expired(float current_game_time);
    };

    enum class custom_transform_spell_type: uint8_t {
        regular = 0U, //regular inventory
        fav_menu, //use fave menu to bind, but still cast all trough bar
        fav_menu_switch // use fav menu, and switch spells instead of casting
    };

    struct Transformation_data {
        uint32_t bar_id;
        custom_transform_spell_type casting_type;
    };

    struct Slotted_Skill_Data {
        RE::FormID form_id;
        slot_type slot;
        consumed_type consumed;
        hand_mode hand;
        bool inherited;
    };

    extern RE::TESGlobal* global_animation_type;
    extern RE::TESGlobal* global_casting_source;
    extern RE::TESGlobal* global_vampire_lord_equip_mode;
    extern RE::TESGlobal* global_casting_conc_spell;

    extern RE::SpellItem* spellhotbar_castfx_spell;
    extern RE::EffectSetting* spellhotbar_spellproc_cd;
    extern RE::SpellItem* spellhotbar_apply_spellproc_cd;

    extern RE::TESGlobal* global_spellhotbar_perks_override;
    extern RE::TESGlobal* global_spellhotbar_perks_timed_block_window;
    extern RE::TESGlobal* global_spellhotbar_perks_block_trigger_chance;
    extern RE::TESGlobal* global_spellhotbar_perks_power_attack_trigger_chance;
    extern RE::TESGlobal* global_spellhotbar_perks_sneak_attack_trigger_chance;
    extern RE::TESGlobal* global_spellhotbar_perks_crit_trigger_chance;
    extern RE::TESGlobal* global_spellhotbar_perks_proc_cooldown;
    extern RE::TESGlobal* global_spellhotbar_perks_require_halfcostperk;

    extern RE::BGSPerk* spellhotbar_perk_cast_on_power_attack;
    extern RE::BGSPerk* spellhotbar_perk_cast_on_sneak_attack;
    extern RE::BGSPerk* spellhotbar_perk_cast_on_crit;
    extern RE::BGSPerk* spellhotbar_perk_cast_on_block;
    extern RE::BGSPerk* spellhotbar_perk_expert;
    extern RE::BGSPerk* spellhotbar_perk_master;

    extern RE::BGSEquipSlot* equip_slot_right_hand;
    extern RE::BGSEquipSlot* equip_slot_left_hand;
    extern RE::BGSEquipSlot* equip_slot_either_hand;
    extern RE::BGSEquipSlot* equip_slot_both_hand;
    extern RE::BGSEquipSlot* equip_slot_voice;

    extern RE::BGSPerk* perk_alteration_dual_casting;
    extern RE::BGSPerk* perk_conjuration_dual_casting;
    extern RE::BGSPerk* perk_destruction_dual_casting;
    extern RE::BGSPerk* perk_illusion_dual_casting;
    extern RE::BGSPerk* perk_restoration_dual_casting;

    extern RE::BGSSoundDescriptorForm* sound_ITMPotionUse;
    extern RE::BGSSoundDescriptorForm* sound_NPCHumanEatSoup;

    extern std::unordered_map<RE::FormID, Spell_cast_data> spell_cast_info;
    extern std::vector<std::tuple<RE::BGSArtObject*, RE::BGSArtObject*, const std::string>> spell_casteffect_art;

    extern std::unordered_map<int, std::pair<std::string, std::string>> key_names;

    extern std::unordered_map<int, std::string> animation_names;

    extern std::unordered_map<RE::FormID, GameData::User_custom_spelldata> user_spell_cast_info;
    extern std::unordered_map<RE::FormID, GameData::User_custom_entry> user_custom_entry_info;

    extern std::unordered_map<RE::ActorValue, ImU32> potion_color_mapping;

    extern float potion_gcd;
    extern float block_timer;

    extern Bars::OblivionBar oblivion_bar;

    void load_from_SKSE_save(SKSE::SerializationInterface* a_intfc);
    void save_to_SKSE_save(SKSE::SerializationInterface* a_intfc);

    void load_user_spell_data_from_SKSE_save(SKSE::SerializationInterface* a_intfc, uint32_t version);
    void save_user_spell_data_to_SKSE_save(SKSE::SerializationInterface* a_intfc);

    void load_user_entry_info_from_SKSE_save(SKSE::SerializationInterface* a_intfc, uint32_t version);
    void save_user_entry_info_to_SKSE_save(SKSE::SerializationInterface* a_intfc);

    //Called when game data is available
    void onDataLoad();

    //read the currently bound key for a spell from global var
    int get_spell_keybind(int slot_index);

    std::string get_keybind_text(int slot_index, key_modifier mod);

    inline RE::TESForm* get_form_from_file(const uint32_t formID, const std::string_view& pluginFile)
    {
        static const auto data_handler = RE::TESDataHandler::GetSingleton();
        if (data_handler != nullptr) {
            return data_handler->LookupForm(formID, pluginFile);
        } else {
            logger::error("data_handler is null");
            return nullptr;
        }
    }

    void load_keynames_file();

    void set_spell_cast_data(RE::FormID spell, Spell_cast_data&& data);
    
    void set_spell_cooldown_effect(RE::FormID spell, RE::FormID cd_effect);

    void reload_data();

    void add_gametime_cooldown(RE::FormID skill, float hours, bool update_existing);

    void add_gametime_cooldown_with_timescale(RE::FormID skill, float days, bool update_existing);

    void purge_expired_gametime_cooldowns();

    bool is_skill_on_cd(RE::FormID skill);

    std::tuple<float, float> get_gametime_cooldown(float curr_game_time, RE::FormID skill);

    std::string resolve_spellname(RE::FormID formID);

    EquippedType getPlayerEquipmentType();
    bool isVampireLord();
    bool isWerewolf();
    uint32_t isCustomTransform();
    custom_transform_spell_type getCustomTransformCasttype();

    bool hasFavMenuSlotBinding();

    std::tuple<bool, float> shouldShowHUDBar();
    std::tuple<bool, float> shouldShowOblivionHUDBar();

    DefaultIconType get_fallback_icon_type(RE::TESForm* form);

    void add_casteffect(const std::string& key, RE::BGSArtObject* left_art, RE::BGSArtObject* right_art);
    /*
    * Should only be called during spell data loading, otherwise resolution map will be null
    */
    size_t get_cast_effect_id(const std::string& key);

    void update_spell_casting_art_and_time(size_t art_index, uint32_t casttime, hand_mode hands);

    //void update_spell_casting_art_and_time(size_t art_index, uint32_t casttime);

    /**
     * Get spell at index of current ingame bar
     */
    RE::FormID get_current_spell_in_slot(size_t index);

    SlottedSkill get_current_spell_info_in_slot(size_t index);

    /*
    * Is the passed spell the "unbind slot" spell?
    */
    bool is_clear_spell(RE::FormID spell);

    /*
    * Is the passed spell the "toggle dual cast" Power?
    */
    bool is_toggle_dualcast_spell(RE::FormID spell);

    bool should_dual_cast();

    void add_custom_tranformation(uint32_t bar, std::string name, RE::FormID race_id,
                                  custom_transform_spell_type cast_type);

    RE::MagicSystem::CastingSource get_cast_hand_from_equip();

    bool player_can_dualcast_spell(RE::SpellItem* spell);

    void set_animtype_global(int value);

    hand_mode set_weapon_dependent_casting_source(hand_mode hand, bool dual_cast);

    void reset_animation_vars();

    void start_cast_timer();
    void advance_cast_timer(float delta);

    uint16_t chose_default_anim_for_spell(const RE::TESForm* form, int anim, bool anim2);

    GameData::Spell_cast_data get_spell_data(const RE::TESForm* spell, bool fill_defaults = true, bool include_custom_user_data = true);

    void add_animation_data(const std::string& name, int anim_id);

    bool form_has_special_icon(RE::TESForm* form);

    int count_item_in_inv(RE::FormID form);

    bool is_on_binary_cd(RE::FormID skill);

    float get_special_cd(RE::FormID formID);

    /*
    * Checks for this perk, or if the ignore perks global is set
    */
    bool player_has_trigger_perk(RE::BGSPerk* perk);

    bool calc_random_proc(RE::TESGlobal* chance);
}

