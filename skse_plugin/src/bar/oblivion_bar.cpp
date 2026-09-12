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

	void OblivionBar::build_hud_layer(Flick::HudLayer& layer, [[maybe_unused]] float screensize_x, float screensize_y, int highlight_slot,
	                                  float highlight_factor, key_modifier mod, bool highlight_isred, float alpha,
	                                  float shout_cd, float shout_cd_dur)
	{
		Hotbar::HudSlotContext ctx;
		ctx.alpha = alpha;
		ctx.icon_size = static_cast<int>(get_hud_slot_height(screensize_y, Bars::oblivion_slot_scale));
		ctx.text_offset_x = ctx.icon_size * 0.05f;
		ctx.text_offset_y = ctx.icon_size * 0.0125f;
		ctx.gcd_prog = casts::CastingController::get_current_gcd_progress();
		ctx.gcd_dur = casts::CastingController::get_current_gcd_duration();
		ctx.shout_cd = shout_cd;
		ctx.shout_cd_dur = shout_cd_dur;
		if (RE::Calendar* cal = RE::Calendar::GetSingleton()) {
			ctx.game_time = cal->GetCurrentGameTime();
			ctx.time_scale = cal->GetTimescale();
		}
		ctx.highlight_slot = highlight_slot;
		ctx.highlight_factor = highlight_factor;
		ctx.highlight_isred = highlight_isred;
		ctx.mod = mod;
		ctx.bar_name = oblivion_bar_name;
		ctx.pc = RE::PlayerCharacter::GetSingleton();

		const float icon = static_cast<float>(ctx.icon_size);
		layer.text_px = icon * 0.24f;


		if (ctx.pc) {
			//assign power_slot
			auto& dat = ctx.pc->GetActorRuntimeData();
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

			// The slots in a row, or a column when the bar is vertical.
			const bool vertical = Bars::oblivion_bar_vertical;
			const bool draw_potion = Input::key_oblivion_potion.isValidBound();
			const bool show_power = Bars::oblivion_bar_show_power;
			const float pitch = icon + Bars::oblivion_slot_spacing;
			int n = 0;
			const auto place = [&](SlottedSkill& slot, int slot_index) {
				const float x = vertical ? 0.0f : n * pitch;
				const float y = vertical ? n * pitch : 0.0f;
				Hotbar::push_single_skill(layer, slot, slot_index, x, y, ctx);
				++n;
			};
			place(m_spell_slot, static_cast<int>(Input::keybind_id::oblivion_cast));
			if (draw_potion) {
				place(m_potion_slot, static_cast<int>(Input::keybind_id::oblivion_potion));
			}
			if (show_power) {
				place(m_power_slot, static_cast<int>(Input::keybind_id::dummy_key_vanilla_shout));
			}
			const float extent = n * pitch - Bars::oblivion_slot_spacing;
			layer.width = vertical ? icon : extent;
			layer.height = vertical ? extent : icon;
			if (Bars::use_keybind_icons()) {
				layer.height += icon * 0.3f;
			}
			layer.visible = true;
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