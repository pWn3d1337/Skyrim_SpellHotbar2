#pragma once
#include <imgui.h>
namespace SpellHotbar::BindMenu {

	bool is_opened();
	void show();
	void hide();
	void drawFrame(ImFont* font_text, ImFont* font_text_big, ImFont* font_title);

	void update_filter(const std::string filter_text, uint8_t tab_index);
	void load_spells();

}