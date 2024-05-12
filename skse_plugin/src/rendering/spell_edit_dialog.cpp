#include "spell_edit_dialog.h"
#include "spell_editor.h"
#include "imgui.h"
#include "../rendering/render_manager.h"

namespace SpellHotbar::SpellEditor {

    /*
    * return screen_size_x, screen_size_y, window_width
    */
    inline std::tuple<float, float, float> calculate_frame_size()
    {
        auto& io = ImGui::GetIO();
        const float screen_size_x = io.DisplaySize.x, screen_size_y = io.DisplaySize.y;

        /*
        * Fill 95% of screen, with 4:3 dimensions, centered on screen
        */
        float frame_height = screen_size_y * 0.5f;
        float frame_width = frame_height * 4.0f / 3.0f;

        ImGui::SetNextWindowSize(ImVec2(frame_width, frame_height));
        ImGui::SetNextWindowPos(ImVec2((screen_size_x - frame_width) * 0.5f, (screen_size_y - frame_height) * 0.5f));

        return std::make_tuple(screen_size_x, screen_size_y, frame_width);
    }

	void SpellHotbar::SpellEditor::drawEditDialog(const RE::TESForm* form, GameData::Spell_cast_data& dat, GameData::Spell_cast_data& dat_filled, GameData::Spell_cast_data& dat_saved)
	{
        static constexpr ImGuiWindowFlags window_flag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;

        auto& io = ImGui::GetIO();
        io.MouseDrawCursor = true;

        auto [screen_size_x, screen_size_y, window_width] = calculate_frame_size();
        ImGui::SetNextWindowBgAlpha(1.0F);

        float scale_factor = screen_size_y / 1080.0f;

        const RE::SpellItem* spell = nullptr;
        if (form->GetFormType() == RE::FormType::Spell) {
            spell = form->As<RE::SpellItem>();
        }

        const RE::TESShout* shout = nullptr;
        if (form->GetFormType() == RE::FormType::Shout) {
            shout = form->As<RE::TESShout>();
        }

        ImGui::Begin("Edit Spell Data", nullptr, window_flag);

        static ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
            | ImGuiTableFlags_ScrollY;

        constexpr int child_window_height = 500;

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
            SpellHotbar::RenderManager::draw_skill_in_editor(form->GetFormID(), iconpos, static_cast<int>(ic_size));

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("File");
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
            ImGui::TextUnformatted(form->GetFile(0)->fileName);
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
            if (spell) {
                switch (spell->GetSpellType()) {
                case RE::MagicSystem::SpellType::kSpell:
                    ImGui::TextUnformatted("Spell");
                    break;
                case RE::MagicSystem::SpellType::kPower:
                    ImGui::TextUnformatted("Greater Power");
                    break;
                case RE::MagicSystem::SpellType::kLesserPower:
                    ImGui::TextUnformatted("Lesser Power");
                    break;
                default:
                    ImGui::TextUnformatted("???");
                }
            }
            else if (shout) {
                if (shout->formFlags & RE::TESShout::RecordFlags::kTreatSpellsAsPowers) {
                    ImGui::TextUnformatted("Shout (Power)");
                }
                else {
                    ImGui::TextUnformatted("Shout");
                }
            }
            else {
                ImGui::TextUnformatted("???");
            }
            ImGui::PopStyleColor();

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Delivery");
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
            if (spell) {
                switch (spell->GetDelivery()) {
                case RE::MagicSystem::Delivery::kSelf:
                    ImGui::TextUnformatted("Self");
                    break;
                case RE::MagicSystem::Delivery::kTouch:
                    ImGui::TextUnformatted("Touch");
                    break;
                case RE::MagicSystem::Delivery::kAimed:
                    ImGui::TextUnformatted("Aimed");
                    break;
                case RE::MagicSystem::Delivery::kTargetActor:
                    ImGui::TextUnformatted("Target Actor");
                    break;
                case RE::MagicSystem::Delivery::kTargetLocation:
                    ImGui::TextUnformatted("Target Location");
                    break;
                default:
                    ImGui::TextUnformatted("???");
                }
            }
            else {
                ImGui::TextUnformatted("???");
            }
            ImGui::PopStyleColor();


            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Casting Type");
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
            if (spell) {
                switch (spell->GetCastingType()) {
                case RE::MagicSystem::CastingType::kConstantEffect:
                    ImGui::TextUnformatted("Constant Effect");
                    break;
                case RE::MagicSystem::CastingType::kFireAndForget:
                    ImGui::TextUnformatted("Fire And Forget");
                    break;
                case RE::MagicSystem::CastingType::kConcentration:
                    ImGui::TextUnformatted("Concentration");
                    break;
                case RE::MagicSystem::CastingType::kScroll:
                    ImGui::TextUnformatted("Scroll");
                    break;
                default:
                    ImGui::TextUnformatted("???");
                }
            }
            else {
                ImGui::TextUnformatted("???");
            }
            ImGui::PopStyleColor();

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Cast Effect");
            ImGui::TableNextColumn();

            //ImGui::Combo("", &dat.casteffectid, [](void* data, int n) { return ((const char**)data)[n]; }, GameData::spell_casteffect_art, GameData::spell_casteffect_art.size());

            //GameData::get_cast_effect_id
            if (spell && spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                const char* preview_text = nullptr;
                if (dat.casteffectid >= 0 && dat.casteffectid < GameData::spell_casteffect_art.size()) {
                    auto& effect_dat = GameData::spell_casteffect_art.at(dat.casteffectid);
                    preview_text = std::get<2>(effect_dat).c_str();
                }

                if (ImGui::BeginCombo("##cbo_cast_effect", preview_text, 0))
                {
                    for (uint16_t n = 0; n < GameData::spell_casteffect_art.size(); n++)
                    {
                        const bool is_selected = (dat.casteffectid == n);
                        auto& effect_dat = GameData::spell_casteffect_art.at(n);
                        if (ImGui::Selectable(std::get<2>(effect_dat).c_str(), is_selected)) {
                            dat.casteffectid = static_cast<uint16_t>(n);
                        }

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
                ImGui::TextUnformatted("No Effect");
                ImGui::PopStyleColor();
            }

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Globald Cooldown");
            ImGui::TableNextColumn();
            //ImGui::TextUnformatted("TODO");

            static bool custom_gcd = false;
            ImGui::Checkbox("##chk_gcd", &custom_gcd);
            ImGui::SameLine();
            if (custom_gcd)
            {
                constexpr float inc{ 0.5f };
                constexpr float inc_fast{ 5.0f };
                constexpr std::string format{ "%.2fs" };
                if (ImGui::InputScalar("", ImGuiDataType_Float, &dat.gcd, &inc, &inc_fast, format.c_str())) {
                    dat.gcd = std::clamp(dat.gcd, 0.0f, 30.0f);
                }
            }
            else {
                dat.gcd = 0.0f;
                ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
                ImGui::Text("%.2fs", dat_filled.gcd);
                ImGui::PopStyleColor();
            }

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Cooldown");
            ImGui::TableNextColumn();

            static bool custom_cd = false;
            ImGui::Checkbox("##chk_cd", &custom_cd);
            ImGui::SameLine();
            if (custom_cd)
            {
                constexpr float inc{ 1.f };
                constexpr float inc_fast{ 10.0f };
                constexpr std::string format{ "%.1fs" };
                if (ImGui::InputScalar("", ImGuiDataType_Float, &dat.cooldown, &inc, &inc_fast, format.c_str())) {
                    dat.cooldown = std::clamp(dat.cooldown, 0.0f, 3600.0f);
                }
            }
            else {
                dat.cooldown = 0.0f;
                ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
                ImGui::Text("%.1fs", dat_filled.cooldown);
                ImGui::PopStyleColor();
            }

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Cast Time");
            ImGui::TableNextColumn();

            bool no_ct = true;
            if (spell && spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                static bool custom_casttime = false;
                ImGui::Checkbox("##chk_ct", &custom_casttime);
                ImGui::SameLine();
                if (custom_casttime)
                {
                    no_ct = false;
                    dat.casttime = std::clamp(dat.casttime, 0.25f, 10.0f);
                    constexpr float inc{ 0.05f };
                    constexpr float inc_fast{ 0.5f };
                    constexpr std::string format{ "%.2fs" };
                    if (ImGui::InputScalar("", ImGuiDataType_Float, &dat.casttime, &inc, &inc_fast, format.c_str())) {
                        dat.casttime = std::clamp(dat.casttime, 0.25f, 10.0f);
                    }
                }
            }
            if (no_ct) {
                dat.casttime = -1.0f;
                ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
                ImGui::Text("%.2fs", dat_filled.casttime);
                ImGui::PopStyleColor();
            }
            //ImGui::SliderFloat("", &dat.casttime, 0.25f, 10.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Animation");
            ImGui::TableNextColumn();
            
            if (spell && spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                const char* anim_text = nullptr;
                auto& list_of_anims = SpellEditor::get_list_of_anims();

                if (GameData::animation_names.contains(dat.animation)) {
                    auto& name = GameData::animation_names.at(dat.animation);
                    anim_text = name.c_str();
                }
                const std::string cbo_anim_label = std::to_string(dat.animation) + "##cbo_anim";

                if (ImGui::BeginCombo(cbo_anim_label.c_str(), anim_text, 0))
                {
                    for (uint16_t n = 0; n < list_of_anims.size(); n++)
                    {
                        const bool is_selected = (dat.animation == list_of_anims[n]);
                        auto& name = GameData::animation_names.at(list_of_anims[n]);

                        if (ImGui::Selectable(name.c_str(), is_selected)) {
                            dat.animation = list_of_anims[n];
                        }

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
                ImGui::Text("%d", -1);
                ImGui::PopStyleColor();
            }

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Animation2");
            ImGui::TableNextColumn();

            if (spell && spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                const char* anim_text2 = nullptr;
                auto& list_of_anims = SpellEditor::get_list_of_anims();

                if (GameData::animation_names.contains(dat.animation2)) {
                    auto& name = GameData::animation_names.at(dat.animation2);
                    anim_text2 = name.c_str();
                }
                const std::string cbo_anim2_label = std::to_string(dat.animation2) + "##cbo_anim2";

                if (ImGui::BeginCombo(cbo_anim2_label.c_str(), anim_text2, 0))
                {
                    for (uint16_t n = 0; n < list_of_anims.size(); n++)
                    {
                        const bool is_selected = (dat.animation2 == list_of_anims[n]);
                        auto& name = GameData::animation_names.at(list_of_anims[n]);

                        if (ImGui::Selectable(name.c_str(), is_selected)) {
                            dat.animation2 = list_of_anims[n];
                        }

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            } 
            else {
                ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
                ImGui::Text("%d", -1);
                ImGui::PopStyleColor();
            }

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

                        ImVec2 bpos = ImGui::GetCursorScreenPos();
                        ImGui::Button("", button_sz);
                        ImVec2 inner_pos{ bpos.x + inner_pad, bpos.y + inner_pad };
                    SpellHotbar::RenderManager::draw_skill_in_editor(formid, inner_pos, icon_button_size - 2.0f * inner_pad);

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

        if (ImGui::Button("Save")) {
            closeEditDialog();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            closeEditDialog();
        }

        ImGui::End();
	}

}
