#pragma once
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

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

		bool to_json(rapidjson::Document& doc, uint32_t key, rapidjson::Value& parent_node);

		bool from_json(rapidjson::Value& node, bool require_icon_data = true);

		virtual bool extradata_from_json(rapidjson::Value& node);

		bool has_icon_data();

		//Members
		RE::FormID m_form_id;
		RE::FormID m_icon_form;
		std::string m_icon_str;
	protected:
		/*
		* Used for child classes to add extra data
		*/
		virtual bool to_json_additional_data(rapidjson::Document& doc, uint32_t key, rapidjson::Value& parent_node);
	};
}