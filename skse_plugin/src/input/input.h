#pragma once

namespace SpellHotbar::Input {
	static inline const char* sound_UISkillsBackward = "UISkillsBackward";
	static inline const char* sound_UISkillsForward = "UISkillsForward";
	static inline const char* sound_MagFail = "MAGFail";
	static inline const char* sound_UIFavorite = "UIFavorite";
	static inline const char* sound_UISkillsFocus = "UISkillsFocus";
	static inline const char* sound_UIMenuCancel = "UIMenuCancel";

	void install_hook();

	void processAndFilter(RE::InputEvent** a_event);

	bool in_binding_menu();

	bool in_ingame_state();

	std::tuple<RE::INPUT_DEVICE, uint8_t> dx_scan_code_to_input(int dx_scancode);
	int input_to_dx_scancode(RE::INPUT_DEVICE device, uint8_t code);

	std::tuple<RE::INPUT_DEVICE, uint8_t> get_shout_key_and_device();
	int get_shout_key_dxcode();

	/**
	* allowed to use powers, less restrictive than regular casts
	*/
	bool allowed_to_instantcast(RE::FormID skill);

	/**
	* Assumes to be called after in_ingame_state, checks for additional player based restrictions (mounted, swimming, ...)
	*/
	bool allowed_to_cast(RE::FormID skill, bool allow_sprint = false);

	RE::TESForm* get_current_selected_spell_in_menu();

	bool slot_spell(RE::TESForm* form, size_t index);

	class KeyModifier
	{
    public:
        KeyModifier(RE::INPUT_DEVICE device, uint8_t code1, uint8_t code2 = 0Ui8);
		~KeyModifier() = default;

		void update(uint32_t key_code, RE::INPUT_DEVICE key_device, bool is_pressed);

		void rebind(int dx_scancode);
		int get_dx_scancode();
		int get_dx_scancode2();

		bool isValidBound();

		inline bool isDown();
    private:
		RE::INPUT_DEVICE input_device;

        uint8_t keycode;
        uint8_t keycode2;

		bool isDown1;
        bool isDown2;
	};

	inline bool KeyModifier::isDown()
	{
		return isDown1 || isDown2;
	}

	class KeyBind {
	public:
		KeyBind(RE::INPUT_DEVICE device, uint8_t code);
		~KeyBind() = default;

		bool matches(uint32_t key_code, RE::INPUT_DEVICE key_device) const;
		/*
		* device independant code used by game
		*/
		int get_dx_scancode() const;

		void assign_from_dx_scancode(int code);

		void update(uint32_t key_code, RE::INPUT_DEVICE key_device, bool is_pressed);

		inline bool isDown() const
		{
			return m_isDown;
		}
		void unbind();

		inline bool isValidBound() const {
			return input_device == RE::INPUT_DEVICE::kKeyboard ||
				   input_device == RE::INPUT_DEVICE::kMouse ||
				   input_device == RE::INPUT_DEVICE::kGamepad;
		};

	private:
		RE::INPUT_DEVICE input_device;
		uint8_t keycode;
		bool m_isDown;
	};
}