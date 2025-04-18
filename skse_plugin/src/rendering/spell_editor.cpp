#include "spell_editor.h"

#include "../bar/hotbars.h"
#include "../input/keybinds.h"
#include "../game_data/game_data.h"
#include "../rendering/render_manager.h"
#include "../rendering/spell_edit_dialog.h"
#include "../game_data/user_custom_spelldata.h"

namespace SpellHotbar::SpellEditor {
	
    bool show_frame = false;

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

    bool is_opened()
    {
        return show_frame;
    }

    void update_filter(const std::string filter_text, bool filter_predefined, bool filter_custom_dat) {
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
                    if(!GameData::spell_cast_info.contains(list_of_skills[i]->GetFormID()) &&
                       !RenderManager::has_custom_icon(list_of_skills[i]->GetFormID()) &&
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

    void load_spells() {
        list_of_skills.clear();
        list_of_skills_filtered.clear();

        RE::PlayerCharacter* pc = RE::PlayerCharacter::GetSingleton();
        if (pc && pc->GetActorBase() != nullptr) {
            GameData::get_player_known_spells(pc, list_of_skills);

            filter_buf[0] = '\0';
            update_filter("", filter_predefined_data, filter_user_data);
        }
    }

    void load_anims() {
        list_of_anims.clear();
        list_of_anims.reserve(GameData::animation_names.size());
        for (auto& [k, v] : GameData::animation_names) {
            list_of_anims.push_back(k);
        }
        std::sort(list_of_anims.begin(), list_of_anims.end());
    }

    void show()
    {
        if (!show_frame)
        {
            edit_form = nullptr;
            current_edit_data.reset();
            current_edit_data_filled.reset();
            current_edit_data_unfilled.reset();
            current_edit_data_saved.reset();
            load_spells();
            load_anims();
            show_frame = true;
        }
    }

    void hide()
    {
        show_frame = false;

        edit_form = nullptr;
        current_edit_data.reset();
        current_edit_data_filled.reset();
        current_edit_data_unfilled.reset();
        current_edit_data_saved.reset();

        list_of_skills.clear();
        list_of_skills_filtered.clear();
        list_of_anims.clear();
    }

    void renderEditor()
    {
        if (edit_form) {
            drawEditDialog(edit_form, current_edit_data.value(), current_edit_data_filled.value(), current_edit_data_unfilled.value(), current_edit_data_saved.value());
        }
        else {
            drawTableFrame();
        }
    }

    inline SpellHotbar::GameData::Spell_cast_data _get_spell_data(const RE::TESForm* form) {
        SpellHotbar::GameData::Spell_cast_data data;
        data = GameData::get_spell_data(form);
        return data;
    }

    inline bool compare_cast_effect(const RE::TESForm* lhs, const RE::TESForm* rhs) {
        auto l_data = _get_spell_data(lhs);
        auto r_data = _get_spell_data(rhs);
        return l_data.casteffectid < r_data.casteffectid;
    }

    inline bool compare_anim(const RE::TESForm* lhs, const RE::TESForm* rhs) {
        auto l_data = _get_spell_data(lhs);
        auto r_data = _get_spell_data(rhs);
        return l_data.animation < r_data.animation;
    }

    inline bool compare_anim2(const RE::TESForm* lhs, const RE::TESForm* rhs) {
        auto l_data = _get_spell_data(lhs);
        auto r_data = _get_spell_data(rhs);
        return l_data.animation2 < r_data.animation2;
    }

    inline bool compare_cd(const RE::TESForm* lhs, const RE::TESForm* rhs) {
        auto l_data = _get_spell_data(lhs);
        auto r_data = _get_spell_data(rhs);
        return l_data.cooldown < r_data.cooldown;
    }

    inline bool compare_gcd(const RE::TESForm* lhs, const RE::TESForm* rhs) {
        auto l_data = _get_spell_data(lhs);
        auto r_data = _get_spell_data(rhs);
        return l_data.gcd < r_data.gcd;
    }

    inline bool compare_casttime(const RE::TESForm* lhs, const RE::TESForm* rhs) {
        auto l_data = _get_spell_data(lhs);
        auto r_data = _get_spell_data(rhs);
        return l_data.casttime < r_data.casttime;
    }


    const ImGuiTableSortSpecs* s_current_sort_specs;
    bool compare_entries_for_sort(const RE::TESForm* lhs, const RE::TESForm* rhs) {
        for (int n = 0; n < s_current_sort_specs->SpecsCount; n++)
        {
            // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
            // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
            const ImGuiTableColumnSortSpecs* sort_spec = &s_current_sort_specs->Specs[n];
            bool is_less = 0;

            switch (sort_spec->ColumnUserID)
            {
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

    void drawTableFrame() {
        static constexpr ImGuiWindowFlags window_flag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground;

        auto& io = ImGui::GetIO();
        io.MouseDrawCursor = true;

        constexpr float window_aspect_ratio = 4.0f / 3.0f;
        RenderManager::calculate_frame_size(0.975f, window_aspect_ratio);
        RenderManager::draw_frame_bg(&show_frame);

        auto [screen_size_x, screen_size_y, window_width] = RenderManager::calculate_frame_size(0.95f, window_aspect_ratio);
        ImGui::SetNextWindowBgAlpha(1.0F);

        float scale_factor = screen_size_y / 1080.0f;

        RenderManager::ImGui_push_title_style();
        ImGui::Begin(translate_c("$SPELL_EDITOR"), &show_frame, window_flag);
        RenderManager::ImGui_pop_title_style();

        ImGui::BeginChild("##spell_editor", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_None);
        bool button_clear_filter_clicked{ false };
        if (ImGui::SmallButton("X")) {
            button_clear_filter_clicked = true;
        }
        ImGui::SameLine();

        static std::string last_filter = "";

        static auto filter_input_flags = ImGuiInputTextFlags_EscapeClearsAll;

        bool filter_dirty = false;

        if (button_clear_filter_clicked) {
            filter_buf[0] = '\0';
        }

        if (last_filter != filter_buf) {
            filter_dirty = true;
        }
        last_filter = filter_buf;

        ImGui::InputTextWithHint(translate_id("$FILTER").c_str(), translate_c("$FILTER_INFO"), filter_buf, filter_buf_size, filter_input_flags);

        ImGui::SameLine();
        if (ImGui::Checkbox(translate_id("$CHECK_EDITED").c_str(), &filter_user_data)) {
            filter_dirty = true;
        };

        ImGui::SameLine();
        if (ImGui::Checkbox(translate_id("$CHECK_NO_PREDEFINED").c_str(), &filter_predefined_data)) {
            filter_dirty = true;
        }

        ImGui::SameLine();
        std::string reset_popup = translate("$RESET") + "##reset_popup";
        if (ImGui::Button(translate_id("$RESET_ALL").c_str())) {

            ImGui::OpenPopup(reset_popup.c_str());
        }

        if (ImGui::BeginPopupModal(reset_popup.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
        {
            ImGui::Text(translate("$RESET_ALL_PROMPT").c_str());
            ImGui::Separator();

            if (ImGui::Button((translate("$OK") + "##_reset_ok").c_str(), ImVec2(120, 0))) {
                GameData::user_spell_cast_info.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button((translate("$CANCEL") + "##_reset_cancel").c_str(), ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        static ImGuiTableFlags flags =
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
            | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
            | ImGuiTableFlags_ScrollY;

        if (ImGui::BeginTable("List of Spells", spell_editor_column_id::column_count, flags, ImVec2(0.0f, 0.0f), 0.0f))
        {
            // Declare columns
            // We use the "user_id" parameter of TableSetupColumn() to specify a user id that will be stored in the sort specifications.
            // This is so our sort function can identify a column given our own identifier. We could also identify them based on their index!
            // Demonstrate using a mixture of flags among available sort-related flags:
            // - ImGuiTableColumnFlags_DefaultSort
            // - ImGuiTableColumnFlags_NoSort / ImGuiTableColumnFlags_NoSortAscending / ImGuiTableColumnFlags_NoSortDescending
            // - ImGuiTableColumnFlags_PreferSortAscending / ImGuiTableColumnFlags_PreferSortDescending
            ImGui::TableSetupColumn(translate_id("$COLUMN_ID").c_str(), ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortAscending | ImGuiTableColumnFlags_WidthFixed, 0.0f, spell_editor_column_id::column_id_ID);
            ImGui::TableSetupColumn(translate_id("$COLUMN_PLUGIN").c_str(), ImGuiTableColumnFlags_WidthFixed, 0.0f, spell_editor_column_id::column_id_Plugin);
            ImGui::TableSetupColumn(translate_id("$COLUMN_ICON").c_str(), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 0.0f, spell_editor_column_id::column_id_Icon);
            ImGui::TableSetupColumn(translate_id("$COLUMN_NAME").c_str(), ImGuiTableColumnFlags_WidthStretch, 0.0f, spell_editor_column_id::column_id_Name);
            ImGui::TableSetupColumn(translate_id("$COLUMN_TYPE").c_str(), ImGuiTableColumnFlags_WidthFixed, 0.0f, spell_editor_column_id::column_id_Type);
            ImGui::TableSetupColumn(translate_id("$COLUMN_CASTEFFECT").c_str(), ImGuiTableColumnFlags_WidthFixed, 0.0f, spell_editor_column_id::column_id_Effect);
            ImGui::TableSetupColumn(translate_id("$COLUMN_GCD").c_str(), ImGuiTableColumnFlags_WidthFixed, 0.0f, spell_editor_column_id::column_id_GCD);
            ImGui::TableSetupColumn(translate_id("$COLUMN_CD").c_str(), ImGuiTableColumnFlags_WidthFixed, 0.0f, spell_editor_column_id::column_id_CD);
            ImGui::TableSetupColumn(translate_id("$COLUMN_CASTTIME").c_str(), ImGuiTableColumnFlags_WidthFixed, 0.0f, spell_editor_column_id::column_id_Casttime);
            ImGui::TableSetupColumn(translate_id("$COLUMN_ANIM").c_str(), ImGuiTableColumnFlags_WidthFixed, 0.0f, spell_editor_column_id::column_id_Anim);
            ImGui::TableSetupColumn(translate_id("$COLUMN_ANIM2").c_str(), ImGuiTableColumnFlags_WidthFixed, 0.0f, spell_editor_column_id::column_id_Anim2);
            ImGui::TableSetupColumn(translate_id("$EDIT").c_str(), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 0.0f, spell_editor_column_id::column_id_Edit);
            ImGui::TableSetupColumn(translate_id("$RESET").c_str(), ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 0.0f, spell_editor_column_id::column_id_Reset);
            ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
            ImGui::TableHeadersRow();

            // Sort our data if sort specs have been changed!
            if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
                if (sort_specs->SpecsDirty)
                {
                    s_current_sort_specs = sort_specs;
                    std::sort(list_of_skills.begin(), list_of_skills.end(), compare_entries_for_sort);
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

            // Demonstrate using clipper for large vertical lists
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(list_of_skills_filtered.size()));
            while (clipper.Step()) 
            {
                for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
                {
                    // Display a data item
                    const RE::TESForm* item = list_of_skills_filtered[row_n];
                    ImGui::PushID(item->GetFormID());
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%08X", item->GetFormID());
                    ImGui::TableNextColumn();
                    auto file = item->GetFile(0);
                    if (file != nullptr) {
                        ImGui::TextUnformatted(file->fileName);
                    }
                    else {
                        ImGui::TextUnformatted(translate_c("$DYNAMIC_FORM"));
                    }

                    ImGui::TableNextColumn();
                    RenderManager::draw_skill(item->GetFormID(), static_cast<int>(std::round(40.0f * scale_factor)));
                    RenderManager::show_skill_tooltip(item);

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(item->GetName());
                    //ImGui::TableNextColumn();
                    //ImGui::SmallButton("None");

                    if (item->GetFormType() == RE::FormType::Spell) {
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(translate_c("$TYPE_SPELL"));
                    }
                    else if (item->GetFormType() == RE::FormType::Shout) {
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(translate_c("$TYPE_SHOUT"));
                    }
                    else {
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(translate_c("$QUESTION_MARKS"));
                    }

                    auto data = SpellHotbar::GameData::get_spell_data(item);

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", data.casteffectid);

                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f", data.gcd);

                    float constexpr days_to_seconds = 24.0f * 60.0f * 60.0f;
                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f", data.cooldown * days_to_seconds);

                    ImGui::TableNextColumn();
                    ImGui::Text("%.2f", data.casttime);

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", data.animation);

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", data.animation2);

                    ImGui::TableNextColumn();
                    if (ImGui::SmallButton(translate_id("$EDIT").c_str())) {
                        button_edit_clicked = row_n;
                    }

                    ImGui::TableNextColumn();

                    if (GameData::user_spell_cast_info.contains(item->GetFormID())) {
                        if (ImGui::SmallButton(translate_id("$RESET").c_str())) {
                            ImGui::OpenPopup((translate("$RESET") + "?").c_str());
                        }

                        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

                        if (ImGui::BeginPopupModal((translate("$RESET") + "?").c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
                        {
                            ImGui::Text((translate("$RESET_PROMT") + " '%s'.").c_str(), item->GetName());
                            ImGui::Separator();

                            if (ImGui::Button(translate_id("$OK").c_str(), ImVec2(120, 0))) {
                                if (GameData::user_spell_cast_info.contains(item->GetFormID())) {
                                    GameData::user_spell_cast_info.erase(item->GetFormID());
                                }
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::SetItemDefaultFocus();
                            ImGui::SameLine();
                            if (ImGui::Button(translate_id("$CANCEL").c_str(), ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                            ImGui::EndPopup();
                        }
                    }

                    ImGui::PopID();
                }
            }
            ImGui::EndTable();

            if (button_edit_clicked > -1) {
                if (button_edit_clicked >=0 && button_edit_clicked < list_of_skills_filtered.size())
                {
                    edit_form = list_of_skills_filtered[button_edit_clicked];
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
            }

        }
        ImGui::EndChild();
        ImGui::End();

        RenderManager::draw_custom_mouse_cursor();
    }

    void closeEditDialog()
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