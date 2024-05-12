#pragma once
#include <filesystem>
#include "../game_data/game_data.h"

namespace SpellHotbar::TextureCSVLoader
{
	extern std::unordered_map<std::string, GameData::DefaultIconType> const default_icon_names;

	void load_icons(std::filesystem::path folder);

}