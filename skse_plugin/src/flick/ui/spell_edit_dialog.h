#pragma once

#include "../../game_data/game_data.h"
#include "../../game_data/user_custom_spelldata.h"

namespace SpellHotbar::FlickUi::SpellEditor {
    // The per-spell override dialog, drawn in place of the table while a spell is being edited.
    void draw_edit_dialog(const RE::TESForm* a_form, GameData::User_custom_spelldata& a_data,
                          GameData::Spell_cast_data& a_filled, GameData::Spell_cast_data& a_unfilled,
                          GameData::Spell_cast_data& a_saved);
}
