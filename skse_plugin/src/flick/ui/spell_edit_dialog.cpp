#include "spell_edit_dialog.h"

#include "spell_editor.h"
#include "widgets.h"
#include "../../game_data/localization.h"

// The per-spell override dialog, ported from rendering/spell_edit_dialog.cpp to FLICK.
// Same rows, same checkbox-gates-the-field shape, same Save/Cancel semantics: Save writes into
// GameData::user_spell_cast_info only when the data differs from the unfilled defaults or an
// icon was picked, and erases the entry otherwise.
//
// Two things changed shape because FUCK:: has no equivalent. ImGui::InputScalar with its
// increment buttons is a FUCK::DragFloat over the same range and clamp; BeginCombo/Selectable is
// FlickUi::combo over an index. The icon grid is the shared FlickUi::icon_picker, which is that
// same catalogue with the same wrap rule.
namespace SpellHotbar::FlickUi::SpellEditor {
    namespace {
        bool initialized{ false };
        bool custom_casteffect{ false };
        bool custom_gcd{ false };
        bool custom_cd{ false };
        bool custom_casttime{ false };
        bool custom_anim{ false };
        bool custom_anim2{ false };

        const ImVec4 col_gray{ 0.498f, 0.498f, 0.498f, 1.0f };

        void gray_text(const char* text)
        {
            FUCK::PushStyleColor(ImGuiCol_Text, col_gray);
            FUCK::TextUnformatted(text);
            FUCK::PopStyleColor(1);
        }

        void gray_textf(const char* fmt, auto value)
        {
            FUCK::PushStyleColor(ImGuiCol_Text, col_gray);
            FUCK::Text(fmt, value);
            FUCK::PopStyleColor(1);
        }

        void check_init(GameData::User_custom_spelldata& data, GameData::Spell_cast_data& dat_unfilled)
        {
            if (!initialized) {
                auto& dat = data.m_spell_data;

                custom_casteffect = dat.casteffectid != dat_unfilled.casteffectid;
                custom_gcd = dat.gcd != dat_unfilled.gcd;
                custom_cd = dat.cooldown != dat_unfilled.cooldown;
                custom_casttime = dat.casttime != dat_unfilled.casttime;
                custom_anim = dat.animation != dat_unfilled.animation;
                custom_anim2 = dat.animation2 != dat_unfilled.animation2;

                initialized = true;
            }
        }

        void close()
        {
            initialized = false;
            custom_casteffect = false;
            custom_gcd = false;
            custom_cd = false;
            custom_casttime = false;
            custom_anim = false;
            custom_anim2 = false;

            close_edit_dialog();
        }

        // A row of the left table: label in column one, the caller's widget in column two.
        void row_label(int& id, const char* label)
        {
            FUCK::PushID(id++);
            FUCK::TableNextRow();
            FUCK::TableNextColumn();
            FUCK::TextUnformatted(label);
            FUCK::TableNextColumn();
        }

        // The two animation combos are the same widget over the same list; only which field they
        // write differs.
        void anim_row(int& anim_value, bool& custom, int dat_filled_value, const char* combo_suffix)
        {
            checkbox(std::string("##chk").append(combo_suffix).c_str(), &custom);
            FUCK::SameLine();

            if (!custom) {
                FUCK::BeginDisabled();
                anim_value = dat_filled_value;
            }

            auto& anims = get_list_of_anims();
            std::vector<std::string> labels;
            labels.reserve(anims.size());
            int index = -1;
            for (std::size_t n = 0; n < anims.size(); ++n) {
                const auto it = GameData::animation_names.find(anims[n]);
                labels.emplace_back(it != GameData::animation_names.end() ? it->second : std::to_string(anims[n]));
                if (anims[n] == anim_value) {
                    index = static_cast<int>(n);
                }
            }

            // The old combo's label was the raw animation id, so the number stays readable
            // beside the name the list shows.
            const std::string label = std::to_string(anim_value) + "##cbo" + combo_suffix;
            if (combo(label.c_str(), &index, labels) && index >= 0 && index < static_cast<int>(anims.size())) {
                anim_value = anims[static_cast<std::size_t>(index)];
            }

            if (!custom) {
                FUCK::EndDisabled();
            }
        }
    }

    void draw_edit_dialog(const RE::TESForm* a_form, GameData::User_custom_spelldata& a_data,
                          GameData::Spell_cast_data& a_filled, GameData::Spell_cast_data& a_unfilled,
                          GameData::Spell_cast_data& a_saved)
    {
        check_init(a_data, a_unfilled);

        static RE::FormID last_tooltip = 0;
        static std::string description = "";

        auto& dat = a_data.m_spell_data;

        const RE::SpellItem* spell = nullptr;
        if (a_form->GetFormType() == RE::FormType::Spell) {
            spell = a_form->As<RE::SpellItem>();
        }

        const RE::TESShout* shout = nullptr;
        if (a_form->GetFormType() == RE::FormType::Shout) {
            shout = a_form->As<RE::TESShout>();
        }
        if (a_form->GetFormID() != last_tooltip) {
            description = UiBridge::skill_tooltip(a_form);
        }
        last_tooltip = a_form->GetFormID();

        using TF = FUCK::TableFlags;
        const TF flags = TF::kRowBg | TF::kBordersOuterH | TF::kBordersOuterV | TF::kBordersInnerV |
                         static_cast<TF>(ImGuiTableFlags_NoBordersInBody) | static_cast<TF>(ImGuiTableFlags_ScrollY);

        //calc button height:
        push_large_font();
        const float button_height = FUCK::CalcTextSize("Cancel").y + FUCK::GetStyleVarVec(ImGuiStyleVar_FramePadding).y * 2.0f +
                                    FUCK::GetStyleVarVec(ImGuiStyleVar_ItemSpacing).y * 2.0f;
        pop_font();
        const float child_window_height = FUCK::GetContentRegionAvail().y - button_height;

        FUCK::BeginChild("LeftTab", ImVec2(FUCK::GetContentRegionAvail().x * 0.5f, child_window_height), false,
                         ImGuiWindowFlags_HorizontalScrollbar);

        if (FUCK::BeginTable("Data", 2, flags, ImVec2(0.0f, 0.0f), 0.0f)) {
            using CF = FUCK::TableColumnFlags;
            FUCK::TableSetupColumn("", CF::kNoSort | CF::kWidthFixed, 0.0f, 0);
            FUCK::TableSetupColumn("", CF::kNoSort | CF::kWidthStretch, 0.0f, 1);
            FUCK::TableSetupScrollFreeze(0, 1); // Make row always visible
            FUCK::TableHeadersRow();

            int id{ 0 };

            row_label(id, translate_c("$COLUMN_NAME"));
            FUCK::TextUnformatted(a_form->GetName());
            FUCK::PopID();

            row_label(id, translate_c("$COLUMN_ICON"));
            {
                const ImVec2 iconpos = FUCK::GetCursorScreenPos();
                const float ic_size = std::round(scale(60.0f));
                FUCK::Dummy(ImVec2(ic_size, ic_size));

                const bool show_reset_button = a_data.m_icon_form > 0 || !a_data.m_icon_str.empty();
                if (show_reset_button) {
                    draw_custom_icon_at(a_data.m_icon_form, a_data.m_icon_str, iconpos, ic_size);
                }
                else {
                    //default
                    draw_skill_at(a_form->GetFormID(), iconpos, ic_size);
                }

                if (show_reset_button) {
                    FUCK::SameLine();
                    if (FUCK::Button((translate("$RESET") + "##reset_icon").c_str())) {
                        a_data.m_icon_form = 0;
                        a_data.m_icon_str = "";
                    }
                }
            }
            FUCK::PopID();

            row_label(id, translate_c("$DESCRIPTION"));
            gray_text(description.c_str());
            FUCK::PopID();

            row_label(id, translate_c("$FILE"));
            {
                auto file = a_form->GetFile(0);
                gray_text(file != nullptr ? file->fileName : translate_c("$DYNAMIC_FORM"));
            }
            FUCK::PopID();

            row_label(id, translate_c("$FORM_ID"));
            gray_textf("%08x", a_form->GetFormID());
            FUCK::PopID();

            bool is_greater_power{ false };
            bool is_shout{ false };
            row_label(id, translate_c("$COLUMN_TYPE"));
            if (spell) {
                switch (spell->GetSpellType()) {
                case RE::MagicSystem::SpellType::kSpell:
                    gray_text(translate_c("$TYPE_SPELL"));
                    break;
                case RE::MagicSystem::SpellType::kPower:
                    gray_text(translate_c("$TYPE_GREATER_POWER"));
                    is_greater_power = true;
                    break;
                case RE::MagicSystem::SpellType::kLesserPower:
                    gray_text(translate_c("$TYPE_LESSER_POWER"));
                    break;
                default:
                    gray_text(translate_c("$QUESTION_MARKS"));
                }
            }
            else if (shout) {
                is_shout = true;
                if (shout->formFlags & RE::TESShout::RecordFlags::kTreatSpellsAsPowers) {
                    gray_text(translate_c("$SHOUT_POWER"));
                    is_greater_power = true;
                }
                else {
                    gray_text(translate_c("$TYPE_SHOUT"));
                }
            }
            else {
                gray_text(translate_c("$QUESTION_MARKS"));
            }
            FUCK::PopID();

            row_label(id, translate_c("$DELIVERY"));
            if (spell) {
                switch (spell->GetDelivery()) {
                case RE::MagicSystem::Delivery::kSelf:
                    gray_text(translate_c("$DELIVERY_SELF"));
                    break;
                case RE::MagicSystem::Delivery::kTouch:
                    gray_text(translate_c("$DELIVERY_TOUCH"));
                    break;
                case RE::MagicSystem::Delivery::kAimed:
                    gray_text(translate_c("$DELIVERY_AIMED"));
                    break;
                case RE::MagicSystem::Delivery::kTargetActor:
                    gray_text(translate_c("$DELIVERY_TARGET_ACTOR"));
                    break;
                case RE::MagicSystem::Delivery::kTargetLocation:
                    gray_text(translate_c("$DELIVERY_TARGET_LOCATION"));
                    break;
                default:
                    gray_text(translate_c("$QUESTION_MARKS"));
                }
            }
            else {
                gray_text(translate_c("$QUESTION_MARKS"));
            }
            FUCK::PopID();

            row_label(id, translate_c("$CASTING_TYPE"));
            if (spell) {
                switch (spell->GetCastingType()) {
                case RE::MagicSystem::CastingType::kConstantEffect:
                    gray_text(translate_c("$CASTING_TYPE_CONSTANT_EFFECT"));
                    break;
                case RE::MagicSystem::CastingType::kFireAndForget:
                    gray_text(translate_c("$CASTING_TYPE_FIRE_AND_FORGET"));
                    break;
                case RE::MagicSystem::CastingType::kConcentration:
                    gray_text(translate_c("$CASTING_TYPE_CONCENTRATION"));
                    break;
                case RE::MagicSystem::CastingType::kScroll:
                    gray_text(translate_c("$CASTING_TYPE_SCROLL"));
                    break;
                default:
                    gray_text(translate_c("$QUESTION_MARKS"));
                }
            }
            else {
                gray_text(translate_c("$QUESTION_MARKS"));
            }
            FUCK::PopID();

            row_label(id, translate_c("$CAST_EFFECT"));
            if (spell && spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                checkbox("##chk_casteffect", &custom_casteffect);
                FUCK::SameLine();

                if (!custom_casteffect) {
                    FUCK::BeginDisabled();
                    dat.casteffectid = a_filled.casteffectid;
                }

                std::vector<std::string> effects;
                effects.reserve(GameData::spell_casteffect_art.size());
                for (const auto& effect_dat : GameData::spell_casteffect_art) {
                    effects.emplace_back(std::get<2>(effect_dat));
                }
                int index = (dat.casteffectid < effects.size()) ? static_cast<int>(dat.casteffectid) : -1;
                if (combo("##cbo_cast_effect", &index, effects) && index >= 0) {
                    dat.casteffectid = static_cast<uint16_t>(index);
                }

                if (!custom_casteffect) FUCK::EndDisabled();
            }
            else {
                gray_text(translate_c("$NO_EFFECT"));
            }
            FUCK::PopID();

            row_label(id, translate_c("$GLOBAL_COOLDOWN"));
            {
                //disabled for now
                constexpr bool custom_gcd_enabled{ false };
                if constexpr (custom_gcd_enabled) {
                    checkbox("##chk_gcd", &custom_gcd);
                    FUCK::SameLine();
                    if (custom_gcd) {
                        dat.gcd = std::clamp(dat.gcd, 0.0f, 30.0f);
                        if (FUCK::DragFloat("", &dat.gcd, 0.5f, 0.0f, 30.0f, "%.2fs")) {
                            dat.gcd = std::clamp(dat.gcd, 0.0f, 30.0f);
                        }
                    }
                }
                else {
                    dat.gcd = -1.0f;
                    gray_textf("%.2fs", a_filled.gcd);
                }
            }
            FUCK::PopID();

            row_label(id, translate_c("$COOLDOWN"));
            {
                bool no_cd = true;
                static float input_cd_value{ 0.0f }; //in seconds
                constexpr float seconds_to_days = 1.0f / (60.0f * 60.0f * 24.0f);
                if (!is_greater_power && !is_shout) {
                    checkbox("##chk_cd", &custom_cd);
                    FUCK::SameLine();
                    if (custom_cd) {
                        no_cd = false;
                        input_cd_value = std::clamp(input_cd_value, 0.0f, 4320.0f);
                        if (FUCK::DragFloat("", &input_cd_value, 1.0f, 0.0f, 4320.0f, "%.1fs")) {
                            input_cd_value = std::clamp(input_cd_value, 0.0f, 4320.0f);
                            dat.cooldown = input_cd_value * seconds_to_days;
                        }
                    }
                }
                if (no_cd) {
                    dat.cooldown = -1.0f;
                    gray_textf("%.1fs", a_filled.cooldown * (1.0f / seconds_to_days));
                }
            }
            FUCK::PopID();

            row_label(id, translate_c("$CAST_TIME"));
            {
                bool no_ct = true;
                if (spell && spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                    checkbox("##chk_ct", &custom_casttime);
                    FUCK::SameLine();
                    if (custom_casttime) {
                        no_ct = false;
                        dat.casttime = std::clamp(dat.casttime, 0.25f, 10.0f);
                        if (FUCK::DragFloat("", &dat.casttime, 0.05f, 0.25f, 10.0f, "%.2fs")) {
                            dat.casttime = std::clamp(dat.casttime, 0.25f, 10.0f);
                        }
                    }
                }
                if (no_ct) {
                    dat.casttime = -1.0f;
                    gray_textf("%.2fs", a_filled.casttime);
                }
            }
            FUCK::PopID();

            row_label(id, translate_c("$ANIMATION"));
            if (spell && spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                anim_row(dat.animation, custom_anim, a_filled.animation, "_anim");
            }
            else {
                gray_textf("%d", -1);
            }
            FUCK::PopID();

            row_label(id, translate_c("$ANIMATION2"));
            if (spell && spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                anim_row(dat.animation2, custom_anim2, a_filled.animation2, "_anim2");
            }
            else {
                gray_textf("%d", -1);
            }
            FUCK::PopID();

            FUCK::EndTable();
        }
        FUCK::EndChild();

        FUCK::SameLine();
        FUCK::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        FUCK::BeginChild("RighttTab", ImVec2(0, child_window_height), false, ImGuiWindowFlags_HorizontalScrollbar);

        icon_picker("spell_icon_picker", a_data.m_icon_form, a_data.m_icon_str);

        FUCK::EndChild();
        FUCK::PopStyleVar(1);

        //Check if there is something to save
        const bool dat_diff = a_data.has_different_data(a_saved);
        const bool icon_change = a_data.has_icon_data();
        const bool save_enabled = dat_diff || icon_change;

        push_large_font();

        if (!save_enabled) FUCK::BeginDisabled();
        if (FUCK::Button(translate_id("$SAVE").c_str())) {
            //save changes

            if (a_data.has_different_data(a_unfilled) || a_data.has_icon_data()) {
                GameData::user_spell_cast_info.insert_or_assign(a_data.m_form_id, a_data);
            }
            else {
                if (GameData::user_spell_cast_info.contains(a_data.m_form_id)) {
                    GameData::user_spell_cast_info.erase(a_data.m_form_id);
                }
            }
            close();
        }
        if (!save_enabled) FUCK::EndDisabled();

        FUCK::SameLine();
        if (FUCK::Button(translate_id("$CANCEL").c_str())) {
            close();
        }
        pop_font();
    }
}
