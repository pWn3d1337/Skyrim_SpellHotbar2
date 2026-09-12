#pragma once

// The spell editor: every spell and shout in the load order, with the per-save overrides
// (icon, cast effect, cooldowns, cast time, animations) the player can set on each. An
// author-and-tuner tool. FLICK-side only.
#include "../../game_data/game_data.h"
#include "../../game_data/user_custom_spelldata.h"

namespace SpellHotbar::FlickUi::SpellEditor {
    bool is_opened();
    void show();
    void hide();

    const char* title();
    void draw();

    // The table half. The dialog half is spell_edit_dialog.cpp; it calls close_edit_dialog()
    // to return to the table and reads the animation list through get_list_of_anims().
    void draw_table();
    void close_edit_dialog();
    std::vector<int>& get_list_of_anims();
}
