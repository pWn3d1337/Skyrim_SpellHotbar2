#include "potion_editor.h"
#include "render_manager.h"
#include "../game_data/game_data.h"
#include "icon_edit_dialog.h"

namespace SpellHotbar::PotionEditor {
	bool show_frame = false;

    bool filter_predefined_data = false;
    bool filter_user_data = false;

    const RE::TESForm* edit_form = nullptr;

    std::vector<RE::TESForm*> list_of_entries;
    std::vector<RE::TESForm*> list_of_entries_filtered;

    //Spell currently beeing edited
    std::optional<GameData::User_custom_entry> current_edit_data = std::nullopt;

    enum potion_editor_column_id : ImGuiID {
        column_id_ID = 0U,
        column_id_Plugin,
        column_id_Icon,
        column_id_Name,
        column_id_Type,
        column_id_Edit,
        column_id_Reset,

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
            edit_form = nullptr;
            show_frame = true;
            current_edit_data.reset();
            load_entries();
        }
    }

    void hide()
    {
        show_frame = false;

        edit_form = nullptr;
        current_edit_data.reset();

        list_of_entries.clear();
        list_of_entries_filtered.clear();
    }

    void closeEditDialog()
    {
        edit_form = nullptr;
    }

    void renderEditor()
    {
        if (edit_form) {
            drawEditDialog(edit_form, current_edit_data.value());
        }
        else {
            drawTableFrame();
        }
    }

    /*
    * return screen_size_x, screen_size_y, window_width
    */
    inline std::tuple<float, float, float> calculate_frame_size()
    {
        auto& io = ImGui::GetIO();
        const float screen_size_x = io.DisplaySize.x, screen_size_y = io.DisplaySize.y;

        float frame_height = screen_size_y * 0.8f;
        float frame_width = frame_height * 4.0f / 3.0f;

        ImGui::SetNextWindowSize(ImVec2(frame_width, frame_height));
        ImGui::SetNextWindowPos(ImVec2((screen_size_x - frame_width) * 0.5f, (screen_size_y - frame_height) * 0.5f));

        return std::make_tuple(screen_size_x, screen_size_y, frame_width);
    }

    void load_entries() {
        list_of_entries.clear();
        list_of_entries_filtered.clear();

        RE::PlayerCharacter* pc = RE::PlayerCharacter::GetSingleton();
        if (pc) {

            auto refs = pc->GetInventoryCounts([](const RE::TESBoundObject& object) { return object.formType == RE::FormType::AlchemyItem; });
            for (auto& [k, v] : refs) {
                if (!GameData::user_custom_entry_info.contains(k->GetFormID())) {
                    list_of_entries.push_back(k);
                }
            }

            for (auto& [k, v] : GameData::user_custom_entry_info) {
                RE::TESForm* f = RE::TESForm::LookupByID(k);
                if (f) {
                    list_of_entries.push_back(f);
                }
            }

            update_filter("", filter_predefined_data, filter_user_data);
        }
    }

    void update_filter(const std::string filter_text, bool filter_predefined, bool filter_custom_dat) {
        if (filter_text.empty() && !filter_predefined && !filter_custom_dat) {
            list_of_entries_filtered = list_of_entries;
        }
        else {
            list_of_entries_filtered.clear();
            list_of_entries_filtered.reserve(list_of_entries.size());
            for (size_t i = 0U; i < list_of_entries.size(); i++) {

                bool match_text{ false };
                bool match_filter_predef{ false };
                bool match_filter_custom_dat{ false };

                if (!filter_text.empty()) {
                    //Check if name matches filter
                    const std::string name = list_of_entries[i]->GetName();
                    if (name.find(filter_text) != std::string::npos) {
                        match_text = true;
                    }
                }
                else {
                    match_text = true;
                }

                if (filter_predefined) {
                    if (!GameData::spell_cast_info.contains(list_of_entries[i]->GetFormID()) &&
                        !RenderManager::has_custom_icon(list_of_entries[i]->GetFormID()) &&
                        !GameData::form_has_special_icon(list_of_entries[i])) {

                        match_filter_predef = true;
                    }
                }
                else {
                    match_filter_predef = true;
                }

                if (filter_custom_dat) {
                    if (GameData::user_custom_entry_info.contains(list_of_entries[i]->GetFormID())) {
                        match_filter_custom_dat = true;
                    }
                }
                else {
                    match_filter_custom_dat = true;
                }

                if (match_text && match_filter_predef && match_filter_custom_dat) list_of_entries_filtered.emplace_back(list_of_entries[i]);
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
            case potion_editor_column_id::column_id_Type:
                is_less = static_cast<int>(lhs->GetFormType()) < static_cast<int>(rhs->GetFormType());
                break;
            case potion_editor_column_id::column_id_Plugin:
                is_less = lhs->GetFile(0)->GetFilename() < rhs->GetFile(0)->GetFilename();
                break;
            case potion_editor_column_id::column_id_Name:
                is_less = lhs->GetName() < rhs->GetName();
                break;
            case potion_editor_column_id::column_id_ID:
            default:
                is_less = lhs->formID < rhs->formID;
            }
            return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? is_less : !is_less;
        }
        return lhs->formID < rhs->formID;
    }

    void drawTableFrame()
    {
        static constexpr ImGuiWindowFlags window_flag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;

        auto& io = ImGui::GetIO();
        io.MouseDrawCursor = true;

        auto [screen_size_x, screen_size_y, window_width] = calculate_frame_size();
        ImGui::SetNextWindowBgAlpha(1.0F);

        float scale_factor = screen_size_y / 1080.0f;

        ImGui::Begin("Potion Editor", &show_frame, window_flag);

        bool button_clear_filter_clicked{ false };
        if (ImGui::SmallButton("X")) {
            button_clear_filter_clicked = true;
        }
        ImGui::SameLine();

        static std::string last_filter = "";

        static auto filter_input_flags = ImGuiInputTextFlags_EscapeClearsAll;
        constexpr int filter_buf_size = 256;
        static char filter_buf[filter_buf_size] = "";
        bool filter_dirty = false;

        if (button_clear_filter_clicked) {
            filter_buf[0] = '\0';
        }

        if (last_filter != filter_buf) {
            filter_dirty = true;
        }
        last_filter = filter_buf;

        ImGui::InputTextWithHint("Filter", "Filter names containing text", filter_buf, filter_buf_size, filter_input_flags);

        ImGui::SameLine();
        if (ImGui::Checkbox("Edited Only", &filter_user_data)) {
            filter_dirty = true;
        };

        ImGui::SameLine();
        if (ImGui::Checkbox("No Predefined data", &filter_predefined_data)) {
            filter_dirty = true;
        }

        static ImGuiTableFlags flags =
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
            | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
            | ImGuiTableFlags_ScrollY;

        if (ImGui::BeginTable("List of Potions", potion_editor_column_id::column_count, flags, ImVec2(0.0f, 0.0f), 0.0f))
        {
            // Declare columns
            // We use the "user_id" parameter of TableSetupColumn() to specify a user id that will be stored in the sort specifications.
            // This is so our sort function can identify a column given our own identifier. We could also identify them based on their index!
            // Demonstrate using a mixture of flags among available sort-related flags:
            // - ImGuiTableColumnFlags_DefaultSort
            // - ImGuiTableColumnFlags_NoSort / ImGuiTableColumnFlags_NoSortAscending / ImGuiTableColumnFlags_NoSortDescending
            // - ImGuiTableColumnFlags_PreferSortAscending / ImGuiTableColumnFlags_PreferSortDescending
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortAscending | ImGuiTableColumnFlags_WidthFixed, 0.0f, potion_editor_column_id::column_id_ID);
            ImGui::TableSetupColumn("Plugin", ImGuiTableColumnFlags_WidthFixed, 0.0f, potion_editor_column_id::column_id_Plugin);
            ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 0.0f, potion_editor_column_id::column_id_Icon);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.0f, potion_editor_column_id::column_id_Name);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 0.0f, potion_editor_column_id::column_id_Type);
            ImGui::TableSetupColumn("Edit", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 0.0f, potion_editor_column_id::column_id_Edit);
            ImGui::TableSetupColumn("Reset", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 0.0f, potion_editor_column_id::column_id_Reset);
            ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
            ImGui::TableHeadersRow();

            // Sort our data if sort specs have been changed!
            if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
                if (sort_specs->SpecsDirty)
                {
                    s_current_sort_specs = sort_specs;
                    std::sort(list_of_entries.begin(), list_of_entries.end(), compare_entries_for_sort);
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
            clipper.Begin(static_cast<int>(list_of_entries_filtered.size()));
            while (clipper.Step())
            {
                for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
                {
                    // Display a data item
                    const RE::TESForm* item = list_of_entries_filtered[row_n];
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
                        ImGui::TextUnformatted("<Dynamic Form>");
                    }

                    ImGui::TableNextColumn();
                    RenderManager::draw_skill(item->GetFormID(), static_cast<int>(std::round(40.0f * scale_factor)));

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(item->GetName());
                    //ImGui::TableNextColumn();
                    //ImGui::SmallButton("None");

                    ImGui::TableNextColumn();
                    std::string type_text = "Potion";
                    if (item->GetFormType() == RE::FormType::AlchemyItem) {
                        auto alch = item->As<RE::AlchemyItem>();
                        if (alch) {
                            if (alch->IsPoison()) {
                                type_text = "Poison";
                            }
                            else if (alch->IsFood()) {
                                type_text = "Food";
                            }
                        }
                    }
                    ImGui::TextUnformatted(type_text.c_str());


                    ImGui::TableNextColumn();
                    if (ImGui::SmallButton("Edit")) {
                        button_edit_clicked = row_n;
                    }

                    ImGui::TableNextColumn();

                    if (GameData::user_custom_entry_info.contains(item->GetFormID())) {
                        if (ImGui::SmallButton("Reset")) {
                            ImGui::OpenPopup("Reset?");
                        }

                        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

                        if (ImGui::BeginPopupModal("Reset?", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
                        {
                            ImGui::Text("Reset all edits to '%s'.", item->GetName());
                            ImGui::Separator();

                            if (ImGui::Button("Ok", ImVec2(120, 0))) {
                                if (GameData::user_custom_entry_info.contains(item->GetFormID())) {
                                    GameData::user_custom_entry_info.erase(item->GetFormID());
                                }
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::SetItemDefaultFocus();
                            ImGui::SameLine();
                            if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                            ImGui::EndPopup();
                        }
                    }

                    ImGui::PopID();
                }
            }
            ImGui::EndTable();

            if (button_edit_clicked > -1) {
                if (button_edit_clicked >= 0 && button_edit_clicked < list_of_entries_filtered.size())
                {
                    edit_form = list_of_entries_filtered[button_edit_clicked];
                    current_edit_data = GameData::User_custom_entry(edit_form->GetFormID());

                    if (GameData::user_custom_entry_info.contains(edit_form->GetFormID())) {
                        //also set custom icon if present
                        auto& user_dat = GameData::user_custom_entry_info.at(edit_form->GetFormID());
                        if (user_dat.has_icon_data()) {
                            current_edit_data->m_icon_form = user_dat.m_icon_form;
                            current_edit_data->m_icon_str = user_dat.m_icon_str;
                        }
                    }
                }
            }
        }

        ImGui::End();
    }
}