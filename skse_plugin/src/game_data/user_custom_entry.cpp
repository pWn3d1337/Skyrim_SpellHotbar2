#include "user_custom_entry.h"
#include "../logger/logger.h"
#include <rapidjson/document.h>
#include "game_data.h"

namespace rj = rapidjson;

namespace SpellHotbar::GameData {

	User_custom_entry::User_custom_entry(RE::FormID form)
		: m_form_id(form),
		m_icon_form(0),
		m_icon_str("")
	{
	}

	void User_custom_entry::serialize(SKSE::SerializationInterface* serializer) const
	{
		bool ok{ true };
		ok = serializer->WriteRecordData(&m_form_id, sizeof(RE::FormID));

		if (ok) ok = serializer->WriteRecordData(&m_icon_form, sizeof(RE::FormID));

		if (ok) {
			if (!m_icon_str.empty()) {
				uint16_t len = static_cast<uint16_t>(m_icon_str.length());
				ok = serializer->WriteRecordData(&len, sizeof(uint16_t));
				if (ok) ok = serializer->WriteRecordData(m_icon_str.c_str(), len);
			}
			else {
				constexpr uint16_t zero{ 0Ui16 };
				ok = serializer->WriteRecordData(&zero, sizeof(uint16_t));
			}
		}

		if (!ok) {
			logger::error("Error during loading User_custom_entry: {0:x}", m_form_id);
		}

	}

	User_custom_entry User_custom_entry::deserialize(SKSE::SerializationInterface* serializer, uint32_t version)
	{
		bool ok{ true };
		RE::FormID form_id{ 0 };
		ok = serializer->ReadRecordData(&form_id, sizeof(RE::FormID));
		if (!ok) {
			logger::error("Error during loading User_custom_spelldata (formID)");
			return User_custom_entry(0);
		}
		serializer->ResolveFormID(form_id, form_id);

		User_custom_entry data(form_id);


		ok = serializer->ReadRecordData(&data.m_icon_form, sizeof(RE::FormID));
		if (ok && data.m_icon_form != 0) ok = serializer->ResolveFormID(data.m_icon_form, data.m_icon_form);

		if (ok) {
			uint16_t len{ 0U };
			ok = serializer->ReadRecordData(&len, sizeof(uint16_t));

			if (len > 0) {
				std::vector<char> buf(len, '\0');
				if (ok) ok = serializer->ReadRecordData(&buf[0], len);
				data.m_icon_str = std::string(buf.begin(), buf.end());
			}
			else {
				data.m_icon_str = "";
			}
		}

		if (ok) {
			return data;
		}
		else {
			logger::error("Error during loading User_custom_entry");
			return User_custom_entry(0);
		}
	}

	bool User_custom_entry::is_non_default()
	{
		return !m_icon_str.empty() || m_icon_form > 0;
	}

	bool User_custom_entry::to_json(rapidjson::Document& doc, uint32_t key, rapidjson::Value& parent_node)
	{
		rj::Value node(rj::kObjectType);

		auto form = RE::TESForm::LookupByID(m_form_id);
		if (form != nullptr) {
			auto file = form->GetFile(0);
			if (file != nullptr) {
				const std::string_view& name = file->GetFilename();
				RE::FormID localid = form->GetLocalFormID();
				rj::Value fn(name.data(), doc.GetAllocator());
				node.AddMember("form_id", localid, doc.GetAllocator());
				node.AddMember("form_file", fn, doc.GetAllocator());

				if (m_icon_form > 0) {
					auto icon_form = RE::TESForm::LookupByID(m_icon_form);
					if (icon_form != nullptr) {
						auto icon_file = icon_form->GetFile(0);
						if (icon_file != nullptr) {
							const std::string_view& icon_name = icon_file->GetFilename();
							RE::FormID icon_localid = icon_form->GetLocalFormID();
							rj::Value icon_fn(icon_name.data(), doc.GetAllocator());
							node.AddMember("icon_id", icon_localid, doc.GetAllocator());
							node.AddMember("icon_file", icon_fn, doc.GetAllocator());
						}
					}
				}
				if (!m_icon_str.empty()) {
					rj::Value icon_str_fn(m_icon_str.data(), doc.GetAllocator());
					node.AddMember("icon_str", icon_str_fn, doc.GetAllocator());
				}
				to_json_additional_data(doc, key, node);
				parent_node.PushBack(node, doc.GetAllocator());
				return true;
			}
		}
		return false;
	}

	bool User_custom_entry::from_json(rapidjson::Value& node, bool require_icon_data)
	{
		if (node.HasMember("form_id") && node.HasMember("form_file")) {
			RE::FormID local_id = static_cast<RE::FormID>(node["form_id"].GetInt64());
			std::string file = node["form_file"].GetString();

			auto form = GameData::get_form_from_file(local_id, file);
			if (form != nullptr) {
				m_form_id = form->GetFormID();
			}
			else {
				logger::error("Could not load {:08x} from {}", local_id, file);
				return false;
			}

			if (node.HasMember("icon_id") && node.HasMember("icon_file")) {
				RE::FormID local_icon_id = static_cast<RE::FormID>(node["icon_id"].GetInt64());
				std::string icon_file = node["icon_file"].GetString();
				
				auto icon_form = GameData::get_form_from_file(local_icon_id, icon_file);
				if (icon_form != nullptr) {
					m_icon_form = icon_form->GetFormID();
				}
				else {
					logger::error("Could not load {:08x} from {}", local_icon_id, icon_file);
					return false;
				}
			}

			if (node.HasMember("icon_str")) {
				m_icon_str = node["icon_str"].GetString();
			}

			if (require_icon_data && !has_icon_data()) {
				logger::error("Error Loading {:08x}: no icon data", m_form_id);
				return false;
			}

			return extradata_from_json(node);
		}
		else {
			logger::error("Cannot parse entry, missing required fields 'form_id' or 'form_file'");
			return false;
		}

		return false;
	}

	bool User_custom_entry::extradata_from_json(rapidjson::Value& node)
	{
		return true;
	}

	bool User_custom_entry::has_icon_data()
	{
		return m_icon_form > 0 || !m_icon_str.empty();
	}
	bool User_custom_entry::to_json_additional_data(rapidjson::Document& doc, uint32_t key, rapidjson::Value& parent_node)
	{
		return true;
	}
}