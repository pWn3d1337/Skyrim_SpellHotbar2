#pragma once
#include "input.h"

namespace SpellHotbar::Input {

	enum keybind_id : int {
		spell_1 = 0,
		spell_2 = 1,
		spell_3 = 2,
		spell_4 = 3,
		spell_5 = 4,
		spell_6 = 5,
		spell_7 = 6,
		spell_8 = 7,
		spell_9 = 8,
		spell_10 = 9,
		spell_11 = 10,
		spell_12 = 11,
		ui_next = 12,
		ui_prev = 13,
		mod_1 = 14,
		mod_2 = 15,
		mod_3 = 16,
		dual_casting_mod = 17,
		num_keys
	};


	extern std::array<KeyBind,12> key_spells;
	extern KeyBind key_prev;
	extern KeyBind key_next;

	extern KeyModifier mod_ctrl;
	extern KeyModifier mod_shift;
	extern KeyModifier mod_alt;

	extern KeyModifier mod_dual_cast;

	int rebind_key(int slot, int code);
	int get_keybind(int slot);
}