#pragma once


//#define DEBUG_LOG_SERIALIZATION

namespace SpellHotbar::Storage {
    /*
    * 2 - SpellHotbar2, alpha until 0.0.4
    * 3 - SpellHotbar2, alpha 0.0.5
    * 4 - SpellHotbar2, alpha 0.0.7
    * 5 - SpellHotbar2, alpha 0.0.13
    * 6 - SpellHotbar2, the in-menu dock's position, scale, spacing, anchor and lock
    */
    constexpr uint32_t save_format = 6U;

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

    /**
     * SKSE reverts before every load and new game; LoadCallback then restores what the cosave
     * carries. Clears the loaded-settings flag and the lifecycle state.
     */
    void RevertCallback(SKSE::SerializationInterface* a_intfc);

    /**
     * True once LoadCallback has read a 'HOTB' record for the current game; false after a
     * revert. Lifecycle uses it to tell a save that has seen the mod from one that has not.
     */
    bool loaded_existing_settings();

    bool slotSpell(RE::FormID form, size_t index, menu_slot_type slot_type);

}