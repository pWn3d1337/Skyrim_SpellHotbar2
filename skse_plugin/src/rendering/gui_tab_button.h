#pragma once
#include "../game_data/game_data.h"

namespace SpellHotbar::Rendering {
namespace GuiTabButton {
	
	void draw(const char* id, int index, GameData::DefaultIconType icon_type, int icon_size, int& out_index, bool & changed, const char* tooltip_text);
	
}
}