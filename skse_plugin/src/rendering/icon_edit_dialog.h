#pragma once

#include "../game_data/game_data.h"
#include "../rendering/render_manager.h"

namespace SpellHotbar::PotionEditor {

	extern std::vector<SubTextureImage*> available_icons;

	void drawEditDialog(const RE::TESForm* form, GameData::User_custom_entry & data);
}