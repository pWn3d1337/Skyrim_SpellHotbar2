#include "oblivion_bar.h"
#include "../rendering/render_manager.h"
#include "../casts/casting_controller.h"
#include "../input/keybinds.h"

namespace SpellHotbar::Bars {
	OblivionBar::OblivionBar() : m_spell_slot(), m_potion_slot(), m_power_slot()
	{
	}
	void OblivionBar::set_spell(SlottedSkill& spell)
	{
		m_spell_slot = spell;
	}
	void OblivionBar::set_potion(SlottedSkill& potion)
	{
		m_potion_slot = potion;
	}

	void OblivionBar::draw_in_hud(ImFont* font, float screensize_x, float screensize_y, int highlight_slot, float highlight_factor, key_modifier mod, bool hightlight_isred, float alpha, float shout_cd, float shout_cd_dur)
	{
		//TODO duplicate to bar draw_in_hud
		ImGui::PushFont(font);

		int icon_size = static_cast<int>(get_hud_slot_height(screensize_y));
		float text_offset_x = icon_size * 0.05f;
		float text_offset_y = icon_size * 0.0125f;

		float gcd_prog = casts::CastingController::get_current_gcd_progress();
		float gcd_dur = casts::CastingController::get_current_gcd_duration();

		float game_time{ 0 };
		float time_scale{ 20.0f };
		RE::Calendar* cal = RE::Calendar::GetSingleton();
		if (cal) {
			game_time = cal->GetCurrentGameTime();
			time_scale = cal->GetTimescale();
		}
		auto pc = RE::PlayerCharacter::GetSingleton();
		//end dupe

		if (pc) {
			//assign power_slot
			auto& dat = pc->GetActorRuntimeData();
			if (dat.selectedPower) {
				if (dat.selectedPower->GetFormID() != m_power_slot.formID) {
					m_power_slot.formID = dat.selectedPower->GetFormID();
					m_power_slot.hand = hand_mode::voice;

					if (dat.selectedPower->formType == RE::FormType::Shout) {
						m_power_slot.type = slot_type::shout;
					}
					else if (dat.selectedPower->formType == RE::FormType::Spell) {
						auto spell = dat.selectedPower->As<RE::SpellItem>();

						if (spell->GetSpellType() == RE::MagicSystem::SpellType::kLesserPower) {
							m_power_slot.type = slot_type::lesser_power;
						}
						else if (spell->GetSpellType() == RE::MagicSystem::SpellType::kPower) {
							m_power_slot.type = slot_type::power;
						}
						else {
							m_power_slot.type = slot_type::unknown;
						}
					}
				}
			}
			else {
				if (!m_power_slot.isEmpty()) m_power_slot.clear();
			}

			ImVec2 p = ImGui::GetCursorScreenPos();
			bool vertical = Bars::oblivion_bar_vertical;
			bool draw_potion = Input::key_oblivion_potion.isValidBound();
			bool show_power = Bars::oblivion_bar_show_power;

			Hotbar::draw_single_skill(m_spell_slot, alpha, icon_size, text_offset_x, text_offset_y, gcd_prog, gcd_dur, shout_cd, shout_cd_dur,
				game_time, time_scale, highlight_slot, highlight_factor, hightlight_isred, mod, oblivion_bar_name, pc, static_cast<int>(Input::keybind_id::oblivion_cast), p, vertical && (draw_potion || show_power));

			if (draw_potion) {
				p = ImGui::GetCursorScreenPos();
				Hotbar::draw_single_skill(m_potion_slot, alpha, icon_size, text_offset_x, text_offset_y, gcd_prog, gcd_dur, shout_cd, shout_cd_dur,
					game_time, time_scale, highlight_slot, highlight_factor, hightlight_isred, mod, oblivion_bar_name, pc, static_cast<int>(Input::keybind_id::oblivion_potion), p, vertical && show_power);
			}
			if (show_power) {
				p = ImGui::GetCursorScreenPos();
				Hotbar::draw_single_skill(m_power_slot, alpha, icon_size, text_offset_x, text_offset_y, gcd_prog, gcd_dur, shout_cd, shout_cd_dur,
					game_time, time_scale, highlight_slot, highlight_factor, hightlight_isred, mod, oblivion_bar_name, pc, static_cast<int>(Input::keybind_id::dummy_key_vanilla_shout), p);
			}

		}
	}

	void OblivionBar::clear()
	{
		m_spell_slot.clear();
		m_potion_slot.clear();
	}
	SlottedSkill OblivionBar::get_slotted_spell()
	{
		return m_spell_slot;
	}
	SlottedSkill OblivionBar::get_slotted_potion()
	{
		return m_potion_slot;
	}
}