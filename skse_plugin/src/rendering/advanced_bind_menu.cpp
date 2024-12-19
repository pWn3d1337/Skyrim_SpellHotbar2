#include "advanced_bind_menu.h"
#include <imgui.h>
#include "render_manager.h"
#include "../input/keybinds.h"

namespace SpellHotbar::BindMenu {

    bool show_frame = false;

    constexpr int filter_buf_size = 256;
    char filter_buf[filter_buf_size] = "";

    std::vector<RE::TESForm*> list_of_skills;
    std::vector<RE::TESForm*> list_of_skills_filtered;

    struct dragged_skill {
        const RE::TESForm* form;

        dragged_skill(const RE::TESForm* tesform) : form(tesform) {};

        inline RE::FormID get_form_id() const
        {
            return form->GetFormID();
        }
    };

    std::unique_ptr<dragged_skill> current_dragged_skill{ nullptr };

    enum bind_menu_column_id : ImGuiID {
        column_id_Icon = 0U,
        column_id_Name,
        column_id_School,
        column_id_Rank,

        column_count
    };

    bool is_opened()
    {
        return show_frame;
    }

    void show()
    {
        if (!show_frame)
        {
            load_spells();
            show_frame = true;
        }
    }

    void hide()
    {
        show_frame = false;

        list_of_skills.clear();
        list_of_skills_filtered.clear();
    }

    void load_spells() {
        list_of_skills.clear();
        list_of_skills_filtered.clear();

        RE::PlayerCharacter* pc = RE::PlayerCharacter::GetSingleton();
        if (pc && pc->GetActorBase() != nullptr) {
            GameData::get_player_known_spells(pc, list_of_skills);

            filter_buf[0] = '\0';
            update_filter("");
        }
    }

    /*
    * return screen_size_x, screen_size_y, window_width
    */
    inline std::tuple<float, float, float> calculate_frame_size()
    {
        auto& io = ImGui::GetIO();
        const float screen_size_x = io.DisplaySize.x, screen_size_y = io.DisplaySize.y;

        float frame_height = screen_size_y * 0.9f;
        float frame_width = frame_height * 16.0f / 9.0f;

        ImGui::SetNextWindowSize(ImVec2(frame_width, frame_height));
        ImGui::SetNextWindowPos(ImVec2((screen_size_x - frame_width) * 0.5f, (screen_size_y - frame_height) * 0.5f));

        return std::make_tuple(screen_size_x, screen_size_y, frame_width);
    }

    void update_filter(const std::string filter_text) {
        if (filter_text.empty()) {
            list_of_skills_filtered = list_of_skills;
        }
        else {
            list_of_skills_filtered.clear();
            list_of_skills_filtered.reserve(list_of_skills.size());
            for (size_t i = 0U; i < list_of_skills.size(); i++) {

                bool match_text{ false };

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

                if (match_text) list_of_skills_filtered.emplace_back(list_of_skills[i]);
            }
        }
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
            case bind_menu_column_id::column_id_Name:
                is_less = lhs->GetName() < rhs->GetName();
                break;
                //TODO Rank & school
            default:
                is_less = lhs->formID < rhs->formID;
            }
            return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? is_less : !is_less;
        }
        return lhs->formID < rhs->formID;
    }

    void set_drag_source(const RE::TESForm* item, float scale_factor) {
        if (item != nullptr) {
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {

                current_dragged_skill = std::make_unique<dragged_skill>(item);
                // Set payload to carry the index of our item (could be anything)
                ImGui::SetDragDropPayload("SPELL_SLOT", &current_dragged_skill, sizeof(dragged_skill));

                // Display preview (could be anything, e.g. when dragging an image we could decide to display
                // the filename and a small preview of the image, etc.)
                //ImGui::Text("%08x", item->formID);
                if (current_dragged_skill != nullptr) {
                    RenderManager::draw_skill(current_dragged_skill->get_form_id(), static_cast<int>(std::round(60.0f * scale_factor)));
                    ImGui::SameLine();
                    ImGui::Text(current_dragged_skill->form->GetName());
                }
                ImGui::EndDragDropSource();
            }
        }
    }

	void drawFrame(ImFont* font_text) {
        static constexpr ImGuiWindowFlags window_flag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;

        auto& io = ImGui::GetIO();
        io.MouseDrawCursor = true;

        auto [screen_size_x, screen_size_y, window_width] = calculate_frame_size();
        ImGui::SetNextWindowBgAlpha(1.0F);

        float scale_factor = screen_size_y / 1080.0f;

        ImGui::Begin("Binding Menu", &show_frame, window_flag);
        float child_window_height = ImGui::GetContentRegionAvail().y;
        ImGui::BeginChild("BindMenuTabLeft", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, child_window_height), false, ImGuiWindowFlags_HorizontalScrollbar);

        static int tab_index = 0;
        ImGui::RadioButton("All", & tab_index, 0); ImGui::SameLine();
        ImGui::RadioButton("Alteration", & tab_index, 1); ImGui::SameLine();
        ImGui::RadioButton("Illusion", & tab_index, 2); ImGui::SameLine();
        ImGui::RadioButton("Destruction", & tab_index, 3); ImGui::SameLine();
        ImGui::RadioButton("Conjuration", & tab_index, 4); ImGui::SameLine();
        ImGui::RadioButton("Restoration", & tab_index, 5); ImGui::SameLine();
        ImGui::RadioButton("Shouts", & tab_index, 6); ImGui::SameLine();
        ImGui::RadioButton("Powers", & tab_index, 7);

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

        ImGui::InputTextWithHint("Filter", "Filter spell names containing text", filter_buf, filter_buf_size, filter_input_flags);

        static ImGuiTableFlags flags =
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
            | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
            | ImGuiTableFlags_ScrollY;

        if (ImGui::BeginTable("Known Spells", bind_menu_column_id::column_count, flags, ImVec2(0.0f, 0.0f), 0.0f))
        {
            // Declare columns
            // We use the "user_id" parameter of TableSetupColumn() to specify a user id that will be stored in the sort specifications.
            // This is so our sort function can identify a column given our own identifier. We could also identify them based on their index!
            // Demonstrate using a mixture of flags among available sort-related flags:
            // - ImGuiTableColumnFlags_DefaultSort
            // - ImGuiTableColumnFlags_NoSort / ImGuiTableColumnFlags_NoSortAscending / ImGuiTableColumnFlags_NoSortDescending
            // - ImGuiTableColumnFlags_PreferSortAscending / ImGuiTableColumnFlags_PreferSortDescending
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortAscending, 0.0f, bind_menu_column_id::column_id_Name);
            ImGui::TableSetupColumn("School", ImGuiTableColumnFlags_WidthFixed, 0.0f, bind_menu_column_id::column_id_School);
            ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthFixed, 0.0f, bind_menu_column_id::column_id_Rank);
            ImGui::TableSetupColumn("Drag Icon", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 65.0f, bind_menu_column_id::column_id_Icon);
            ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
            ImGui::TableHeadersRow();

            // Sort our data if sort specs have been changed!
            if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
                if (sort_specs->SpecsDirty)
                {
                    s_current_sort_specs = sort_specs;
                    std::sort(list_of_skills.begin(), list_of_skills.end(), compare_entries_for_sort);
                    sort_specs->SpecsDirty = false;

                    update_filter(filter_buf);
                    filter_dirty = false;
                }
            }

            //apply filtering
            if (filter_dirty) {
                update_filter(filter_buf);
                filter_dirty = false;
            }

            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(list_of_skills_filtered.size()));
            while (clipper.Step())
            {
                for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
                {
                    // Display a data item
                    const RE::TESForm* item = list_of_skills_filtered[row_n];
                    const RE::SpellItem* spell = nullptr;

                    int rank = 0;
                    RE::ActorValue school = RE::ActorValue::kNone;
                    if (item->GetFormType() == RE::FormType::Spell) {
                        spell = item->As<RE::SpellItem>();
                       
                        if (spell != nullptr && spell->effects.size() > 0U) {

                            bool found{ false };
                            for (RE::BSTArrayBase::size_type i = 0U; i < spell->effects.size() && !found; i++) {
                                //find first spell effect that has a magic school
                                RE::Effect* effect = spell->effects[i];
                                if (effect->baseEffect) {
                                    school = effect->baseEffect->GetMagickSkill();
                                    rank = GameData::get_spell_rank(effect->baseEffect->GetMinimumSkillLevel());
                                }
                            }
                        }
                    }
                    ImGui::PushID(item->GetFormID());
                    ImGui::TableNextRow();
                   
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(item->GetName());

                    ImGui::TableNextColumn();
                    if (rank > 0) {
                        ImGui::Text("%d", rank);
                    }
                    else {
                        ImGui::TextUnformatted("");
                    }

                    ImGui::TableNextColumn();
                    if (school != RE::ActorValue::kNone) {
                        ImGui::Text("%d", static_cast<int>(school));
                    }
                    else {
                        ImGui::TextUnformatted("");
                    }

                    ImGui::TableNextColumn();

                    RenderManager::draw_skill(item->GetFormID(), static_cast<int>(std::round(60.0f * scale_factor)));
                    set_drag_source(item, scale_factor);

                    ImGui::PopID();
                }
            }
            ImGui::EndTable();

        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("BindMenuTabRight", ImVec2(0, child_window_height), false, ImGuiWindowFlags_HorizontalScrollbar);

        auto list_of_bars = Bars::get_list_of_bars();

        auto& bar = Bars::hotbars.at(Bars::menu_bar_id);
        if (ImGui::BeginCombo("##Hotbar", bar.get_name().c_str(), 0))
        {
            for (uint16_t n = 0; n < list_of_bars.size(); n++)
            {
                auto bar_id = list_of_bars.at(n).first;
                const bool is_selected = (bar_id == Bars::menu_bar_id);
                auto & name = list_of_bars.at(n).second;

                if (ImGui::Selectable(name.c_str(), is_selected)) {
                    Bars::menu_bar_id = list_of_bars.at(n).first;
                }

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        static int modifier_index = 0;
        std::string none_text = GameData::get_modifier_text(key_modifier::none);
        ImGui::RadioButton(none_text.c_str(), &modifier_index, 0);

        if (Input::mod_1.isValidBound()) {
            ImGui::SameLine();
            std::string ctrl_text = GameData::get_modifier_text(key_modifier::ctrl);
            ImGui::RadioButton(ctrl_text.c_str(), &modifier_index, 1);
        }
        if (Input::mod_2.isValidBound()) {
            ImGui::SameLine();
            std::string shift_text = GameData::get_modifier_text(key_modifier::shift);
            ImGui::RadioButton(shift_text.c_str(), &modifier_index, 2);
        }
        if (Input::mod_3.isValidBound()) {
            ImGui::SameLine();
            std::string alt_text = GameData::get_modifier_text(key_modifier::alt);
            ImGui::RadioButton(alt_text.c_str(), &modifier_index, 3);
        }

        //draw Hotbar slots
        ImGui::PushFont(font_text);

        int icon_size = static_cast<int>(std::round(60.0f * scale_factor));
        float text_offset_x = icon_size * 0.05f;
        float text_offset_x_right = icon_size * 0.95f - ImGui::CalcTextSize("R").x;
        float text_offset_y = icon_size * 0.0125f;
        //float text_height = ImGui::CalcTextSize("M").y;

        key_modifier mod = static_cast<key_modifier>(modifier_index);

        //SpellHotbar::Hotbar& bar = Bars::hotbars[Bars::menu_bar_id];

        for (int i = 0; i < bar.get_bar_size(); i++) {
            auto [skill, inherited] = bar.get_skill_in_bar_with_inheritance(i, mod, false);

            GameData::Spell_cast_data skill_dat;
            auto form = RE::TESForm::LookupByID(skill.formID);
            if (form) {
                skill_dat = GameData::get_spell_data(form, true, true);
            }

            ImVec2 p = ImGui::GetCursorScreenPos();

            size_t count{ 0 };
            if (skill.consumed != consumed_type::none) {
                count = GameData::count_item_in_inv(skill.formID);
            }

            ImVec2 bpos = ImGui::GetCursorScreenPos();
            std::string button_label = "##slot_button_" + std::to_string(i);
            if (ImGui::Button(button_label.c_str(), ImVec2(static_cast<float>(icon_size), static_cast<float>(icon_size)))) {
                
            }

            bool drawn_skill{ false };
            if (ImGui::IsItemHovered() && inherited) {
                if (GameData::spellhotbar_unbind_slot != nullptr) {
                    RenderManager::draw_skill_in_editor(GameData::spellhotbar_unbind_slot->GetFormID(), bpos, icon_size);
                }
                else {
                    //RenderManager::draw_bg(icon_size);
                    RenderManager::draw_default_icon_in_editor(GameData::DefaultIconType::BAR_EMPTY, bpos, icon_size);
                }
            }
            else {
                drawn_skill = RenderManager::draw_skill_in_editor(skill.formID, bpos, icon_size);
                //bool drawn = RenderManager::draw_skill(skill.formID, icon_size, skill.color);
                if (!drawn_skill) {
                    //RenderManager::draw_bg(icon_size);
                    RenderManager::draw_default_icon_in_editor(GameData::DefaultIconType::BAR_EMPTY, bpos, icon_size);
                }
            }

            if (!inherited) {
                set_drag_source(form, scale_factor);
            }
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPELL_SLOT"))
                {
                    IM_ASSERT(payload->DataSize == sizeof(dragged_skill));
                    dragged_skill payload_n = *(const dragged_skill*)payload->Data;
                    logger::info("Dropped: {:08x}", payload_n.get_form_id());
                }
                ImGui::EndDragDropTarget();
            }

            if (drawn_skill) {
                if (RenderManager::should_overlay_be_rendered(skill_dat.overlay_icon)) {
                    RenderManager::draw_icon_overlay(p, icon_size, skill_dat.overlay_icon, IM_COL32_WHITE);
                }

                ImU32 col = IM_COL32_WHITE;
                /*if (highlight_slot == i) {
                    col = IM_COL32(255, 255, static_cast<int>(127 + 128 * (1.0 - highlight_factor)), 255);
                }*/

                RenderManager::draw_slot_overlay(p, icon_size, col);
            }
            ImGui::SameLine();

            std::string key_text = GameData::get_keybind_text(i, mod);
            ImVec2 tex_pos(p.x + text_offset_x, p.y + text_offset_y);
            ImGui::GetWindowDrawList()->AddText(tex_pos, ImColor(255, 255, 255), key_text.c_str());

            if (skill.hand == hand_mode::left_hand || skill.hand == hand_mode::right_hand || skill.hand == hand_mode::dual_hand) {
                std::string hand_text;
                if (skill.hand == hand_mode::left_hand) {
                    hand_text = "L";
                }
                else if (skill.hand == hand_mode::right_hand) {
                    hand_text = "R";
                }
                else if (skill.hand == hand_mode::dual_hand) {
                    hand_text = "D";
                }
                ImVec2 tex_pos_hand(p.x + text_offset_x_right, p.y + text_offset_y);
                ImGui::GetWindowDrawList()->AddText(tex_pos_hand, ImColor(255, 255, 255), hand_text.c_str());
            }

            if (skill.consumed != consumed_type::none) {
                //clamp text to 999
                std::string text = std::to_string(std::clamp(count, 0Ui64, 999Ui64));
                ImVec2 textsize = ImGui::CalcTextSize(text.c_str());
                ImVec2 count_text_pos(p.x + icon_size - textsize.x, p.y + icon_size - textsize.y);
                ImGui::GetWindowDrawList()->AddText(count_text_pos, ImColor(255, 255, 255), text.c_str());
            }

            std::string text = GameData::resolve_spellname(skill.formID);

            //draw text in grey if it was inherited from parent bar
            int grey_val = inherited ? 127 : 255;
            auto color = ImColor(grey_val, grey_val, grey_val);

            /*if (highlight_slot == i) {
                color = ImColor(255, 255, 127 + static_cast<int>(128.0 * (1.0f - highlight_factor)));
            }*/

            ImGui::TextColored(color, text.c_str());
        }

        //Draw Unbind target
        if (GameData::spellhotbar_unbind_slot != nullptr) {
            RenderManager::draw_skill(GameData::spellhotbar_unbind_slot->GetFormID(), icon_size);
        }
        else {
            RenderManager::draw_bg(icon_size);
        }
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPELL_SLOT"))
            {
                IM_ASSERT(payload->DataSize == sizeof(dragged_skill));
                dragged_skill payload_n = *(const dragged_skill*)payload->Data;
                logger::info("Deleting: {:08x}", payload_n.get_form_id());
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::SameLine();
        ImGui::Text("Unbind");

        ImGui::PopFont();

        ImGui::EndChild();

        ImGui::End();
    }
}