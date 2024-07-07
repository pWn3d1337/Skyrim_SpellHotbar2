#pragma once

namespace SpellHotbar::GameData {

	class User_custom_entry {
	public:

		User_custom_entry(RE::FormID form);
		virtual ~User_custom_entry() = default;

		virtual void serialize(SKSE::SerializationInterface* serializer) const;
		static User_custom_entry deserialize(SKSE::SerializationInterface* serializer, uint32_t version);

		/*
		* has changes that require saving
		*/
		virtual bool is_non_default();

		bool has_icon_data();

		//Members
		RE::FormID m_form_id;
		RE::FormID m_icon_form;
		std::string m_icon_str;
	};
}