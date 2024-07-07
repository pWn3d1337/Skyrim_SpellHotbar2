#pragma once
#include "spell_cast_data.h"
#include "user_custom_entry.h"

namespace SpellHotbar::GameData {

	class User_custom_spelldata : public User_custom_entry {
	public:

		enum save_flags : uint8_t
		{
			none =         0,
			gcd =          1 << 0,
			cooldown =     1 << 1,
			casttime =     1 << 2,
			animation =    1 << 3,
			animation2 =   1 << 4,
			casteffectid = 1 << 5,
			icon_form =    1 << 6,
			icon_str =     1 << 7
		};

		User_custom_spelldata(RE::FormID form);
		virtual ~User_custom_spelldata() = default;

		virtual void serialize(SKSE::SerializationInterface* serializer) const override;
		static User_custom_spelldata deserialize(SKSE::SerializationInterface* serializer, uint32_t version);

		/*
		* has changes that require saving
		*/
		virtual bool is_non_default() override;

		/**
		* Check which fields need to be saved (non default)
		*/
		save_flags calc_save_flags() const;

		bool has_different_data(const Spell_cast_data& other);

		//Members
		Spell_cast_data m_spell_data;
	};

	inline User_custom_spelldata::save_flags operator~ (User_custom_spelldata::save_flags a) { return (User_custom_spelldata::save_flags)~(int)a; }
	inline User_custom_spelldata::save_flags operator| (User_custom_spelldata::save_flags a, User_custom_spelldata::save_flags b) { return (User_custom_spelldata::save_flags)((int)a | (int)b); }
	inline User_custom_spelldata::save_flags operator& (User_custom_spelldata::save_flags a, User_custom_spelldata::save_flags b) { return (User_custom_spelldata::save_flags)((int)a & (int)b); }
	inline User_custom_spelldata::save_flags& operator|= (User_custom_spelldata::save_flags& a, User_custom_spelldata::save_flags b) { return (User_custom_spelldata::save_flags&)((int&)a |= (int)b); }
	inline User_custom_spelldata::save_flags& operator&= (User_custom_spelldata::save_flags& a, User_custom_spelldata::save_flags b) { return (User_custom_spelldata::save_flags&)((int&)a &= (int)b); }
	inline User_custom_spelldata::save_flags& operator^= (User_custom_spelldata::save_flags& a, User_custom_spelldata::save_flags b) { return (User_custom_spelldata::save_flags&)((int&)a ^= (int)b); }

}