#pragma once

#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <imgui_internal.h>

namespace SpellHotbar::PotionEditor {

	bool is_opened();
	void show();
	void hide();

	void renderEditor();

	void drawTableFrame();
	void closeEditDialog();

	void load_entries();
	void update_filter(const std::string filter_text, bool filter_predefined, bool filter_custom_dat);
}