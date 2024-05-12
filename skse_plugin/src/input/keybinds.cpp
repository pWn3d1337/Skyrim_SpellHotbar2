#include "keybinds.h"
#include "../logger/logger.h"

namespace SpellHotbar::Input {

	std::array<KeyBind, 12> key_spells{
		KeyBind{RE::INPUT_DEVICE::kKeyboard, 2},
		KeyBind{RE::INPUT_DEVICE::kKeyboard, 3},
		KeyBind{RE::INPUT_DEVICE::kKeyboard, 4},
		KeyBind{RE::INPUT_DEVICE::kKeyboard, 5},
		KeyBind{RE::INPUT_DEVICE::kKeyboard, 6},
		KeyBind{RE::INPUT_DEVICE::kKeyboard, 7},
		KeyBind{RE::INPUT_DEVICE::kKeyboard, 8},
		KeyBind{RE::INPUT_DEVICE::kKeyboard, 9},
		KeyBind{RE::INPUT_DEVICE::kKeyboard, 10},
		KeyBind{RE::INPUT_DEVICE::kKeyboard, 11},
		KeyBind{RE::INPUT_DEVICE::kKeyboard, 12},
		KeyBind{RE::INPUT_DEVICE::kKeyboard, 13}
	};

	KeyBind key_prev {RE::INPUT_DEVICE::kKeyboard, 75};
	KeyBind key_next {RE::INPUT_DEVICE::kKeyboard,77};

	KeyModifier mod_ctrl(RE::INPUT_DEVICE::kKeyboard, 29, 157);
	KeyModifier mod_shift(RE::INPUT_DEVICE::kKeyboard, 42, 54);
	KeyModifier mod_alt(RE::INPUT_DEVICE::kKeyboard, 56, 184);

	KeyModifier mod_dual_cast(RE::INPUT_DEVICE::kNone, 0, 0);

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
		_check_unbind(mod_ctrl, code);
		_check_unbind(mod_shift, code);
		_check_unbind(mod_alt, code);
	}

	int rebind_key(int slot, int code)
	{
		unbind_if_already_used(code);
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
		else if (slot == keybind_id::mod_1) {
			mod_ctrl.rebind(code);
			return mod_ctrl.get_dx_scancode();
		}
		else if (slot == keybind_id::mod_2) {
			mod_shift.rebind(code);
			return mod_shift.get_dx_scancode();
		}
		else if (slot == keybind_id::mod_3) {
			mod_alt.rebind(code);
			return mod_alt.get_dx_scancode();
		}
		else if (slot == keybind_id::dual_casting_mod) {
			mod_dual_cast.rebind(code);
			return mod_dual_cast.get_dx_scancode();
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
		else if (slot == keybind_id::mod_1) {
			return mod_ctrl.get_dx_scancode();
		}
		else if (slot == keybind_id::mod_2) {
			return mod_shift.get_dx_scancode();
		}
		else if (slot == keybind_id::mod_3) {
			return mod_alt.get_dx_scancode();
		}
		else if (slot == keybind_id::dual_casting_mod) {
			return mod_dual_cast.get_dx_scancode();
		}
		return 0;
	}

}