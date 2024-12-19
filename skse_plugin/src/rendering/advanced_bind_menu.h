#pragma once
#include <imgui.h>
namespace SpellHotbar::BindMenu {

	bool is_opened();
	void show();
	void hide();
	void drawFrame(ImFont* font_text);

	void update_filter(const std::string filter_text);
	void load_spells();

}