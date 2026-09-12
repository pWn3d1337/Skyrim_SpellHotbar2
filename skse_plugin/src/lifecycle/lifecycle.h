#pragma once

// What the mod's Papyrus scripts used to do, done from the DLL instead:
//  - SpellHotbarInitQuestScript: grant the unbind / dual-cast powers, load auto_profile.json
//    and auto_edits.json once per game.
//  - SpellHotbarToggleDualCastingEffect: flip SpellHotbar_UseDualCasting when the toggle power
//    is cast, with the cloak sounds.
//  - SpellHotbarOpenBattleMagePerkTree / SpellHotbarBattleMageInitQuestScript: the perk tree
//    opens from the config tool; the opener power and its quest no longer exist.
// The quests and the opener power are gone from the ESPs (plugin-src/); the dual-cast toggle
// effect stays as the toggle power's carrier and carries no script.
namespace SpellHotbar::Lifecycle {
    void reset();
    void on_new_game();
    void on_post_load_game();
    // Called from the game loop once the player is in the world. Covers a console `coc` from
    // the main menu, which sends neither kNewGame nor kPostLoadGame, and retries a deferred
    // bootstrap.
    void on_player_in_world();

    bool player_has_power(int type);
    bool toggle_player_power(int type);
    bool open_battlemage_tree();

    /** Return true when the cast was the Spell Hotbar dual-cast toggle. */
    bool process_spell_cast(RE::FormID spell);
}
