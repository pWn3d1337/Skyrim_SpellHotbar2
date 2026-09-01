#pragma once

namespace SpellHotbar::SpellEditor {
	
	bool is_opened();
	void show();
	void hide();

	void renderEditor();

	void drawTableFrame();
	void closeEditDialog();
	std::vector<int> & get_list_of_anims();

}
