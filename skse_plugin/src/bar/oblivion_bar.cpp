#include "oblivion_bar.h"
#include "../rendering/render_manager.h"
#include "../casts/casting_controller.h"
#include "../input/keybinds.h"
#include "../storage/storage.h"

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

	void OblivionBar::serialize(SKSE::SerializationInterface* serializer, uint32_t key) const
	{
		uint8_t count{ 0 };
		if (!m_spell_slot.isEmpty()) {
			count++;
		}
		if (!m_potion_slot.isEmpty()) {
			count++;
		}

		if (count > 0Ui8) {
			if (!serializer->OpenRecord(key, Storage::save_format)) {
				logger::error("Could not save oblivion_bar_with_key {}!", key);
			}
			else {

				if (!serializer->WriteRecordData(&count, sizeof(uint8_t))) {
					logger::error("Failed to write bar size for oblivion_bar");
					return;
				}

				if (!m_spell_slot.isEmpty()) {
					if (!m_spell_slot.serialize_skill(0Ui8, serializer, "oblivion_bar")) return;
				}
				if (!m_potion_slot.isEmpty()) {
					m_potion_slot.serialize_skill(1Ui8, serializer, "oblivion_bar");
				}
			}
		}
	}

	void OblivionBar::deserialize(SKSE::SerializationInterface* serializer, uint32_t type, uint32_t /*version*/, uint32_t /*length*/)
	{
		uint8_t slots{ 0Ui8 };
		if (!serializer->ReadRecordData(&slots, sizeof(uint8_t))) {
			logger::error("Failed to read slots count for oblivion_bar!");
		}

		for (uint8_t i = 0U; i < slots; i++) {
			uint8_t read_slot{ 0Ui8 };
			RE::FormID read_id{ 0U };
			uint8_t read_hand{ 0Ui8 };

			if (!serializer->ReadRecordData(&read_slot, sizeof(uint8_t))) {
				logger::error("Failed to load oblivion_bar!");
				break;
			}
			else {
				read_slot = std::clamp(read_slot, 0Ui8, static_cast<uint8_t>(max_bar_size));
			}

			if (!serializer->ReadRecordData(&read_id, sizeof(RE::FormID))) {
				logger::error("Failed to load oblivion_bar!");
				break;
			}
			else {
				RE::FormID resolved_id{ 0 };
				serializer->ResolveFormID(read_id, resolved_id);
				RE::TESForm* form = RE::TESForm::LookupByID(resolved_id);
				if (form != nullptr && Hotbar::is_valid_formtype_for_hotbar(form)) {

					if (read_slot == 0Ui8) {
						m_spell_slot = resolved_id;
					}
					else if (read_slot == 1Ui8) {
						m_potion_slot = resolved_id;
					}
				}
				else {
					logger::info("Removing {:8x} from bar, form no longer exists or not valid for hotbar.", resolved_id);
					if (read_slot == 0Ui8) {
						m_spell_slot = 0;
					}
					else if (read_slot == 1Ui8) {
						m_potion_slot = 0;
					}
				}
			}

			if (!serializer->ReadRecordData(&read_hand, sizeof(uint8_t))) {
				logger::error("Failed to load oblivion_bar!");
				break;
			}
			else {
				auto hm = hand_mode(std::clamp(read_hand, 0Ui8, static_cast<uint8_t>(hand_mode::end - 1)));
				if (read_slot == 0Ui8) {
					m_spell_slot.hand = hm;
				}
				else if (read_slot == 1Ui8) {
					m_potion_slot.hand = hm;
				}
			}
		}
	}

	void OblivionBar::draw_in_hud(ImFont* font, float screensize_x, float screensize_y, int highlight_slot, float highlight_factor, key_modifier mod, bool hightlight_isred, float alpha, float shout_cd, float shout_cd_dur)
	{
		//TODO duplicate to bar draw_in_hud
		ImGui::PushFont(font);

		int icon_size = static_cast<int>(get_hud_slot_height(screensize_y, Bars::oblivion_slot_scale));
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
		ImGui::PopFont();
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