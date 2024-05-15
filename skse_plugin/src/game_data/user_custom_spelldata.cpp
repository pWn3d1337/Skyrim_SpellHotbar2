#include "user_custom_spelldata.h"
#include "../logger/logger.h"

namespace SpellHotbar::GameData {

	User_custom_spelldata::User_custom_spelldata(RE::FormID form)
		: m_form_id(form),
		m_spell_data(),
		m_icon_form(0),
		m_icon_str("")
	{
	}

	void User_custom_spelldata::serialize(SKSE::SerializationInterface* serializer) const
	{
		save_flags flags = calc_save_flags();

		bool ok{ true };
		ok = serializer->WriteRecordData(&m_form_id, sizeof(RE::FormID));
		//logger::info("Saving Flags: {:b}", std::to_underlying(flags));
		if (ok) ok = serializer->WriteRecordData(&flags, sizeof(save_flags));

		if (ok && (flags & save_flags::gcd)) ok = serializer->WriteRecordData(&m_spell_data.gcd, sizeof(float));
		if (ok && (flags & save_flags::cooldown)) ok = serializer->WriteRecordData(&m_spell_data.cooldown, sizeof(float));
		if (ok && (flags & save_flags::casttime)) ok = serializer->WriteRecordData(&m_spell_data.casttime, sizeof(float));
		if (ok && (flags & save_flags::animation)) ok = serializer->WriteRecordData(&m_spell_data.animation, sizeof(int));
		if (ok && (flags & save_flags::animation2)) ok = serializer->WriteRecordData(&m_spell_data.animation2, sizeof(int));
		if (ok && (flags & save_flags::casteffectid)) ok = serializer->WriteRecordData(&m_spell_data.casteffectid, sizeof(uint16_t));
		if (ok && (flags & save_flags::icon_form)) ok = serializer->WriteRecordData(&m_icon_form, sizeof(RE::FormID));
						
		if (ok && (flags & save_flags::icon_str)) {
			uint16_t len = static_cast<uint16_t>(m_icon_str.length()); //Hopefully there is no icon string longer than 65536 chars :)
			ok = serializer->WriteRecordData(&len, sizeof(uint16_t));
			if (ok) ok = serializer->WriteRecordData(&m_icon_str, len);
		}

		if (!ok) {
			logger::error("Error during loading User_custom_spelldata: {0:x}", m_form_id);
		}

	}

	User_custom_spelldata User_custom_spelldata::deserialize(SKSE::SerializationInterface* serializer, uint32_t version)
	{
		bool ok{ true };
		RE::FormID form_id{ 0 };
		save_flags flags{ save_flags::none };
		ok = serializer->ReadRecordData(&form_id, sizeof(RE::FormID));
		if (!ok) {
			logger::error("Error during loading User_custom_spelldata (formID)");
			return User_custom_spelldata(0);
		}
		serializer->ResolveFormID(form_id, form_id);

		ok = serializer->ReadRecordData(&flags, sizeof(save_flags));
		if (!ok) {
			logger::error("Error during loading User_custom_spelldata (flags)");
			return User_custom_spelldata(0);
		}
		else {
			//logger::info("Flags: {:b}", std::to_underlying(flags));
		}

		User_custom_spelldata data(form_id);

		if (ok && (flags & save_flags::gcd)) ok = serializer->ReadRecordData(&data.m_spell_data.gcd, sizeof(float));
		if (ok && (flags & save_flags::cooldown)) ok = serializer->ReadRecordData(&data.m_spell_data.cooldown, sizeof(float));
		if (ok && (flags & save_flags::casttime)) ok = serializer->ReadRecordData(&data.m_spell_data.casttime, sizeof(float));
		if (ok && (flags & save_flags::animation)) ok = serializer->ReadRecordData(&data.m_spell_data.animation, sizeof(int));
		if (ok && (flags & save_flags::animation2)) ok = serializer->ReadRecordData(&data.m_spell_data.animation2, sizeof(int));
		if (ok && (flags & save_flags::casteffectid)) ok = serializer->ReadRecordData(&data.m_spell_data.casteffectid, sizeof(uint16_t));
		if (ok && (flags & save_flags::icon_form))
		{
			ok = serializer->ReadRecordData(&data.m_icon_form, sizeof(RE::FormID));
			if (ok) ok = serializer->ResolveFormID(data.m_icon_form, data.m_icon_form);
		}
					
		if (ok && (flags & save_flags::icon_str)) {
			uint16_t len{ 0U };
			ok = serializer->ReadRecordData(&len, sizeof(uint16_t));
			std::vector<char> buf(len, '\0');
			if (ok) ok = serializer->ReadRecordData(&buf[0], len);
			data.m_icon_str = std::string(buf.begin(), buf.end());
		}

		if (ok) {
			return data;
		} else {
			logger::error("Error during loading User_custom_spelldata");
			return User_custom_spelldata(0);
		}
	}

	bool User_custom_spelldata::is_non_default()
	{
		return !m_spell_data.is_empty() || !m_icon_str.empty() || m_icon_form > 0;
	}

	User_custom_spelldata::save_flags User_custom_spelldata::calc_save_flags() const
	{
		save_flags flags{ save_flags::none };

		if (m_spell_data.gcd > 0.0f) flags |= save_flags::gcd;
		if (m_spell_data.cooldown > 0.0f) flags |= save_flags::cooldown;
		if (m_spell_data.casttime > 0.0f) flags |= save_flags::casttime;
		if (m_spell_data.animation >= 0) flags |= save_flags::animation;
		if (m_spell_data.animation2 >= 0) flags |= save_flags::animation2;
		if (m_spell_data.casteffectid > 0U) flags |= save_flags::casteffectid;
		if (m_icon_form > 0) flags |= save_flags::icon_form;
		if (!m_icon_str.empty()) flags |= save_flags::icon_str;

		return flags;
	}

	bool User_custom_spelldata::has_different_data(const Spell_cast_data& other)
	{
		//TODO remove comment
		//logger::info("Self: {}, {}, {}, {}, {}, {}", m_spell_data.gcd, m_spell_data.cooldown, m_spell_data.casttime, m_spell_data.animation, m_spell_data.animation2, m_spell_data.casteffectid);
		//logger::info("Other: {}, {}, {}, {}, {}, {}", other.gcd, other.cooldown, other.casttime, other.animation, other.animation2, other.casteffectid);

		return m_spell_data.gcd != other.gcd ||
			m_spell_data.cooldown != other.cooldown ||
			m_spell_data.casttime != other.casttime ||
			m_spell_data.animation != other.animation ||
			m_spell_data.animation2 != other.animation2 ||
			m_spell_data.casteffectid != other.casteffectid;
	}

	bool User_custom_spelldata::has_icon_data()
	{
		return m_icon_form > 0 || !m_icon_str.empty();
	}

}