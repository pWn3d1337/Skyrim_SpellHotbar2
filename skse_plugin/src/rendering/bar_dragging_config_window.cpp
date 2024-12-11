#include "bar_dragging_config_window.h"
#include "imgui.h"
#include "../bar/hotbars.h"

namespace SpellHotbar::BarDraggingConfigWindow {


/*
* return screen_size_x, screen_size_y, window_width
*/
inline std::tuple<float, float, float> calculate_frame_size()
{
    auto& io = ImGui::GetIO();
    const float screen_size_x = io.DisplaySize.x, screen_size_y = io.DisplaySize.y;

    float frame_height = screen_size_y * 0.35f;
    float frame_width = frame_height * 16.0f / 9.0f;

    ImGui::SetNextWindowSize(ImVec2(frame_width, frame_height));
    ImGui::SetNextWindowPos(ImVec2((screen_size_x - frame_width) * 0.5f, (screen_size_y - frame_height) * 0.5f));

    return std::make_tuple(screen_size_x, screen_size_y, frame_width);
}

/*
* return screen_size_x, screen_size_y, window_width
*/
inline std::tuple<float, float, float> calculate_info_frame_size()
{
    auto& io = ImGui::GetIO();
    const float screen_size_x = io.DisplaySize.x, screen_size_y = io.DisplaySize.y;

    float frame_height = screen_size_y * 0.2f;
    float frame_width = frame_height * 16.0f / 9.0f;

    ImGui::SetNextWindowSize(ImVec2(frame_width, frame_height));
    ImGui::SetNextWindowPos(ImVec2((screen_size_x - frame_width) * 0.5f, (screen_size_y - frame_height) * 0.6f));

    return std::make_tuple(screen_size_x, screen_size_y, frame_width);
}

inline std::string get_layout_text(Bars::bar_layout layout) {
    switch (layout) {
    case Bars::bar_layout::BARS:
        return "Bar";
    case Bars::bar_layout::CIRCLE:
        return "Circle";
    case Bars::bar_layout::CROSS:
        return "Cross";
    }
    return "?";
}

const std::array<std::string, 9> anchor_names = {
        "BOTTOM",
        "LEFT",
        "TOP",
        "RIGHT",
        "BOTTOM_LEFT",
        "TOP_LEFT",
        "BOTTOM_RIGHT",
        "TOP_RIGHT",
        "CENTER"
    };

std::string get_anchor_text(Bars::anchor_point anchor) {
    size_t anchor_idx = static_cast<size_t>(anchor);
    if (anchor_idx < anchor_names.size()) {
        return anchor_names[anchor_idx];
    }
    else {
        return "?";
    }
}

void draw_window()
{
    static constexpr ImGuiWindowFlags window_flag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;

    auto& io = ImGui::GetIO();
    io.MouseDrawCursor = true;

    auto [screen_size_x, screen_size_y, window_width] = calculate_frame_size();
    ImGui::SetNextWindowBgAlpha(1.0F);

    //float scale_factor = screen_size_y / 1080.0f;

    ImGui::Begin("Bar Settings", nullptr, window_flag);

    static ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
        | ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("tab_bar_settings", 2, flags, ImVec2(0.0f, 0.0f), 0.0f))
    {
        // Declare columns
        // We use the "user_id" parameter of TableSetupColumn() to specify a user id that will be stored in the sort specifications.
        // This is so our sort function can identify a column given our own identifier. We could also identify them based on their index!
        // Demonstrate using a mixture of flags among available sort-related flags:
        // - ImGuiTableColumnFlags_DefaultSort
        // - ImGuiTableColumnFlags_NoSort / ImGuiTableColumnFlags_NoSortAscending / ImGuiTableColumnFlags_NoSortDescending
        // - ImGuiTableColumnFlags_PreferSortAscending / ImGuiTableColumnFlags_PreferSortDescending
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, 0);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch, 0.0f, 1);

        int id{ 0 };
        ImGui::PushID(id++);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Layout");
        ImGui::TableNextColumn();

        std::string _layout_text = get_layout_text(Bars::layout);
        const char* preview_text_layout = _layout_text.c_str();

        if (ImGui::BeginCombo("##cbo_layout", preview_text_layout, 0))
        {
            for (uint16_t n = 0; n < 3; n++)
            {
                Bars::bar_layout layout = Bars::bar_layout(n);

                const bool is_selected = layout == Bars::layout;
                std::string _text = get_layout_text(layout);
                if (ImGui::Selectable(_text.c_str(), is_selected)) {
                    Bars::layout = Bars::bar_layout(n);
                }

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::PushID(id++);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Anchor Point");
        ImGui::TableNextColumn();

        std::string _anchor_text = get_anchor_text(Bars::bar_anchor_point);
        const char* preview_text_anchor = _anchor_text.c_str();

        if (ImGui::BeginCombo("##cbo_anchor", preview_text_anchor, 0))
        {
            for (uint16_t n = 0; n < anchor_names.size(); n++)
            {
                Bars::anchor_point anchor = Bars::anchor_point(n);

                const bool is_selected = anchor == Bars::bar_anchor_point;
                std::string _text = get_anchor_text(anchor);
                if (ImGui::Selectable(_text.c_str(), is_selected)) {
                    Bars::bar_anchor_point = Bars::anchor_point(n);
                }

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::PushID(id++);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Icon Size");
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("TODO: Slider Size");

        ImGui::PushID(id++);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("Icon Spacing");
        ImGui::TableNextColumn();
        ImGui::TextUnformatted("TODO: Slider Spacing");

        ImGui::EndTable();
    }

    ImGui::End();
}

void draw_info()
{
    static constexpr ImGuiWindowFlags window_flag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground;

    auto& io = ImGui::GetIO();
    io.MouseDrawCursor = true;

    auto [screen_size_x, screen_size_y, window_width] = calculate_info_frame_size();
    ImGui::SetNextWindowBgAlpha(1.0F);

    //float scale_factor = screen_size_y / 1080.0f;

    ImGui::Begin("##drag_info", nullptr, window_flag);
    ImGui::TextUnformatted("ESC: finish dragging");
    ImGui::TextUnformatted("TAB: leave menus while still dragging");
    ImGui::TextUnformatted("Mouse Wheel: Icon Size");
    ImGui::TextUnformatted("ALT + Mouse Wheel: Spacing");
    ImGui::End();
}

}