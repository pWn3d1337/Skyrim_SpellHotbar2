#include "keybinds.h"
#include "../logger/logger.h"

namespace SpellHotbar::Input {

	std::array<KeyBind, 12> key_spells{
		KeyBind{RE::INPUT_DEVICE::kNone, 0},  //2
		KeyBind{RE::INPUT_DEVICE::kNone, 0},  //3
		KeyBind{RE::INPUT_DEVICE::kNone, 0},  //4
		KeyBind{RE::INPUT_DEVICE::kNone, 0},  //5
		KeyBind{RE::INPUT_DEVICE::kNone, 0},  //6
		KeyBind{RE::INPUT_DEVICE::kNone, 0},  //7
		KeyBind{RE::INPUT_DEVICE::kNone, 0},  //8
		KeyBind{RE::INPUT_DEVICE::kNone, 0},  //9
		KeyBind{RE::INPUT_DEVICE::kNone, 0}, //10
		KeyBind{RE::INPUT_DEVICE::kNone, 0}, //11
		KeyBind{RE::INPUT_DEVICE::kNone, 0}, //12
		KeyBind{RE::INPUT_DEVICE::kNone, 0}  //13
	};

	KeyBind key_prev {RE::INPUT_DEVICE::kNone, 0}; //75
	KeyBind key_next {RE::INPUT_DEVICE::kNone, 0};  //77

	KeyBind oblivion_cast{ RE::INPUT_DEVICE::kNone, 0 };
	KeyBind oblivion_potion{ RE::INPUT_DEVICE::kNone, 0 };

	KeyModifier mod_1(RE::INPUT_DEVICE::kNone, 0, 0); //ctrl 29, 157
	KeyModifier mod_2(RE::INPUT_DEVICE::kNone, 0, 0); //shift 42, 54
	KeyModifier mod_3(RE::INPUT_DEVICE::kNone, 0, 0);  //alt 56, 184

	KeyModifier mod_dual_cast(RE::INPUT_DEVICE::kNone, 0, 0);
	KeyModifier mod_show_bar(RE::INPUT_DEVICE::kNone, 0, 0);

	KeyModifier mod_alt(RE::INPUT_DEVICE::kKeyboard, 56, 184);  //fixed modifier, used for gui
	//KeyModifier mod_shift(RE::INPUT_DEVICE::kKeyboard, 42, 54);  //fixed modifier, used for gui

	inline void _check_unbind(KeyBind & bind, int code) {
		if (bind.get_dx_scancode() == code) {
			bind.unbind();
		}
	}

	inline void _check_unbind(KeyModifier& mod, int code) {
		if (mod.get_dx_scancode() == code || mod.get_dx_scancode2() == code) {
			mod.rebind(-1);
		}
	}

	void unbind_if_already_used(int code) {
		//check remove binds if already used
		for (size_t i = 0Ui64; i < key_spells.size(); ++i) {
			_check_unbind(key_spells[i], code);
		}
		_check_unbind(key_next, code);
		_check_unbind(key_prev, code);
		_check_unbind(mod_1, code);
		_check_unbind(mod_2, code);
		_check_unbind(mod_3, code);
		_check_unbind(mod_show_bar, code);
	}

	int rebind_key(int slot, int code, bool check_conflicts)
	{
		if (check_conflicts) unbind_if_already_used(code);
		if (slot >= 0 && slot <= keybind_id::spell_12) {
			key_spells[slot].assign_from_dx_scancode(code);
			return key_spells[slot].get_dx_scancode();
		}
		else if (slot == keybind_id::ui_next) {
			key_next.assign_from_dx_scancode(code);
			return key_next.get_dx_scancode();
		}
		else if (slot == keybind_id::ui_prev) {
			key_prev.assign_from_dx_scancode(code);
			return key_prev.get_dx_scancode();
		}
		else if (slot == keybind_id::modifier_1) {
			mod_1.rebind(code);
			return mod_1.get_dx_scancode();
		}
		else if (slot == keybind_id::modifier_2) {
			mod_2.rebind(code);
			return mod_2.get_dx_scancode();
		}
		else if (slot == keybind_id::modifier_3) {
			mod_3.rebind(code);
			return mod_3.get_dx_scancode();
		}
		else if (slot == keybind_id::dual_casting_mod) {
			mod_dual_cast.rebind(code);
			return mod_dual_cast.get_dx_scancode();
		}
		else if (slot == keybind_id::show_bar_mod) {
			mod_show_bar.rebind(code);
			return mod_show_bar.get_dx_scancode();
		}
		else if (slot == keybind_id::oblivion_cast) {
			oblivion_cast.assign_from_dx_scancode(code);
			return oblivion_cast.get_dx_scancode();
		}
		else if (slot == keybind_id::oblivion_potion) {
			oblivion_potion.assign_from_dx_scancode(code);
			return oblivion_potion.get_dx_scancode();
		}

		return 0;
	}

	int get_keybind(int slot)
	{
		if (slot >= 0 && slot <= keybind_id::spell_12) {
			return key_spells[slot].get_dx_scancode();
		}
		else if (slot == keybind_id::ui_next) {
			return key_next.get_dx_scancode();
		}
		else if (slot == keybind_id::ui_prev) {
			return key_prev.get_dx_scancode();
		}
		else if (slot == keybind_id::modifier_1) {
			return mod_1.get_dx_scancode();
		}
		else if (slot == keybind_id::modifier_2) {
			return mod_2.get_dx_scancode();
		}
		else if (slot == keybind_id::modifier_3) {
			return mod_3.get_dx_scancode();
		}
		else if (slot == keybind_id::dual_casting_mod) {
			return mod_dual_cast.get_dx_scancode();
		}
		else if (slot == keybind_id::show_bar_mod) {
			return mod_show_bar.get_dx_scancode();
		}
		else if (slot == keybind_id::oblivion_cast) {
			return oblivion_cast.get_dx_scancode();
		}
		else if (slot == keybind_id::oblivion_potion) {
			return oblivion_potion.get_dx_scancode();
		}
		return 0;
	}

}