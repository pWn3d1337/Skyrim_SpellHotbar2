#include "potion_editor.h"

#include "icon_edit_dialog.h"
#include "widgets.h"
#include "../../game_data/localization.h"

// The potion editor, ported from rendering/potion_editor.cpp and rendering/icon_edit_dialog.cpp
// to FLICK. The data half -- what the player's alchemy list holds, how it filters and
// sorts, what a save writes into GameData::user_custom_entry_info -- is the same code. The
// drawing half speaks FUCK:: and nothing else (flick_pch.h says why).
//
// What did not come across, on purpose: the nine-slice parchment background, the transparent
// title style, and the drawn cursor. The host draws the chrome and owns the cursor.
namespace SpellHotbar::FlickUi::PotionEditor {
    namespace {
        std::atomic_bool show_frame{ false };

        bool filter_predefined_data{ false };
        bool filter_user_data{ false };

        const RE::TESForm* edit_form{ nullptr };
        std::optional<GameData::User_custom_entry> current_edit_data{ std::nullopt };

        std::vector<RE::TESForm*> list_of_entries;
        std::vector<RE::TESForm*> list_of_entries_filtered;

        constexpr int filter_buf_size = 256;
        char filter_buf[filter_buf_size] = "";

        // FLICK's TableFlags stop short of these three; the C-ABI takes the int, so the real
        // ImGui bits go through as-is (bind_menu.cpp does the same).
        constexpr FUCK::TableFlags table_flag_sort_multi = static_cast<FUCK::TableFlags>(ImGuiTableFlags_SortMulti);
        constexpr FUCK::TableFlags table_flag_no_borders_in_body = static_cast<FUCK::TableFlags>(ImGuiTableFlags_NoBordersInBody);
        constexpr FUCK::TableFlags table_flag_scroll_y = static_cast<FUCK::TableFlags>(ImGuiTableFlags_ScrollY);

        enum potion_editor_column_id : int {
            column_id_ID = 0,
            column_id_Plugin,
            column_id_Icon,
            column_id_Name,
            column_id_Type,
            column_id_Edit,
            column_id_Reset,

            column_count
        };

        void update_filter(const std::string& filter_text, bool filter_predefined, bool filter_custom_dat)
        {
            if (filter_text.empty() && !filter_predefined && !filter_custom_dat) {
                list_of_entries_filtered = list_of_entries;
                return;
            }
            list_of_entries_filtered.clear();
            list_of_entries_filtered.reserve(list_of_entries.size());
            for (std::size_t i = 0U; i < list_of_entries.size(); i++) {
                bool match_text{ false };
                bool match_filter_predef{ false };
                bool match_filter_custom_dat{ false };

                if (!filter_text.empty()) {
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
                        !UiBridge::has_custom_icon(list_of_entries[i]->GetFormID()) &&
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

                if (match_text && match_filter_predef && match_filter_custom_dat) {
                    list_of_entries_filtered.emplace_back(list_of_entries[i]);
                }
            }
        }

        void load_entries()
        {
            list_of_entries.clear();
            list_of_entries_filtered.clear();

            RE::PlayerCharacter* pc = RE::PlayerCharacter::GetSingleton();
            if (pc == nullptr) {
                return;
            }

            auto refs = pc->GetInventoryCounts([](const RE::TESBoundObject& object) {
                return object.formType == RE::FormType::AlchemyItem;
            });
            for (auto& [k, v] : refs) {
                if (!GameData::user_custom_entry_info.contains(k->GetFormID())) {
                    list_of_entries.push_back(k);
                }
            }

            for (auto& [k, v] : GameData::user_custom_entry_info) {
                if (RE::TESForm* f = RE::TESForm::LookupByID(k); f != nullptr) {
                    list_of_entries.push_back(f);
                }
            }

            filter_buf[0] = '\0';
            update_filter("", filter_predefined_data, filter_user_data);
        }

        const ImGuiTableSortSpecs* s_current_sort_specs{ nullptr };
        bool compare_entries_for_sort(const RE::TESForm* lhs, const RE::TESForm* rhs)
        {
            for (int n = 0; n < s_current_sort_specs->SpecsCount; n++) {
                // Columns are identified by the ColumnUserID passed to TableSetupColumn().
                const ImGuiTableColumnSortSpecs* sort_spec = &s_current_sort_specs->Specs[n];
                bool is_less{ false };

                switch (sort_spec->ColumnUserID) {
                case potion_editor_column_id::column_id_Type:
                    is_less = static_cast<int>(lhs->GetFormType()) < static_cast<int>(rhs->GetFormType());
                    break;
                case potion_editor_column_id::column_id_Plugin:
                    is_less = lhs->GetFile(0)->GetFilename() < rhs->GetFile(0)->GetFilename();
                    break;
                case potion_editor_column_id::column_id_Name:
                    is_less = std::string(lhs->GetName()) < std::string(rhs->GetName());
                    break;
                case potion_editor_column_id::column_id_ID:
                default:
                    is_less = lhs->formID < rhs->formID;
                    break;
                }
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? is_less : !is_less;
            }
            return lhs->formID < rhs->formID;
        }

        const char* type_text_for(const RE::TESForm* a_item)
        {
            if (a_item->GetFormType() == RE::FormType::AlchemyItem) {
                if (const RE::AlchemyItem* alch = a_item->As<RE::AlchemyItem>(); alch != nullptr) {
                    if (alch->IsPoison()) {
                        return translate_c("$TYPE_POISON");
                    }
                    if (alch->IsFood()) {
                        return translate_c("$TYPE_FOOD");
                    }
                }
            }
            return translate_c("$TYPE_POTION");
        }
    }

    bool is_opened() { return show_frame.load(std::memory_order_relaxed); }

    void show()
    {
        if (!is_opened()) {
            edit_form = nullptr;
            current_edit_data.reset();
            load_entries();
            show_frame.store(true, std::memory_order_relaxed);
        }
    }

    void hide()
    {
        show_frame.store(false, std::memory_order_relaxed);
        edit_form = nullptr;
        current_edit_data.reset();
        list_of_entries.clear();
        list_of_entries_filtered.clear();
    }

    const char* title()
    {
        if (edit_form != nullptr) {
            return translate_c("$EDIT_ICON");
        }
        return translate_c("$POTION_EDITOR");
    }

    void draw()
    {
        if (edit_form != nullptr && current_edit_data.has_value()) {
            draw_edit_dialog(edit_form, current_edit_data.value());
            return;
        }
        draw_table();
    }

    void close_edit_dialog() { edit_form = nullptr; }

    void draw_table()
    {
        bool filter_dirty = false;

        // The clear button, the filter field, the two filter checkboxes, the reset-all button.
        const float bsize = std::floor(scale(30.0f));
        const ImVec2 clear_pos = FUCK::GetCursorScreenPos();
        bool button_clear_filter_clicked{ false };
        if (FUCK::InvisibleButton("##clear_filter", ImVec2(bsize, bsize))) {
            button_clear_filter_clicked = true;
        }
        const bool clear_hovered = FUCK::IsItemHovered();
        draw_default_icon_at(GameData::DefaultIconType::UNBIND_SLOT, clear_pos, bsize);
        if (clear_hovered) {
            draw_highlight_overlay(clear_pos, bsize, rgba(255, 127, 127));
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

        FUCK::PushItemWidth(FUCK::GetContentRegionAvail().x * 0.3f);
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
        const std::string reset_popup = translate("$RESET") + "##reset_popup";
        if (FUCK::Button(translate_id("$RESET_ALL").c_str())) {
            FUCK::OpenPopup(reset_popup.c_str());
        }
        if (FUCK::BeginPopupModal(reset_popup.c_str(), nullptr, FUCK::WindowFlags::kAutoResize | FUCK::WindowFlags::kNoMove)) {
            FUCK::TextUnformatted(translate("$RESET_ALL_PROMPT").c_str());
            FUCK::Separator();
            if (FUCK::Button((translate("$OK") + "##_reset_ok").c_str())) {
                GameData::user_custom_entry_info.clear();
                FUCK::CloseCurrentPopup();
            }
            FUCK::SetItemDefaultFocus();
            FUCK::SameLine();
            if (FUCK::Button((translate("$CANCEL") + "##_reset_cancel").c_str())) {
                FUCK::CloseCurrentPopup();
            }
            FUCK::EndPopup();
        }

        using TF = FUCK::TableFlags;
        const TF flags = TF::kResizable | TF::kReorderable | TF::kHideable | TF::kSortable | table_flag_sort_multi |
                         TF::kRowBg | TF::kBordersOuterH | TF::kBordersOuterV | TF::kBordersInnerV |
                         table_flag_no_borders_in_body | table_flag_scroll_y;

        if (FUCK::BeginTable("List of Potions", potion_editor_column_id::column_count, flags, ImVec2(0.0f, 0.0f), 0.0f)) {
            using CF = FUCK::TableColumnFlags;
            FUCK::TableSetupColumn(translate_id("$COLUMN_ID").c_str(), CF::kDefaultSort | CF::kPreferSortAscending | CF::kWidthFixed, 0.0f, potion_editor_column_id::column_id_ID);
            FUCK::TableSetupColumn(translate_id("$COLUMN_PLUGIN").c_str(), CF::kWidthFixed, 0.0f, potion_editor_column_id::column_id_Plugin);
            FUCK::TableSetupColumn(translate_id("$COLUMN_ICON").c_str(), CF::kWidthFixed | CF::kNoSort, 0.0f, potion_editor_column_id::column_id_Icon);
            FUCK::TableSetupColumn(translate_id("$COLUMN_NAME").c_str(), CF::kWidthStretch, 0.0f, potion_editor_column_id::column_id_Name);
            FUCK::TableSetupColumn(translate_id("$COLUMN_TYPE").c_str(), CF::kWidthFixed, 0.0f, potion_editor_column_id::column_id_Type);
            FUCK::TableSetupColumn(translate_id("$EDIT").c_str(), CF::kWidthFixed | CF::kNoSort, 0.0f, potion_editor_column_id::column_id_Edit);
            FUCK::TableSetupColumn(translate_id("$RESET").c_str(), CF::kWidthFixed | CF::kNoSort, 0.0f, potion_editor_column_id::column_id_Reset);
            FUCK::TableSetupScrollFreeze(0, 1);  // Make row always visible
            FUCK::TableHeadersRow();

            if (ImGuiTableSortSpecs* sort_specs = FUCK::GetTableSortSpecs()) {
                if (sort_specs->SpecsDirty) {
                    s_current_sort_specs = sort_specs;
                    if (!list_of_entries.empty()) {
                        std::sort(list_of_entries.begin(), list_of_entries.end(), compare_entries_for_sort);
                    }
                    sort_specs->SpecsDirty = false;

                    update_filter(filter_buf, filter_predefined_data, filter_user_data);
                    filter_dirty = false;
                }
            }

            if (filter_dirty) {
                update_filter(filter_buf, filter_predefined_data, filter_user_data);
                filter_dirty = false;
            }

            int button_edit_clicked = -1;

            const float icon_f = scale(40.0f);
            const float cell_pad_y = FUCK::GetStyleVarVec(ImGuiStyleVar_CellPadding).y;
            const float row_height = std::max(icon_f, FUCK::GetTextLineHeight()) + cell_pad_y * 2.0f;

            const auto clip = RowClipper::begin(static_cast<int>(list_of_entries_filtered.size()), row_height);
            for (int row_n = clip.first; row_n < clip.last; row_n++) {
                const RE::TESForm* item = list_of_entries_filtered[static_cast<std::size_t>(row_n)];
                FUCK::PushID(static_cast<int>(item->GetFormID()));
                FUCK::TableNextRow(0, clip.row_h);

                FUCK::TableNextColumn();
                FUCK::Text("%08X", item->GetFormID());

                FUCK::TableNextColumn();
                if (const auto* file = item->GetFile(0); file != nullptr) {
                    FUCK::TextUnformatted(file->fileName);
                }
                else {
                    FUCK::TextUnformatted(translate_c("$DYNAMIC_FORM"));
                }

                FUCK::TableNextColumn();
                draw_skill_item(item->GetFormID(), icon_f, UiBridge::skill_color(item));
                skill_tooltip(item);

                FUCK::TableNextColumn();
                FUCK::TextUnformatted(item->GetName());

                FUCK::TableNextColumn();
                FUCK::TextUnformatted(type_text_for(item));

                FUCK::TableNextColumn();
                if (FUCK::Button(translate_id("$EDIT").c_str())) {
                    button_edit_clicked = row_n;
                }

                FUCK::TableNextColumn();
                if (GameData::user_custom_entry_info.contains(item->GetFormID())) {
                    const std::string row_popup = translate("$RESET") + "?";
                    if (FUCK::Button(translate_id("$RESET").c_str())) {
                        FUCK::OpenPopup(row_popup.c_str());
                    }
                    if (FUCK::BeginPopupModal(row_popup.c_str(), nullptr, FUCK::WindowFlags::kAutoResize | FUCK::WindowFlags::kNoMove)) {
                        FUCK::Text((translate("$RESET_PROMT") + " '%s'.").c_str(), item->GetName());
                        FUCK::Separator();
                        if (FUCK::Button(translate_id("$OK").c_str())) {
                            GameData::user_custom_entry_info.erase(item->GetFormID());
                            FUCK::CloseCurrentPopup();
                        }
                        FUCK::SetItemDefaultFocus();
                        FUCK::SameLine();
                        if (FUCK::Button(translate_id("$CANCEL").c_str())) {
                            FUCK::CloseCurrentPopup();
                        }
                        FUCK::EndPopup();
                    }
                }

                FUCK::PopID();
            }
            clip.end();
            FUCK::EndTable();

            if (button_edit_clicked >= 0 && button_edit_clicked < static_cast<int>(list_of_entries_filtered.size())) {
                edit_form = list_of_entries_filtered[static_cast<std::size_t>(button_edit_clicked)];
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

    // ---- The icon override dialog (rendering/icon_edit_dialog.cpp). Drawn in place of the
    // table while an item is being edited: the item's facts on the left, the shared icon
    // picker on the right, Save / Cancel underneath. ----
    void draw_edit_dialog(const RE::TESForm* a_form, GameData::User_custom_entry& a_data)
    {
        static RE::FormID last_tooltip = 0;
        static std::string description = "";

        if (a_form->GetFormID() != last_tooltip) {
            description = UiBridge::skill_tooltip(a_form);
            last_tooltip = a_form->GetFormID();
        }

        const RE::AlchemyItem* alchitem = a_form->GetFormType() == RE::FormType::AlchemyItem
                                              ? a_form->As<RE::AlchemyItem>()
                                              : nullptr;

        push_large_font();
        const float button_height = FUCK::CalcTextSize("Cancel").y + FUCK::GetStyleVarVec(ImGuiStyleVar_FramePadding).y * 2.0f +
                                    FUCK::GetStyleVarVec(ImGuiStyleVar_ItemSpacing).y * 2.0f;
        pop_font();
        const float child_window_height = FUCK::GetContentRegionAvail().y - button_height;

        const unsigned potion_color = UiBridge::skill_color(a_form);

        FUCK::BeginChild("LeftTab", ImVec2(FUCK::GetContentRegionAvail().x * 0.5f, child_window_height), false,
                         ImGuiWindowFlags_HorizontalScrollbar);

        using TF = FUCK::TableFlags;
        const TF flags = TF::kRowBg | TF::kBordersOuterH | TF::kBordersOuterV | TF::kBordersInnerV |
                         table_flag_no_borders_in_body | table_flag_scroll_y;

        if (FUCK::BeginTable("Data", 2, flags, ImVec2(0.0f, 0.0f), 0.0f)) {
            using CF = FUCK::TableColumnFlags;
            FUCK::TableSetupColumn("", CF::kNoSort | CF::kWidthFixed, 0.0f, 0);
            FUCK::TableSetupColumn("", CF::kNoSort | CF::kWidthStretch, 0.0f, 1);
            FUCK::TableSetupScrollFreeze(0, 1);
            FUCK::TableHeadersRow();

            int id{ 0 };
            const auto label_row = [&id](const char* a_label) {
                FUCK::PushID(id++);
                FUCK::TableNextRow();
                FUCK::TableNextColumn();
                FUCK::TextUnformatted(a_label);
                FUCK::TableNextColumn();
            };
            const auto gray_text = [](const char* a_text) {
                FUCK::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                FUCK::TextUnformatted(a_text);
                FUCK::PopStyleColor(1);
            };

            label_row(translate_c("$COLUMN_NAME"));
            FUCK::TextUnformatted(a_form->GetName());
            FUCK::PopID();

            label_row(translate_c("$COLUMN_ICON"));
            const ImVec2 iconpos = FUCK::GetCursorScreenPos();
            const float ic_size = std::round(scale(60.0f));
            FUCK::Dummy(ImVec2(ic_size, ic_size));

            bool show_reset_button{ false };
            if (a_data.m_icon_form > 0) {
                draw_skill_at(a_data.m_icon_form, iconpos, ic_size, potion_color);
                show_reset_button = true;
            }
            else if (!a_data.m_icon_str.empty()) {
                // The bridge resolves a default-icon name and an extra icon alike, so the old
                // two-way split on TextureCSVLoader::default_icon_names is one call here.
                draw_custom_icon_at(0, a_data.m_icon_str, iconpos, ic_size, potion_color);
                show_reset_button = true;
            }
            else {
                draw_skill_at(a_form->GetFormID(), iconpos, ic_size, potion_color);
            }
            if (show_reset_button) {
                FUCK::SameLine();
                if (FUCK::Button((translate("$RESET") + "##reset_icon").c_str())) {
                    a_data.m_icon_form = 0;
                    a_data.m_icon_str = "";
                }
            }
            FUCK::PopID();

            label_row(translate_c("$DESCRIPTION"));
            gray_text(description.c_str());
            FUCK::PopID();

            label_row(translate_c("$FILE"));
            if (const auto* file = a_form->GetFile(0); file != nullptr) {
                gray_text(file->fileName);
            }
            else {
                gray_text(translate_c("$DYNAMIC_FORM"));
            }
            FUCK::PopID();

            label_row(translate_c("$FORM_ID"));
            {
                char id_text[16]{};
                std::snprintf(id_text, sizeof(id_text), "%08x", a_form->GetFormID());
                gray_text(id_text);
            }
            FUCK::PopID();

            label_row(translate_c("$COLUMN_TYPE"));
            if (alchitem != nullptr) {
                if (alchitem->IsPoison()) {
                    gray_text(translate_c("$TYPE_POISON"));
                }
                else if (alchitem->IsFood()) {
                    gray_text(translate_c("$TYPE_FOOD"));
                }
                else {
                    // Upstream says POISON here too; kept so the dialog reads as it did.
                    gray_text(translate_c("$TYPE_POISON"));
                }
            }
            else {
                gray_text(translate_c("$QUESTION_MARKS"));
            }
            FUCK::PopID();

            FUCK::EndTable();
        }
        FUCK::EndChild();

        FUCK::SameLine();
        FUCK::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        FUCK::BeginChild("RightTab", ImVec2(0.0f, child_window_height), false, ImGuiWindowFlags_HorizontalScrollbar);
        icon_picker("potion_icons", a_data.m_icon_form, a_data.m_icon_str, potion_color);
        FUCK::EndChild();
        FUCK::PopStyleVar(1);

        // Nothing to save unless an icon override is set.
        const bool save_enabled = a_data.has_icon_data();

        push_large_font();
        if (!save_enabled) {
            FUCK::BeginDisabled();
        }
        if (FUCK::Button(translate_id("$SAVE").c_str())) {
            if (a_data.has_icon_data()) {
                GameData::user_custom_entry_info.insert_or_assign(a_data.m_form_id, a_data);
            }
            else {
                GameData::user_custom_entry_info.erase(a_data.m_form_id);
            }
            close_edit_dialog();
        }
        if (!save_enabled) {
            FUCK::EndDisabled();
        }
        FUCK::SameLine();
        if (FUCK::Button(translate_id("$CANCEL").c_str())) {
            close_edit_dialog();
        }
        pop_font();
    }
}
