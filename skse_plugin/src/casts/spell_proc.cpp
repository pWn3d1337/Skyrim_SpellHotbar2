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

	float get_proc_cd(RE::PlayerCharacter* pc) {
		float cd = GameData::global_spellhotbar_perks_proc_cooldown ? GameData::global_spellhotbar_perks_proc_cooldown->value : 10.0f;
		if (GameData::spellhotbar_perk_master && pc->HasPerk(GameData::spellhotbar_perk_master)) {
			cd *= 0.8f;
		}
		else if (GameData::spellhotbar_perk_expert && pc->HasPerk(GameData::spellhotbar_perk_expert)) {
			cd *= 0.6f;
		}

		return cd;
	}

	bool SpellProc::trigger_spellproc()
	{
		auto pc = RE::PlayerCharacter::GetSingleton();
		constexpr float proc_duration = 10.0f;

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
				if (has_spell_proc(casted_spell)) {

					auto pc = RE::PlayerCharacter::GetSingleton();
					auto effect = casted_spell->GetCostliestEffectItem();

					bool disable_perk_req = GameData::global_spellhotbar_perks_override && GameData::global_spellhotbar_perks_override->value > 0.0f;
					if (pc && effect && effect->baseEffect) {
						int skill_level = effect->baseEffect->data.minimumSkill;

						if (skill_level > 75) {
							return 0.5f;
						}
						else if (skill_level >= 50) {
							return disable_perk_req || (GameData::spellhotbar_perk_master && pc->HasPerk(GameData::spellhotbar_perk_master)) ? 0.25f : 0.5f;
						}
						else {
							return disable_perk_req || (GameData::spellhotbar_perk_expert && pc->HasPerk(GameData::spellhotbar_perk_expert)) ? 0.25f : 0.5f;
						}
					}
					else {
						return 0.5f;
					}
				}
			}
		}
		return std::nullopt;
	}

	float SpellProc::adjust_casttime(float original_ct, RE::SpellItem* casted_spell)
	{
		if (original_ct > 0.25f) {
			original_ct = 0.25f;
		}
		return original_ct;
	}

	bool SpellProc::has_spell_proc(RE::SpellItem* spell) {

		auto pc = RE::PlayerCharacter::GetSingleton();
		if (pc && spell && spell->data.spellType == RE::MagicSystem::SpellType::kSpell && current_proc != nullptr &&
			(spell->data.castingType == RE::MagicSystem::CastingType::kFireAndForget || spell->data.castingType == RE::MagicSystem::CastingType::kConcentration)) {

			if (GameData::global_spellhotbar_perks_require_halfcostperk && GameData::global_spellhotbar_perks_require_halfcostperk->value > 0.0f && spell->data.castingPerk) {
				if (!pc->HasPerk(spell->data.castingPerk)) {
					return false;
				}
			}

			auto effect = spell->GetCostliestEffectItem();
			if (effect && effect->baseEffect) {
				int skill_level = effect->baseEffect->data.minimumSkill;
				bool disable_perk_req = GameData::global_spellhotbar_perks_override && GameData::global_spellhotbar_perks_override->value > 0.0f;

				if (skill_level > 75) {
					return disable_perk_req || (GameData::spellhotbar_perk_master && pc->HasPerk(GameData::spellhotbar_perk_master));
				}
				else if (skill_level >= 50) {
					return disable_perk_req || (GameData::spellhotbar_perk_expert && pc->HasPerk(GameData::spellhotbar_perk_expert));
				}
			}

			return true;
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

			if (!GameData::spellhotbar_apply_spellproc_cd->effects.empty() && GameData::spellhotbar_apply_spellproc_cd->effects[0]) {
				GameData::spellhotbar_apply_spellproc_cd->effects[0]->effectItem.duration = static_cast<uint32_t>(std::round(get_proc_cd(pc)));
			}

			playerMagicCaster->CastSpellImmediate(GameData::spellhotbar_apply_spellproc_cd, true, pc, 1.0f, false, 0.0f, nullptr);
		}
	}
}