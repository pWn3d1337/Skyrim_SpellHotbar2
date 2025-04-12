#include "localization.h"
#include <unordered_map>
#include <rapidcsv.h>
#include "../logger/logger.h"

namespace SpellHotbar {

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
			{ "$TYPE_POISON", "Poison" },
			{ "$TYPE_FOOD", "Food" },
			{ "$DASH", "-" },
			{ "$ICON_SIZE", "Icon Size" },
			{ "$BIND_MENU", "Binding Menu" },
			{ "$UNBIND", "Unbind" },
			{ "$WORDS_0", "0 Words" },
			{ "$WORDS_1", "1 Word" },
			{ "$WORDS_2", "2 Words" },
			{ "$WORDS_3", "3 Words" },
			{ "$RANK_NOVICE", "Novice" },
			{ "$RANK_APPRENTICE", "Apprentice" },
			{ "$RANK_ADEPT", "Adept" },
			{ "$RANK_EXPERT", "Expert" },
			{ "$RANK_MASTER", "Master" },
			{ "$BAR_EMPTY", "EMPTY" },
			{ "$BAR_BLOCKED", "BLOCKED" },
			{ "$KEY_MOD", "Mod" },
			{ "$KEY_NO_MOD", "No Mod" },
			{ "$MAIN_BAR", "Default" },
			{ "$MAIN_BAR_SNEAK", "Sneak" },
			{ "$MELEE_BAR", "Melee" },
			{ "$MELEE_BAR_SNEAK", "Sneak Melee" },
			{ "$ONE_HAND_SHIELD_BAR", "1H + Shield" },
			{ "$ONE_HAND_SHIELD_BAR_SNEAK", "Sneak 1H + Shield" },
			{ "$ONE_HAND_SPELL_BAR", "1H + Spell" },
			{ "$ONE_HAND_SPELL_BAR_SNEAK", "Sneak 1H + Spell" },
			{ "$DUAL_WIELD_BAR", "Dual Wield" },
			{ "$DUAL_WIELD_BAR_SNEAK", "Sneak Dual Wield" },
			{ "$TWO_HANDED_BAR", "Two-Handed" },
			{ "$TWO_HANDED_BAR_SNEAK", "Sneak Two-Handed" },
			{ "$RANGED_BAR", "Ranged" },
			{ "$RANGED_BAR_SNEAK", "Sneak Ranged" },
			{ "$MAGIC_BAR", "Magic" },
			{ "$MAGIC_BAR_SNEAK", "Sneak Magic" },
			{ "$VAMPIRE_LORD_BAR", "Vampire Lord" },
			{ "$WEREWOLF_BAR", "Werewolf" },
			{ "$SPELL_EDITOR", "Spell Editor" },
			{ "$FILTER","Filter" },
			{ "$FILTER_INFO", "Filter spell names containing text" },
			{ "$CHECK_EDITED", "Edited Only" },
			{ "$CHECK_NO_PREDEFINED", "No Predefined data" },
			{ "$COLUMN_ID", "ID" },
			{ "$COLUMN_PLUGIN", "Plugin" },
			{ "$COLUMN_ICON", "Icon" },
			{ "$COLUMN_CASTEFFECT", "Casteffect" },
			{ "$COLUMN_GCD", "GCD" },
			{ "$COLUMN_CD", "CD" },
			{ "$COLUMN_CASTTIME", "Casttime" },
			{ "$COLUMN_ANIM", "Anim" },
			{ "$COLUMN_ANIM2", "Anim2" },
			{ "$EDIT", "Edit" },
			{ "$RESET", "Reset" },
			{ "$DYNAMIC_FORM", "<Dynamic Form>" },
			{ "$QUESTION_MARKS", "???" },
			{ "$RESET_PROMT", "Reset all edits to" },
			{ "$OK", "Ok" },
			{ "$CANCEL", "Cancel" },
			{ "$EDIT_SPELL_DATA", "Edit Spell Data" },
			{ "$DESCRIPTION", "Description" },
			{ "$FILE", "File" },
			{ "$FORM_ID", "FormID" },
			{ "$SHOUT_POWER", "Shout (Power)" },
			{ "$DELIVERY", "Delivery" },
			{ "$DELIVERY_SELF", "Self" },
			{ "$DELIVERY_TOUCH", "Touch" },
			{ "$DELIVERY_AIMED", "Aimed" },
			{ "$DELIVERY_TARGET_ACTOR", "Target Actor" },
			{ "$DELIVERY_TARGET_LOCATION", "Target Location" },
			{ "$CASTING_TYPE", "Casting Type" },
			{ "$CASTING_TYPE_CONSTANT_EFFECT", "Constant Effect" },
			{ "$CASTING_TYPE_FIRE_AND_FORGET", "Fire And Forget" },
			{ "$CASTING_TYPE_CONCENTRATION", "Concentration" },
			{ "$CASTING_TYPE_SCROLL", "Scroll" },
			{ "$CAST_EFFECT", "Cast Effect" },
			{ "$NO_EFFECT", "No Effect" },
			{ "$GLOBAL_COOLDOWN", "Globald Cooldown" },
			{ "$COOLDOWN", "Cooldown" },
			{ "$CAST_TIME", "Cast Time" },
			{ "$ANIMATION", "Animation" },
			{ "$ANIMATION2", "Animation2" },
			{ "$SAVE", "Save" },
			{ "$POTION_EDITOR", "Potion Editor" },
			{ "$EDIT_ICON", "Edit Icon" },
			{ "$NO_PREDEFINED", "No Predefined Data" },
			{ "$BAR_TEXT", "Bar Text" },
			{ "$BAR_DRAG_INFO_ESC", "ESC: finish dragging" },
			{ "$BAR_DRAG_INFO_TAB", "TAB: leave menus while still dragging" },
			{ "$BAR_DRAG_INFO_WHEEL", "Mouse Wheel: Icon Size" },
			{ "$BAR_DRAG_INFO_ALT", "ALT + Mouse Wheel : Spacing" },
			{ "$MCM_PROMPT_BAR_LOAD", "Load bars from" },
			{ "$MCM_PROMPT_BAR_SAVE", "Save current bars as" },
			{ "$MCM_PROMPT_OVERWRITE", "exists, overwrite ?" },
			{ "$MCM_PROMPT_SETTINGS_LOAD", "Load settings from preset" },
			{ "$MCM_PROMPT_SETTINGS_SAVE", "Save current settings as" },
			{ "$SLOT_BLOCKED_TITLE", "Blocked from Inheritance" },
			{ "$SLOT_BLOCKED_INFO", "<Click to unblock>" },
			{ "$MCM_PROMPT_ICON_EDITS_SAVE", "Save Spell and Item edits as" },
			{ "$MCM_PROMPT_ICON_EDITS_LOAD", "Load Spell and Item edits from" },
			{ "$RESET_ALL", "Reset All" },
			{ "$RESET_ALL_PROMPT", "Reset all edits?" }
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
			return key;
		}
	}

	const char* translate_c(const std::string& key) {
		if (translations.contains(key)) {
			return translations.at(key).c_str();
		}
		else
		{
			return key.c_str();
		}
	}

	std::string translate_id(const std::string& key)
	{
		return translate(key) + "##" + key;
	}

	void load_translations(const std::filesystem::path& translation_file_path)
	{
		translations = _get_default_translations();

		if (std::filesystem::exists(translation_file_path)) {
			std::string path = translation_file_path.string();

			rapidcsv::Document doc(path, rapidcsv::LabelParams(-1, -1), rapidcsv::SeparatorParams('\t'));
			if (doc.GetColumnCount() == 2)
			{
				for (size_t i = 0; i < doc.GetRowCount(); i++) {
					try {
						std::string key = doc.GetCell<std::string>(0, i);
						std::string text = doc.GetCell<std::string>(1, i);

						if (translations.contains(key)) {
							translations.insert_or_assign(key, text);
						}
						else
						{
							logger::warn("Skipping translation, unknown key: {}", key);
						}
					}
					catch (const std::exception& e) {
						std::string msg = e.what();
						logger::error("Error Loading {}: {}", path, msg);
					}
				}
			}
			else {
				logger::error("Malformed translation file: {}", path);
			}
		}
	}

}