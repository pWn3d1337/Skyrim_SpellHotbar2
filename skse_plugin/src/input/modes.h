#pragma once
#include "../bar/hotbar.h"
#include "input.h"

namespace SpellHotbar::Input
{
	bool is_oblivion_mode();
	bool is_equip_mode();

	int get_current_mode_index();
	void set_input_mode(int index);

	class InputModeBase {
	public:

		static InputModeBase* current_mode;

		virtual void process_input(SlottedSkill& skill, RE::InputEvent*& addEvent, size_t& i, const KeyBind& bind, RE::INPUT_DEVICE& shoutKeyDev, uint8_t& shoutKey) = 0;
		virtual void process_key_update(const KeyBind& bind, size_t i, float held_down_sec);
	protected:
		InputModeBase() = default;
		virtual ~InputModeBase() = default;
	};

	class InputModeCast : public InputModeBase {
	public:

		virtual void process_input(SlottedSkill& skill, RE::InputEvent*& addEvent, size_t& i, const KeyBind& bind, RE::INPUT_DEVICE& shoutKeyDev, uint8_t& shoutKey) override;

		static InputModeCast* getSingleton();

	protected:
		InputModeCast() = default;
		virtual ~InputModeCast() = default;
	};

	class InputModeEquip : public InputModeBase {
	public:

		virtual void process_input(SlottedSkill& skill, RE::InputEvent*& addEvent, size_t& i, const KeyBind& bind, RE::INPUT_DEVICE& shoutKeyDev, uint8_t& shoutKey) override;

		static InputModeEquip* getSingleton();

	protected:
		InputModeEquip() = default;
		virtual ~InputModeEquip() = default;
	};

	class InputModeOblivion : public InputModeBase {
	public:

		virtual void process_input(SlottedSkill& skill, RE::InputEvent*& addEvent, size_t& i, const KeyBind& bind, RE::INPUT_DEVICE& shoutKeyDev, uint8_t& shoutKey) override;
		virtual void process_key_update(const KeyBind& bind, size_t i, float held_down_sec) override;

		static InputModeOblivion* getSingleton();

	protected:
		InputModeOblivion() = default;
		virtual ~InputModeOblivion() = default;
	};

	class InputModeVampireLord : public InputModeBase {
	public:

		virtual void process_input(SlottedSkill& skill, RE::InputEvent*& addEvent, size_t& i, const KeyBind& bind, RE::INPUT_DEVICE& shoutKeyDev, uint8_t& shoutKey) override;

		static InputModeVampireLord* getSingleton();

	protected:
		InputModeVampireLord() = default;
		virtual ~InputModeVampireLord() = default;
	};
}