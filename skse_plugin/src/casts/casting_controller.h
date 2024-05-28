#pragma once
#include <optional>
#include "../input/keybinds.h"
#include "../bar/hotbar.h"

namespace SpellHotbar::casts::CastingController {

	/**
	* Casttime at which ritual spells play the fast anim
	*/
	constexpr float fast_cast_threshold{ 1.51f };

	class BaseCastingInstance {
	public:
		BaseCastingInstance(RE::TESForm* form, float casttime);
		virtual ~BaseCastingInstance() = default;

		float get_current_casttime() const;
		virtual float get_current_gcd_progress() const;
		float get_current_gcd_duration() const;

		virtual bool advance_time(float delta);
		virtual bool is_gcd_expired() const;

		virtual const std::string_view get_start_anim() const;

		virtual bool update(RE::PlayerCharacter* pc, float delta) = 0;

		virtual bool blocks_movement() const;

		virtual void on_reset();

		void consume_items();

		inline void set_casted() {
			m_casted = true;
		}
		inline bool casted() const {
			return m_casted;
		}
		inline RE::TESForm* get_form() {
			return m_form;
		}
		inline void updateGCD(float new_val) {
			m_gcd = -m_cast_timer + new_val;
		}

		void apply_cooldown();

	protected:
		RE::TESForm* m_form;
		float m_cast_timer;
		float m_total_casttime;
		float m_gcd;
		bool m_casted;
	};

	//Abstract base for Spell casts
	class CastingInstance : public BaseCastingInstance {
	public:
		CastingInstance(RE::SpellItem* spell, float casttime, float manacost, hand_mode used_hand, uint16_t casteffect);
		virtual ~CastingInstance() = default;

		virtual bool is_anim_ok(RE::PlayerCharacter* pc) const;
		virtual const std::string_view get_end_anim() const;
		virtual const std::string_view get_cancel_anim() const;

		virtual void apply_cast_start_spell(RE::PlayerCharacter* pc);

		virtual void on_reset() override;
		/*
		* Game loop update logic, return if cast should be cleared afterwards. 
		*/
		virtual bool update(RE::PlayerCharacter* pc, float delta) override;

		void play_charge_sound();
		void stop_charge_sound();
		void play_release_sound() const ;
		void play_cast_loop_sound();
		void stop_cast_loop_sound();

		virtual bool is_first_time_update() const;

		bool should_play_release_anim();

		inline RE::SpellItem* get_spell() {
			return m_form->As<RE::SpellItem>();
		}

		inline const RE::SpellItem* get_spell() const {
			return m_form->As<RE::SpellItem>();
		}

		inline void set_release_played() {
			m_played_release = true;
		}

	protected:
		RE::BGSSoundDescriptorForm* m_charge_sound;
		RE::BGSSoundDescriptorForm* m_release_sound;
		RE::BGSSoundDescriptorForm* m_cast_loop_sound;
		RE::BSSoundHandle m_charge_sound_instance;
		RE::BSSoundHandle m_loop_sound_instance;
		bool m_played_release;
		float m_release_anim_time;
		bool m_played_pre_release;
		float m_pre_release_anim_time;
		float m_manacost;
		hand_mode m_used_hand;
		RE::SpellItem* m_equip_ability;
		uint16_t m_casteffect;
	};

	//Regular spell cast (1h, movement allowed)
	class CastingInstanceSpell : public CastingInstance {
	public:
		CastingInstanceSpell(RE::SpellItem* spell, float casttime, float manacost, hand_mode used_hand, uint16_t casteffect);
		virtual ~CastingInstanceSpell() = default;
	};

	//Ritual cast with blocked movement
	class CastingInstanceRitual : public CastingInstance {
	public:
		CastingInstanceRitual(RE::SpellItem* spell, float casttime, float manacost, hand_mode used_hand, uint16_t casteffect);
		virtual ~CastingInstanceRitual() = default;

		virtual bool blocks_movement() const override;
	};

	//Single handed concentration spell with movement
	class CastingInstanceSpellConcentration : public CastingInstance {
	public:
		CastingInstanceSpellConcentration(RE::SpellItem* spell, float casttime, float manacost, hand_mode used_hand, uint16_t casteffect, const Input::KeyBind& keybind, int slot, bool blocksMovement = false);
		virtual ~CastingInstanceSpellConcentration() = default;

		virtual void apply_cast_start_spell(RE::PlayerCharacter* pc) override;
		virtual void on_reset() override;

		virtual bool advance_time(float delta) override;
		virtual bool is_first_time_update() const override;
		virtual bool update(RE::PlayerCharacter* pc, float delta) override;
		virtual bool is_gcd_expired() const override;
		virtual bool blocks_movement() const override;

		virtual bool has_duration() const;
		virtual float get_current_gcd_progress() const override;
	protected:
		const Input::KeyBind& m_keybind;
		int m_slot;
		bool m_blocks_movement;
	};

	//Ritual style concentration spell, 2hands and blocked movement
	class CastingInstanceSpellRitualConcentration : public CastingInstanceSpellConcentration {
	public:
		CastingInstanceSpellRitualConcentration(RE::SpellItem* spell, float casttime, float manacost, hand_mode used_hand, uint16_t casteffect, const Input::KeyBind& keybind, int slot, float pre_release_anim_time);
		virtual ~CastingInstanceSpellRitualConcentration() = default;

		virtual bool blocks_movement() const override;
	};

	class CastingInstancePower : public BaseCastingInstance {
	public:
		CastingInstancePower(RE::TESForm* form);
		virtual ~CastingInstancePower() = default;

		virtual void on_reset() override;

		void reequip_old_power();

		virtual bool update(RE::PlayerCharacter* pc, float delta) override;
	protected:
		RE::TESForm* m_old_form;
		bool m_reequiped;
	};

	class CastingInstanceShout : public CastingInstancePower {
	public:
		CastingInstanceShout(RE::TESForm* form);
		virtual ~CastingInstanceShout() = default;
	protected:
	};

	class CastingInstancePotionUse : public BaseCastingInstance {
	public:
		CastingInstancePotionUse(RE::TESForm* form);
		virtual ~CastingInstancePotionUse() = default;

		virtual bool update(RE::PlayerCharacter* pc, float delta) override;
	protected:
	};

	/**
	* Wraps the shared information needed for all spell cast types
	*/
	struct CastingInstanceSpellData
	{
		CastingInstanceSpellData(RE::SpellItem* spell, float casttime, float manacost, hand_mode hand, bool dual_cast, int animamtion, int animation2, uint16_t casteffect);

		RE::SpellItem* m_spell;
		float m_casttime;
		float m_manacost; 
		hand_mode m_hand;
		bool m_dual_cast;
		int m_animation;
		int m_animation2;
		uint16_t m_casteffect;
	};


	void update_cast(float delta);

	extern std::unique_ptr<BaseCastingInstance> current_cast;

	bool try_start_cast(RE::TESForm* form, const Input::KeyBind& keybind, size_t slot, hand_mode hand);
	
	bool try_cast_power(RE::TESForm* form, const Input::KeyBind& keybind, size_t slot, hand_mode hand);

	bool start_potion_use(RE::TESForm* alch_item);

	bool is_currently_using_power();

	float get_current_casttime();

	bool can_start_new_cast();

	/*
	* check if a power is currently tracked as casting instance, and finish it.
	*/
	void try_finish_power_cast(RE::FormID formID);
	void try_finish_shout_cast(RE::FormID formID);

	float get_current_gcd_progress();
	float get_current_gcd_duration();

	bool is_movement_blocking_cast();
	/**
	* Actually casts the spell, do not call directly
	*/
	bool cast_spell(RE::SpellItem* spell, bool dual_cast);

	void cast_spell_on_player(RE::SpellItem* spell, float magnitude = 0.0f, bool no_art = false);

	bool player_has_equip_ability(RE::SpellItem* equip_ability);
}