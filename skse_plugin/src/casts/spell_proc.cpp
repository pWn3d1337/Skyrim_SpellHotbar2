#include "spell_proc.h"
#include "../game_data/game_data.h"

namespace SpellHotbar::casts {
	inline static const char* sound_spellProc = "UISpellLearned";
	std::unique_ptr<SpellProc> SpellProc::current_proc = nullptr;

	SpellProc::SpellProc(float time) : m_timer(0), m_total_time(time)
	{
	}

	void SpellProc::update_timer(float delta)
	{
		if (current_proc) {
			if (current_proc->update(delta)) {
				current_proc.reset();
			}
		}
	}

	bool SpellProc::trigger_spellproc()
	{
		auto pc = RE::PlayerCharacter::GetSingleton();
		constexpr float proc_duration = 10.0f; //TODO check perks

		if (current_proc) {
			current_proc->reproc(proc_duration);

		} else if (GameData::spellhotbar_spellproc_cd && pc->AsMagicTarget()->HasMagicEffect(GameData::spellhotbar_spellproc_cd)) {
			// cooldown
			return false;
		}
		else {
			//new proc
			RE::PlaySound(sound_spellProc);
			current_proc = std::make_unique<SpellProc>(proc_duration);
		}
		return true;
	}

	std::optional<float> SpellProc::has_matching_proc(RE::SpellItem* casted_spell)
	{
		if (current_proc != nullptr) {
			if (casted_spell->data.castingType == RE::MagicSystem::CastingType::kFireAndForget || casted_spell->data.castingType == RE::MagicSystem::CastingType::kConcentration) {
				//TODO check valid spell ranks and adjust magicka factor
				return std::make_optional(0.25f);
			}
		}
		return std::nullopt;
	}

	float SpellProc::adjust_casttime(float original_ct, RE::SpellItem* casted_spell)
	{
		//TODO check perks, spell...
		if (original_ct > 0.25f) {
			original_ct = 0.25f;
		}
		return original_ct;
	}

	bool SpellProc::has_spell_proc(RE::SpellItem* spell) {
		//TODO check perks
		if (spell && spell->data.spellType == RE::MagicSystem::SpellType::kSpell) {
			return current_proc != nullptr;
		}

		return false;
	}

	bool SpellProc::update(float delta)
	{
		m_timer += delta;
		return m_timer > m_total_time;
	}

	void SpellProc::reproc(float time)
	{
		m_total_time = time;
		m_timer = 0.0f;
	}
	float SpellProc::get_spell_proc_prog()
	{
		if (current_proc) {
			return current_proc->m_timer / current_proc->m_total_time;
		}
		return 0.0f;
	}
	float SpellProc::get_spell_proc_timer()
	{
		if (current_proc) {
			return current_proc->m_timer;
		}
		return 0.0f;
	}
	float SpellProc::get_spell_proc_total()
	{
		if (current_proc) {
			return current_proc->m_total_time;
		}
		return 0.0f;
	}
	void SpellProc::consume_proc()
	{
		if (current_proc) {
			current_proc.reset();
		}
		auto pc = RE::PlayerCharacter::GetSingleton();
		if (pc && GameData::spellhotbar_apply_spellproc_cd) {
			auto playerMagicCaster = pc->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
			playerMagicCaster->CastSpellImmediate(GameData::spellhotbar_apply_spellproc_cd, true, pc, 1.0f, false, 0.0f, nullptr);
		}
	}
}