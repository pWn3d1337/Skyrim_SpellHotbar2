#include "spell_editor.h"

#include "spell_edit_dialog.h"
#include "widgets.h"
#include "../../bar/hotbars.h"
#include "../../game_data/localization.h"
#include "../../game_data/spell_cast_data.h"

#include <cfloat>

// The spell editor's table half, ported from rendering/spell_editor.cpp to FLICK. What is
// listed, how it filters and sorts, and what pressing Edit loads into the dialog is the same
// code; only the drawing vocabulary changed.
//
// What did not come across, on purpose: the nine-slice parchment background, the title style and
// the drawn cursor. The host owns the window frame and the cursor, so draw() emits
// content only, and the two confirmation popups the old code opened per-row are now one modal
// each, driven by a pending form id -- a popup submitted from inside a clipped table row only
// exists on the frames that row is drawn.
namespace SpellHotbar::FlickUi::SpellEditor {
    namespace {
        // FLICK's TableFlags mirror ImGui's bit values but stop short of these two; the C-ABI
        // takes the int, so the real ImGui bits go through as-is.
        constexpr FUCK::TableFlags table_flag_sort_multi = static_cast<FUCK::TableFlags>(ImGuiTableFlags_SortMulti);
        constexpr FUCK::TableFlags table_flag_no_borders_in_body = static_cast<FUCK::TableFlags>(ImGuiTableFlags_NoBordersInBody);
        constexpr FUCK::TableFlags table_flag_scroll_y = static_cast<FUCK::TableFlags>(ImGuiTableFlags_ScrollY);

        std::atomic_bool show_frame{ false };

        bool filter_predefined_data = false;
        bool filter_user_data = false;

        std::vector<RE::TESForm*> list_of_skills;
        std::vector<RE::TESForm*> list_of_skills_filtered;
        const RE::TESForm* edit_form = nullptr;
        //Spell currently beeing edited
        std::optional<GameData::User_custom_spelldata> current_edit_data = std::nullopt;
        //filled default values for current spell
        std::optional<GameData::Spell_cast_data> current_edit_data_filled = std::nullopt;
        //Not filled values from spell, only csv data
        std::optional<GameData::Spell_cast_data> current_edit_data_unfilled = std::nullopt;
        //currently saved spell info, at start of editing
        std::optional<GameData::Spell_cast_data> current_edit_data_saved = std::nullopt;
        std::vector<int> list_of_anims;

        // The two confirmations, hoisted out of the rows they are asked from.
        bool reset_all_pending{ false };
        bool reset_all_open{ false };
        RE::FormID reset_row_pending{ 0 };
        bool reset_row_open{ false };
        std::string reset_row_name;

        constexpr int filter_buf_size = 256;
        char filter_buf[filter_buf_size] = "";

        enum spell_editor_column_id : ImGuiID {
            column_id_ID = 0U,
            column_id_Plugin,
            column_id_Icon,
            column_id_Name,
            column_id_Type,
            column_id_Effect,
            column_id_GCD,
            column_id_CD,
            column_id_Casttime,
            column_id_Anim,
            column_id_Anim2,
            column_id_Edit,
            column_id_Reset,

            column_count
        };

        void update_filter(const std::string filter_text, bool filter_predefined, bool filter_custom_dat)
        {
            if (filter_text.empty() && !filter_predefined && !filter_custom_dat) {
                list_of_skills_filtered = list_of_skills;
            }
            else {
                list_of_skills_filtered.clear();
                list_of_skills_filtered.reserve(list_of_skills.size());
                for (size_t i = 0U; i < list_of_skills.size(); i++) {

                    bool match_text{ false };
                    bool match_filter_predef{ false };
                    bool match_filter_custom_dat{ false };

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

                    if (filter_predefined) {
                        if (!GameData::spell_cast_info.contains(list_of_skills[i]->GetFormID()) &&
                            !UiBridge::has_custom_icon(list_of_skills[i]->GetFormID()) &&
                            !GameData::form_has_special_icon(list_of_skills[i])) {

                            match_filter_predef = true;
                        }
                    }
                    else {
                        match_filter_predef = true;
                    }

                    if (filter_custom_dat) {
                        if (GameData::user_spell_cast_info.contains(list_of_skills[i]->GetFormID())) {
                            match_filter_custom_dat = true;
                        }
                    }
                    else {
                        match_filter_custom_dat = true;
                    }

                    if (match_text && match_filter_predef && match_filter_custom_dat) list_of_skills_filtered.emplace_back(list_of_skills[i]);
                }
            }
        }

        void load_spells()
        {
            list_of_skills.clear();
            list_of_skills_filtered.clear();

            RE::PlayerCharacter* pc = RE::PlayerCharacter::GetSingleton();
            if (pc && pc->GetActorBase() != nullptr) {
                GameData::get_player_known_spells(pc, list_of_skills);

                filter_buf[0] = '\0';
                update_filter("", filter_predefined_data, filter_user_data);
            }
        }

        void load_anims()
        {
            list_of_anims.clear();
            list_of_anims.reserve(GameData::animation_names.size());
            for (auto& [k, v] : GameData::animation_names) {
                list_of_anims.push_back(k);
            }
            std::sort(list_of_anims.begin(), list_of_anims.end());
        }

        GameData::Spell_cast_data _get_spell_data(const RE::TESForm* form)
        {
            SpellHotbar::GameData::Spell_cast_data data;
            data = GameData::get_spell_data(form);
            return data;
        }

        bool compare_cast_effect(const RE::TESForm* lhs, const RE::TESForm* rhs)
        {
            auto l_data = _get_spell_data(lhs);
            auto r_data = _get_spell_data(rhs);
            return l_data.casteffectid < r_data.casteffectid;
        }

        bool compare_anim(const RE::TESForm* lhs, const RE::TESForm* rhs)
        {
            auto l_data = _get_spell_data(lhs);
            auto r_data = _get_spell_data(rhs);
            return l_data.animation < r_data.animation;
        }

        bool compare_anim2(const RE::TESForm* lhs, const RE::TESForm* rhs)
        {
            auto l_data = _get_spell_data(lhs);
            auto r_data = _get_spell_data(rhs);
            return l_data.animation2 < r_data.animation2;
        }

        bool compare_cd(const RE::TESForm* lhs, const RE::TESForm* rhs)
        {
            auto l_data = _get_spell_data(lhs);
            auto r_data = _get_spell_data(rhs);
            return l_data.cooldown < r_data.cooldown;
        }

        bool compare_gcd(const RE::TESForm* lhs, const RE::TESForm* rhs)
        {
            auto l_data = _get_spell_data(lhs);
            auto r_data = _get_spell_data(rhs);
            return l_data.gcd < r_data.gcd;
        }

        bool compare_casttime(const RE::TESForm* lhs, const RE::TESForm* rhs)
        {
            auto l_data = _get_spell_data(lhs);
            auto r_data = _get_spell_data(rhs);
            return l_data.casttime < r_data.casttime;
        }

        const ImGuiTableSortSpecs* s_current_sort_specs{ nullptr };
        bool compare_entries_for_sort(const RE::TESForm* lhs, const RE::TESForm* rhs)
        {
            for (int n = 0; n < s_current_sort_specs->SpecsCount; n++) {
                // Columns are identified by the ColumnUserID passed to TableSetupColumn().
                const ImGuiTableColumnSortSpecs* sort_spec = &s_current_sort_specs->Specs[n];
                bool is_less{ false };

                switch (sort_spec->ColumnUserID) {
                case spell_editor_column_id::column_id_Type:
                    is_less = static_cast<int>(lhs->GetFormType()) < static_cast<int>(rhs->GetFormType());
                    break;
                case spell_editor_column_id::column_id_Plugin:
                    is_less = lhs->GetFile(0)->GetFilename() < rhs->GetFile(0)->GetFilename();
                    break;
                case spell_editor_column_id::column_id_Name:
                    is_less = lhs->GetName() < rhs->GetName();
                    break;
                case spell_editor_column_id::column_id_Effect:
                    is_less = compare_cast_effect(lhs, rhs);
                    break;
                case spell_editor_column_id::column_id_CD:
                    is_less = compare_cd(lhs, rhs);
                    break;
                case spell_editor_column_id::column_id_GCD:
                    is_less = compare_gcd(lhs, rhs);
                    break;
                case spell_editor_column_id::column_id_Casttime:
                    is_less = compare_casttime(lhs, rhs);
                    break;
                case spell_editor_column_id::column_id_Anim:
                    is_less = compare_anim(lhs, rhs);
                    break;
                case spell_editor_column_id::column_id_Anim2:
                    is_less = compare_anim2(lhs, rhs);
                    break;
                case spell_editor_column_id::column_id_ID:
                default:
                    is_less = lhs->formID < rhs->formID;
                }
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? is_less : !is_less;
            }
            return lhs->formID < rhs->formID;
        }

        void begin_edit(int row)
        {
            if (row < 0 || row >= static_cast<int>(list_of_skills_filtered.size())) {
                return;
            }
            edit_form = list_of_skills_filtered[static_cast<std::size_t>(row)];
            current_edit_data = GameData::User_custom_spelldata(edit_form->GetFormID());
            current_edit_data->m_spell_data = GameData::get_spell_data(edit_form, false, true);

            if (GameData::user_spell_cast_info.contains(edit_form->GetFormID())) {
                //also set custom icon if present
                auto& user_dat = GameData::user_spell_cast_info.at(edit_form->GetFormID());
                if (user_dat.has_icon_data()) {
                    current_edit_data->m_icon_form = user_dat.m_icon_form;
                    current_edit_data->m_icon_str = user_dat.m_icon_str;
                }
            }

            current_edit_data_saved = current_edit_data->m_spell_data;

            current_edit_data_filled = GameData::get_spell_data(edit_form, true, false);
            current_edit_data_unfilled = GameData::get_spell_data(edit_form, false, false);
        }

        // Both confirmations are the same shape: a prompt, a separator, OK and Cancel. OpenPopup
        // fires once, on the frame the button was pressed; after that the popup is ImGui's to
        // keep open, and `answered` goes true the moment it is gone, whichever way it closed.
        enum class Answer { pending, ok, dismissed };

        Answer confirm_modal(const char* id, bool& open_it, const std::string& prompt)
        {
            if (open_it) {
                FUCK::OpenPopup(id);
                open_it = false;
            }
            Answer answer = Answer::pending;
            if (FUCK::BeginPopupModal(id, nullptr, FUCK::WindowFlags::kAutoResize | FUCK::WindowFlags::kNoMove)) {
                FUCK::TextUnformatted(prompt.c_str());
                FUCK::Separator();
                if (FUCK::Button(translate_id("$OK").c_str())) {
                    answer = Answer::ok;
                    FUCK::CloseCurrentPopup();
                }
                FUCK::SetItemDefaultFocus();
                FUCK::SameLine();
                if (FUCK::Button(translate_id("$CANCEL").c_str())) {
                    answer = Answer::dismissed;
                    FUCK::CloseCurrentPopup();
                }
                FUCK::EndPopup();
            }
            else if (!FUCK::IsPopupOpen(id)) {
                // Closed from outside the buttons (Escape, click-away).
                answer = Answer::dismissed;
            }
            return answer;
        }

        void draw_pending_confirmations()
        {
            if (reset_all_pending) {
                const std::string id = translate("$RESET") + "##reset_popup";
                const Answer a = confirm_modal(id.c_str(), reset_all_open, translate("$RESET_ALL_PROMPT"));
                if (a == Answer::ok) {
                    GameData::user_spell_cast_info.clear();
                }
                if (a != Answer::pending) {
                    reset_all_pending = false;
                }
            }

            if (reset_row_pending != 0) {
                const std::string id = translate("$RESET") + "?";
                const Answer a = confirm_modal(id.c_str(), reset_row_open, translate("$RESET_PROMT") + " '" + reset_row_name + "'.");
                if (a == Answer::ok && GameData::user_spell_cast_info.contains(reset_row_pending)) {
                    GameData::user_spell_cast_info.erase(reset_row_pending);
                }
                if (a != Answer::pending) {
                    reset_row_pending = 0;
                    reset_row_name.clear();
                }
            }
        }
    }

    bool is_opened()
    {
        return show_frame.load(std::memory_order_relaxed);
    }

    void show()
    {
        if (!is_opened()) {
            edit_form = nullptr;
            current_edit_data.reset();
            current_edit_data_filled.reset();
            current_edit_data_unfilled.reset();
            current_edit_data_saved.reset();
            reset_all_pending = false;
            reset_all_open = false;
            reset_row_pending = 0;
            reset_row_open = false;
            load_spells();
            load_anims();
            show_frame.store(true, std::memory_order_relaxed);
        }
    }

    void hide()
    {
        show_frame.store(false, std::memory_order_relaxed);

        close_edit_dialog();
        reset_all_pending = false;
        reset_all_open = false;
        reset_row_pending = 0;
        reset_row_open = false;
        reset_row_name.clear();

        list_of_skills.clear();
        list_of_skills_filtered.clear();
        list_of_anims.clear();
    }

    const char* title()
    {
        // The dialog takes the window over, so it takes the title with it.
        return edit_form != nullptr ? translate_c("$EDIT_SPELL_DATA") : translate_c("$SPELL_EDITOR");
    }

    void draw()
    {
        if (edit_form) {
            draw_edit_dialog(edit_form, current_edit_data.value(), current_edit_data_filled.value(),
                             current_edit_data_unfilled.value(), current_edit_data_saved.value());
        }
        else {
            draw_table();
        }
    }

    void draw_table()
    {
        draw_pending_confirmations();

        bool filter_dirty = false;

        // Filter row: clear button, the text field, the two filter checkboxes, Reset all.
        bool button_clear_filter_clicked{ false };
        if (FUCK::Button("X")) {
            button_clear_filter_clicked = true;
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

        FUCK::PushItemWidth(FUCK::GetContentRegionAvail().x * 0.35f);
        FUCK::InputText(translate_id("$FILTER").c_str(), filter_buf, filter_buf_size, ImGuiInputTextFlags_EscapeClearsAll);
        FUCK::PopItemWidth();

        FUCK::SameLine();
        if (checkbox(translate_id("$CHECK_EDITED").c_str(), &filter_user_data)) {
            filter_dirty = true;
        }

        FUCK::SameLine();
        if (checkbox(translate_id("$CHECK_NO_PREDEFINED").c_str(), &filter_predefined_data)) {
            filter_dirty = true;
        }

        FUCK::SameLine();
        if (FUCK::Button(translate_id("$RESET_ALL").c_str())) {
            reset_all_pending = true;
            reset_all_open = true;
        }

        using TF = FUCK::TableFlags;
        const TF flags = TF::kResizable | TF::kReorderable | TF::kHideable | TF::kSortable | table_flag_sort_multi |
                         TF::kRowBg | TF::kBordersOuterH | TF::kBordersOuterV | TF::kBordersInnerV |
                         table_flag_no_borders_in_body | table_flag_scroll_y;

        if (FUCK::BeginTable("List of Spells", spell_editor_column_id::column_count, flags, ImVec2(0.0f, 0.0f), 0.0f)) {
            using CF = FUCK::TableColumnFlags;
            FUCK::TableSetupColumn(translate_id("$COLUMN_ID").c_str(), CF::kDefaultSort | CF::kPreferSortAscending | CF::kWidthFixed, 0.0f, spell_editor_column_id::column_id_ID);
            FUCK::TableSetupColumn(translate_id("$COLUMN_PLUGIN").c_str(), CF::kWidthFixed, 0.0f, spell_editor_column_id::column_id_Plugin);
            FUCK::TableSetupColumn(translate_id("$COLUMN_ICON").c_str(), CF::kWidthFixed | CF::kNoSort, 0.0f, spell_editor_column_id::column_id_Icon);
            FUCK::TableSetupColumn(translate_id("$COLUMN_NAME").c_str(), CF::kWidthStretch, 0.0f, spell_editor_column_id::column_id_Name);
            FUCK::TableSetupColumn(translate_id("$COLUMN_TYPE").c_str(), CF::kWidthFixed, 0.0f, spell_editor_column_id::column_id_Type);
            FUCK::TableSetupColumn(translate_id("$COLUMN_CASTEFFECT").c_str(), CF::kWidthFixed, 0.0f, spell_editor_column_id::column_id_Effect);
            FUCK::TableSetupColumn(translate_id("$COLUMN_GCD").c_str(), CF::kWidthFixed, 0.0f, spell_editor_column_id::column_id_GCD);
            FUCK::TableSetupColumn(translate_id("$COLUMN_CD").c_str(), CF::kWidthFixed, 0.0f, spell_editor_column_id::column_id_CD);
            FUCK::TableSetupColumn(translate_id("$COLUMN_CASTTIME").c_str(), CF::kWidthFixed, 0.0f, spell_editor_column_id::column_id_Casttime);
            FUCK::TableSetupColumn(translate_id("$COLUMN_ANIM").c_str(), CF::kWidthFixed, 0.0f, spell_editor_column_id::column_id_Anim);
            FUCK::TableSetupColumn(translate_id("$COLUMN_ANIM2").c_str(), CF::kWidthFixed, 0.0f, spell_editor_column_id::column_id_Anim2);
            FUCK::TableSetupColumn(translate_id("$EDIT").c_str(), CF::kWidthFixed | CF::kNoSort, 0.0f, spell_editor_column_id::column_id_Edit);
            FUCK::TableSetupColumn(translate_id("$RESET").c_str(), CF::kWidthFixed | CF::kNoSort, 0.0f, spell_editor_column_id::column_id_Reset);
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

                    update_filter(filter_buf, filter_predefined_data, filter_user_data);
                    filter_dirty = false;
                }
            }

            //apply filtering
            if (filter_dirty) {
                update_filter(filter_buf, filter_predefined_data, filter_user_data);
                filter_dirty = false;
            }

            int button_edit_clicked = -1;

            const float icon_f = std::round(scale(40.0f));
            const float cell_pad_y = FUCK::GetStyleVarVec(ImGuiStyleVar_CellPadding).y;
            const float row_height = std::max(icon_f, FUCK::GetTextLineHeight()) + cell_pad_y * 2.0f;
            constexpr float days_to_seconds = 24.0f * 60.0f * 60.0f;

            const auto clip = RowClipper::begin(static_cast<int>(list_of_skills_filtered.size()), row_height);
            for (int row_n = clip.first; row_n < clip.last; row_n++) {
                // Display a data item
                const RE::TESForm* item = list_of_skills_filtered[static_cast<std::size_t>(row_n)];
                FUCK::PushID(static_cast<int>(item->GetFormID()));
                FUCK::TableNextRow(0, clip.row_h);
                FUCK::TableNextColumn();
                FUCK::Text("%08X", item->GetFormID());
                FUCK::TableNextColumn();
                auto file = item->GetFile(0);
                if (file != nullptr) {
                    FUCK::TextUnformatted(file->fileName);
                }
                else {
                    FUCK::TextUnformatted(translate_c("$DYNAMIC_FORM"));
                }

                FUCK::TableNextColumn();
                {
                    const ImVec2 icon_pos = FUCK::GetCursorScreenPos();
                    FUCK::Dummy(ImVec2(icon_f, icon_f));
                    skill_tooltip(item);
                    draw_skill_at(item->GetFormID(), icon_pos, icon_f);
                }

                FUCK::TableNextColumn();
                FUCK::TextUnformatted(item->GetName());

                FUCK::TableNextColumn();
                if (item->GetFormType() == RE::FormType::Spell) {
                    FUCK::TextUnformatted(translate_c("$TYPE_SPELL"));
                }
                else if (item->GetFormType() == RE::FormType::Shout) {
                    FUCK::TextUnformatted(translate_c("$TYPE_SHOUT"));
                }
                else {
                    FUCK::TextUnformatted(translate_c("$QUESTION_MARKS"));
                }

                auto data = SpellHotbar::GameData::get_spell_data(item);

                FUCK::TableNextColumn();
                FUCK::Text("%d", data.casteffectid);

                FUCK::TableNextColumn();
                FUCK::Text("%.2f", data.gcd);

                FUCK::TableNextColumn();
                FUCK::Text("%.2f", data.cooldown * days_to_seconds);

                FUCK::TableNextColumn();
                FUCK::Text("%.2f", data.casttime);

                FUCK::TableNextColumn();
                FUCK::Text("%d", data.animation);

                FUCK::TableNextColumn();
                FUCK::Text("%d", data.animation2);

                FUCK::TableNextColumn();
                if (FUCK::Button(translate_id("$EDIT").c_str())) {
                    button_edit_clicked = row_n;
                }

                FUCK::TableNextColumn();
                if (GameData::user_spell_cast_info.contains(item->GetFormID())) {
                    if (FUCK::Button(translate_id("$RESET").c_str())) {
                        reset_row_pending = item->GetFormID();
                        reset_row_open = true;
                        reset_row_name = item->GetName();
                    }
                }

                FUCK::PopID();
            }
            clip.end();
            FUCK::EndTable();

            if (button_edit_clicked > -1) {
                begin_edit(button_edit_clicked);
            }
        }
    }

    void close_edit_dialog()
    {
        edit_form = nullptr;
        current_edit_data.reset();
        current_edit_data_saved.reset();
        current_edit_data_filled.reset();
        current_edit_data_unfilled.reset();
    }

    std::vector<int>& get_list_of_anims()
    {
        return list_of_anims;
    }
}
