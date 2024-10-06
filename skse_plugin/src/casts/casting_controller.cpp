#include "casting_controller.h"
#include "../logger/logger.h"
#include "../game_data/game_data.h"
#include "../input/keybinds.h"
#include "../rendering/render_manager.h"
#include "spell_proc.h"

namespace SpellHotbar::casts::CastingController {

	std::unique_ptr<BaseCastingInstance> current_cast = nullptr;

	void reset_cast() {
		current_cast->on_reset();
		current_cast.reset();
	}

	//Play sound on actor and return soundhandle
	RE::BSSoundHandle playSound(RE::Actor* a, RE::BGSSoundDescriptorForm* a_descriptor)
	{
		RE::BSSoundHandle handle;
		handle.soundID = static_cast<uint32_t>(-1);
		handle.assumeSuccess = false;
		handle.state = RE::BSSoundHandle::AssumedState::kInitialized;

		auto am = RE::BSAudioManager::GetSingleton();
		if (am) {
			am->BuildSoundDataFromDescriptor(handle, a_descriptor, 16); // 16 used by https://github.com/D7ry/EldenParry
			if (handle.SetPosition(a->data.location)) {
				handle.SetObjectToFollow(a->Get3D());
				handle.Play();
			}
		}
		return handle;
	}

	BaseCastingInstance::BaseCastingInstance(RE::TESForm* form, float casttime) : m_form(form),
		m_cast_timer(casttime),
		m_total_casttime(casttime),
		m_gcd(1.5f),
		m_casted(false)
	{
	}

	CastingInstance::CastingInstance(RE::SpellItem* spell, float casttime, float manacost, hand_mode used_hand, uint16_t casteffect, bool is_spell_proc) : BaseCastingInstance(spell, casttime),
		m_charge_sound(nullptr),
		m_release_sound(nullptr),
		m_cast_loop_sound(nullptr),
		m_charge_sound_instance(),
		m_loop_sound_instance(),
		m_played_release(false),
		m_release_anim_time(0.0f),
		m_played_pre_release(false),
		m_pre_release_anim_time(0.0f),
		m_manacost(manacost),
		m_used_hand(used_hand),
		m_equip_ability(nullptr),
		m_casteffect(casteffect),
		m_spell_proc(is_spell_proc)
	{
		if (m_form && (m_form->GetFormType() == RE::FormType::Spell || m_form->GetFormType() == RE::FormType::Scroll)) {
			uint32_t size = get_spell()->effects.size();
			for (uint32_t i = 0U; i < size; ++i) {
				const auto& eff = get_spell()->effects[i];

				if (eff->baseEffect) {
					for (const auto& effectSound : eff->baseEffect->effectSounds) {

						if (effectSound.id == RE::MagicSystem::SoundID::kCharge && !m_charge_sound) {
							m_charge_sound = effectSound.sound;
						}

						if (effectSound.id == RE::MagicSystem::SoundID::kRelease && !m_release_sound) {
							m_release_sound = effectSound.sound;
						}

						if (effectSound.id == RE::MagicSystem::SoundID::kCastLoop && !m_cast_loop_sound) {
							m_cast_loop_sound = effectSound.sound;
						}

					}
				}
			}

			if (size > 0 && get_spell()->effects[0]->baseEffect) {
				m_equip_ability = spell->effects[0]->baseEffect->data.equipAbility;
			}
		}

		m_charge_sound_instance.soundID = static_cast<uint32_t>(-1);
		m_charge_sound_instance.assumeSuccess = false;
		m_charge_sound_instance.state = RE::BSSoundHandle::AssumedState::kInitialized;

		m_loop_sound_instance.soundID = static_cast<uint32_t>(-1);
		m_loop_sound_instance.assumeSuccess = false;
		m_loop_sound_instance.state = RE::BSSoundHandle::AssumedState::kInitialized;
	}

	bool CastingInstance::is_first_time_update() const
	{
		return m_cast_timer >= m_total_casttime;
	}

	bool BaseCastingInstance::is_gcd_expired() const
	{
		return m_cast_timer <= -m_gcd;
	}

	bool CastingInstance::should_play_release_anim()
	{
		return !m_played_release && (m_cast_timer <= m_release_anim_time);
	}

	bool BaseCastingInstance::advance_time(float delta)
	{
		if (m_cast_timer >= -m_gcd) {
			m_cast_timer -= delta;
		}

		//logger::info("Cast Progres: {}%", 100.0f * (1.0f - (m_cast_timer / m_total_casttime)));
		return m_cast_timer <= 0.0f;
	}

	float BaseCastingInstance::get_current_casttime() const
	{
		return m_total_casttime - std::clamp(m_cast_timer, 0.0f, m_total_casttime);
	}

	float BaseCastingInstance::get_current_gcd_progress() const
	{
		float f = (m_cast_timer + m_gcd) / (m_total_casttime + m_gcd);

		return std::clamp(1.0f - f, 0.0f, 1.0f);
	}

	float BaseCastingInstance::get_current_gcd_duration() const
	{
		return m_total_casttime + m_gcd;
	}

	const std::string_view CastingInstance::get_end_anim() const
	{
		return "MT_BreathExhaleShort"sv;
	}

	const std::string_view BaseCastingInstance::get_start_anim() const
	{
		return "ShoutStart"sv;
	}

	const std::string_view CastingInstance::get_cancel_anim() const
	{
		return "ShoutStop"sv;
	}

	void CastingInstance::apply_cast_start_spell(RE::PlayerCharacter* pc)
	{
		if (m_equip_ability) {
			//if (!pc->HasSpell(m_equip_ability)) { //hasSpell seems to not work on equip abilities from current spells
			if (!player_has_equip_ability(m_equip_ability)) {
				pc->AddSpell(m_equip_ability);
			}
			else {
				//do not remove it
				m_equip_ability = nullptr;
			}
		}

		if (GameData::spellhotbar_castfx_spell) {
			GameData::update_spell_casting_art_and_time(m_casteffect, static_cast<uint32_t>(std::ceil(m_total_casttime)), m_used_hand);
			//cast_spell_on_player(GameData::spellhotbar_castfx_spell, 1.0f);
			pc->AddSpell(GameData::spellhotbar_castfx_spell);
		}
	}

	void CastingInstance::on_reset()
	{
		BaseCastingInstance::on_reset();
		auto pc = RE::PlayerCharacter::GetSingleton();
		if (pc) {
			/*auto magic_target = pc->GetMagicTarget();

			if (magic_target) {
				auto effect_list = magic_target->GetActiveEffectList();

				if (effect_list) {
					for (auto it = effect_list->begin(); it != effect_list->end(); ++it) {

						auto effect = *it;
						if (!effect->flags.all(RE::ActiveEffect::Flag::kInactive) && !(effect->flags.all(RE::ActiveEffect::Flag::kDispelled))) {
							if (effect->spell && effect->spell->formID == GameData::spellhotbar_castfx_spell->GetFormID()) {
								effect->Dispel(false);
							}
						}
					}

				}
			}*/
			pc->RemoveSpell(GameData::spellhotbar_castfx_spell);
		}
	}

	bool CastingInstance::is_anim_ok(RE::PlayerCharacter* pc) const
	{
		bool is_shouting{ false };
		pc->GetGraphVariableBool("IsShouting"sv, is_shouting);
		return is_shouting;
	}

	RE::BSSoundHandle _play_sound_if_exists(RE::BGSSoundDescriptorForm* snd)
	{
		RE::BSSoundHandle handle;
		handle.soundID = static_cast<uint32_t>(-1);
		handle.assumeSuccess = false;
		handle.state = RE::BSSoundHandle::AssumedState::kInitialized;

		if (snd) {
			const auto id = snd->GetFormEditorID();
			if (id) {
				auto* pc = RE::PlayerCharacter::GetSingleton();
				if (pc) {
					handle = playSound(pc, snd);
				}
			}
		}
		return handle;
	}

	bool BaseCastingInstance::blocks_movement() const
	{
		return false;
	}

	void BaseCastingInstance::on_reset()
	{
		//called before cast instance is deleted
	}

	void BaseCastingInstance::consume_items()
	{
		if (m_form && m_form->GetFormType() == RE::FormType::Scroll) {
			//consume the scroll
			auto pc = RE::PlayerCharacter::GetSingleton();
			if (pc) {
				RE::FormID id = m_form->GetFormID();
				auto refs = pc->GetInventory([id](const RE::TESBoundObject& object) { return object.formID == id; });
				for (auto& [k, v] : refs) {
					pc->RemoveItem(k, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				}
			}
		}
	}

	void BaseCastingInstance::apply_cooldown()
	{
		auto data = GameData::get_spell_data(m_form, true, true);
		if (data.cooldown > 0.0f) {
			SpellHotbar::GameData::add_gametime_cooldown_with_timescale(m_form->GetFormID(), data.cooldown, true);
		}
	}

	bool CastingInstance::update(RE::PlayerCharacter* pc, float delta)
	{
		if (is_anim_ok(pc)) {
			if (is_first_time_update()) {
				play_charge_sound();
				apply_cast_start_spell(pc);
				GameData::start_cast_timer();
			}

			//Regular spell
			bool should_cast = advance_time(delta);
			GameData::advance_cast_timer(delta);

			if (should_play_release_anim()) {
				pc->NotifyAnimationGraph(get_end_anim());
				set_release_played();
			}

			if (should_cast) {
				stop_charge_sound();
				play_release_sound();
				if (cast_spell(get_spell(), m_used_hand == hand_mode::dual_hand)) {
					if (m_spell_proc) {
						casts::SpellProc::consume_proc();
					}
					apply_cooldown();
					pc->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kMagicka, -m_manacost);
					consume_items();
				}
				set_casted();
			}
		}
		else {
			GameData::reset_animation_vars();
			return true;
		}
		return false;
	}

	void CastingInstance::play_charge_sound()
	{
		m_charge_sound_instance = _play_sound_if_exists(m_charge_sound);
	}

	void CastingInstance::stop_charge_sound()
	{
		if (m_charge_sound_instance.IsValid() && m_charge_sound_instance.IsPlaying()) {
			m_charge_sound_instance.FadeOutAndRelease(25); //fadeout in ms
		}
	}

	void CastingInstance::play_release_sound() const
	{
		_play_sound_if_exists(m_release_sound);
	}

	void CastingInstance::stop_cast_loop_sound()
	{
		if (m_loop_sound_instance.IsValid()) { //&& m_loop_sound_instance.IsPlaying()) {
			m_loop_sound_instance.FadeOutAndRelease(25); //fadeout in ms
		}
	}

	void CastingInstance::play_cast_loop_sound()
	{
		m_loop_sound_instance = _play_sound_if_exists(m_cast_loop_sound);
	}

	CastingInstanceSpell::CastingInstanceSpell(RE::SpellItem* spell, float casttime, float manacost, hand_mode used_hand, uint16_t casteffect, bool spell_proc) : CastingInstance(spell, casttime, manacost, used_hand, casteffect, spell_proc)
	{
	}

	CastingInstanceRitual::CastingInstanceRitual(RE::SpellItem* spell, float casttime, float manacost, hand_mode used_hand, uint16_t casteffect, bool spell_proc) : CastingInstance(spell, casttime, manacost, used_hand, casteffect, spell_proc)
	{
		m_release_anim_time = 0.25f;
		m_gcd = 2.0f;
	}

	bool CastingInstanceRitual::blocks_movement() const
	{
		return m_cast_timer >= -0.5f; //!m_casted &&
	}

	CastingInstanceSpellConcentration::CastingInstanceSpellConcentration(RE::SpellItem* spell, float casttime, float manacost, hand_mode used_hand, uint16_t casteffect, bool spell_proc, const Input::KeyBind& keybind, int slot, bool blocksMovement)
		: CastingInstance(spell, casttime, manacost, used_hand, casteffect, spell_proc),
		m_keybind(keybind),
		m_slot(slot),
		m_blocks_movement(blocksMovement)
	{
		m_cast_timer = 0;
		m_total_casttime = spell->data.castDuration;
		m_release_anim_time = casttime;
		m_gcd = 0.5f;
	}

	void CastingInstanceSpellConcentration::apply_cast_start_spell(RE::PlayerCharacter* pc)
	{
		if(m_equip_ability) {
			//if (!pc->HasSpell(m_equip_ability)) { //hasSpell seems to not work on equip abilities from current spells
			if (!player_has_equip_ability(m_equip_ability)) {
				pc->AddSpell(m_equip_ability);
			}
			else {
				//do not remove it
				m_equip_ability = nullptr;
			}
		}
	}

	void CastingInstanceSpellConcentration::on_reset()
	{
		CastingInstance::on_reset();
		RE::PlayerCharacter* pc = RE::PlayerCharacter::GetSingleton();
		if (pc && m_equip_ability) {
			pc->RemoveSpell(m_equip_ability);
		}
	}

	bool CastingInstanceSpellConcentration::advance_time(float delta)
	{
		m_cast_timer += delta;
		if (casted()) {
			m_gcd -= delta;
		}
		//logger::info("CastTimer: {}, gcd: {}", m_cast_timer, m_gcd);
		return false;
	}

	bool CastingInstanceSpellConcentration::is_first_time_update() const
	{
		return m_cast_timer <= 0;
	}

	bool CastingInstanceSpellConcentration::update(RE::PlayerCharacter* pc, float delta)
	{
		//duration spells cast for duration without holding key
		bool keydown = has_duration() || m_keybind.isDown();
		bool cancel{ false };

		float timer_old = m_cast_timer;
		advance_time(delta);

		if (timer_old == 0) {
			//startup
			//stop if not anim or key not down directly at start
			if (!keydown || !is_anim_ok(pc)) {
				cancel = true;
			}
			else {
				//start the chargeup
				play_charge_sound();
				apply_cast_start_spell(pc);
				GameData::start_cast_timer();
			}
		}
		else if (m_cast_timer < m_release_anim_time) {
			//during charge loop
			if (!keydown || !is_anim_ok(pc)) {
				//trigger gcd and stop cast
				set_casted();
				stop_charge_sound();
				GameData::reset_animation_vars();
				pc->NotifyAnimationGraph(get_cancel_anim());
			}

			//check for pre_release_anim_time
			if (m_pre_release_anim_time > 0.0f && !m_played_pre_release && (m_cast_timer + m_pre_release_anim_time >= m_release_anim_time))
			{
				m_played_pre_release = true;
				pc->NotifyAnimationGraph(get_end_anim());
			}

		}
		else if (m_cast_timer >= m_release_anim_time) {
			//charge finished

			constexpr float loop_timer = 0.5f;

			if (timer_old < m_release_anim_time) {
				//first cast update
				cast_spell(get_spell(), m_used_hand == hand_mode::dual_hand, m_manacost);
				if (m_spell_proc) {
					casts::SpellProc::consume_proc();
				}
				stop_charge_sound();
				play_cast_loop_sound();
				GameData::global_casting_conc_spell->value = 1.0f;
				if (!m_played_pre_release) {
					pc->NotifyAnimationGraph(get_end_anim());
					pc->NotifyAnimationGraph(get_start_anim());
				}
			}
			else if (static_cast<int>(timer_old / loop_timer) < static_cast<int>(m_cast_timer / loop_timer)) {
				//Check for anim reloop

				pc->NotifyAnimationGraph(get_start_anim());
				if (!is_anim_ok(pc)) {
					//do same exit code as if key was no longer held down, anim got interrupted
					keydown = false;
				}
				else {
					RenderManager::highlight_skill_slot(m_slot, loop_timer*2.0f, false);
				}
			}

			if ((m_manacost*0.5f) > pc->AsActorValueOwner()->GetActorValue(RE::ActorValue::kMagicka)) {
				//cancel if out of magicka
				keydown = false;
				RE::HUDMenu::FlashMeter(RE::ActorValue::kMagicka);
				RE::PlaySound(Input::sound_MagFail);
			}

			if (has_duration() && m_cast_timer > m_release_anim_time + m_total_casttime) {
				//abort a channel spell with duration
				keydown = false;
			}

			if (!keydown) {
				//trigger gcd and stop cast
				set_casted();
				stop_cast_loop_sound();
				stop_charge_sound();
				RE::MagicSystem::CastingSource src = static_cast<RE::MagicSystem::CastingSource>(std::clamp(static_cast<int>(SpellHotbar::GameData::global_casting_source->value), 0, 3));
				auto playerCaster = pc->GetMagicCaster(src);
				if (playerCaster) {
					playerCaster->InterruptCast(false);
				}
				GameData::reset_animation_vars();
				pc->NotifyAnimationGraph(get_cancel_anim());
				play_release_sound();
				apply_cooldown();
				consume_items();
			}

		}

		return cancel;
	}

	bool CastingInstanceSpellConcentration::is_gcd_expired() const
	{
		return m_gcd <= 0;
	}

	bool CastingInstanceSpellConcentration::blocks_movement() const
	{
		return m_blocks_movement && !m_casted;
	}

	bool CastingInstanceSpellConcentration::has_duration() const
	{
		return m_total_casttime > 0.0f;
	}

	float CastingInstanceSpellConcentration::get_current_gcd_progress() const
	{
		if (has_duration()) {
			return m_cast_timer / (m_release_anim_time + m_total_casttime);
		}
		return 0.0f;
	}

	void update_cast(float delta)
	{
		if (current_cast) {
			if (!current_cast->casted()) {
				auto pc = RE::PlayerCharacter::GetSingleton();
				if (pc) {

					if (current_cast->update(pc, delta)) {
						reset_cast();
					}

				}
			}
			else {
				//update timer until gcd expires
				current_cast->advance_time(delta);
				if (current_cast->is_gcd_expired()) {
					GameData::reset_animation_vars();
					reset_cast();
				}
			}
		}
	}

	bool start_cast(CastingInstanceSpellData& cast_info)
	{
		if (!current_cast) {
			auto pc = RE::PlayerCharacter::GetSingleton();
			if (pc) {
				
				int anim = cast_info.m_animation;
				if (anim < 0) {
					anim = GameData::chose_default_anim_for_spell(cast_info.m_spell, -1, false);
				}
				GameData::set_animtype_global(anim);

				hand_mode used_hand = GameData::set_weapon_dependent_casting_source(cast_info.m_hand, cast_info.m_dual_cast);
				current_cast = std::make_unique<CastingInstanceSpell>(cast_info.m_spell, cast_info.m_casttime, cast_info.m_manacost, used_hand, cast_info.m_casteffect, cast_info.m_spellproc);
				pc->NotifyAnimationGraph(current_cast->get_start_anim());
				return true;
			}
		}
		return false;
	}

	bool start_conc_cast(CastingInstanceSpellData& cast_info, const Input::KeyBind& keybind, size_t slot)
	{
		if (!current_cast) {
			auto pc = RE::PlayerCharacter::GetSingleton();
			if (pc) {

				int anim = cast_info.m_dual_cast ? cast_info.m_animation2 : cast_info.m_animation;
				if (anim < 0) {
					anim = GameData::chose_default_anim_for_spell(cast_info.m_spell, -1, cast_info.m_dual_cast);
				}
				GameData::set_animtype_global(anim);

				hand_mode used_hand = GameData::set_weapon_dependent_casting_source(cast_info.m_hand, cast_info.m_dual_cast);
				current_cast = std::make_unique<CastingInstanceSpellConcentration>(cast_info.m_spell, cast_info.m_casttime, cast_info.m_manacost, used_hand, cast_info.m_casteffect, cast_info.m_spellproc, keybind, static_cast<int>(slot), cast_info.m_dual_cast);

				pc->NotifyAnimationGraph(current_cast->get_start_anim());
				return true;
			}
		}
		return false;
	}

	bool start_ritual_conc_cast(CastingInstanceSpellData & cast_info, const Input::KeyBind& keybind, size_t slot)
	{
		if (!current_cast) {
			auto pc = RE::PlayerCharacter::GetSingleton();
			if (pc) {
				
				bool is_fast_anim = cast_info.m_casttime <= fast_cast_threshold;

				int anim = is_fast_anim ? cast_info.m_animation2 : cast_info.m_animation;
				float pre_release_anim = GameData::Spell_cast_data::get_ritual_conc_anim_prerelease_time(anim);

				GameData::set_animtype_global(anim);

				hand_mode used_hand = GameData::set_weapon_dependent_casting_source(cast_info.m_hand, cast_info.m_dual_cast);
				current_cast = std::make_unique<CastingInstanceSpellRitualConcentration>(cast_info.m_spell, cast_info.m_casttime, cast_info.m_manacost, used_hand, cast_info.m_casteffect, cast_info.m_spellproc, keybind, static_cast<int>(slot), pre_release_anim);

				pc->NotifyAnimationGraph(current_cast->get_start_anim());
				return true;
			}
		}
		return false;
	}

	bool start_ritual_cast(CastingInstanceSpellData& cast_info)
	{
		if (!current_cast) {
			auto pc = RE::PlayerCharacter::GetSingleton();
			if (pc) {
				
				bool is_fast_cast = cast_info.m_casttime <= fast_cast_threshold;

				int anim = is_fast_cast ? cast_info.m_animation2 : cast_info.m_animation;
				if (anim < 0) {
					anim = GameData::chose_default_anim_for_spell(cast_info.m_spell, -1, is_fast_cast);
				}
				GameData::set_animtype_global(anim);

				hand_mode used_hand = GameData::set_weapon_dependent_casting_source(cast_info.m_hand, cast_info.m_dual_cast);
				current_cast = std::make_unique<CastingInstanceRitual>(cast_info.m_spell, cast_info.m_casttime, cast_info.m_manacost, used_hand, cast_info.m_casteffect, cast_info.m_spellproc);
				pc->NotifyAnimationGraph(current_cast->get_start_anim());
				return true;
			}
		}
		return false;
	}

	bool try_start_cast(RE::TESForm* form, const Input::KeyBind& keybind, size_t slot, hand_mode hand)
	{
		if (can_start_new_cast()) {
			auto pc = RE::PlayerCharacter::GetSingleton();
			if (form && pc) {
				bool is_shouting{ false };
				pc->GetGraphVariableBool("IsShouting"sv, is_shouting);

				if (!is_shouting && (form->GetFormType() == RE::FormType::Spell || form->GetFormType() == RE::FormType::Scroll)) {
					RE::SpellItem* spell = form->As<RE::SpellItem>();

					//check if spell is still known/enough scrolls in inv
					bool spell_allowed{true};
					if (form->GetFormType() == RE::FormType::Spell && !pc->HasSpell(spell)) {
						spell_allowed = false;
						RE::DebugNotification("Spell is no longer known!");
					}
					else if (form->GetFormType() == RE::FormType::Scroll && GameData::count_item_in_inv(form->GetFormID()) <= 0) {
						spell_allowed = false;
						RE::DebugNotification("No more scrolls left!");
					}
					
					if (spell_allowed) {

						float manacost{ 0.0f };
						bool is_spell_proc{ false };

						if (form->GetFormType() == RE::FormType::Spell) {
							manacost = spell->CalculateMagickaCost(pc);

							auto spell_proc = SpellProc::has_matching_proc(spell);
							if (spell_proc.has_value()) {
								manacost *= spell_proc.value();
								is_spell_proc = true;
							}
						}
						bool dual_cast{ false };
						if (!spell->GetNoDualCastModifications() && ((hand == auto_hand && GameData::should_dual_cast() || hand == dual_hand)) && GameData::player_can_dualcast_spell(spell)) {

							RE::GameSettingCollection* game_settings = RE::GameSettingCollection::GetSingleton();
							if (game_settings) {
								auto setting = game_settings->GetSetting("fMagicDualCastingCostMult");
								if (setting) {
									manacost *= setting->GetFloat();
									dual_cast = true;
								}
							}
						}
						else {
							if (!spell->IsTwoHanded() && hand == dual_hand && !dual_cast) {
								hand = auto_hand;
							}
						}

						if (manacost <= pc->AsActorValueOwner()->GetActorValue(RE::ActorValue::kMagicka)) {
							//set casttime

							auto spell_data = GameData::get_spell_data(spell);

							float casttime = spell_data.casttime;

							if (is_spell_proc) {
								casttime = SpellProc::adjust_casttime(casttime, spell);
							}
							CastingInstanceSpellData cast_info{ spell, casttime, manacost, hand, dual_cast, spell_data.animation, spell_data.animation2, spell_data.casteffectid, is_spell_proc};

							if (spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration) {
								if (spell->IsTwoHanded()) {
									return start_ritual_conc_cast(cast_info, keybind, slot);
								}
								else
								{
									return start_conc_cast(cast_info, keybind, slot);
								}
							}
							else {
								if (spell->IsTwoHanded() || dual_cast) {
									return start_ritual_cast(cast_info);
								}
								else
								{
									return start_cast(cast_info);
								}
							}
						}
						else {
							RE::HUDMenu::FlashMeter(RE::ActorValue::kMagicka);
							RE::PlaySound(Input::sound_MagFail);
						}
					}
					else {
						RE::PlaySound(Input::sound_MagFail);
					}
				}
				else if (!is_shouting && (form->GetFormType() == RE::FormType::AlchemyItem)) {
					return start_potion_use(form);
				}
			}
		}

		return false;
	}

	bool start_potion_use(RE::TESForm* alch_item)
	{
		if (!current_cast) {
			auto pc = RE::PlayerCharacter::GetSingleton();
			if (pc) {
				if (GameData::count_item_in_inv(alch_item->GetFormID()) > 0) {
					current_cast = std::make_unique<CastingInstancePotionUse>(alch_item);
					return true;
				}
				else
				{
					RE::DebugNotification("No more potions left!");
					return false;
				}
			}
		}
		return false;
	}

	bool try_cast_power(RE::TESForm* form, const Input::KeyBind& keybind, size_t slot, hand_mode hand)
	{
		if (can_start_new_cast()) {
			auto pc = RE::PlayerCharacter::GetSingleton();
			if (form && pc) {
				bool is_shouting{ false };
				pc->GetGraphVariableBool("IsShouting"sv, is_shouting);

				if (!is_shouting && form->GetFormType() == RE::FormType::Shout) {

					if (pc->GetVoiceRecoveryTime() <= 0.0f) {

						current_cast = std::make_unique<CastingInstanceShout>(form);
						return true;
					}
					else {
						RE::HUDMenu::FlashMeter(RE::ActorValue::kShoutRecoveryMult);
						RE::PlaySound(Input::sound_UIMenuCancel);
					}
				}
				else if (!is_shouting && form->GetFormType() == RE::FormType::Spell) {
					RE::SpellItem* spell = form->As<RE::SpellItem>();

					if (spell->GetEquipSlot() == GameData::equip_slot_voice) {
						//logger::info("Start Power Cast");
						current_cast = std::make_unique<CastingInstancePower>(form);
					
						return true;
					}
				
				}

			}
		}
		return false;
	}

	bool is_currently_using_power()
	{
		if (current_cast) {
			CastingInstancePower* curr_shout = dynamic_cast<CastingInstancePower*>(current_cast.get());
			if (curr_shout) {
				return true;
			}
		}
		return false;
	}

	bool is_currently_using_procced_spell()
	{
		if (current_cast) {
			CastingInstance* curr_cast = dynamic_cast<CastingInstance*>(current_cast.get());
			if (curr_cast) {
				return curr_cast->is_procced();
			}
		}
		return false;
	}

	float get_current_casttime()
	{
		if (current_cast) {
			return current_cast->get_current_casttime();
		}
		return 0.0f;
	}

	bool can_start_new_cast()
	{
		return current_cast == nullptr;
	}

	void try_finish_power_cast(RE::FormID formID)
	{
		if (current_cast && current_cast->get_form() && current_cast->get_form()->formID == formID)
		{
			current_cast->set_casted();
			current_cast->apply_cooldown();
		}
	}

	void try_finish_shout_cast(RE::FormID formID)
	{
		if (current_cast && current_cast->get_form() && current_cast->get_form()->formID == formID)
		{
			current_cast->set_casted();
			//update gcd
			current_cast->updateGCD(0.5f);
		}
	}

	float get_current_gcd_progress()
	{
		if (current_cast) {
			return current_cast->get_current_gcd_progress();
		}
		return 0.0f;
	}
	float get_current_gcd_duration()
	{
		if (current_cast) {
			return current_cast->get_current_gcd_duration();
		}
		return 0.0f;
	}

	bool is_movement_blocking_cast()
	{
		if (current_cast) {
			return current_cast->blocks_movement();
		}
		return false;
	}

	void cast_spell_on_player(RE::SpellItem* spell, float magnitude, bool no_art) {
		if (!spell) return;

		auto pc = RE::PlayerCharacter::GetSingleton();
		if (!pc) return;

		auto playerMagicCaster = pc->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
		if (!playerMagicCaster) return;

		playerMagicCaster->CastSpellImmediate(spell, no_art, pc, 1.0f, false, magnitude, nullptr);
	}

	inline bool check_slot_for_equip_ability(RE::PlayerCharacter* pc, RE::SpellItem* equip_ability, RE::BGSEquipSlot* slot) {
		auto form = pc->GetEquippedObjectInSlot(slot);
		if (form && form->GetFormType() == RE::FormType::Spell) {
			auto spell = form->As<RE::SpellItem>();
			if (spell && !spell->effects.empty() && spell->effects[0]->baseEffect) {
				auto eq_ab = spell->effects[0]->baseEffect->data.equipAbility;
				if (eq_ab == equip_ability) {
					return true;
				}
			}
		}
		return false;
	}

	bool player_has_equip_ability(RE::SpellItem* equip_ability)
	{
		auto pc = RE::PlayerCharacter::GetSingleton();
		if (pc && GameData::equip_slot_left_hand && GameData::equip_slot_right_hand) {

			return check_slot_for_equip_ability(pc, equip_ability, GameData::equip_slot_left_hand) ||
				check_slot_for_equip_ability(pc, equip_ability, GameData::equip_slot_right_hand) ||
				check_slot_for_equip_ability(pc, equip_ability, GameData::equip_slot_both_hand);
		
		}

		return false;
	}

	bool cast_spell(RE::SpellItem* spell, bool dual_cast, std::optional<float> concentration_manacost)
	{
		//Credits to https://github.com/ArcEarth/DynamicAnimationCasting/

		if (!spell) return false;

		auto pc = RE::PlayerCharacter::GetSingleton();
		if (!pc) return false;

		RE::MagicSystem::CastingSource castsource = SpellHotbar::GameData::global_casting_source ? static_cast<RE::MagicSystem::CastingSource>(std::clamp(static_cast<int>(SpellHotbar::GameData::global_casting_source->value), 0, 3)) : RE::MagicSystem::CastingSource::kOther;

		auto playerMagicCaster = pc->GetMagicCaster(castsource);
		if (!playerMagicCaster) return false;

		bool targetSelf = spell->GetDelivery() == RE::MagicSystem::Delivery::kSelf;
		RE::Actor* target = targetSelf ? pc : pc->GetActorRuntimeData().currentCombatTarget.get().get();

		if (!spell->GetNoDualCastModifications() && dual_cast) {
			playerMagicCaster->SetDualCasting(true);
		}

		if (concentration_manacost.has_value()) {
			playerMagicCaster->currentSpellCost = concentration_manacost.value();
		}
		//logger::info("Cost: {}", playerMagicCaster->currentSpellCost);
		playerMagicCaster->CastSpellImmediate(spell, false, target, 1.0f, false, 0.0f, targetSelf ? nullptr : pc);

		return true;
	}

	CastingInstanceSpellRitualConcentration::CastingInstanceSpellRitualConcentration(RE::SpellItem* spell, float casttime, float manacost, hand_mode used_hand, uint16_t casteffect, bool spell_proc, const Input::KeyBind& keybind, int slot, float pre_release_anim_time)
		: CastingInstanceSpellConcentration(spell, casttime, manacost, used_hand, casteffect, spell_proc, keybind, slot)
	{
		m_pre_release_anim_time = pre_release_anim_time;
	}

	bool CastingInstanceSpellRitualConcentration::blocks_movement() const
	{
		return !m_casted;
	}

	CastingInstancePower::CastingInstancePower(RE::TESForm* form) : BaseCastingInstance(form, 0.0f), m_old_form(nullptr), m_reequiped(false)
	{
		m_gcd = 0.5f;
		auto pc = RE::PlayerCharacter::GetSingleton();
		if (pc) {
			auto& dat = pc->GetActorRuntimeData();
			m_old_form = dat.selectedPower;
			dat.selectedPower = form;
		}
	}

	void CastingInstancePower::on_reset()
	{
		BaseCastingInstance::on_reset();
		reequip_old_power();
	}

	void CastingInstancePower::reequip_old_power()
	{
		if (!m_reequiped) {
			auto pc = RE::PlayerCharacter::GetSingleton();
			if (pc) {
				auto& dat = pc->GetActorRuntimeData();
				if (dat.selectedPower == m_form) {
					dat.selectedPower = m_old_form;
				}
				m_reequiped = true;
				consume_items();
			}
		}
	}

	bool CastingInstancePower::update(RE::PlayerCharacter* pc, float delta)
	{
		advance_time(delta);
		if (casted() && !m_reequiped) {
			reequip_old_power();
		}
		return is_gcd_expired();
	}

	CastingInstanceShout::CastingInstanceShout(RE::TESForm* form) : CastingInstancePower(form)
	{
		m_gcd = 1.5f;
	}

	CastingInstanceSpellData::CastingInstanceSpellData(RE::SpellItem* spell, float casttime, float manacost, hand_mode hand, bool dual_cast, int animation, int animation2, uint16_t casteffect, bool is_spell_proc) :
		m_spell(spell), m_casttime(casttime), m_manacost(manacost), m_hand(hand), m_dual_cast(dual_cast), m_animation(animation), m_animation2(animation2), m_casteffect(casteffect), m_spellproc(is_spell_proc)
	{
	}

	CastingInstancePotionUse::CastingInstancePotionUse(RE::TESForm* form): BaseCastingInstance(form, 0.0f)
	{
		m_gcd = 1.0f;
		if (form->GetFormType() == RE::FormType::AlchemyItem) {
			auto alch = form->As<RE::AlchemyItem>();
			if (!alch->IsFood() && !alch->IsPoison()) {
				//potion
				m_gcd = GameData::potion_gcd;
			}
		}
	}

	bool CastingInstancePotionUse::update(RE::PlayerCharacter* pc, float delta)
	{
		if (!casted()) {
			RE::ActorEquipManager::GetSingleton()->EquipObject(pc, m_form->As<RE::AlchemyItem>());
			set_casted();
		}
		advance_time(delta);
		return is_gcd_expired();
	}

}