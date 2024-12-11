#pragma once

namespace SpellHotbar::Storage {
    /*
    * 2 - SpellHotbar2, alpha until 0.0.4
    * 3 - SpellHotbar2, alpha 0.0.5
    */
    constexpr uint32_t save_format = 3U;

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