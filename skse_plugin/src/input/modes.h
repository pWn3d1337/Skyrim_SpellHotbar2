#pragma once
#include "../bar/hotbar.h"
#include "input.h"

namespace SpellHotbar::Input
{
	

	class InputModeBase {
	public:

		static InputModeBase* current_mode;

		virtual void process_input(SlottedSkill& skill, RE::InputEvent*& addEvent, size_t& i, const KeyBind& bind, uint32_t& shoutkey) = 0;

	protected:
		InputModeBase() = default;
		virtual ~InputModeBase() = default;
	};

	class InputModeCast : public InputModeBase {
	public:

		virtual void process_input(SlottedSkill& skill, RE::InputEvent*& addEvent, size_t& i, const KeyBind& bind, uint32_t& shoutkey) override;

		static InputModeCast* getSingleton();

	protected:
		InputModeCast() = default;
		virtual ~InputModeCast() = default;
	};

	class InputModeEquip : public InputModeBase {
	public:

		virtual void process_input(SlottedSkill& skill, RE::InputEvent*& addEvent, size_t& i, const KeyBind& bind, uint32_t& shoutkey) override;

		static InputModeEquip* getSingleton();

	protected:
		InputModeEquip() = default;
		virtual ~InputModeEquip() = default;
	};

	class InputModeOblivion : public InputModeBase {
	public:

		virtual void process_input(SlottedSkill& skill, RE::InputEvent*& addEvent, size_t& i, const KeyBind& bind, uint32_t& shoutkey) override;

		static InputModeOblivion* getSingleton();

	protected:
		InputModeOblivion() = default;
		virtual ~InputModeOblivion() = default;
	};
}