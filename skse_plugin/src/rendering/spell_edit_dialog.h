#pragma once

#include "../game_data/game_data.h"
#include "../rendering/render_manager.h"
#include "../game_data/user_custom_spelldata.h"

namespace SpellHotbar::SpellEditor {

	extern std::vector<SubTextureImage*> available_icons;

	void drawEditDialog(const RE::TESForm* form, GameData::User_custom_spelldata& dat, GameData::Spell_cast_data& dat_filled, GameData::Spell_cast_data& dat_unfilled, GameData::Spell_cast_data& dat_saved);
}