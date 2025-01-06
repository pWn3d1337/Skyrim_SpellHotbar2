#include "spell_edit_dialog.h"
#include "spell_editor.h"
#include "imgui.h"
#include "../rendering/render_manager.h"
#include "../rendering/texture_csv_loader.h"

namespace SpellHotbar::SpellEditor {

    bool initialized{ false };
    bool custom_casteffect { false };
    bool custom_gcd { false };
    bool custom_cd { false };
    bool custom_casttime { false };
    bool custom_anim { false };
    bool custom_anim2 { false };

    void check_init(GameData::User_custom_spelldata& data, GameData::Spell_cast_data& dat_unfilled) {
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

    void close() {
        initialized = false;
        custom_casteffect = false;
        custom_gcd = false;
        custom_cd = false;
        custom_casttime = false;
        custom_anim = false;
        custom_anim2 = false;

        closeEditDialog();
    }

	void SpellHotbar::SpellEditor::drawEditDialog(const RE::TESForm* form, GameData::User_custom_spelldata &data, GameData::Spell_cast_data& dat_filled, GameData::Spell_cast_data& dat_unfilled, GameData::Spell_cast_data& dat_saved)
	{
        static constexpr ImGuiWindowFlags window_flag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground;
        check_init(data, dat_unfilled);

        static RE::FormID last_tooltip = 0;
        static std::string description = "";

        auto& dat = data.m_spell_data;

        auto& io = ImGui::GetIO();
        io.MouseDrawCursor = true;

        RenderManager::calculate_frame_size(0.775f);
        RenderManager::draw_frame_bg(nullptr);

        auto [screen_size_x, screen_size_y, window_width] = RenderManager::calculate_frame_size(0.75f);
        ImGui::SetNextWindowBgAlpha(0.0F);

        float scale_factor = screen_size_y / 1080.0f;

        const RE::SpellItem* spell = nullptr;
        if (form->GetFormType() == RE::FormType::Spell) {
            spell = form->As<RE::SpellItem>();
        }

        const RE::TESShout* shout = nullptr;
        if (form->GetFormType() == RE::FormType::Shout) {
            shout = form->As<RE::TESShout>();
        }
        if (form->GetFormID() != last_tooltip) {
            description = RenderManager::get_skill_tooltip(form);
        }
        last_tooltip = form->GetFormID();

        RenderManager::ImGui_push_title_style();
        ImGui::Begin("Edit Spell Data", nullptr, window_flag);
        RenderManager::ImGui_pop_title_style();
        ImGui::BeginChild("##spell_data_editor", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_None);

        static ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
            | ImGuiTableFlags_ScrollY;

        //calc button height:
        RenderManager::set_large_font();
        float button_height = ImGui::CalcTextSize("Cancel").y + ImGui::GetStyle().FramePadding.y * 2.0f + ImGui::GetStyle().ItemSpacing.y * 2.0f;
        RenderManager::revert_font();
        float child_window_height = ImGui::GetContentRegionAvail().y - button_height;

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
            ImGui::PopID();

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
            ImGui::PopID();

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Description");
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
            ImGui::TextUnformatted(description.c_str());
            ImGui::PopStyleColor();
            ImGui::PopID();

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
            ImGui::PopID();

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("FormID");
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
            ImGui::Text("%08x", form->GetFormID());
            ImGui::PopStyleColor();
            ImGui::PopID();

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Type");
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
            bool is_greater_power{ false };
            bool is_shout{ false };
            if (spell) {
                switch (spell->GetSpellType()) {
                case RE::MagicSystem::SpellType::kSpell:
                    ImGui::TextUnformatted("Spell");
                    break;
                case RE::MagicSystem::SpellType::kPower:
                    ImGui::TextUnformatted("Greater Power");
                    is_greater_power = true;
                    break;
                case RE::MagicSystem::SpellType::kLesserPower:
                    ImGui::TextUnformatted("Lesser Power");
                    break;
                default:
                    ImGui::TextUnformatted("???");
                }
            }
            else if (shout) {
                is_shout = true;
                if (shout->formFlags & RE::TESShout::RecordFlags::kTreatSpellsAsPowers) {
                    ImGui::TextUnformatted("Shout (Power)");
                    is_greater_power = true;
                }
                else {
                    ImGui::TextUnformatted("Shout");
                }
            }
            else {
                ImGui::TextUnformatted("???");
            }
            ImGui::PopStyleColor();
            ImGui::PopID();

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
            ImGui::PopID();

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
            ImGui::PopID();

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Cast Effect");
            ImGui::TableNextColumn();

            if (spell && spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {

                ImGui::Checkbox("##chk_casteffect", &custom_casteffect);
                ImGui::SameLine();

                if (!custom_casteffect) {
                    ImGui::BeginDisabled();
                    dat.casteffectid = dat_filled.casteffectid;
                }

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
                if (!custom_casteffect) ImGui::EndDisabled();

            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
                ImGui::TextUnformatted("No Effect");
                ImGui::PopStyleColor();
            }
            ImGui::PopID();

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Globald Cooldown");
            ImGui::TableNextColumn();

            //disabled for now
            bool custom_gcd_enabled{ false };
            if (custom_gcd_enabled) {
                ImGui::Checkbox("##chk_gcd", &custom_gcd);
                ImGui::SameLine();
                if (custom_gcd)
                {
                    dat.gcd = std::clamp(dat.gcd, 0.0f, 30.0f);
                    constexpr float inc{ 0.5f };
                    constexpr float inc_fast{ 5.0f };
                    const std::string format{ "%.2fs" };
                    if (ImGui::InputScalar("", ImGuiDataType_Float, &dat.gcd, &inc, &inc_fast, format.c_str())) {
                        dat.gcd = std::clamp(dat.gcd, 0.0f, 30.0f);
                    }
                }
            }
            else {
                dat.gcd = -1.0f;
                ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
                ImGui::Text("%.2fs", dat_filled.gcd);
                ImGui::PopStyleColor();
            }
            ImGui::PopID();

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Cooldown");
            ImGui::TableNextColumn();

            bool no_cd = true;
            static float input_cd_value{ 0.0f }; //in seconds
            constexpr float seconds_to_days = 1.0f / (60.0f * 60.0f * 24.0f);
            if (!is_greater_power && !is_shout) {
                ImGui::Checkbox("##chk_cd", &custom_cd);
                ImGui::SameLine();
                if (custom_cd)
                {
                    no_cd = false;
                    input_cd_value = std::clamp(input_cd_value, 0.0f, 4320.0f);
                    constexpr float inc{ 1.f };
                    constexpr float inc_fast{ 10.0f };
                    const std::string format{ "%.1fs" };
                    if (ImGui::InputScalar("", ImGuiDataType_Float, &input_cd_value, &inc, &inc_fast, format.c_str())) {
                        input_cd_value = std::clamp(input_cd_value, 0.0f, 4320.0f);
                        dat.cooldown = input_cd_value * seconds_to_days;
                    }
                }
            }
            if (no_cd) {
                dat.cooldown = -1.0f;
                ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
                ImGui::Text("%.1fs", dat_filled.cooldown * (1.0f/seconds_to_days));
                ImGui::PopStyleColor();
            }
            ImGui::PopID();

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Cast Time");
            ImGui::TableNextColumn();

            bool no_ct = true;
            if (spell && spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                ImGui::Checkbox("##chk_ct", &custom_casttime);
                ImGui::SameLine();
                if (custom_casttime)
                {
                    no_ct = false;
                    dat.casttime = std::clamp(dat.casttime, 0.25f, 10.0f);
                    constexpr float inc{ 0.05f };
                    constexpr float inc_fast{ 0.5f };
                    const std::string format{ "%.2fs" };
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
            ImGui::PopID();

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Animation");
            ImGui::TableNextColumn();
            
            if (spell && spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {

                ImGui::Checkbox("##chk_anim", &custom_anim);
                ImGui::SameLine();

                if (!custom_anim) {
                    ImGui::BeginDisabled();
                    dat.animation = dat_filled.animation;
                }

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

                if (!custom_anim) {
                    ImGui::EndDisabled();
                }
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
                ImGui::Text("%d", -1);
                ImGui::PopStyleColor();
            }
            ImGui::PopID();

            ImGui::PushID(id++);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Animation2");
            ImGui::TableNextColumn();

            if (spell && spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {

                ImGui::Checkbox("##chk_anim2", &custom_anim2);
                ImGui::SameLine();

                if (!custom_anim2) {
                    ImGui::BeginDisabled();
                    dat.animation2 = dat_filled.animation2;
                }

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
                if (!custom_anim2) {
                    ImGui::EndDisabled();
                }
            } 
            else {
                ImGui::PushStyleColor(ImGuiCol_Text, col_gray);
                ImGui::Text("%d", -1);
                ImGui::PopStyleColor();
            }
            ImGui::PopID();

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
                    else if(!icon_str.empty()){
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
        bool dat_diff = data.has_different_data(dat_saved);
        bool icon_change = data.has_icon_data();
        //logger::info("Changes: {} {}", dat_diff, icon_change);

        save_enabled = dat_diff || icon_change;

        RenderManager::set_large_font();

        if (!save_enabled) ImGui::BeginDisabled();
        if (ImGui::Button("Save")) {
            //save changes

            if (data.has_different_data(dat_unfilled) || data.has_icon_data()) {
                GameData::user_spell_cast_info.insert_or_assign(data.m_form_id, data);
            }
            else {
                if (GameData::user_spell_cast_info.contains(data.m_form_id)) {
                    GameData::user_spell_cast_info.erase(data.m_form_id);
                }
            }
            close();
        }
        if (!save_enabled) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            close();
        }
        RenderManager::revert_font();
        ImGui::EndChild();
        ImGui::End();

        RenderManager::draw_custom_mouse_cursor();
	}

}
