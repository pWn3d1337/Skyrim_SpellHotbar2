#pragma once

#include "../game_data/game_data.h"
#include "../rendering/render_manager.h"

namespace SpellHotbar::SpellEditor {

	extern std::vector<SubTextureImage*> available_icons;

	void drawEditDialog(const RE::TESForm* form, GameData::Spell_cast_data & dat, GameData::Spell_cast_data& dat_filled, GameData::Spell_cast_data& dat_saved);
}