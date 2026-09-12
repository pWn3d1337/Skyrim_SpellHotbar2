#pragma once

// Pure decision helpers for the first-run bootstrap, kept free of RE:: so they
// can be reasoned about without the game.
namespace SpellHotbar::Lifecycle {
    // The retired SpellHotbarInitQuest was RunOnce + StartGameEnabled: it ran on a new game
    // and on the first load of a save that had never seen the mod. A save that carries our
    // cosave record has already been through it.
    constexpr bool should_run_first_initialization(bool is_new_game, bool loaded_existing_settings)
    {
        return is_new_game || !loaded_existing_settings;
    }

    // kNewGame / kPostLoadGame normally have a player. If they do not, retry
    // on the main loop this many times rather than skipping the bootstrap.
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
