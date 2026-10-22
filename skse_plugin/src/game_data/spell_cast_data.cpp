#include "spell_cast_data.h"
#include "game_data.h"

namespace SpellHotbar::GameData {

    Spell_cast_data::Spell_cast_data() : gcd(-1.0f), cooldown(-1.0f), casttime(-1.0f), overlay_icon(GameData::DefaultIconType::UNKNOWN) {};
    bool Spell_cast_data::is_empty()
    {
        return gcd < 0.0f && cooldown < 0.0f && casttime < 0.0f && overlay_icon == GameData::DefaultIconType::UNKNOWN;
    }

    void Spell_cast_data::fill_default_values_from_spell(const RE::SpellItem* spell)
    {
        if (casttime < 0.0f) {
            casttime = spell->GetChargeTime();
        }
        if (spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell || spell->GetSpellType() == RE::MagicSystem::SpellType::kScroll) {
            //min casttime of 0.25f for actual spells
            casttime = std::max(0.25f, casttime);
        }

        if (gcd < 0.0f) {
            gcd = 0.0f;
        }
        if (cooldown < 0.0f) {
            cooldown = 0.0f;
        }
        if (overlay_icon == DefaultIconType::UNKNOWN && spell->GetSpellType() == RE::MagicSystem::SpellType::kScroll) {
            overlay_icon = DefaultIconType::SCROLL_OVERLAY;
        }

    }

    void Spell_cast_data::fill_default_values_from_shout(const RE::TESShout*)
    {
        if (casttime < 0.0f) {
            casttime = 0.0f;
        }
        if (gcd < 0.0f) {
            gcd = 0.0f;
        }
        if (cooldown < 0.0f) {
            cooldown = 0.0f;
        }
        //TODO generic shout overlay?
    }

    void Spell_cast_data::fill_and_override_from_non_default_values(const Spell_cast_data& other)
    {
        if (other.casttime > 0.0f) {
            casttime = other.casttime;
        }

        if (other.gcd > 0.0f) {
            gcd = other.gcd;
        }
        if (other.cooldown > 0.0f) {
            cooldown = other.cooldown;
        }
  
        if (other.overlay_icon != DefaultIconType::UNKNOWN) {
            overlay_icon = other.overlay_icon;
        }

    }
}
