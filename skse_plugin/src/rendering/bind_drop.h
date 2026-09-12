#pragma once

#include <cstdint>

namespace SpellHotbar::BindMenu {

// A bind-menu drop is a TESForm FormID or empty.
struct SlotBind {
	uint32_t form_id{ 0 };
};

struct BindPayload {
	uint32_t form_id{ 0 };
};

inline BindPayload empty_bind()
{
	return {};
}

inline BindPayload form_bind(uint32_t form_id)
{
	return BindPayload{ .form_id = form_id };
}

inline SlotBind apply_bind_drop(SlotBind, BindPayload incoming)
{
	if (incoming.form_id != 0) {
		return SlotBind{ .form_id = incoming.form_id };
	}
	return {};
}

}  // namespace SpellHotbar::BindMenu
