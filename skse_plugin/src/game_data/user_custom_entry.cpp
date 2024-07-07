#include "user_custom_entry.h"
#include "../logger/logger.h"

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
				if (ok) ok = serializer->WriteRecordData(&m_icon_str, len);
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

	bool User_custom_entry::has_icon_data()
	{
		return m_icon_form > 0 || !m_icon_str.empty();
	}
}