#include "localization.h"
#include <unordered_map>

namespace SpellHotbar {

	const std::string default_text = "NO_TRANSLATION";


	std::unordered_map<std::string, std::string> _get_default_translations()
	{
		std::unordered_map<std::string, std::string> t = {
			{"$HAND_TEXT_LEFT", "L"},
			{"$HAND_TEXT_RIGHT", "R"},
			{"$HAND_TEXT_DUAL", "D"},
			{"$TAB_TEXT_ALL",  "All"},
			{"$TAB_TEXT_SPELLS",  "Spells"},
			{"$TAB_TEXT_ALTERATION",  "Alteration"},
			{"$TAB_TEXT_ILLUSION",  "Illusion"},
			{"$TAB_TEXT_DESTRUCTION",  "Destruction"},
			{"$TAB_TEXT_CONJURATION",  "Conjuration"},
			{"$TAB_TEXT_RESTORATION",  "Restoration"},
			{"$TAB_TEXT_SHOUTS",  "Shouts"},
			{"$TAB_TEXT_POWERS",  "Powers"},
			{"$TAB_TEXT_SCROLLS",  "Scrolls"},
			{"$TAB_TEXT_POTIONS",  "Potions"},
			{"$TAB_TEXT_POISONS",  "Poisons"},
			{"$TAB_TEXT_FOOD",  "Food"},
			{"$FILTER_TEXT", "Filter text"},
			{"$COLUMN_DRAG", "Drag Icon"},
			{"$COLUMN_NAME", "Name"},
			{"$COLUMN_TYPE", "Type"},
			{"$COLUMN_SCHOOL", "School"},
			{"$COLUMN_RANK", "Rank"},
			{"$COLUMN_MAG", "Mag"},
			{"$COLUMN_TIME", "Time"},
			{"$TYPE_POTION", "Potion"},
			{"$TYPE_SPELL", "Spell"},
			{"$TYPE_LESSER_POWER", "Lesser Power"},
			{"$TYPE_GREATER_POWER", "Greater Power"},
			{"$TYPE_SHOUT", "Shout"},
			{"$TYPE_SCROLL", "Scroll"},
			{"$TYPE_POISON", "Poison"},
			{"$TYPE_FOOD", "Food"},
			{"$DASH", "-"},
			{"$ICON_SIZE", "Icon Size"},
			{"$BIND_MENU", "Binding Menu"},
			{"$UNBIND", "Unbind"},
			{"$WORDS_0", "0 Words"},
			{"$WORDS_1", "1 Word"},
			{"$WORDS_2", "2 Words"},
			{"$WORDS_3", "3 Words"},
			{"$RANK_NOVICE", "Novice"},
			{"$RANK_APPRENTICE", "Apprentice"},
			{"$RANK_ADEPT", "Adept"},
			{"$RANK_EXPERT", "Expert"},
			{"$RANK_MASTER", "Master"},
		};
		return t;
	}

	//Initialized with default translations, updated from file
	std::unordered_map<std::string, std::string> translations = _get_default_translations();



	std::string translate(const std::string& key)
	{
		if (translations.contains(key)) {
			return translations.at(key);
		}
		else
		{
			return default_text;
		}
	}

	const char* translate_c(const std::string& key) {
		if (translations.contains(key)) {
			return translations.at(key).c_str();
		}
		else
		{
			return default_text.c_str();
		}
	}

	std::string translate_id(const std::string& key)
	{
		return translate(key) + "##" + key;
	}

}