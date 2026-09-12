#pragma once

#include "../../game_data/game_data.h"
#include "../../game_data/user_custom_entry.h"

namespace SpellHotbar::FlickUi::PotionEditor {
    // The icon override dialog, drawn in place of the potion table while an item is being edited.
    void draw_edit_dialog(const RE::TESForm* a_form, GameData::User_custom_entry& a_data);
}
