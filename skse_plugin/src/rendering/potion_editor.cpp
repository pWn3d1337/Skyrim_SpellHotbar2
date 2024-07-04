#include "potion_editor.h"

namespace SpellHotbar::PotionEditor {
	bool show_frame = false;

    bool filter_predefined_data = false;
    bool filter_user_data = false;

    const RE::TESForm* edit_form = nullptr;

    std::vector<RE::TESForm*> list_of_entries;
    std::vector<RE::TESForm*> list_of_entries_filtered;

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
        }
    }

    void hide()
    {
        show_frame = false;

        edit_form = nullptr;

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
            //TODO
            //drawEditDialog(edit_form);
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



            ImGui::EndTable();
        }

        ImGui::End();
    }
}