#include "spell_cast_data.h"

namespace SpellHotbar::GameData {

    Spell_cast_data::Spell_cast_data() : gcd(-1.0f), cooldown(-1.0f), casttime(-1.0f), animation(-1), animation2(-1), casteffectid{ 0 } {}
    bool Spell_cast_data::is_empty()
    {
        return animation <= 0 && animation2 <= 0 && gcd < 0.0f && cooldown < 0.0f && casttime < 0.0f && casteffectid == 0U;
    }

    void Spell_cast_data::fill_default_values_from_spell(const RE::SpellItem* spell)
    {
        if (casttime < 0.0f) {
            casttime = spell->GetChargeTime();
        }
        if (spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
            //min casttime of 0.25f for actual spells
            casttime = std::max(0.25f, casttime);
        }

        if (gcd < 0.0f) {
            gcd = 0.0f;
        }
        if (cooldown < 0.0f) {
            if (spell->GetSpellType() == RE::MagicSystem::SpellType::kPower) {
                cooldown = 1.0f;
            }
            else {
                cooldown = 0.0f;
            }
        }
        if (animation < 0) {
            animation = Spell_cast_data::chose_default_anim_for_spell(spell, -1, false);
        }
        if (animation2 < 0) {
            animation2 = Spell_cast_data::chose_default_anim_for_spell(spell, -1, true);
        }

    }

    void Spell_cast_data::fill_default_values_from_shout(const RE::TESShout* shout)
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
        if (animation < 0) {
            animation = -1;
        }
        if (animation2 < 0) {
            animation2 = -1;
        }
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
        if (other.animation >= 0) {
            animation = other.animation;
        }
        if (other.animation2 >= 0) {
            animation2 = other.animation2;
        }
  
        casteffectid = other.casteffectid;

    }

    constexpr std::array<std::uint16_t, 2> cast_anims_aimed{ 1U, 10016U };
    constexpr std::array<std::uint16_t, 2> cast_anims_self{ 2U, 10017U };
    constexpr std::array<std::uint16_t, 2> cast_anims_aimed_conc{ 1001U, 11003U };
    constexpr std::array<std::uint16_t, 2> cast_anims_self_conc{ 1002U, 11004U };
    constexpr std::array<std::uint16_t, 2> cast_anims_ritual{ 10000U, 10016U };
    constexpr std::array<std::uint16_t, 2> cast_anims_ritual_conc{ 11001U, 11003U };
    constexpr std::array<std::uint16_t, 2> cast_anims_ritual_self{ 10000U, 10017U }; //same regular anim, but self cast fast anim

    inline bool is_ward_spell(const RE::SpellItem* spell) {
        return spell->effects.size() > 0 && spell->effects[0]->baseEffect && spell->effects[0]->baseEffect->HasKeywordID(0x1EA69);
    }

    uint16_t Spell_cast_data::chose_default_anim_for_spell(const RE::TESForm* form, int anim, bool anim2)
    {
        uint16_t ret{ 0U };

        if (anim < 0) {
            size_t ind = anim2 ? 1U : 0U;
            if (form->GetFormType() == RE::FormType::Spell) {
                const RE::SpellItem* spell = form->As<RE::SpellItem>();

                bool self = spell->GetDelivery() == RE::MagicSystem::Delivery::kSelf;
                if (spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration) {
                    if (spell->IsTwoHanded()) {
                        //Ritual Conc
                        ret = cast_anims_ritual_conc[ind]; // ritual self conc is not used or provided by vanilla game
                    }
                    else
                    {
                        if (is_ward_spell(spell)) {
                            //Ward anim, ward has no dual cast
                            ret = 1003U;
                        }
                        else {
                            //1H conc
                            ret = self ? cast_anims_self_conc[ind] : cast_anims_aimed_conc[ind];
                        }
                    }
                }
                else {
                    if (spell->IsTwoHanded()) {
                        //Ritual Cast
                        ret = self ? cast_anims_ritual_self[ind] : cast_anims_ritual[ind];
                    }
                    else
                    {
                        //Regular Cast
                        ret = self ? cast_anims_self[ind] : cast_anims_aimed[ind];
                    }
                }

            }
        }
        else {
            ret = static_cast<uint16_t>(anim);
        }
        return ret;
    }

    float Spell_cast_data::get_ritual_conc_anim_prerelease_time(int anim) {
        float pre_release_anim{ 0.0f };
        if (anim == cast_anims_ritual_conc[0]) {
            auto cam = RE::PlayerCamera::GetSingleton();
            if (cam && cam->IsInFirstPerson()) {
                pre_release_anim = 1.5f;
            }
            else
            {
                pre_release_anim = 1.0f;
            }
        }
        return pre_release_anim;
    }
}