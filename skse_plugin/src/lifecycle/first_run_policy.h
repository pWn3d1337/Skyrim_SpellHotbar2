#pragma once

namespace SpellHotbar::Lifecycle {
    constexpr bool should_run_first_initialization(bool is_new_game, bool loaded_existing_settings)
    {
        return is_new_game || !loaded_existing_settings;
    }

    // kNewGame / kPostLoadGame normally have a player. If they do not, retry
    // on the main loop this many times rather than skipping native bootstrap.
    constexpr int k_max_first_init_retries = 180;

    constexpr bool battlemage_tree_may_dispatch(bool battlemage_plugin_loaded, bool csf_open_menu_present)
    {
        return battlemage_plugin_loaded && csf_open_menu_present;
    }

    enum class FirstInitAttempt : unsigned char {
        skipped,
        complete,
        retry,
        give_up
    };

    constexpr FirstInitAttempt classify_first_init_attempt(
        bool should_init,
        bool already_initialized,
        bool player_available,
        int retries_remaining)
    {
        if (!should_init || already_initialized) {
            return FirstInitAttempt::skipped;
        }
        if (player_available) {
            return FirstInitAttempt::complete;
        }
        return retries_remaining > 0 ? FirstInitAttempt::retry : FirstInitAttempt::give_up;
    }
}
