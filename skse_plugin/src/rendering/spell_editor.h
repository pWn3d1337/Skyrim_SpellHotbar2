#pragma once

#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <imgui_internal.h>

namespace SpellHotbar::SpellEditor {
	
	bool is_opened();
	void show();
	void hide();

	void renderEditor();

	void drawTableFrame();
	void closeEditDialog();
	std::vector<int> & get_list_of_anims();

}