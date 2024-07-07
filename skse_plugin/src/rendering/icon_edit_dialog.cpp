#include "icon_edit_dialog.h"
#include "potion_editor.h"
#include "imgui.h"
#include "texture_csv_loader.h"

namespace SpellHotbar::PotionEditor {

    /*
    * return screen_size_x, screen_size_y, window_width
    */
    inline std::tuple<float, float, float> calculate_frame_size()
    {
        auto& io = ImGui::GetIO();
        const float screen_size_x = io.DisplaySize.x, screen_size_y = io.DisplaySize.y;

        float frame_height = screen_size_y * 0.75f;
        float frame_width = frame_height * 16.0f / 9.0f;

        ImGui::SetNextWindowSize(ImVec2(frame_width, frame_height));
        ImGui::SetNextWindowPos(ImVec2((screen_size_x - frame_width) * 0.5f, (screen_size_y - frame_height) * 0.5f));

        return std::make_tuple(screen_size_x, screen_size_y, frame_width);
    }

    void close() {
        PotionEditor::closeEditDialog();
    }

    void drawEditDialog(const RE::TESForm* form, GameData::User_custom_entry& data)
    {
        static constexpr ImGuiWindowFlags window_flag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;

        auto& io = ImGui::GetIO();
        io.MouseDrawCursor = true;

        auto [screen_size_x, screen_size_y, window_width] = calculate_frame_size();
        ImGui::SetNextWindowBgAlpha(1.0F);

        float scale_factor = screen_size_y / 1080.0f;

        const RE::AlchemyItem* alchitem = nullptr;
        if (form->GetFormType() == RE::FormType::AlchemyItem) {
            alchitem = form->As<RE::AlchemyItem>();
        }

        ImGui::Begin("Edit Icon", nullptr, window_flag);

        static ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
            | ImGuiTableFlags_ScrollY;

        //constexpr int child_window_height = 500;
        float child_window_height = ImGui::GetContentRegionAvail().y * 0.9f;

        constexpr ImU32 col_gray = IM_COL32(127, 127, 127, 255);

        ImGui::BeginChild("LeftTab", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, child_window_height), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (ImGui::BeginTable("Data", 2, flags, ImVec2(0.0f, 0.0f), 0.0f))
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
            ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
            ImGui::TableHeadersRow();

            int id{ 0 };
            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Name");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(form->GetName());

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Icon");
            ImGui::TableNextColumn();

            ImVec2 iconpos = ImGui::GetCursorScreenPos();
            float ic_size = std::round(60.0f * scale_factor);
            ImGui::Dummy(ImVec2(ic_size, ic_size));

            bool show_reset_button{ false };
            if (data.m_icon_form > 0) {
                SpellHotbar::RenderManager::draw_skill_in_editor(data.m_icon_form, iconpos, static_cast<int>(ic_size));
                show_reset_button = true;
            }
            else if (!data.m_icon_str.empty()) {
                if (TextureCSVLoader::default_icon_names.contains(data.m_icon_str)) {
                    auto type = TextureCSVLoader::default_icon_names.at(data.m_icon_str);
                    SpellHotbar::RenderManager::draw_default_icon_in_editor(type, iconpos, static_cast<int>(ic_size));
                }
                else {
                    SpellHotbar::RenderManager::draw_extra_icon_in_editor(data.m_icon_str, iconpos, static_cast<int>(ic_size));
                }
                show_reset_button = true;
            }
            else {
                //default
                SpellHotbar::RenderManager::draw_skill_in_editor(form->GetFormID(), iconpos, static_cast<int>(ic_size));
            }

            if (show_reset_button) {
                ImGui::SameLine();
                if (ImGui::Button("Reset##icon")) {
                    data.m_icon_form = 0;
                    data.m_icon_str = "";
                }
            }

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("File");
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
            auto file = form->GetFile(0);
            if (file != nullptr) {
                ImGui::TextUnformatted(file->fileName);
            }
            else {
                ImGui::TextUnformatted("<Dynamic Form>");
            }
            ImGui::PopStyleColor();

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("FormID");
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
            ImGui::Text("%08x", form->GetFormID());
            ImGui::PopStyleColor();

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Type");
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, col_gray);

            if (alchitem != nullptr) {
                if (alchitem->IsPoison()) {
                    ImGui::TextUnformatted("Poison");
                }
                else if (alchitem->IsFood()) {
                    ImGui::TextUnformatted("Food");
                }
                else {
                    ImGui::TextUnformatted("Potion");
                }
            }
            else {
                ImGui::TextUnformatted("???");
            }
            ImGui::PopStyleColor();

            ImGui::EndTable();
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        ImGui::BeginChild("RighttTab", ImVec2(0, child_window_height), false, ImGuiWindowFlags_HorizontalScrollbar);

        ImGuiStyle& style = ImGui::GetStyle();

        float icon_button_size = 60 * scale_factor;

        float inner_pad = std::max(icon_button_size * 0.02f, 1.0f);

        ImVec2 button_sz(icon_button_size, icon_button_size);
        float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;


        auto icon_list = RenderManager::get_editor_icon_list();

        int file_id = 0;
        for (auto& file : icon_list) {

            auto& name = std::get<0>(file);
            auto& file_icons = std::get<1>(file);

            if (ImGui::CollapsingHeader(name.c_str())) {
                //ImGui::SeparatorText(name.c_str());

                for (int n = 0; n < file_icons.size(); n++)
                {
                    ImGui::PushID(file_id + n);

                    RE::FormID formid = std::get<0>(file_icons.at(n));
                    const std::string& icon_str = std::get<1>(file_icons.at(n));

                    ImVec2 bpos = ImGui::GetCursorScreenPos();
                    std::string button_label = "##" + name + std::to_string(n);
                    if (ImGui::Button(button_label.c_str(), button_sz)) {
                        if (formid > 0) {
                            data.m_icon_form = formid;
                            data.m_icon_str = "";
                        }
                        else if (!icon_str.empty()) {
                            data.m_icon_str = icon_str;
                            data.m_icon_form = 0;
                        }
                    }
                    ImVec2 inner_pos{ bpos.x + inner_pad, bpos.y + inner_pad };

                    int icon_size = static_cast<int>(icon_button_size - 2.0f * inner_pad);
                    if (formid > 0) {
                        SpellHotbar::RenderManager::draw_skill_in_editor(formid, inner_pos, icon_size);
                    }
                    else if (!icon_str.empty()) {
                        if (TextureCSVLoader::default_icon_names.contains(icon_str)) {
                            auto type = TextureCSVLoader::default_icon_names.at(icon_str);
                            SpellHotbar::RenderManager::draw_default_icon_in_editor(type, inner_pos, icon_size);
                        }
                        else {
                            SpellHotbar::RenderManager::draw_extra_icon_in_editor(icon_str, inner_pos, icon_size);
                        }
                    }

                    float last_button_x2 = ImGui::GetItemRectMax().x;
                    float next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x; // Expected position if next button was on same line
                    if (n + 1 < file_icons.size() && next_button_x2 < window_visible_x2)
                        ImGui::SameLine();
                    ImGui::PopID();
                }
            }
            file_id += 100000;
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();

        bool save_enabled{ false };

        //Check if there is something to save
        bool icon_change = data.has_icon_data();
        //logger::info("Changes: {} {}", dat_diff, icon_change);

        save_enabled = icon_change;

        if (!save_enabled) ImGui::BeginDisabled();
        if (ImGui::Button("Save")) {
            //save changes

            if (data.has_icon_data()) {
                GameData::user_custom_entry_info.insert_or_assign(data.m_form_id, data);
            }
            else {
                if (GameData::user_custom_entry_info.contains(data.m_form_id)) {
                    GameData::user_custom_entry_info.erase(data.m_form_id);
                }
            }
            close();
        }
        if (!save_enabled) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            close();
        }

        ImGui::End();
    }

}