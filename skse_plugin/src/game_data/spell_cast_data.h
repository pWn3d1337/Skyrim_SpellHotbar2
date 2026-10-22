#pragma once

namespace SpellHotbar::GameData {

    enum class DefaultIconType : uint32_t; //forward declaration

    struct Spell_cast_data {
        float gcd;
        float cooldown;
        float casttime;
        DefaultIconType overlay_icon;

        Spell_cast_data();
        bool is_empty();

        /*
        * This fills all defaulted fields with the values from the passed spell
        */
        void fill_default_values_from_spell(const RE::SpellItem* spell);

        /*
        * This fills all defaulted fields with the values from the passed shout
        */
        void fill_default_values_from_shout(const RE::TESShout* shout);

        /**
        * override all members by the passed other struct if they are not default
        */
        void fill_and_override_from_non_default_values(const Spell_cast_data & other);
    };
}