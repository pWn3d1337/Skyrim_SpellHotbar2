#pragma once

namespace SpellHotbar::Storage {
    constexpr uint32_t save_format = 2U;

    extern std::array<RE::FormID, 12> hotbar_main;

    enum class menu_slot_type {
        magic_menu = 0,
        vampire_lord = 1,
        werewolf = 2,
        custom_favmenu = 3,
    };

    /**
    * Store variables on game save
    */
    void SaveCallback(SKSE::SerializationInterface* a_intfc);

    /**
     * Load variables on game load
     */
    void LoadCallback(SKSE::SerializationInterface* a_intfc);

    bool slotSpell(RE::FormID form, size_t index, menu_slot_type slot_type);

}