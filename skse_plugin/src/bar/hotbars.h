#pragma once
#include "hotbar.h"

namespace SpellHotbar::Bars {
    enum class bar_show_mode
    { 
        always = 0,
        never,
        combat,
        drawn_weapon,
        combat_or_drawn,
        combat_and_drawn
    };

    enum text_show_mode : uint8_t
    { 
        never = 0U,
        fade,
        always
    };

    enum class anchor_point : uint8_t {
        BOTTOM = 0Ui8,
        LEFT,
        TOP,
        RIGHT,
        BOTTOM_LEFT,
        TOP_LEFT,
        BOTTOM_RIGHT,
        TOP_RIGHT,
        CENTER
    };

    enum class bar_layout : uint8_t {
        BARS = 0Ui8,
        CIRCLE,
        CROSS
    };

    extern std::unordered_map<uint32_t, Hotbar> hotbars;
    extern bool disable_non_modifier_bar;
    extern uint8_t barsize;

    extern float slot_scale;
    extern float offset_x;
    extern float offset_y;
    extern float slot_spacing;

    extern float oblivion_slot_scale;
    extern float oblivion_offset_x;
    extern float oblivion_offset_y;
    extern float oblivion_slot_spacing;
    extern anchor_point oblivion_bar_anchor_point;
    extern bool oblivion_bar_show_power;
    extern bar_show_mode oblivion_bar_show_setting;
    extern float oblivion_bar_held_show_time_threshold;
    extern bool oblivion_bar_vertical;

    extern text_show_mode text_show_setting;
    extern bar_show_mode bar_show_setting;
    extern bar_show_mode bar_show_setting_vampire_lord;
    extern bar_show_mode bar_show_setting_werewolf;

    extern bool use_default_bar_when_sheathed;
    extern bool disable_menu_rendering;

    extern anchor_point bar_anchor_point;

    extern uint8_t bar_row_len;
    extern bar_layout layout;
    extern float bar_circle_radius;
    extern float bar_cross_distance;

    constexpr uint32_t MAIN_BAR = 'MAIN';
    constexpr uint32_t MAIN_BAR_SNEAK = MAIN_BAR + 1;

    constexpr uint32_t MELEE_BAR = 'MELE';
    constexpr uint32_t MELEE_BAR_SNEAK = MELEE_BAR + 1;

    constexpr uint32_t ONE_HAND_SHIELD_BAR = '1HSD';
    constexpr uint32_t ONE_HAND_SHIELD_BAR_SNEAK = ONE_HAND_SHIELD_BAR + 1;

    constexpr uint32_t ONE_HAND_SPELL_BAR = '1HSP';
    constexpr uint32_t ONE_HAND_SPELL_BAR_SNEAK = ONE_HAND_SPELL_BAR + 1;

    constexpr uint32_t DUAL_WIELD_BAR = '1HDW';
    constexpr uint32_t DUAL_WIELD_BAR_SNEAK = DUAL_WIELD_BAR + 1;

    constexpr uint32_t TWO_HANDED_BAR = '2HND';
    constexpr uint32_t TWO_HANDED_BAR_SNEAK = TWO_HANDED_BAR + 1;

    constexpr uint32_t RANGED_BAR = 'RNGD';
    constexpr uint32_t RANGED_BAR_SNEAK = RANGED_BAR + 1;

    constexpr uint32_t MAGIC_BAR = 'MAGC';
    constexpr uint32_t MAGIC_BAR_SNEAK = MAGIC_BAR + 1;

    constexpr uint32_t VAMPIRE_LORD_BAR = 'VMPL';

    constexpr uint32_t WEREWOLF_BAR = 'WWOL';

    inline std::unordered_map<uint32_t, std::string> bar_names = {
        {MAIN_BAR, "Default"},
        {MAIN_BAR_SNEAK, "Sneak"},
        {MELEE_BAR, "Melee"},
        {MELEE_BAR_SNEAK, "Sneak Melee"},
        {ONE_HAND_SHIELD_BAR, "1H + Shield"},
        {ONE_HAND_SHIELD_BAR_SNEAK, "Sneak 1H + Shield"},
        {ONE_HAND_SPELL_BAR, "1H + Spell"},
        {ONE_HAND_SPELL_BAR_SNEAK, "Sneak 1H + Spell"},
        {DUAL_WIELD_BAR, "Dual Wield"},
        {DUAL_WIELD_BAR_SNEAK, "Sneak Dual Wield"},
        {TWO_HANDED_BAR, "Two-Handed"},
        {TWO_HANDED_BAR_SNEAK, "Sneak Two-Handed"},
        {RANGED_BAR, "Ranged"},
        {RANGED_BAR_SNEAK, "Sneak Ranged"},
        {MAGIC_BAR, "Magic"},
        {MAGIC_BAR_SNEAK, "Sneak Magic"},
        {VAMPIRE_LORD_BAR, "Vampire Lord"},
        {WEREWOLF_BAR, "Werewolf"}};

    // non save-persistent vars:
    extern uint32_t menu_bar_id;
    extern float oblivion_bar_press_show_timer;

    // functions
    uint32_t getCurrentHotbar_ingame();

    void update_oblivion_bar_press_show_timer(float delta);

    void init();

    uint32_t getPreviousMenuBar(uint32_t current_id);
    uint32_t getNextMenuBar(uint32_t current_id);

    key_modifier get_current_modifier();

    bool save_bars_to_json(std::string path);

    void clear_bars();

    bool load_bars_from_json(std::string path);

    void add_special_bar(uint32_t name, std::optional<uint32_t> parent = std::nullopt);

    int get_num_rows(int barsize, int row_len);

}