#include "bind_menu.h"

#include "widgets.h"
#include "../../bar/hotbars.h"
#include "../../game_data/game_data.h"
#include "../../game_data/localization.h"
#include "../../input/keybinds.h"
#include "../../rendering/bind_drop.h"
#include "../flick_images.h"

#include <cfloat>

// The Spellbind window, ported from rendering/advanced_bind_menu.cpp to FLICK. The data half --
// what is listed, how it filters and sorts, what a drop does to a slot -- is the same code. The
// drawing half speaks FUCK:: and nothing else (flick_pch.h says why).
//
// What did not come across, on purpose: the nine-slice parchment background, the transparent
// title style, and the drawn cursor. The host draws the chrome and owns the cursor.
namespace SpellHotbar::FlickUi::BindMenu {
    using SpellHotbar::BindMenu::apply_bind_drop;
    using SpellHotbar::BindMenu::BindPayload;
    using SpellHotbar::BindMenu::empty_bind;
    using SpellHotbar::BindMenu::form_bind;
    using SpellHotbar::BindMenu::SlotBind;

    namespace {
        std::atomic_bool show_frame{ false };
        std::string window_title;

        // FLICK's TableFlags mirror ImGui's bit values but stop short of these three; the C-ABI
        // takes the int, so the real ImGui bits go through as-is.
        constexpr FUCK::TableFlags table_flag_sort_multi = static_cast<FUCK::TableFlags>(ImGuiTableFlags_SortMulti);
        constexpr FUCK::TableFlags table_flag_no_borders_in_body = static_cast<FUCK::TableFlags>(ImGuiTableFlags_NoBordersInBody);
        constexpr FUCK::TableFlags table_flag_scroll_y = static_cast<FUCK::TableFlags>(ImGuiTableFlags_ScrollY);

        constexpr int filter_buf_size = 256;
        char filter_buf[filter_buf_size] = "";
        int tab_index{ 0 };

        std::vector<RE::TESForm*> list_of_skills;
        std::vector<RE::TESForm*> list_of_skills_filtered;

        std::array<std::string, 6> rank_texts = { "$DASH", "$RANK_NOVICE", "$RANK_APPRENTICE", "$RANK_ADEPT", "$RANK_EXPERT", "$RANK_MASTER" };
        std::array<std::string, 4> rank_texts_shout = { "$WORDS_0", "$WORDS_1", "$WORDS_2", "$WORDS_3" };
        std::array<std::string, 6> school_texts = { "$DASH", "$TAB_TEXT_ALTERATION", "$TAB_TEXT_CONJURATION", "$TAB_TEXT_DESTRUCTION", "$TAB_TEXT_ILLUSION", "$TAB_TEXT_RESTORATION" };

        std::array<std::string, 9> type_texts = { "$TYPE_POTION", "$TYPE_SPELL", "$TYPE_LESSER_POWER", "$TYPE_GREATER_POWER", "$TYPE_SHOUT", "$TYPE_SCROLL", "$TYPE_POISON", "$TYPE_FOOD", "$DASH" };

        std::array<std::string, 13> tab_texts = {
            "$TAB_TEXT_ALL",
            "$TAB_TEXT_SPELLS",
            "$TAB_TEXT_ALTERATION",
            "$TAB_TEXT_ILLUSION",
            "$TAB_TEXT_DESTRUCTION",
            "$TAB_TEXT_CONJURATION",
            "$TAB_TEXT_RESTORATION",
            "$TAB_TEXT_SHOUTS",
            "$TAB_TEXT_POWERS",
            "$TAB_TEXT_SCROLLS",
            "$TAB_TEXT_POTIONS",
            "$TAB_TEXT_POISONS",
            "$TAB_TEXT_FOOD",
        };

        enum tab_index_id : uint8_t {
            TabIndex_All = 0Ui8,
            TabIndex_Spells,
            TabIndex_Alteration,
            TabIndex_Illusion,
            TabIndex_Destruction,
            TabIndex_Conjuration,
            TabIndex_Restoration,
            TabIndex_Shouts,
            TabIndex_Powers,
            TabIndex_Scrolls,
            TabIndex_Potions,
            TabIndex_Poisons,
            TabIndex_Food
        };

        struct Dragged_skill {
            const RE::TESForm* form;
            SlottedSkill* source_slot;

            Dragged_skill() : form(nullptr), source_slot(nullptr) {};

            void set_dragged(const RE::TESForm* tesform, SlottedSkill* source = nullptr)
            {
                form = tesform;
                source_slot = source;
            }

            inline bool has_dragged_from() const
            {
                return form != nullptr;
            }

            inline RE::FormID get_form_id() const
            {
                return form != nullptr ? form->GetFormID() : 0;
            }

            inline BindPayload to_payload() const
            {
                if (form != nullptr) {
                    return form_bind(form->GetFormID());
                }
                return empty_bind();
            }
        };

        Dragged_skill current_dragged_skill;
        // FLICK's C-ABI has no GetDragDropPayload, so the source remembers that a drag is in
        // flight: set inside BeginDragDropSource, cleared once the mouse is up. Read by the slot
        // strip to keep the slot being dragged highlighted while its payload is elsewhere.
        bool drag_in_flight{ false };

        enum bind_menu_column_id : ImGuiID {
            column_id_Icon = 0U,
            column_id_Name,
            column_id_Type,
            column_id_School,
            column_id_Rank,
            column_id_Magnitude,
            column_id_Time,

            column_count
        };

        void update_filter(const std::string filter_text, uint8_t tab_ind);

        void load_spells()
        {
            list_of_skills.clear();
            list_of_skills_filtered.clear();

            RE::PlayerCharacter* pc = RE::PlayerCharacter::GetSingleton();
            if (pc && pc->GetActorBase() != nullptr) {
                GameData::get_player_known_spells(pc, list_of_skills, false);
                GameData::add_player_owned_bindable_items(pc, list_of_skills);
            }
            filter_buf[0] = '\0';
            update_filter("", 0Ui8);
        }

        const char* get_rank_text(int rank, bool is_shout = false)
        {
            if (is_shout) {
                if (rank >= 0 && rank < 4) {
                    return translate_c(rank_texts_shout.at(rank));
                }
                return translate_c(rank_texts_shout.at(0));
            }
            if (rank >= 0 && rank < 6) {
                return translate_c(rank_texts.at(rank));
            }
            return translate_c(rank_texts.at(0));
        }

        size_t get_school_order(RE::ActorValue av)
        {
            switch (av) {
            case RE::ActorValue::kAlteration:
                return 1;
            case RE::ActorValue::kIllusion:
                return 4;
            case RE::ActorValue::kDestruction:
                return 3;
            case RE::ActorValue::kConjuration:
                return 2;
            case RE::ActorValue::kRestoration:
                return 5;
            default:
                return 0;
            }
        }

        const char* get_school_text(RE::ActorValue av)
        {
            return translate_c(school_texts.at(get_school_order(av)));
        }

        const char* get_type_text(const RE::TESForm* item)
        {
            if (item != nullptr) {
                if (item->GetFormType() == RE::FormType::Shout) {
                    return type_texts[4].c_str();
                }
                else if (item->GetFormType() == RE::FormType::AlchemyItem) {
                    const RE::AlchemyItem* alch = item->As<RE::AlchemyItem>();
                    if (alch != nullptr) {
                        if (alch->IsPoison()) {
                            return type_texts[6].c_str();
                        }
                        else if (alch->IsFood()) {
                            return type_texts[7].c_str();
                        }
                    }
                    return type_texts[0].c_str();
                }
                else if (item->GetFormType() == RE::FormType::Scroll) {
                    return type_texts[5].c_str();
                }
                else if (item->GetFormType() == RE::FormType::Spell) {
                    const RE::SpellItem* spell = item->As<RE::SpellItem>();
                    if (spell != nullptr) {
                        if (spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                            return type_texts[1].c_str();
                        }
                        else if (spell->GetSpellType() == RE::MagicSystem::SpellType::kPower) {
                            return type_texts[3].c_str();
                        }
                        else if (spell->GetSpellType() == RE::MagicSystem::SpellType::kLesserPower) {
                            return type_texts[2].c_str();
                        }
                    }
                }
            }
            return type_texts[8].c_str();
        }

        std::tuple<int, RE::ActorValue, int, float, int> get_rank_school_time_mag_count(const RE::TESForm* item)
        {
            int rank{ 0 };
            RE::ActorValue school{ RE::ActorValue::kNone };
            int time{ 0 };
            float mag{ 0.0f };
            int count{ -1 };

            if (item->GetFormType() == RE::FormType::Spell || item->GetFormType() == RE::FormType::Scroll) {
                const RE::SpellItem* spell = item->As<RE::SpellItem>();

                if (spell != nullptr && spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                    if (spell->effects.size() > 0U) {
                        for (RE::BSTArrayBase::size_type i = 0U; i < spell->effects.size() && school == RE::ActorValue::kNone; i++) {
                            //find first spell effect that has a magic school
                            const RE::Effect* effect = spell->effects[i];
                            if (effect->baseEffect != nullptr) {
                                auto av = effect->baseEffect->GetMagickSkill();
                                if (school == RE::ActorValue::kNone && av != RE::ActorValue::kNone) {
                                    school = av;
                                    rank = GameData::get_spell_rank(effect->baseEffect->GetMinimumSkillLevel()) + 1;
                                }
                            }
                        }
                        auto main_effect = spell->GetCostliestEffectItem();
                        if (main_effect != nullptr) {
                            mag = main_effect->GetMagnitude();
                            time = main_effect->GetDuration();
                        }
                    }
                }
                if (item->GetFormType() == RE::FormType::Scroll) {
                    count = GameData::count_item_in_inv(item->GetFormID());
                }
            }
            else if (item->GetFormType() == RE::FormType::AlchemyItem) {
                const RE::AlchemyItem* alch = item->As<RE::AlchemyItem>();

                if (alch != nullptr) {
                    time = alch->GetLongestDuration();

                    auto main_effect = alch->GetCostliestEffectItem();
                    if (main_effect != nullptr) {
                        mag = main_effect->GetMagnitude();
                    }
                }
                count = GameData::count_item_in_inv(item->GetFormID());
            }
            else if (item->GetFormType() == RE::FormType::Shout) {
                const RE::TESShout* shout = item->As<RE::TESShout>();

                // this flag seems to get set when a word gets unlocked
                constexpr uint32_t flag_unlocked = 1 << 16;

                if (shout != nullptr) {
                    for (int i = 2; i >= 0 && rank == 0; i--) {
                        auto word = shout->variations[i].word;
                        if (word != nullptr && word->GetKnown() && (word->GetFormFlags() & flag_unlocked)) {
                            rank = i + 1;
                        }
                    }
                }
            }
            return std::make_tuple(rank, school, time, mag, count);
        }

        void update_filter(const std::string filter_text, uint8_t tab_ind)
        {
            if (filter_text.empty() && tab_ind == 0Ui8) {
                list_of_skills_filtered = list_of_skills;
            }
            else {
                list_of_skills_filtered.clear();
                list_of_skills_filtered.reserve(list_of_skills.size());
                for (size_t i = 0U; i < list_of_skills.size(); i++) {
                    bool match_text{ false };
                    bool match_tab{ tab_ind == 0Ui8 };

                    if (!filter_text.empty()) {
                        //Check if name matches filter
                        const std::string name = list_of_skills[i]->GetName();
                        if (name.find(filter_text) != std::string::npos) {
                            match_text = true;
                        }
                    }
                    else {
                        match_text = true;
                    }

                    //check_tab
                    if (!match_tab) {
                        auto form_type = list_of_skills[i]->GetFormType();
                        if (form_type == RE::FormType::Shout) {
                            match_tab = tab_ind == TabIndex_Shouts;
                        }
                        else if (form_type == RE::FormType::Scroll) {
                            match_tab = tab_ind == TabIndex_Scrolls;
                        }
                        else if (form_type == RE::FormType::Spell) {
                            if (tab_ind == TabIndex_Spells) {
                                RE::SpellItem* spell = list_of_skills[i]->As<RE::SpellItem>();
                                if (spell != nullptr && spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                                    match_tab = true;
                                }
                            }
                            else {
                                RE::SpellItem* spell = list_of_skills[i]->As<RE::SpellItem>();
                                if (spell != nullptr) {
                                    if (spell->GetSpellType() == RE::MagicSystem::SpellType::kPower || spell->GetSpellType() == RE::MagicSystem::SpellType::kLesserPower) {
                                        match_tab = tab_ind == TabIndex_Powers;
                                    }
                                    else {
                                        auto [rank, school, _t, _m, _c] = get_rank_school_time_mag_count(list_of_skills[i]);
                                        switch (tab_ind) {
                                        case TabIndex_Alteration:
                                            match_tab = school == RE::ActorValue::kAlteration;
                                            break;
                                        case TabIndex_Conjuration:
                                            match_tab = school == RE::ActorValue::kConjuration;
                                            break;
                                        case TabIndex_Destruction:
                                            match_tab = school == RE::ActorValue::kDestruction;
                                            break;
                                        case TabIndex_Illusion:
                                            match_tab = school == RE::ActorValue::kIllusion;
                                            break;
                                        case TabIndex_Restoration:
                                            match_tab = school == RE::ActorValue::kRestoration;
                                            break;
                                        default:
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        else if (form_type == RE::FormType::AlchemyItem) {
                            RE::AlchemyItem* alch = list_of_skills[i]->As<RE::AlchemyItem>();
                            if (alch != nullptr) {
                                if ((alch->IsFood() && tab_ind == TabIndex_Food) || (alch->IsPoison() && tab_ind == TabIndex_Poisons)) {
                                    match_tab = true;
                                }
                                else if (!alch->IsFood() && !alch->IsPoison() && (tab_ind == TabIndex_Potions)) {
                                    match_tab = true;
                                }
                            }
                        }
                    }

                    if (match_text && match_tab) list_of_skills_filtered.emplace_back(list_of_skills[i]);
                }
            }
        }

        const ImGuiTableSortSpecs* s_current_sort_specs{ nullptr };
        bool compare_entries_for_sort(const RE::TESForm* lhs, const RE::TESForm* rhs)
        {
            for (int n = 0; n < s_current_sort_specs->SpecsCount; n++) {
                // Columns are identified by the ColumnUserID passed to TableSetupColumn().
                const ImGuiTableColumnSortSpecs* sort_spec = &s_current_sort_specs->Specs[n];
                bool is_less{ false };

                if (sort_spec->ColumnUserID == bind_menu_column_id::column_id_Name) {
                    is_less = std::string(lhs->GetName()) < std::string(rhs->GetName());
                }
                else if (sort_spec->ColumnUserID == bind_menu_column_id::column_id_Rank) {
                    auto [lrank, _ls, _lt, _lm, _lc] = get_rank_school_time_mag_count(lhs);
                    auto [rrank, _rs, _rt, _rm, _rc] = get_rank_school_time_mag_count(rhs);
                    //sort shouts after spells
                    if (lhs->GetFormType() == RE::FormType::Shout) {
                        lrank += 6;
                    }
                    if (rhs->GetFormType() == RE::FormType::Shout) {
                        rrank += 6;
                    }
                    is_less = lrank < rrank;
                }
                else if (sort_spec->ColumnUserID == bind_menu_column_id::column_id_School) {
                    auto [_lr, lschool, _lt, _lm, _lc] = get_rank_school_time_mag_count(lhs);
                    auto [_rr, rschool, _rt, _rm, _rc] = get_rank_school_time_mag_count(rhs);
                    is_less = get_school_order(lschool) < get_school_order(rschool);
                }
                else if (sort_spec->ColumnUserID == bind_menu_column_id::column_id_Magnitude) {
                    auto [_lr, _ls, _lt, mag_l, _lc] = get_rank_school_time_mag_count(lhs);
                    auto [_rr, _rs, _rt, mag_r, _rc] = get_rank_school_time_mag_count(rhs);
                    is_less = mag_l < mag_r;
                }
                else if (sort_spec->ColumnUserID == bind_menu_column_id::column_id_Time) {
                    auto [_lr, _ls, time_l, _lm, _lc] = get_rank_school_time_mag_count(lhs);
                    auto [_rr, _rs, time_r, _rm, _rc] = get_rank_school_time_mag_count(rhs);
                    is_less = time_l < time_r;
                }
                else if (sort_spec->ColumnUserID == bind_menu_column_id::column_id_Type) {
                    is_less = translate(get_type_text(lhs)) < translate(get_type_text(rhs));
                }
                else {
                    is_less = lhs->formID < rhs->formID;
                }
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? is_less : !is_less;
            }
            return lhs->formID < rhs->formID;
        }

        // ---- Drag sources. The preview under the cursor is the icon and the name. ----

        void set_drag_source(const RE::TESForm* item, SlottedSkill* source = nullptr)
        {
            if (item == nullptr) {
                return;
            }
            if (FUCK::BeginDragDropSource(FUCK::DragDropFlags::kSourceAllowNullID)) {
                current_dragged_skill.set_dragged(item, source);
                drag_in_flight = true;
                FUCK::SetDragDropPayload("SPELL_SLOT", &current_dragged_skill, sizeof(Dragged_skill));
                auto skill_dat = GameData::get_spell_data(item, true, true);
                const float icon_size = scale(60.0f);
                const ImVec2 p = FUCK::GetCursorScreenPos();
                draw_skill_item(current_dragged_skill.get_form_id(), icon_size, UiBridge::skill_color(item));
                if (UiBridge::should_overlay_be_rendered(skill_dat.overlay_icon)) {
                    draw_icon_overlay(p, icon_size, skill_dat.overlay_icon);
                }
                FUCK::SameLine();
                FUCK::TextUnformatted(item->GetName());
                FUCK::EndDragDropSource();
            }
        }

        void apply_payload_to_skill(SlottedSkill& skill, BindPayload payload)
        {
            const SlotBind next = apply_bind_drop(SlotBind{ .form_id = skill.formID }, payload);
            if (next.form_id != 0) {
                skill.update_skill_assignment(next.form_id);
            }
            else {
                skill.clear();
            }
        }

        BindPayload payload_from_skill(const SlottedSkill& skill)
        {
            if (skill.formID != 0) {
                return form_bind(skill.formID);
            }
            return empty_bind();
        }

        /*
        * Check if the current tab index is active and if not use the first valid one
        */
        void check_tab_index_valid(int& index)
        {
            bool valid = (index == 0 && !Bars::disable_non_modifier_bar) ||
                         (index == 1 && Input::mod_1.isValidBound()) ||
                         (index == 2 && Input::mod_2.isValidBound()) ||
                         (index == 3 && Input::mod_3.isValidBound());
            if (!valid) {
                if (!Bars::disable_non_modifier_bar) {
                    index = 0;
                }
                else if (Input::mod_1.isValidBound()) {
                    index = 1;
                }
                else if (Input::mod_2.isValidBound()) {
                    index = 2;
                }
                else if (Input::mod_3.isValidBound()) {
                    index = 3;
                }
                else {
                    index = 0;
                }
            }
        }

        // A modifier's key glyph beside its radio button.
        void draw_inline_keybind_icon(float icon_size, key_modifier mod)
        {
            auto [_kb, tex_id_mod] = GameData::get_keybind_icon_index(0, mod);
            const float key_icon_size = icon_size * 2.0f / 3.0f;
            if (tex_id_mod >= 0) {
                const float key_icon_length = key_icons_length(tex_id_mod, -1);
                const ImVec2 p = FUCK::GetCursorScreenPos();
                FUCK::Dummy(ImVec2(key_icon_size * key_icon_length, icon_size));
                draw_key_icons_at(p, tex_id_mod, -1, key_icon_size);
            }
        }

        // One tab of the strip. `changed` is set when the selection moved, which dirties the
        // filter.
        void tab(const char* id, int index, float size, bool& changed, const char* tooltip,
                 const std::function<void(ImVec2, float)>& draw_icon)
        {
            if (tab_button(id, tab_index == index, size, tooltip, draw_icon)) {
                if (tab_index != index) {
                    changed = true;
                }
                tab_index = index;
                RE::PlaySound(Input::sound_UIFavorite);
            }
        }

        void tab_default(const char* id, int index, GameData::DefaultIconType icon, float size, bool& changed)
        {
            tab(id, index, size, changed, translate_c(tab_texts[static_cast<std::size_t>(index)]),
                [icon](ImVec2 p, float s) { draw_icon_overlay(p, s, icon); });
            FUCK::SameLine();
        }

        // ---- The left pane: tabs, filter, the table. ----

        void draw_list_pane(float pane_height, int& table_icon_size)
        {
            FUCK::BeginChild("BindMenuTabLeft", ImVec2(FUCK::GetContentRegionAvail().x * 0.5f, pane_height), false,
                             ImGuiWindowFlags_HorizontalScrollbar);

            bool filter_dirty = false;

            push_large_font();
            FUCK::TextUnformatted(translate_c(tab_texts[static_cast<std::size_t>(tab_index)]));
            pop_font();

            FUCK::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));
            const float natural_tab_icon_size = scale(60.0f);
            const float tab_count = static_cast<float>(tab_texts.size());
            const float tab_spacing = FUCK::GetStyleVarVec(ImGuiStyleVar_ItemSpacing).x;
            const float max_fitting_tab_icon_size = (FUCK::GetContentRegionAvail().x - tab_spacing * (tab_count - 1.0f)) / tab_count;
            const float tab_icon_size = std::floor(std::min(natural_tab_icon_size, std::max(1.0f, max_fitting_tab_icon_size)));
            tab_default("##TabAll", TabIndex_All, GameData::DefaultIconType::TAB_ALL, tab_icon_size, filter_dirty);
            tab_default("##TabSpells", TabIndex_Spells, GameData::DefaultIconType::TAB_SPELLS, tab_icon_size, filter_dirty);
            tab_default("##TabAlteration", TabIndex_Alteration, GameData::DefaultIconType::ALTERATION_ADEPT, tab_icon_size, filter_dirty);
            tab_default("##TabIllusion", TabIndex_Illusion, GameData::DefaultIconType::ILLUSION_FRIENDLY_ADEPT, tab_icon_size, filter_dirty);
            tab_default("##TabDestruction", TabIndex_Destruction, GameData::DefaultIconType::DESTRUCTION_FIRE_EXPERT, tab_icon_size, filter_dirty);
            tab_default("##TabConjuration", TabIndex_Conjuration, GameData::DefaultIconType::CONJURATION_SUMMON_ADEPT, tab_icon_size, filter_dirty);
            tab_default("##TabRestoration", TabIndex_Restoration, GameData::DefaultIconType::RESTORATION_FRIENDLY_EXPERT, tab_icon_size, filter_dirty);
            tab_default("##TabShouts", TabIndex_Shouts, GameData::DefaultIconType::SHOUT_GENERIC, tab_icon_size, filter_dirty);
            tab_default("##TabPowers", TabIndex_Powers, GameData::DefaultIconType::GREATER_POWER, tab_icon_size, filter_dirty);
            tab_default("##TabScrolls", TabIndex_Scrolls, GameData::DefaultIconType::TAB_SCROLLS, tab_icon_size, filter_dirty);
            tab_default("##TabPotions", TabIndex_Potions, GameData::DefaultIconType::TAB_POTIONS, tab_icon_size, filter_dirty);
            tab_default("##TabPoisons", TabIndex_Poisons, GameData::DefaultIconType::TAB_POISONS, tab_icon_size, filter_dirty);
            tab_default("##TabFood", TabIndex_Food, GameData::DefaultIconType::TAB_FOOD, tab_icon_size, filter_dirty);
            FUCK::PopStyleVar(1);

            // Filter row: clear button, the text field, the icon size slider.
            const float bsize = std::floor(scale(30.0f));
            const float row_h = std::max(bsize, FUCK::GetFrameHeight());
            // One row tall and never scrolling: the content ran a few pixels past the child
            // and each half grew a scrollbar. The flags forbid it and the spacing gives the row
            // its headroom.
            const float row_child_h = row_h + FUCK::GetStyleVarVec(ImGuiStyleVar_ItemSpacing).y;
            constexpr int row_child_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
            FUCK::BeginChild("subtableft_left", ImVec2(FUCK::GetContentRegionAvail().x * 0.6f, row_child_h), false, row_child_flags);
            const ImVec2 p = FUCK::GetCursorScreenPos();

            bool button_clear_filter_clicked{ false };
            if (FUCK::InvisibleButton("X", ImVec2(bsize, bsize))) {
                button_clear_filter_clicked = true;
                RE::PlaySound(Input::sound_UIFavorite);
            }
            const bool clear_button_hovered = FUCK::IsItemHovered();
            draw_default_icon_at(GameData::DefaultIconType::UNBIND_SLOT, p, bsize);
            if (clear_button_hovered) {
                draw_highlight_overlay(p, bsize, rgba(255, 127, 127));
            }
            FUCK::SameLine();

            static std::string last_filter = "";

            if (button_clear_filter_clicked) {
                filter_buf[0] = '\0';
            }

            if (last_filter != filter_buf) {
                filter_dirty = true;
            }
            last_filter = filter_buf;

            FUCK::PushItemWidth(-FLT_MIN);
            FUCK::InputText("##Filter", filter_buf, filter_buf_size, ImGuiInputTextFlags_EscapeClearsAll);
            FUCK::PopItemWidth();

            FUCK::EndChild();
            FUCK::SameLine();
            FUCK::BeginChild("subtableft_right", ImVec2(FUCK::GetContentRegionAvail().x, row_child_h), false, row_child_flags);

            const int table_icon_size_min_value = static_cast<int>(std::round(scale(24.0f)));
            const int table_icon_size_max_value = static_cast<int>(std::round(scale(80.0f)));

            FUCK::PushItemWidth(FUCK::GetContentRegionAvail().x * 0.75f);
            FUCK::SliderInt(translate_id("$ICON_SIZE").c_str(), &table_icon_size, table_icon_size_min_value, table_icon_size_max_value, "%d");
            FUCK::PopItemWidth();
            FUCK::EndChild();

            using TF = FUCK::TableFlags;
            const TF flags = TF::kResizable | TF::kReorderable | TF::kHideable | TF::kSortable | table_flag_sort_multi |
                             TF::kRowBg | TF::kBordersOuterH | TF::kBordersOuterV | TF::kBordersInnerV |
                             table_flag_no_borders_in_body | table_flag_scroll_y;

            if (FUCK::BeginTable("Known Spells", bind_menu_column_id::column_count, flags, ImVec2(0.0f, 0.0f), 0.0f)) {
                using CF = FUCK::TableColumnFlags;
                FUCK::TableSetupColumn(translate_id("$COLUMN_DRAG").c_str(), CF::kWidthFixed | CF::kNoSort, 65.0f, bind_menu_column_id::column_id_Icon);
                FUCK::TableSetupColumn(translate_id("$COLUMN_NAME").c_str(), CF::kWidthStretch | CF::kDefaultSort | CF::kPreferSortAscending, 0.0f, bind_menu_column_id::column_id_Name);
                FUCK::TableSetupColumn(translate_id("$COLUMN_TYPE").c_str(), CF::kWidthFixed | CF::kPreferSortAscending, 0.0f, bind_menu_column_id::column_id_Type);
                FUCK::TableSetupColumn(translate_id("$COLUMN_SCHOOL").c_str(), CF::kWidthFixed | CF::kPreferSortAscending, 0.0f, bind_menu_column_id::column_id_School);
                FUCK::TableSetupColumn(translate_id("$COLUMN_RANK").c_str(), CF::kWidthFixed | CF::kPreferSortAscending, 0.0f, bind_menu_column_id::column_id_Rank);
                FUCK::TableSetupColumn(translate_id("$COLUMN_MAG").c_str(), CF::kWidthFixed | CF::kPreferSortAscending, 0.0f, bind_menu_column_id::column_id_Magnitude);
                FUCK::TableSetupColumn(translate_id("$COLUMN_TIME").c_str(), CF::kWidthFixed | CF::kPreferSortAscending, 0.0f, bind_menu_column_id::column_id_Time);
                FUCK::TableSetupScrollFreeze(0, 1); // Make row always visible
                FUCK::TableHeadersRow();

                // Sort our data if sort specs have been changed!
                if (ImGuiTableSortSpecs* sort_specs = FUCK::GetTableSortSpecs()) {
                    if (sort_specs->SpecsDirty) {
                        s_current_sort_specs = sort_specs;
                        if (!list_of_skills.empty()) {
                            std::sort(list_of_skills.begin(), list_of_skills.end(), compare_entries_for_sort);
                        }
                        sort_specs->SpecsDirty = false;

                        update_filter(filter_buf, static_cast<uint8_t>(tab_index));
                        filter_dirty = false;
                    }
                }

                //apply filtering
                if (filter_dirty) {
                    update_filter(filter_buf, static_cast<uint8_t>(tab_index));
                    filter_dirty = false;
                }

                const float icon_f = static_cast<float>(table_icon_size);
                const float cell_pad_y = FUCK::GetStyleVarVec(ImGuiStyleVar_CellPadding).y;
                const float row_height = std::max(icon_f, FUCK::GetTextLineHeight()) + cell_pad_y * 2.0f;

                {
                    const auto clip = RowClipper::begin(static_cast<int>(list_of_skills_filtered.size()), row_height);
                    for (int row_n = clip.first; row_n < clip.last; row_n++) {
                        const RE::TESForm* item = list_of_skills_filtered[static_cast<size_t>(row_n)];
                        const bool is_shout = item->GetFormType() == RE::FormType::Shout;
                        auto [rank, school, time, mag, count] = get_rank_school_time_mag_count(item);

                        FUCK::PushID(static_cast<int>(item->GetFormID()));
                        FUCK::TableNextRow(0, clip.row_h);

                        FUCK::TableNextColumn();
                        const ImVec2 icon_pos = FUCK::GetCursorScreenPos();
                        draw_skill_item(item->GetFormID(), icon_f, UiBridge::skill_color(item));
                        skill_tooltip(item);
                        if (!is_shout || rank > 0) {
                            set_drag_source(item);
                        }
                        auto skill_dat = GameData::get_spell_data(item, true, true);
                        if (UiBridge::should_overlay_be_rendered(skill_dat.overlay_icon)) {
                            draw_icon_overlay(icon_pos, icon_f, skill_dat.overlay_icon);
                        }

                        //If count available, display
                        if (count > -1) {
                            const std::string text = std::to_string(std::clamp(count, -9999, 9999));
                            const ImVec2 textsize = FUCK::CalcTextSize(text.c_str());
                            const float text_offset_y = icon_f * 0.0125f;
                            text_at(ImVec2(icon_pos.x + icon_f - textsize.x, icon_pos.y + icon_f - textsize.y - text_offset_y),
                                    col_white, text.c_str());
                        }

                        FUCK::TableNextColumn();
                        if (is_shout && rank == 0) {
                            FUCK::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", item->GetName());
                        }
                        else {
                            FUCK::TextUnformatted(item->GetName());
                        }

                        FUCK::TableNextColumn();
                        FUCK::TextUnformatted(translate_c(get_type_text(item)));

                        FUCK::TableNextColumn();
                        FUCK::TextUnformatted(get_school_text(school));

                        FUCK::TableNextColumn();
                        FUCK::TextUnformatted(get_rank_text(rank, is_shout));

                        FUCK::TableNextColumn();
                        if (mag != 0.0f) {
                            FUCK::Text("%.0f", mag);
                        }
                        else {
                            FUCK::TextUnformatted("-");
                        }

                        FUCK::TableNextColumn();
                        if (time > 0) {
                            FUCK::Text("%d", time);
                        }
                        else {
                            FUCK::TextUnformatted("-");
                        }

                        FUCK::PopID();
                    }
                    clip.end();
                }
                FUCK::EndTable();
            }
            FUCK::EndChild();
        }

        // ---- The right pane: the bar picker, the modifier, the slot strip. ----

        void draw_bar_pane(float pane_height)
        {
            FUCK::BeginChild("BindMenuTabRight", ImVec2(0.0f, pane_height), false, ImGuiWindowFlags_HorizontalScrollbar);

            const bool is_fav_menu_binding = GameData::hasFavMenuSlotBinding();
            if (is_fav_menu_binding) {
                Bars::menu_bar_id = Bars::getCurrentHotbar_ingame();
            }
            if (!Bars::hotbars.contains(Bars::menu_bar_id)) {
                Bars::menu_bar_id = Bars::MAIN_BAR;
            }
            auto& bar = Bars::hotbars.at(Bars::menu_bar_id);

            FUCK::PushItemWidth(std::max(FUCK::GetContentRegionAvail().x * 0.5f, scale(200.0f)));
            if (is_fav_menu_binding) {
                // Handle Vampire Lord and Werewolf bars: the transformed bar is the only choice.
                int index = 0;
                const std::vector<std::string> names{ bar.get_name() };
                combo("##Hotbar", &index, names);
            }
            else {
                const auto list_of_bars = Bars::get_list_of_bars();
                std::vector<std::string> names;
                names.reserve(list_of_bars.size());
                int index = 0;
                for (std::size_t n = 0; n < list_of_bars.size(); ++n) {
                    names.push_back(list_of_bars[n].second);
                    if (list_of_bars[n].first == Bars::menu_bar_id) {
                        index = static_cast<int>(n);
                    }
                }
                if (combo("##Hotbar", &index, names) && index >= 0 && index < static_cast<int>(list_of_bars.size())) {
                    Bars::menu_bar_id = list_of_bars[static_cast<std::size_t>(index)].first;
                }
            }
            FUCK::PopItemWidth();
            if (!Bars::hotbars.contains(Bars::menu_bar_id)) {
                FUCK::EndChild();
                return;
            }
            auto& bar_sel = Bars::hotbars.at(Bars::menu_bar_id);

            static int modifier_index = 0;
            check_tab_index_valid(modifier_index);

            const float icon_size = std::round(scale(60.0f));

            bool first{ true };
            if (!Bars::disable_non_modifier_bar) {
                const std::string none_text = GameData::get_modifier_text_long(key_modifier::none);
                radio_button(none_text.c_str(), &modifier_index, 0);
                first = false;
            }

            constexpr float key_icon_radio_button_scale = 0.75f;
            const auto modifier_radio = [&](const char* id, int value, key_modifier mod) {
                if (!first) {
                    FUCK::SameLine();
                }
                first = false;
                if (Bars::use_keybind_icons()) {
                    radio_button(id, &modifier_index, value);
                    FUCK::SameLine();
                    draw_inline_keybind_icon(icon_size * key_icon_radio_button_scale, mod);
                }
                else {
                    const std::string text = GameData::get_modifier_text_long(mod);
                    radio_button(text.c_str(), &modifier_index, value);
                }
            };
            if (Input::mod_1.isValidBound()) {
                modifier_radio("##radio_mod_ctrl", 1, key_modifier::ctrl);
            }
            if (Input::mod_2.isValidBound()) {
                modifier_radio("##radio_mod_shift", 2, key_modifier::shift);
            }
            if (Input::mod_3.isValidBound()) {
                modifier_radio("##radio_mod_alt", 3, key_modifier::alt);
            }
            FUCK::Spacing();

            //draw Hotbar slots
            const float text_offset_x = icon_size * 0.05f;
            const float text_offset_x_right = icon_size * 0.95f - FUCK::CalcTextSize("R").x;
            const float text_offset_y = icon_size * 0.0125f;
            const ImVec2 item_spacing = FUCK::GetStyleVarVec(ImGuiStyleVar_ItemSpacing);

            const key_modifier mod = static_cast<key_modifier>(modifier_index);

            float key_icon_length{ 0.0f };
            if (Bars::use_keybind_icons()) {
                //Check for longest button combo
                for (int i = 0; i < bar_sel.get_bar_size(); i++) {
                    auto [tex_id_key, _tex_id_mod] = GameData::get_keybind_icon_index(i, mod);
                    const float cur_len = key_icons_length(tex_id_key, -1);
                    if (cur_len > key_icon_length) {
                        key_icon_length = cur_len;
                    }
                }
            }
            const float key_icon_size = icon_size * 2.0f / 3.0f;

            for (int i = 0; i < bar_sel.get_bar_size(); i++) {
                auto [skill, inherited] = bar_sel.get_skill_in_bar_with_inheritance(i, mod, false);

                // The key label and the name are drawn in grey when the slot is inherited from
                // the parent bar, as the ImGui window did.
                const unsigned slot_text_col = inherited ? rgba(127, 127, 127) : col_white;

                GameData::Spell_cast_data skill_dat;
                auto form = RE::TESForm::LookupByID(skill.formID);
                if (form) {
                    skill_dat = GameData::get_spell_data(form, true, true);
                }

                size_t count{ 0 };
                if (skill.consumed != consumed_type::none) {
                    count = GameData::count_item_in_inv(skill.formID);
                }

                if (Bars::use_keybind_icons()) {
                    //Draw keybind buttons
                    ImVec2 icon_pos = FUCK::GetCursorScreenPos();
                    auto [tex_id_key, _tex_id_mod] = GameData::get_keybind_icon_index(i, mod);
                    icon_pos.y += icon_size * 1.0f / 6.0f;
                    FUCK::Dummy(ImVec2(std::max(key_icon_size * key_icon_length - item_spacing.x, 1.0f), icon_size));
                    FUCK::SameLine();
                    draw_key_icons_at(icon_pos, tex_id_key, -1, key_icon_size, slot_text_col);
                }

                const ImVec2 bpos = FUCK::GetCursorScreenPos();
                const std::string button_label = "##slot_button_" + std::to_string(i);
                if (FUCK::InvisibleButton(button_label.c_str(), ImVec2(icon_size, icon_size))) {
                    if (!inherited) {
                        if (form != nullptr && form->GetFormType() == RE::FormType::Spell) {
                            if (GameData::is_clear_spell(form->formID)) {
                                auto& skill_ref = bar_sel.get_skill_in_bar_by_ref(i, mod);
                                if (!skill_ref.isEmpty()) {
                                    //unblock
                                    skill_ref.clear();
                                }
                            }
                            else {
                                RE::SpellItem* spell = form->As<RE::SpellItem>();

                                if (spell != nullptr && spell->GetEquipSlot() == GameData::equip_slot_either_hand) {
                                    //skill for rendering is a copy, we need to get the ref here, no inheritence is needed
                                    auto& skill_ref = bar_sel.get_skill_in_bar_by_ref(i, mod);
                                    if (!skill_ref.isEmpty() && skill_ref.formID == form->GetFormID()) {
                                        Hotbar::rotate_skill_hand_assingment(spell, skill_ref);
                                        RE::PlaySound(Input::sound_UISkillsFocus);
                                    }
                                }
                            }
                        }
                    }
                    else {
                        //inherited skill, toggle between blocked and empty
                        auto& skill_ref = bar_sel.get_skill_in_bar_by_ref(i, mod);
                        if (!skill_ref.isEmpty() && GameData::is_clear_spell(skill_ref.formID)) {
                            //unblock
                            skill_ref.clear();
                        }
                        else {
                            //set to blocked
                            if (GameData::spellhotbar_unbind_slot != nullptr) {
                                skill_ref = GameData::spellhotbar_unbind_slot->formID;
                            }
                        }
                        RE::PlaySound(Input::sound_UISkillsFocus);
                    }
                }

                bool drawn_skill{ false };
                const bool button_hovered = FUCK::IsItemHovered();
                if (button_hovered && inherited) {
                    if (GameData::spellhotbar_unbind_slot != nullptr) {
                        draw_skill_at(GameData::spellhotbar_unbind_slot->GetFormID(), bpos, icon_size);
                    }
                    else {
                        draw_default_icon_at(GameData::DefaultIconType::BAR_EMPTY, bpos, icon_size);
                    }
                }
                else {
                    drawn_skill = draw_skill_at(skill.formID, bpos, icon_size, skill.color);
                    if (!drawn_skill) {
                        draw_default_icon_at(GameData::DefaultIconType::BAR_EMPTY, bpos, icon_size, skill.color);
                    }
                }

                if (!inherited && !GameData::is_clear_spell(skill.formID)) {
                    SlottedSkill* source_skill{ nullptr };
                    if (!skill.isEmpty()) {
                        source_skill = bar_sel.get_skill_in_bar_ptr(i, mod);
                    }
                    set_drag_source(form, source_skill);
                }
                if (FUCK::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = FUCK::AcceptDragDropPayload("SPELL_SLOT")) {
                        if (payload->DataSize == sizeof(Dragged_skill)) {
                            Dragged_skill payload_n = *static_cast<const Dragged_skill*>(payload->Data);
                            if (payload_n.has_dragged_from()) {
                                auto& target = bar_sel.get_skill_in_bar_by_ref(i, mod);
                                const hand_mode target_hand = target.hand;
                                std::optional<hand_mode> source_hand(std::nullopt);
                                BindPayload previous = empty_bind();
                                if (!target.isEmpty() && !GameData::is_clear_spell(target.formID)) {
                                    previous = payload_from_skill(target);
                                }
                                if (payload_n.source_slot != nullptr) {
                                    source_hand = payload_n.source_slot->hand;
                                    apply_payload_to_skill(*payload_n.source_slot, previous);
                                    if (previous.form_id != 0) {
                                        payload_n.source_slot->hand = target_hand;
                                    }
                                }

                                const BindPayload incoming = payload_n.to_payload();
                                apply_payload_to_skill(target, incoming);
                                if (incoming.form_id != 0 && source_hand.has_value()) {
                                    target.hand = source_hand.value();
                                }
                                RE::PlaySound(Input::sound_UISkillsFocus);
                            }
                        }
                    }
                    FUCK::EndDragDropTarget();
                }

                if (drawn_skill) {
                    if (UiBridge::should_overlay_be_rendered(skill_dat.overlay_icon)) {
                        draw_icon_overlay(bpos, icon_size, skill_dat.overlay_icon);
                    }
                    draw_slot_overlay(bpos, icon_size);
                }

                const bool is_being_dragged = drag_in_flight && current_dragged_skill.source_slot != nullptr &&
                                              current_dragged_skill.source_slot == bar_sel.get_skill_in_bar_ptr(i, mod);

                if (button_hovered || is_being_dragged) {
                    draw_highlight_overlay(bpos, icon_size, rgba(127, 127, 255));
                }
                if (button_hovered && form != nullptr) {
                    if (GameData::is_clear_spell(form->formID)) {
                        item_tooltip(translate("$SLOT_BLOCKED_TITLE"), translate("$SLOT_BLOCKED_INFO"));
                    }
                    else {
                        skill_tooltip(form);
                    }
                }
                FUCK::SameLine();

                // The name, then the texts painted over the icon. text_at() puts the layout
                // cursor back where it found it, so these go after the SameLine/name pair and
                // before the next slot's row starts.
                const std::string text = GameData::resolve_spellname(skill.formID);
                FUCK::TextColored(FlickImages::unpack(slot_text_col), "%s", text.c_str());

                if (!Bars::use_keybind_icons()) {
                    const std::string key_text = GameData::get_keybind_text(i, mod);
                    text_at(ImVec2(bpos.x + text_offset_x, bpos.y + text_offset_y), slot_text_col, key_text.c_str());
                }

                if (skill.hand == hand_mode::left_hand || skill.hand == hand_mode::right_hand || skill.hand == hand_mode::dual_hand) {
                    std::string hand_text;
                    if (skill.hand == hand_mode::left_hand) {
                        hand_text = translate("$HAND_TEXT_LEFT");
                    }
                    else if (skill.hand == hand_mode::right_hand) {
                        hand_text = translate("$HAND_TEXT_RIGHT");
                    }
                    else if (skill.hand == hand_mode::dual_hand) {
                        hand_text = translate("$HAND_TEXT_DUAL");
                    }
                    text_at(ImVec2(bpos.x + text_offset_x_right, bpos.y + text_offset_y), col_white, hand_text.c_str());
                }

                if (skill.consumed != consumed_type::none) {
                    //clamp text to 999
                    const std::string count_text = std::to_string(std::clamp(count, 0Ui64, 999Ui64));
                    const ImVec2 textsize = FUCK::CalcTextSize(count_text.c_str());
                    text_at(ImVec2(bpos.x + icon_size - textsize.x, bpos.y + icon_size - textsize.y), col_white, count_text.c_str());
                }
            }

            if (Bars::use_keybind_icons()) {
                FUCK::Dummy(ImVec2(std::max(key_icon_size * key_icon_length - item_spacing.x, 1.0f), icon_size));
                FUCK::SameLine();
            }

            //Draw Unbind target
            if (GameData::spellhotbar_unbind_slot != nullptr) {
                draw_skill_item(GameData::spellhotbar_unbind_slot->GetFormID(), icon_size);
            }
            else {
                icon_item(UiBridge::default_icon(GameData::DefaultIconType::BAR_EMPTY), icon_size);
            }
            if (FUCK::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = FUCK::AcceptDragDropPayload("SPELL_SLOT")) {
                    if (payload->DataSize == sizeof(Dragged_skill)) {
                        const Dragged_skill payload_n = *static_cast<const Dragged_skill*>(payload->Data);
                        if (payload_n.source_slot != nullptr) {
                            payload_n.source_slot->clear();
                        }
                    }
                }
                FUCK::EndDragDropTarget();
            }
            FUCK::SameLine();
            FUCK::TextUnformatted(translate_c("$UNBIND"));

            FUCK::EndChild();
        }
    }

    bool is_opened()
    {
        return show_frame.load(std::memory_order_relaxed);
    }

    void show()
    {
        if (!is_opened()) {
            window_title = translate("$BIND_MENU");
            load_spells();
            tab_index = 0;
            drag_in_flight = false;
            show_frame.store(true, std::memory_order_relaxed);
        }
    }

    void hide()
    {
        show_frame.store(false, std::memory_order_relaxed);
        drag_in_flight = false;

        list_of_skills.clear();
        list_of_skills_filtered.clear();
    }

    const char* title()
    {
        return window_title.empty() ? "Spell Hotbar 2" : window_title.c_str();
    }

    void draw()
    {
        if (drag_in_flight && !FUCK::IsMouseDown(0)) {
            drag_in_flight = false;
        }

        static int table_icon_size = 0;
        if (table_icon_size <= 0) {
            table_icon_size = static_cast<int>(std::round(scale(60.0f)));
        }

        const float pane_height = FUCK::GetContentRegionAvail().y;
        draw_list_pane(pane_height, table_icon_size);
        FUCK::SameLine();
        draw_bar_pane(pane_height);
    }
}
