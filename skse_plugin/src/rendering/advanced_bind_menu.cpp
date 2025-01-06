#include "advanced_bind_menu.h"
#include <imgui.h>
#include "render_manager.h"
#include "../input/keybinds.h"
#include "gui_tab_button.h"

namespace SpellHotbar::BindMenu {

    bool show_frame = false;

    constexpr int filter_buf_size = 256;
    char filter_buf[filter_buf_size] = "";
    int tab_index{ 0 };

    std::vector<RE::TESForm*> list_of_skills;
    std::vector<RE::TESForm*> list_of_skills_filtered;

    std::array<std::string, 6> rank_texts = { "-", "Novice", "Apprentice", "Adept", "Expert", "Master"};
    std::array<std::string, 6> school_texts = { "-", "Alteration", "Conjuration", "Destruction", "Illusion", "Restoration" };

    std::array<std::string, 9> type_texts = { "Potion", "Spell", "Lesser Power", "Greater Power", "Shout", "Scroll", "Poison", "Food", "-" };

    std::array<std::string, 13> tab_texts = {
        "All",
        "Spells",
        "Alteration",
        "Illusion",
        "Destruction",
        "Conjuration",
        "Restoration",
        "Shouts",
        "Powers",
        "Scrolls",
        "Potions",
        "Poisons",
        "Food"
    };

    enum tab_index : uint8_t {
        TabIndex_All = 0Ui8,
        TabIndex_Spells,
        TabIndex_Alteration,
        TabIndex_Illusion,
        TabIndex_Destruction,
        TabIndex_Conjuration,
        TabIndex_Restoration,
        TabIndex_Shouts,
        TabIndex_Powers,
        TabIndex_Scrolls,
        TabIndex_Potions,
        TabIndex_Poisons,
        TabIndex_Food
    };

    struct Dragged_skill {
        const RE::TESForm* form;
        SlottedSkill* source_slot;

        Dragged_skill() : form(nullptr), source_slot(nullptr) {};
        //Dragged_skill(const RE::TESForm* tesform) : form(tesform), source_slot(nullptr) {};
        //Dragged_skill(const RE::TESForm* tesform, SlottedSkill* source) : form(tesform), source_slot(source) {};

        void set_dragged(const RE::TESForm* tesform) {
            form = tesform;
            source_slot = nullptr;
        }
        void set_dragged(const RE::TESForm* tesform, SlottedSkill* source) {
            form = tesform;
            source_slot = source;
        }

        inline bool has_dragged_from() const {
            return form != nullptr;
        }

        inline RE::FormID get_form_id() const
        {
            return form->GetFormID();
        }
    };

    Dragged_skill current_dragged_skill;

    enum bind_menu_column_id : ImGuiID {
        column_id_Icon = 0U,
        column_id_Name,
        column_id_Type,
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
            tab_index = 0;
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
            GameData::get_player_known_spells(pc, list_of_skills, false);
            GameData::add_player_owned_bindable_items(pc, list_of_skills);

            filter_buf[0] = '\0';
            update_filter("", 0Ui8);
        }
    }

    const char* get_rank_text(int rank) {
        if (rank >= 0 && rank < 6) {
            return rank_texts.at(rank).c_str();
        }
        else
        {
            return rank_texts.at(0).c_str();
        }
    }

    size_t get_school_order(RE::ActorValue av) {
        size_t val{ 0 };
        switch (av) {
        case RE::ActorValue::kAlteration:
            val = 1;
            break;
        case RE::ActorValue::kIllusion:
            val = 4;
            break;
        case RE::ActorValue::kDestruction:
            val = 3;
            break;
        case RE::ActorValue::kConjuration:
            val = 2;
            break;
        case RE::ActorValue::kRestoration:
            val = 5;
            break;
        default:
            val = 0;
            break;
        }
        return val;
    }

    const char* get_school_text(RE::ActorValue av) {
        return school_texts.at(get_school_order(av)).c_str();
    }

    const char* get_type_text(const RE::TESForm* item) {
        if (item != nullptr) {
            if (item->GetFormType() == RE::FormType::Shout) {
                return type_texts[4].c_str();
            }
            else if (item->GetFormType() == RE::FormType::AlchemyItem) {
                const RE::AlchemyItem* alch = item->As<RE::AlchemyItem>();
                if (alch != nullptr) {
                    if (alch->IsPoison()) {
                        return type_texts[6].c_str();
                    }
                    else if (alch->IsFood()) {
                        return type_texts[7].c_str();
                    }
                }
                return type_texts[0].c_str();
            }
            else if (item->GetFormType() == RE::FormType::Scroll) {
                return type_texts[5].c_str();
            }
            else if (item->GetFormType() == RE::FormType::Spell)
            {
                const RE::SpellItem* spell = item->As<RE::SpellItem>();
                if (spell != nullptr) {
                    if (spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell)
                    {
                        return type_texts[1].c_str();
                    }
                    else if (spell->GetSpellType() == RE::MagicSystem::SpellType::kPower)
                    {
                        return type_texts[3].c_str();
                    }
                    else if (spell->GetSpellType() == RE::MagicSystem::SpellType::kLesserPower)
                    {
                        return type_texts[2].c_str();
                    }
                }
            }
        }
        return type_texts[8].c_str();;
    }

    std::tuple<int, RE::ActorValue> get_rank_school(const RE::TESForm* item) {
        int rank{ 0 };
        RE::ActorValue school{ RE::ActorValue::kNone };

        if (item->GetFormType() == RE::FormType::Spell) {
            const RE::SpellItem* spell = item->As<RE::SpellItem>();

            if (spell != nullptr && spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                if (spell->effects.size() > 0U) {

                    bool found{ false };
                    for (RE::BSTArrayBase::size_type i = 0U; i < spell->effects.size() && !found; i++) {
                        //find first spell effect that has a magic school
                        const RE::Effect* effect = spell->effects[i];
                        if (effect->baseEffect != nullptr) {
                            auto av = effect->baseEffect->GetMagickSkill();
                            if (av != RE::ActorValue::kNone) {
                                school = av;
                                rank = GameData::get_spell_rank(effect->baseEffect->GetMinimumSkillLevel()) +1;
                                found = true;
                            }
                        }
                    }
                }
            }
        }
        return std::make_tuple(rank, school);
    }

    void update_filter(const std::string filter_text, uint8_t tab_ind) {
        if (filter_text.empty() && tab_ind == 0Ui8) {
            list_of_skills_filtered = list_of_skills;
        }
        else {
            list_of_skills_filtered.clear();
            list_of_skills_filtered.reserve(list_of_skills.size());
            for (size_t i = 0U; i < list_of_skills.size(); i++) {

                bool match_text{ false };
                bool match_tab{ tab_ind == 0Ui8 };

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

                //check_tab
                if (!match_tab) {
                    auto form_type = list_of_skills[i]->GetFormType();
                    if (form_type == RE::FormType::Shout) {
                        match_tab = tab_ind == TabIndex_Shouts;
                    }
                    else if (form_type == RE::FormType::Scroll) {
                        match_tab = tab_ind == TabIndex_Scrolls;
                    }
                    else if (form_type == RE::FormType::Spell) {
                        if (tab_ind == TabIndex_Spells) {
                            RE::SpellItem* spell = list_of_skills[i]->As<RE::SpellItem>();
                            if (spell != nullptr && spell->GetSpellType() == RE::MagicSystem::SpellType::kSpell) {
                                match_tab = true;
                            }
                        }
                        else {
                            RE::SpellItem* spell = list_of_skills[i]->As<RE::SpellItem>();
                            if (spell != nullptr) {

                                if (spell->GetSpellType() == RE::MagicSystem::SpellType::kPower || spell->GetSpellType() == RE::MagicSystem::SpellType::kLesserPower) {
                                    match_tab = tab_ind == TabIndex_Powers;
                                }
                                else {
                                    auto [rank, school] = get_rank_school(list_of_skills[i]);
                                    switch (tab_ind) {
                                    case TabIndex_Alteration:
                                        match_tab = school == RE::ActorValue::kAlteration;
                                        break;
                                    case TabIndex_Conjuration:
                                        match_tab = school == RE::ActorValue::kConjuration;
                                        break;
                                    case TabIndex_Destruction:
                                        match_tab = school == RE::ActorValue::kDestruction;
                                        break;
                                    case TabIndex_Illusion:
                                        match_tab = school == RE::ActorValue::kIllusion;
                                        break;
                                    case TabIndex_Restoration:
                                        match_tab = school == RE::ActorValue::kRestoration;
                                        break;
                                    default:
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    else if (form_type == RE::FormType::AlchemyItem) {
                        RE::AlchemyItem* alch = list_of_skills[i]->As<RE::AlchemyItem>();
                        if (alch != nullptr) {
                            if ((alch->IsFood() && tab_ind == TabIndex_Food) || (alch->IsPoison() && tab_ind == TabIndex_Poisons)) {
                                match_tab = true;
                            }
                            else if (!alch->IsFood() && !alch->IsPoison() && (tab_ind == TabIndex_Potions)) {
                                match_tab = true;
                            }
                        }
                    }                
                }

                if (match_text && match_tab) list_of_skills_filtered.emplace_back(list_of_skills[i]);
            }
        }
    }

    const ImGuiTableSortSpecs* s_current_sort_specs{ nullptr };
    bool compare_entries_for_sort(const RE::TESForm* lhs, const RE::TESForm* rhs) {
        for (int n = 0; n < s_current_sort_specs->SpecsCount; n++)
        {
            // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
            // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
            const ImGuiTableColumnSortSpecs* sort_spec = &s_current_sort_specs->Specs[n];
            bool is_less{ false };

            if (sort_spec->ColumnUserID == bind_menu_column_id::column_id_Name)
            {
                is_less = std::string(lhs->GetName()) < std::string(rhs->GetName());
            }
            else if (sort_spec->ColumnUserID == bind_menu_column_id::column_id_Rank)
            {
                auto [lrank, _l] = get_rank_school(lhs);
                auto [rrank, _r] = get_rank_school(rhs);
                is_less = lrank < rrank;
            }
            else if (sort_spec->ColumnUserID == bind_menu_column_id::column_id_School)
            {
                auto [_l, lschool] = get_rank_school(lhs);
                auto [_r, rschool] = get_rank_school(rhs);
                is_less = get_school_order(lschool) < get_school_order(rschool);
            }
            else if (sort_spec->ColumnUserID == bind_menu_column_id::column_id_Type) {
                is_less = std::string(get_type_text(lhs)) < std::string(get_type_text(rhs));
            }
            else {
                is_less = lhs->formID < rhs->formID;
            }
            return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? is_less : !is_less;
        }
        return lhs->formID < rhs->formID;
    }

    void set_drag_source(const RE::TESForm* item, float scale_factor, SlottedSkill* source = nullptr) {
        if (item != nullptr) {
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                current_dragged_skill.set_dragged(item, source);
                // Set payload to carry the index of our item (could be anything)
                ImGui::SetDragDropPayload("SPELL_SLOT", &current_dragged_skill, sizeof(Dragged_skill));
                // Display preview (could be anything, e.g. when dragging an image we could decide to display
                // the filename and a small preview of the image, etc.)
                //ImGui::Text("%08x", item->formID);
                if (current_dragged_skill.has_dragged_from()) {
                    RenderManager::draw_skill(current_dragged_skill.get_form_id(), static_cast<int>(std::round(60.0f * scale_factor)), RenderManager::get_skill_color(current_dragged_skill.form));
                    ImGui::SameLine();
                    ImGui::Text(current_dragged_skill.form->GetName());
                }
                ImGui::EndDragDropSource();
            }
        }
    }

	void drawFrame(ImFont* font_text, ImFont* font_text_big, ImFont* font_title) {
        ImGui::PushFont(font_text);
        static constexpr ImGuiWindowFlags window_flag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground;

        auto& io = ImGui::GetIO();
        io.MouseDrawCursor = true;

        RenderManager::calculate_frame_size(0.925f);
        RenderManager::draw_frame_bg(&show_frame);

        auto [screen_size_x, screen_size_y, window_width] = RenderManager::calculate_frame_size(0.9f);
        ImGui::SetNextWindowBgAlpha(0.0F);

        float scale_factor = screen_size_y / 1080.0f;

        RenderManager::ImGui_push_title_style();
        ImGui::Begin("Binding Menu", &show_frame, window_flag);
        RenderManager::ImGui_pop_title_style();

        float child_window_height = ImGui::GetContentRegionAvail().y;
        ImGui::BeginChild("BindMenuTabLeft", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, child_window_height), false, ImGuiWindowFlags_HorizontalScrollbar);

        bool filter_dirty = false;

        ImGui::PushFont(font_text_big);
        ImGui::TextUnformatted(tab_texts[tab_index].c_str());
        ImGui::PopFont();

        int tab_icon_size = static_cast<int>(60.0f * scale_factor);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 1));
        Rendering::GuiTabButton::draw("##TabAll", TabIndex_All, GameData::DefaultIconType::TAB_ALL, tab_icon_size, tab_index, filter_dirty, tab_texts[TabIndex_All].c_str()); ImGui::SameLine();
        Rendering::GuiTabButton::draw("##TabSpells", TabIndex_Spells, GameData::DefaultIconType::TAB_SPELLS, tab_icon_size, tab_index, filter_dirty, tab_texts[TabIndex_Spells].c_str()); ImGui::SameLine();
        Rendering::GuiTabButton::draw("##TabAlteration", TabIndex_Alteration, GameData::DefaultIconType::ALTERATION_ADEPT, tab_icon_size, tab_index, filter_dirty, tab_texts[TabIndex_Alteration].c_str()); ImGui::SameLine();
        Rendering::GuiTabButton::draw("##TabIllusion", TabIndex_Illusion, GameData::DefaultIconType::ILLUSION_FRIENDLY_ADEPT, tab_icon_size, tab_index, filter_dirty, tab_texts[TabIndex_Illusion].c_str()); ImGui::SameLine();
        Rendering::GuiTabButton::draw("##TabDestruction", TabIndex_Destruction, GameData::DefaultIconType::DESTRUCTION_FIRE_EXPERT, tab_icon_size, tab_index, filter_dirty, tab_texts[TabIndex_Destruction].c_str()); ImGui::SameLine();
        Rendering::GuiTabButton::draw("##TabConjuration", TabIndex_Conjuration, GameData::DefaultIconType::CONJURATION_SUMMON_ADEPT, tab_icon_size, tab_index, filter_dirty, tab_texts[TabIndex_Conjuration].c_str()); ImGui::SameLine();
        Rendering::GuiTabButton::draw("##TabRestoration", TabIndex_Restoration, GameData::DefaultIconType::RESTORATION_FRIENDLY_EXPERT, tab_icon_size, tab_index, filter_dirty, tab_texts[TabIndex_Restoration].c_str()); ImGui::SameLine();
        Rendering::GuiTabButton::draw("##TabShouts", TabIndex_Shouts, GameData::DefaultIconType::SHOUT_GENERIC, tab_icon_size, tab_index, filter_dirty, tab_texts[TabIndex_Shouts].c_str()); ImGui::SameLine();
        Rendering::GuiTabButton::draw("##TabPowers", TabIndex_Powers, GameData::DefaultIconType::GREATER_POWER, tab_icon_size, tab_index, filter_dirty, tab_texts[TabIndex_Powers].c_str()); ImGui::SameLine();
        Rendering::GuiTabButton::draw("##TabScrolls", TabIndex_Scrolls, GameData::DefaultIconType::TAB_SCROLLS, tab_icon_size, tab_index, filter_dirty, tab_texts[TabIndex_Scrolls].c_str()); ImGui::SameLine();
        Rendering::GuiTabButton::draw("##TabPotions", TabIndex_Potions, GameData::DefaultIconType::TAB_POTIONS, tab_icon_size, tab_index, filter_dirty, tab_texts[TabIndex_Potions].c_str()); ImGui::SameLine();
        Rendering::GuiTabButton::draw("##TabPoisons", TabIndex_Poisons, GameData::DefaultIconType::TAB_POISONS, tab_icon_size, tab_index, filter_dirty, tab_texts[TabIndex_Poisons].c_str()); ImGui::SameLine();
        Rendering::GuiTabButton::draw("##TabFood", TabIndex_Food, GameData::DefaultIconType::TAB_FOOD, tab_icon_size, tab_index, filter_dirty, tab_texts[TabIndex_Food].c_str());
        ImGui::PopStyleVar();

        int bsize = static_cast<int>(30.0f * scale_factor);
        ImGui::BeginChild("subtableft_left", ImVec2(ImGui::GetContentRegionAvail().x * 0.6f, 30.0f * scale_factor));
        ImVec2 p = ImGui::GetCursorScreenPos();

        bool button_clear_filter_clicked{ false };
        if (ImGui::InvisibleButton("X", ImVec2(static_cast<float>(bsize), static_cast<float>(bsize)))) {
            button_clear_filter_clicked = true;
            RE::PlaySound(Input::sound_UIFavorite);
        }
        bool clear_button_hovered = ImGui::IsItemHovered();
        RenderManager::draw_default_icon_in_editor(GameData::DefaultIconType::UNBIND_SLOT, p, bsize);
        if (clear_button_hovered) {
            RenderManager::draw_highlight_overlay(p, bsize, ImColor(255,127,127));
        }
        ImGui::SameLine();

        static std::string last_filter = "";

        static auto filter_input_flags = ImGuiInputTextFlags_EscapeClearsAll;


        if (button_clear_filter_clicked) {
            filter_buf[0] = '\0';
        }

        if (last_filter != filter_buf) {
            filter_dirty = true;
        }
        last_filter = filter_buf;

        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::InputTextWithHint("##Filter", "Filter text", filter_buf, filter_buf_size, filter_input_flags);
        ImGui::PopItemWidth();

        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("subtableft_right", ImVec2(ImGui::GetContentRegionAvail().x, 30.0f * scale_factor));
        
        static int table_icon_size = static_cast<int>(std::round(60.0f * scale_factor));
        int table_icon_size_min_value = static_cast<int>(std::round(24.0f * scale_factor));
        int table_icon_size_max_value = static_cast<int>(std::round(80.0f * scale_factor));
       
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x*0.75f);
        ImGui::SliderInt("Icon Size", &table_icon_size, table_icon_size_min_value, table_icon_size_max_value, "%d");
        ImGui::PopItemWidth();
        ImGui::EndChild();

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
            ImGui::TableSetupColumn("Drag Icon", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 65.0f, bind_menu_column_id::column_id_Icon);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortAscending, 0.0f, bind_menu_column_id::column_id_Name);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortAscending, 0.0f, bind_menu_column_id::column_id_Type);
            ImGui::TableSetupColumn("School", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortAscending, 0.0f, bind_menu_column_id::column_id_School);
            ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortAscending, 0.0f, bind_menu_column_id::column_id_Rank);
            ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
            ImGui::TableHeadersRow();

            // Sort our data if sort specs have been changed!
            if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
                if (sort_specs->SpecsDirty)
                {
                    s_current_sort_specs = sort_specs;
                    std::sort(list_of_skills.begin(), list_of_skills.end(), compare_entries_for_sort);
                    sort_specs->SpecsDirty = false;

                    update_filter(filter_buf, static_cast<uint8_t>(tab_index));
                    filter_dirty = false;
                }
            }

            //apply filtering
            if (filter_dirty) {
                update_filter(filter_buf, static_cast<uint8_t>(tab_index));
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

                    auto [rank, school] = get_rank_school(item);
                   
                    ImGui::PushID(item->GetFormID());
                    ImGui::TableNextRow();
                   
                    ImGui::TableNextColumn();
                    RenderManager::draw_skill(item->GetFormID(), table_icon_size, RenderManager::get_skill_color(item));
                    if (ImGui::IsItemHovered())
                    {
                        RenderManager::show_skill_tooltip(item);
                    }
                    set_drag_source(item, scale_factor);

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(item->GetName());

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(get_type_text(item));

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(get_school_text(school));

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(get_rank_text(rank));

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
        std::string none_text = GameData::get_modifier_text_long(key_modifier::none);
        ImGui::RadioButton(none_text.c_str(), &modifier_index, 0);

        if (Input::mod_1.isValidBound()) {
            ImGui::SameLine();
            std::string ctrl_text = GameData::get_modifier_text_long(key_modifier::ctrl);
            ImGui::RadioButton(ctrl_text.c_str(), &modifier_index, 1);
        }
        if (Input::mod_2.isValidBound()) {
            ImGui::SameLine();
            std::string shift_text = GameData::get_modifier_text_long(key_modifier::shift);
            ImGui::RadioButton(shift_text.c_str(), &modifier_index, 2);
        }
        if (Input::mod_3.isValidBound()) {
            ImGui::SameLine();
            std::string alt_text = GameData::get_modifier_text_long(key_modifier::alt);
            ImGui::RadioButton(alt_text.c_str(), &modifier_index, 3);
        }

        //draw Hotbar slots

        int icon_size = static_cast<int>(std::round(60.0f * scale_factor));
        float text_offset_x = icon_size * 0.05f;
        float text_offset_x_right = icon_size * 0.95f - ImGui::CalcTextSize("R").x;
        float text_offset_y = icon_size * 0.0125f;

        key_modifier mod = static_cast<key_modifier>(modifier_index);

        for (int i = 0; i < bar.get_bar_size(); i++) {
            auto [skill, inherited] = bar.get_skill_in_bar_with_inheritance(i, mod, false);

            GameData::Spell_cast_data skill_dat;
            auto form = RE::TESForm::LookupByID(skill.formID);
            if (form) {
                skill_dat = GameData::get_spell_data(form, true, true);
            }

            size_t count{ 0 };
            if (skill.consumed != consumed_type::none) {
                count = GameData::count_item_in_inv(skill.formID);
            }

            ImVec2 bpos = ImGui::GetCursorScreenPos();
            std::string button_label = "##slot_button_" + std::to_string(i);
            if (ImGui::InvisibleButton(button_label.c_str(), ImVec2(static_cast<float>(icon_size), static_cast<float>(icon_size)))) {
                if (!inherited) {
                    if (form != nullptr && form->GetFormType() == RE::FormType::Spell) {
                        RE::SpellItem* spell = form->As<RE::SpellItem>();

                        if (spell != nullptr && spell->GetEquipSlot() == GameData::equip_slot_either_hand)
                        {
                            //skill for rendering is a copy, we need to get the ref here, no inheritence is needed
                            auto& skill_ref = bar.get_skill_in_bar_by_ref(i, mod);
                            if (!skill_ref.isEmpty() && skill_ref.formID == form->GetFormID()) {
                                Hotbar::rotate_skill_hand_assingment(spell, skill_ref);
                                RE::PlaySound(Input::sound_UISkillsFocus);
                            }
                        }
                    }
                }
                else {
                    //inherited skill, toggle between blocked and empty
                    auto& skill_ref = bar.get_skill_in_bar_by_ref(i, mod);
                    if (!skill_ref.isEmpty() && GameData::is_clear_spell(skill_ref.formID)) {
                        //unblock
                        skill_ref.clear();
                    }
                    else
                    {
                        //set to blocked
                        if (GameData::spellhotbar_unbind_slot != nullptr)
                        {
                            skill_ref = GameData::spellhotbar_unbind_slot->formID;
                        }
                    }
                    RE::PlaySound(Input::sound_UISkillsFocus);
                }
            }

            bool drawn_skill{ false };
            bool button_hovered = ImGui::IsItemHovered();
            if (button_hovered && inherited) {
                if (GameData::spellhotbar_unbind_slot != nullptr) {
                    RenderManager::draw_skill_in_editor(GameData::spellhotbar_unbind_slot->GetFormID(), bpos, icon_size);
                }
                else {
                    RenderManager::draw_default_icon_in_editor(GameData::DefaultIconType::BAR_EMPTY, bpos, icon_size);
                }
            }
            else {
                drawn_skill = RenderManager::draw_skill_in_editor(skill.formID, bpos, icon_size, skill.color);
                if (!drawn_skill) {
                    RenderManager::draw_default_icon_in_editor(GameData::DefaultIconType::BAR_EMPTY, bpos, icon_size, skill.color);
                }
            }

            if (!inherited) {
                SlottedSkill* source_skill{nullptr};
                if (!skill.isEmpty()) {
                    source_skill = bar.get_skill_in_bar_ptr(i, mod);
                }
                set_drag_source(form, scale_factor, source_skill);
            }
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPELL_SLOT"))
                {
                    IM_ASSERT(payload->DataSize == sizeof(Dragged_skill));
                    Dragged_skill payload_n = *(const Dragged_skill*)payload->Data;
                    if (payload_n.has_dragged_from()) {
                        auto& target = bar.get_skill_in_bar_by_ref(i, mod);

                        std::optional<hand_mode> source_hand(std::nullopt);
                        if (payload_n.source_slot != nullptr) {
                            source_hand = payload_n.source_slot->hand;
                            if (!target.isEmpty()) {
                                payload_n.source_slot->update_skill_assignment(target.formID);
                                payload_n.source_slot->hand = target.hand;
                            }
                            else {
                                payload_n.source_slot->clear();
                            }
                        }

                        if (payload_n.get_form_id() != 0U) {
                            target.update_skill_assignment(payload_n.get_form_id());
                            if (source_hand.has_value()) {
                                target.hand = source_hand.value();
                            }
                        }
                        else
                        {
                            target.clear();
                        }
                        RE::PlaySound(Input::sound_UISkillsFocus);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (drawn_skill) {
                if (RenderManager::should_overlay_be_rendered(skill_dat.overlay_icon)) {
                    RenderManager::draw_icon_overlay(bpos, icon_size, skill_dat.overlay_icon, IM_COL32_WHITE);
                }

                ImU32 col = IM_COL32_WHITE;
                /*if (highlight_slot == i) {
                    col = IM_COL32(255, 255, static_cast<int>(127 + 128 * (1.0 - highlight_factor)), 255);
                }*/

                RenderManager::draw_slot_overlay(bpos, icon_size, col);
            }

            bool is_being_dragged{ false };
            if (const ImGuiPayload* payload = ImGui::GetDragDropPayload())
            {
                IM_ASSERT(payload->DataSize == sizeof(Dragged_skill));
                Dragged_skill payload_n = *(const Dragged_skill*)payload->Data;
                if (payload_n.source_slot != nullptr && payload_n.source_slot == bar.get_skill_in_bar_ptr(i, mod)) {
                    is_being_dragged = true;
                }
            }

            if (button_hovered || is_being_dragged) {
                RenderManager::draw_highlight_overlay(bpos, icon_size, ImColor(127, 127, 255));
            }
            if (button_hovered && form != nullptr) {
                RenderManager::show_skill_tooltip(form);
            }
            ImGui::SameLine();

            std::string key_text = GameData::get_keybind_text(i, mod);
            ImVec2 tex_pos(bpos.x + text_offset_x, bpos.y + text_offset_y);
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
                ImVec2 tex_pos_hand(bpos.x + text_offset_x_right, bpos.y + text_offset_y);
                ImGui::GetWindowDrawList()->AddText(tex_pos_hand, ImColor(255, 255, 255), hand_text.c_str());
            }

            if (skill.consumed != consumed_type::none) {
                //clamp text to 999
                std::string text = std::to_string(std::clamp(count, 0Ui64, 999Ui64));
                ImVec2 textsize = ImGui::CalcTextSize(text.c_str());
                ImVec2 count_text_pos(bpos.x + icon_size - textsize.x, bpos.y + icon_size - textsize.y);
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
                IM_ASSERT(payload->DataSize == sizeof(Dragged_skill));
                Dragged_skill payload_n = *(const Dragged_skill*)payload->Data;
                if (payload_n.source_slot != nullptr) {
                    payload_n.source_slot->clear();
                }
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::SameLine();
        ImGui::Text("Unbind");

        ImGui::PopFont();

        ImGui::EndChild();

        ImGui::End();

        RenderManager::draw_custom_mouse_cursor();
    }
}