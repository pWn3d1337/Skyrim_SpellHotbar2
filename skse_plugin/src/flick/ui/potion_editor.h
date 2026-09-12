#pragma once

// The potion editor: the player's alchemy items, with a per-save icon override on each.
// FLICK-side only.
#include "../../game_data/game_data.h"
#include "../../game_data/user_custom_entry.h"

namespace SpellHotbar::FlickUi::PotionEditor {
    bool is_opened();
    void show();
    void hide();

    const char* title();
    void draw();

    void draw_table();
    void close_edit_dialog();
}
