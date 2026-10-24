#include "config_tool.h"
#include "../../lifecycle/lifecycle.h"
#include "../../casts/casting_controller.h"

#include "widgets.h"
#include "../flick_watch.h"
#include "../flick_windows.h"
#include "../../bar/hotbar.h"
#include "../../bar/hotbars.h"
#include "../../game_data/game_data.h"
#include "../../game_data/localization.h"
#include "../../input/keybinds.h"
#include "../../input/modes.h"
#include "../../mcp/bind_capture.h"
#include "../../mcp/mcp_preset_name.h"
#include "../../storage/user_data_io.h"

// The configuration pages as seven tabs of one FLICK sidebar tool (the retired MCM's pages). The
// keybind rows arm Mcp::bind_capture(); the press itself is caught by SH2's own input hook,
// which runs ahead of FLICK's (input/input_hook.h), so a key typed while this tool is open
// reaches the capture before the host zeroes it.
//
// Every call here goes through FUCK::, never ImGui:: (flick_pch.h).
namespace SpellHotbar::FlickUi::ConfigTool {
    namespace {
        // Development helper tab with cheats (skills, perk points). Set to false for releases.
        constexpr bool k_testing_tab_enabled{ true };

        constexpr const char* k_keybind_labels[] = {
            "Hotbar Skill 1",
            "Hotbar Skill 2",
            "Hotbar Skill 3",
            "Hotbar Skill 4",
            "Hotbar Skill 5",
            "Hotbar Skill 6",
            "Hotbar Skill 7",
            "Hotbar Skill 8",
            "Hotbar Skill 9",
            "Hotbar Skill 10",
            "Hotbar Skill 11",
            "Hotbar Skill 12",
            "Next Bar",
            "Previous Bar",
            "Bar Modifier 1",
            "Bar Modifier 2",
            "Bar Modifier 3",
            "Dual Casting Modifier",
            "Show Bar Modifier",
            "Cast Spell",
            "Use Potion",
            "Show Oblivion Bar Modifier",
            "Open Binding Menu"};

        static_assert(
            sizeof(k_keybind_labels) / sizeof(k_keybind_labels[0]) == Input::keybind_id::num_keys,
            "keybind labels must cover IDs 0..22");

        constexpr const char* k_input_modes[] = {"Cast Directly", "Equip", "Oblivion-Style"};
        constexpr const char* k_bar_show[] = {
            "Always", "Never", "Combat", "Drawn Weapon", "Combat or Drawn", "Combat And Drawn"};
        constexpr const char* k_bar_show_transformed[] = {"Always", "Never", "Combat"};
        constexpr const char* k_text_show[] = {"Never", "Fade", "Always"};
        constexpr const char* k_layouts[] = {"Bar", "Circle", "Cross"};
        constexpr const char* k_anchors[] = {
            "Bottom", "Left", "Top", "Right", "Bottom Left", "Top Left", "Bottom Right", "Top Right", "Center"};
        constexpr const char* k_inherit[] = {"Default", "Same Modifier", "No Inheritance"};

        constexpr const char* k_bars_mod_dir = "Data/SKSE/Plugins/SpellHotbar/bars";
        constexpr const char* k_icon_edits_mod_dir = "Data/SKSE/Plugins/SpellHotbar/icon_edits";

        struct ConfigurableBar {
            uint32_t id;
            const char* label;
        };

        constexpr ConfigurableBar k_bars[] = {
            {Bars::MAIN_BAR_SNEAK, "Sneak Bar"},
            {Bars::MELEE_BAR, "Melee Bar"},
            {Bars::MELEE_BAR_SNEAK, "Melee Sneak Bar"},
            {Bars::ONE_HAND_SHIELD_BAR, "1H-Shield Bar"},
            {Bars::ONE_HAND_SHIELD_BAR_SNEAK, "1H-Shield Sneak Bar"},
            {Bars::ONE_HAND_SPELL_BAR, "1H-Spell Bar"},
            {Bars::ONE_HAND_SPELL_BAR_SNEAK, "1H-Spell Sneak Bar"},
            {Bars::DUAL_WIELD_BAR, "Dual Wield Bar"},
            {Bars::DUAL_WIELD_BAR_SNEAK, "Dual Wield Sneak Bar"},
            {Bars::TWO_HANDED_BAR, "Two-Handed Bar"},
            {Bars::TWO_HANDED_BAR_SNEAK, "Two-Handed Sneak Bar"},
            {Bars::RANGED_BAR, "Ranged Bar"},
            {Bars::RANGED_BAR_SNEAK, "Ranged Sneak Bar"},
            {Bars::MAGIC_BAR, "Magic Bar"},
            {Bars::MAGIC_BAR_SNEAK, "Magic Sneak Bar"},
        };

        char config_save_name[64]{};
        char bars_save_name[64]{};
        char icon_save_name[64]{};
        std::string status_message;

        // Offsets and spacing are stored in pixels of this display and edited in 1080p units,
        // as RenderManager::scale_to_resolution / scale_from_resolution do on the plugin side.
        float to_resolution(float a_normalized) { return a_normalized * FUCK::GetDisplaySize().y / 1080.0f; }
        float from_resolution(float a_scaled)
        {
            const float h = FUCK::GetDisplaySize().y;
            return h > 0.0f ? a_scaled / h * 1080.0f : a_scaled;
        }

        // ---- The MCM page layout: a header per section, then the options as rows of a striped
        // table, the label left and its control in a column of its own. With room for it two
        // options share a row, as the MCM's two columns did. The controls carry hidden labels
        // ("##label"), the label column shows the text. ----

        struct OptionTable {
            int per_row{ 1 };
            int count{ 0 };
            bool open{ false };
        };
        OptionTable options;

        std::string hidden(const char* label) { return std::string("##") + label; }

        // The flags every config table shares.
        FUCK::TableFlags table_flags()
        {
            using TF = FUCK::TableFlags;
            return TF::kRowBg | TF::kBordersInnerH | TF::kBordersOuterH | TF::kNoSavedSettings |
                   TF::kSizingStretchProp | static_cast<TF>(ImGuiTableFlags_PadOuterX);
        }

        void section_header(const char* title)
        {
            if (FUCK::GetCursorPos().y > FUCK::GetFrameHeight() * 2.0f) {
                FUCK::Spacing();
            }
            FUCK::SeparatorText(title);
        }

        // `max_per_row` 1 keeps one option per row, for long labels or wide controls.
        bool begin_options(const char* title, int max_per_row = 2)
        {
            section_header(title);
            options = OptionTable{};
            options.per_row = FUCK::GetContentRegionAvail().x >= scale(860.0f) ? max_per_row : 1;
            options.open = FUCK::BeginTable(title, options.per_row * 2, table_flags());
            if (options.open) {
                using CF = FUCK::TableColumnFlags;
                for (int i = 0; i < options.per_row; ++i) {
                    FUCK::TableSetupColumn("", CF::kWidthStretch, 0.5f);
                    FUCK::TableSetupColumn("", CF::kWidthStretch, 0.5f);
                }
            }
            return options.open;
        }

        void end_options()
        {
            if (options.open) {
                FUCK::EndTable();
            }
            options = OptionTable{};
        }

        // The label cell of the next option; leaves the cursor in its control cell, the next
        // item filling it.
        void option(const char* label)
        {
            if (options.count % options.per_row == 0) {
                FUCK::TableNextRow();
            }
            ++options.count;
            FUCK::TableNextColumn();
            FUCK::AlignTextToFramePadding();
            FUCK::TextUnformatted(label);
            FUCK::TableNextColumn();
            FUCK::SetNextItemWidth(-FLT_MIN);
        }

        // A read-only value in the control cell.
        void option_text(const char* label, const char* value)
        {
            option(label);
            FUCK::AlignTextToFramePadding();
            FUCK::TextUnformatted(value);
        }

        bool option_button(const char* label, const char* button)
        {
            option(label);
            return FUCK::Button((std::string(button) + hidden(label)).c_str());
        }

        bool combo(const char* label, int* value, const char* const* items, int count)
        {
            option(label);
            return FUCK::Combo(hidden(label).c_str(), value, items, count);
        }

        bool option_checkbox(const char* label, bool* value)
        {
            option(label);
            return checkbox(hidden(label).c_str(), value);
        }

        bool option_slider(const char* label, float* value, float min, float max, const char* format)
        {
            option(label);
            return FUCK::SliderFloat(hidden(label).c_str(), value, min, max, format);
        }

        bool option_slider(const char* label, int* value, int min, int max)
        {
            option(label);
            return FUCK::SliderInt(hidden(label).c_str(), value, min, max, "%d");
        }

        void status_line()
        {
            if (!status_message.empty()) {
                FUCK::Spacing();
                FUCK::TextUnformatted(status_message.c_str());
            }
        }

        void confirm_modal(const char* id, const char* message, const std::function<void()>& on_yes)
        {
            if (FUCK::BeginPopupModal(id, nullptr, FUCK::WindowFlags::kAutoResize | FUCK::WindowFlags::kNoMove)) {
                FUCK::TextUnformatted(message);
                FUCK::Separator();
                if (FUCK::Button("Yes")) {
                    on_yes();
                    FUCK::CloseCurrentPopup();
                }
                FUCK::SameLine();
                if (FUCK::Button("No")) {
                    FUCK::CloseCurrentPopup();
                }
                FUCK::EndPopup();
            }
        }

        bool global_toggle(const char* label, RE::TESGlobal* global)
        {
            if (global == nullptr) {
                FUCK::BeginDisabled();
                bool dummy = false;
                option_checkbox(label, &dummy);
                FUCK::EndDisabled();
                return false;
            }
            bool value = global->value != 0.0F;
            if (option_checkbox(label, &value)) {
                global->value = value ? 1.0F : 0.0F;
                return true;
            }
            return false;
        }

        bool global_slider(const char* label, RE::TESGlobal* global, float min, float max, const char* format)
        {
            if (global == nullptr) {
                FUCK::BeginDisabled();
                float dummy = min;
                option_slider(label, &dummy, min, max, format);
                FUCK::EndDisabled();
                return false;
            }
            float value = global->value;
            if (option_slider(label, &value, min, max, format)) {
                global->value = std::clamp(value, min, max);
                return true;
            }
            return false;
        }

        void draw_keybind_row(int id)
        {
            auto& capture = Mcp::bind_capture();
            FUCK::PushID(id);
            const int code = Input::get_keybind(id);
            const std::string name = capture.armed() && capture.pending_id() == id
                                         ? "Press a key..."
                                         : GameData::get_key_text_long(code);
            // As the MCM's key map option: the key is the button, a click waits for the new key.
            option(k_keybind_labels[id]);
            const float cell_x = FUCK::GetCursorPos().x;
            const float cell_w = FUCK::GetContentRegionAvail().x;
            if (FUCK::Button((name + "##rebind").c_str())) {
                capture.arm(id);
            }
            if (FUCK::IsItemHovered()) {
                FUCK::SetTooltip("Click, then press the new key.");
            }
            const float unmap_w = FUCK::CalcTextSize("Unmap").x + FUCK::GetStyleVarVec(ImGuiStyleVar_FramePadding).x * 2.0f;
            FUCK::SameLine();
            FUCK::SetCursorPosX(std::max(FUCK::GetCursorPos().x, cell_x + cell_w - unmap_w));
            if (FUCK::Button("Unmap")) {
                Input::rebind_key(id, -1, false);
                if (capture.pending_id() == id) {
                    capture.cancel();
                }
            }
            FUCK::PopID();
        }

        void draw_keybind_section(const char* title, std::initializer_list<int> ids)
        {
            if (begin_options(title)) {
                for (const int id : ids) {
                    draw_keybind_row(id);
                }
            }
            end_options();
        }

        void draw_keybinds()
        {
            using enum Input::keybind_id;
            draw_keybind_section("Skill Bindings", { spell_1, spell_2, spell_3, spell_4, spell_5, spell_6,
                                                     spell_7, spell_8, spell_9, spell_10, spell_11, spell_12 });
            draw_keybind_section("Menu Bindings", { ui_next, ui_prev, open_advanced_bind_menu });
            draw_keybind_section("Modifier Bindings", { modifier_1, modifier_2, modifier_3, dual_casting_mod, show_bar_mod });
            draw_keybind_section("Oblivion Mode Bindings", { oblivion_cast, oblivion_potion, oblivion_show_bar_mod });
        }

        void draw_settings()
        {
            if (begin_options("Bar Configuration")) {
                int mode = Input::get_current_mode_index();
                if (combo("SpellHotbar Mode", &mode, k_input_modes, 3)) {
                    Input::set_input_mode(std::clamp(mode, 0, 2));
                }
                option_checkbox("Disable Non-Modifier Bar", &Bars::disable_non_modifier_bar);
                int slots = Bars::barsize;
                if (option_slider("Slots per Bar", &slots, 1, static_cast<int>(max_bar_size))) {
                    Bars::barsize = static_cast<uint8_t>(std::clamp(slots, 1, static_cast<int>(max_bar_size)));
                }
                int show = static_cast<int>(Bars::bar_show_setting);
                if (combo("Show HUD Bar", &show, k_bar_show, 6)) {
                    Bars::bar_show_setting = Bars::bar_show_mode(std::clamp(show, 0, 5));
                }
                int text = static_cast<int>(Bars::text_show_setting);
                if (combo("Show Bar Text", &text, k_text_show, 3)) {
                    Bars::text_show_setting = Bars::text_show_mode(std::clamp(text, 0, 2));
                }
                int vl = static_cast<int>(Bars::bar_show_setting_vampire_lord);
                if (combo("Show HUD Bar (Vampire Lord)", &vl, k_bar_show_transformed, 3)) {
                    Bars::bar_show_setting_vampire_lord = Bars::bar_show_mode(std::clamp(vl, 0, 2));
                }
                int ww = static_cast<int>(Bars::bar_show_setting_werewolf);
                if (combo("Show HUD Bar (Werewolf)", &ww, k_bar_show_transformed, 3)) {
                    Bars::bar_show_setting_werewolf = Bars::bar_show_mode(std::clamp(ww, 0, 2));
                }
                option_checkbox("Use Default bar when Sheathed", &Bars::use_default_bar_when_sheathed);
                option_checkbox("Disable Menu Rendering", &Bars::disable_menu_rendering);
                option_checkbox("Disable Menu Binding", &Bars::disable_menu_binding);
                option_checkbox("Show Castbar", &Bars::show_castbar);
                bool key_icons = Bars::get_use_keybind_icons();
                if (option_checkbox("Use Key Icons", &key_icons)) {
                    Bars::set_use_keybind_icons(key_icons);
                }
            }
            end_options();

            if (begin_options("Bar Positioning")) {
                int layout = static_cast<int>(Bars::layout);
                if (combo("Layout", &layout, k_layouts, 3)) {
                    Bars::layout = Bars::bar_layout(std::clamp(layout, 0, 2));
                }
                int anchor = static_cast<int>(Bars::bar_anchor_point);
                if (combo("Anchor Point", &anchor, k_anchors, 9)) {
                    Bars::bar_anchor_point = Bars::anchor_point(std::clamp(anchor, 0, 8));
                }
                option_slider("Slot Scale", &Bars::slot_scale, 0.01F, 5.0F, "%.2f");
                float offset_x = from_resolution(Bars::offset_x);
                if (option_slider("Offset X", &offset_x, -2000.0F, 2000.0F, "%.0f")) {
                    Bars::offset_x = to_resolution(offset_x);
                }
                float spacing = from_resolution(Bars::slot_spacing);
                if (option_slider("Slot Spacing", &spacing, 0.0F, 50.0F, "%.0f")) {
                    Bars::slot_spacing = to_resolution(std::max(0.0F, spacing));
                }
                float offset_y = from_resolution(Bars::offset_y);
                if (option_slider("Offset Y", &offset_y, -2000.0F, 2000.0F, "%.0f")) {
                    Bars::offset_y = to_resolution(offset_y);
                }
                int row = Bars::bar_row_len;
                if (option_slider("Slots per Row", &row, 1, static_cast<int>(max_bar_size))) {
                    Bars::bar_row_len = static_cast<uint8_t>(std::clamp(row, 1, static_cast<int>(max_bar_size)));
                }
                option_slider("Circle Radius", &Bars::bar_circle_radius, 0.1F, 10.0F, "%.2f");
                option_slider("Cross Distance", &Bars::bar_cross_distance, 0.0F, 1.0F, "%.3f");
            }
            end_options();

            if (begin_options("Gameplay")) {
                if (option_slider("Potion GCD", &GameData::potion_gcd, 0.1F, 10.0F, "%.2f")) {
                    GameData::potion_gcd = std::clamp(GameData::potion_gcd, 0.1F, 10.0F);
                }
                bool shout_cds = GameData::individual_shout_cooldowns;
                if (option_checkbox("Individual Shout Cooldowns", &shout_cds) &&
                    shout_cds != GameData::individual_shout_cooldowns) {
                    GameData::toggle_individual_shout_cooldowns();
                }
                option_checkbox("Dual Casting Blocks Movement", &GameData::dual_cast_blocks_movement);
                option_checkbox("Cast While Swimming", &GameData::allow_swim_casting);
            }
            end_options();

            if (begin_options("Oblivion Mode Bar")) {
                option_slider("Slot Scale", &Bars::oblivion_slot_scale, 0.01F, 5.0F, "%.2f");
                float ob_x = from_resolution(Bars::oblivion_offset_x);
                if (option_slider("Offset X", &ob_x, -2000.0F, 2000.0F, "%.0f")) {
                    Bars::oblivion_offset_x = to_resolution(ob_x);
                }
                option_slider("Slot Spacing", &Bars::oblivion_slot_spacing, 0.0F, 50.0F, "%.0f");
                float ob_y = from_resolution(Bars::oblivion_offset_y);
                if (option_slider("Offset Y", &ob_y, -2000.0F, 2000.0F, "%.0f")) {
                    Bars::oblivion_offset_y = to_resolution(ob_y);
                }
                int ob_anchor = static_cast<int>(Bars::oblivion_bar_anchor_point);
                if (combo("Anchor Point", &ob_anchor, k_anchors, 9)) {
                    Bars::oblivion_bar_anchor_point = Bars::anchor_point(std::clamp(ob_anchor, 0, 8));
                }
                option_checkbox("Show Power", &Bars::oblivion_bar_show_power);
                int ob_show = static_cast<int>(Bars::oblivion_bar_show_setting);
                if (combo("Show Oblivion Bar", &ob_show, k_bar_show, 6)) {
                    Bars::oblivion_bar_show_setting = Bars::bar_show_mode(std::clamp(ob_show, 0, 5));
                }
                option_slider("Show Main Bar After", &Bars::oblivion_bar_held_show_time_threshold, 0.0F, 5.0F, "%.2f");
                option_checkbox("Vertical Bar", &Bars::oblivion_bar_vertical);
            }
            end_options();
        }

        void draw_bars()
        {
            section_header("Bars");
            FUCK::TextDisabled("The non-sneak main bar is always enabled.");
            if (!FUCK::BeginTable("##bars", 3, table_flags())) {
                return;
            }
            using CF = FUCK::TableColumnFlags;
            FUCK::TableSetupColumn("Bar", CF::kWidthStretch, 0.4f);
            FUCK::TableSetupColumn("Enabled", CF::kWidthStretch, 0.2f);
            FUCK::TableSetupColumn("Inherit Mode", CF::kWidthStretch, 0.4f);
            FUCK::TableHeadersRow();
            for (const auto& bar : k_bars) {
                auto found = Bars::hotbars.find(bar.id);
                if (found == Bars::hotbars.end()) {
                    continue;
                }
                FUCK::PushID(static_cast<int>(bar.id));
                FUCK::TableNextRow();
                FUCK::TableNextColumn();
                FUCK::AlignTextToFramePadding();
                FUCK::TextUnformatted(bar.label);
                FUCK::TableNextColumn();
                bool enabled = found->second.is_enabled();
                if (checkbox("##enabled", &enabled)) {
                    found->second.set_enabled(enabled);
                }
                FUCK::TableNextColumn();
                int inherit = found->second.get_inherit_mode();
                FUCK::SetNextItemWidth(-FLT_MIN);
                if (FUCK::Combo("##inherit", &inherit, k_inherit, 3)) {
                    found->second.set_inherit_mode(inherit);
                }
                FUCK::PopID();
            }
            FUCK::EndTable();
        }

        void draw_perks()
        {
            if (begin_options("Perk Requirements")) {
                global_toggle("Disable Perk Requirements", GameData::global_spellhotbar_perks_override);
                global_toggle("Require Half-Cost Perk", GameData::global_spellhotbar_perks_require_halfcostperk);
            }
            end_options();

            if (begin_options("BattleMage Triggers")) {
                global_slider("Timed Block Window", GameData::global_spellhotbar_perks_timed_block_window, 0.0F, 5.0F, "%.2f");
                global_slider("Block Proc Chance", GameData::global_spellhotbar_perks_block_trigger_chance, 0.0F, 1.0F, "%.2f");
                global_slider("Power Attack Proc Chance", GameData::global_spellhotbar_perks_power_attack_trigger_chance, 0.0F, 1.0F, "%.2f");
                global_slider("Sneak Attack Proc Chance", GameData::global_spellhotbar_perks_sneak_attack_trigger_chance, 0.0F, 1.0F, "%.2f");
                global_slider("Crit Proc Chance", GameData::global_spellhotbar_perks_crit_trigger_chance, 0.0F, 1.0F, "%.2f");
                global_slider("Proc Cooldown", GameData::global_spellhotbar_perks_proc_cooldown, 0.0F, 60.0F, "%.2f");
            }
            end_options();

            if (begin_options("BattleMage Perk Tree", 1)) {
                if (!GameData::battlemage_plugin_loaded) {
                    option_text("Perk Tree", "SpellHotbar_BattleMage.esp is not loaded.");
                }
                else if (option_button("Perk Tree", "Open")) {
                    if (!Lifecycle::open_battlemage_tree()) {
                        status_message = "Custom Skills Framework is unavailable; the tree was not opened.";
                    } else {
                        status_message.clear();
                    }
                }
            }
            end_options();
            status_line();
        }

        std::filesystem::path resolve_existing(const std::filesystem::path& user, const std::filesystem::path& mod)
        {
            if (std::filesystem::exists(user)) {
                return user;
            }
            return mod;
        }

        void request_save(const char* popup, const std::filesystem::path& path, const std::function<void()>& save)
        {
            if (std::filesystem::exists(path)) {
                FUCK::OpenPopup(popup);
            } else {
                save();
            }
        }

        // "<file> exists, overwrite ?" -- the MCM's wording, from the translation files.
        std::string overwrite_prompt(const char* name)
        {
            return Mcp::with_json_extension(name) + " " + translate("$MCM_PROMPT_OVERWRITE");
        }

        // One kind of preset: a row to save under a name, then a row per preset found with its Load button.
        struct PresetKind {
            const char* title;
            const char* what;  // for the status line, "configuration"
            const char* save_prompt_key;
            const char* popup;
            char* name;
            size_t name_size;
            std::function<std::filesystem::path(const std::string&)> path_for;  // json filename -> save path
            std::function<bool(const std::filesystem::path&)> save;
            std::vector<std::string> presets;
            std::function<bool(const std::string&)> load;
        };

        void draw_preset_kind(const PresetKind& kind)
        {
            if (!begin_options(kind.title, 1)) {
                end_options();
                return;
            }
            const auto save = [&kind]() {
                const auto path = kind.path_for(Mcp::with_json_extension(kind.name));
                std::filesystem::create_directories(path.parent_path());
                status_message = std::string(kind.save(path) ? "Saved " : "Failed to save ") + kind.what + ".";
            };

            option(translate(kind.save_prompt_key).c_str());
            const float save_w = FUCK::CalcTextSize("Save").x + FUCK::GetStyleVarVec(ImGuiStyleVar_FramePadding).x * 2.0f;
            FUCK::SetNextItemWidth(FUCK::GetContentRegionAvail().x - save_w - FUCK::GetStyleVarVec(ImGuiStyleVar_ItemSpacing).x);
            FUCK::InputText("##name", kind.name, kind.name_size);
            FUCK::SameLine();
            if (FUCK::Button("Save")) {
                if (!Mcp::valid_preset_name(kind.name)) {
                    status_message = std::string("Invalid ") + kind.what + " filename.";
                } else {
                    request_save(kind.popup, kind.path_for(Mcp::with_json_extension(kind.name)), save);
                }
            }
            confirm_modal(kind.popup, overwrite_prompt(kind.name).c_str(), save);

            bool any = false;
            for (const auto& preset : kind.presets) {
                if (!Mcp::is_listed_preset(preset)) {
                    continue;
                }
                any = true;
                FUCK::PushID(preset.c_str());
                option(preset.c_str());
                if (FUCK::Button("Load")) {
                    status_message = (kind.load(preset) ? "Loaded " : "Failed to load ") + preset;
                }
                FUCK::PopID();
            }
            if (!any) {
                FUCK::TableNextRow();
                FUCK::TableNextColumn();
                FUCK::AlignTextToFramePadding();
                FUCK::TextDisabled("No presets found.");
            }
            end_options();
        }

        void draw_presets()
        {
            draw_preset_kind({
                "Configuration", "configuration", "$MCM_PROMPT_SETTINGS_SAVE", "Overwrite config?",
                config_save_name, sizeof(config_save_name),
                [](const std::string& file) { return Storage::IO::get_preset_user_dir() / file; },
                [](const std::filesystem::path& path) { return Storage::IO::save_preset(path.filename().string()); },
                Storage::IO::get_config_presets(),
                [](const std::string& name) { return Storage::IO::load_preset(name, true); },
            });
            draw_preset_kind({
                "Bars", "bars", "$MCM_PROMPT_BAR_SAVE", "Overwrite bars?",
                bars_save_name, sizeof(bars_save_name),
                [](const std::string& file) { return Storage::IO::get_bars_user_dir() / file; },
                [](const std::filesystem::path& path) { return Bars::save_bars_to_json(path.string()); },
                Storage::IO::get_bar_presets(),
                [](const std::string& name) {
                    const auto path = resolve_existing(
                        Storage::IO::get_bars_user_dir() / name, std::filesystem::path{k_bars_mod_dir} / name);
                    return Bars::load_bars_from_json(path.string());
                },
            });
            draw_preset_kind({
                "Icon Edits", "icon edits", "$MCM_PROMPT_ICON_EDITS_SAVE", "Overwrite icon edits?",
                icon_save_name, sizeof(icon_save_name),
                [](const std::string& file) { return Storage::IO::get_icon_edits_user_dir() / file; },
                [](const std::filesystem::path& path) { return GameData::save_icon_edits_to_json(path.string()); },
                Storage::IO::get_icon_edits_presets(),
                [](const std::string& name) {
                    const auto path = resolve_existing(
                        Storage::IO::get_icon_edits_user_dir() / name,
                        std::filesystem::path{k_icon_edits_mod_dir} / name);
                    return GameData::load_icon_edits_from_json(path.string());
                },
            });
            status_line();
        }

        // The window openers. Each closes the host's menu first: the windows are FLICK guests
        // too, and one opened under the menu that opened it would be unreachable.
        void open_from_menu(Flick::Window a_window)
        {
            Flick::close_host_menu();
            Flick::open_window(a_window);
        }

        void draw_windows()
        {
            if (begin_options("Windows", 1)) {
                if (option_button("Spell Bind Menu", "Open")) {
                    open_from_menu(Flick::Window::bind_menu);
                }
                if (FUCK::IsItemHovered()) {
                    FUCK::SetTooltip("Slot known spells, shouts, potions, and powers onto the current hotbar.");
                }
                if (option_button("Spell Editor", "Open")) {
                    open_from_menu(Flick::Window::spell_editor);
                }
                if (option_button("Potion Editor", "Open")) {
                    open_from_menu(Flick::Window::potion_editor);
                }
            }
            end_options();

            if (begin_options("Bar Positions", 1)) {
                constexpr const char* bars[] = { "Main Bar", "Oblivion Mode Bar", "In-Menu Dock" };
                for (int i = 0; i < 3; ++i) {
                    if (option_button(bars[i], "Drag")) {
                        Flick::close_host_menu();
                        Flick::open_bar_drag(i);
                    }
                }
            }
            end_options();
        }

        // A button that asks first; the modal has to be drawn in the ID scope of the button.
        void confirmed_button(const char* label, const char* button, const char* popup, const char* question,
                              const std::function<void()>& on_yes)
        {
            if (option_button(label, button)) {
                FUCK::OpenPopup(popup);
            }
            confirm_modal(popup, question, on_yes);
        }

        void draw_util()
        {
            if (begin_options("Maintenance", 1)) {
                confirmed_button("Resources", "Reload", "Reload resources?", "Reload textures and key names from disk?", []() {
                    UiBridge::reload_resources();
                    status_message = "Reloaded resources.";
                });
                confirmed_button("Spell Data", "Reload", "Reload spell data?", "Reload spell data from CSV files?", []() {
                    GameData::reload_data();
                    status_message = "Reloaded spell data.";
                });
                confirmed_button("All Bars", "Clear", "Clear bars?", "Clear every hotbar slot, including the Oblivion bar?", []() {
                    Bars::clear_bars();
                    GameData::oblivion_bar.clear();
                    status_message = "Cleared bars.";
                });
            }
            end_options();

            if (begin_options("Powers", 1)) {
                bool unbind = Lifecycle::player_has_power(0);
                if (option_checkbox("Unbind Slot", &unbind) && unbind != Lifecycle::player_has_power(0)) {
                    Lifecycle::toggle_player_power(0);
                }
                bool dual = Lifecycle::player_has_power(1);
                if (option_checkbox("Hotbar Dual Casting", &dual) && dual != Lifecycle::player_has_power(1)) {
                    Lifecycle::toggle_player_power(1);
                }
            }
            end_options();
            status_line();
        }

        // Cheats that shorten test setups, the tab only exists while k_testing_tab_enabled is set.
        void draw_testing()
        {
            auto* pc = RE::PlayerCharacter::GetSingleton();
            if (!pc) {
                return;
            }

            if (begin_options("Cheats", 1)) {
                if (option_button("Skills", "Set All to 100")) {
                    constexpr auto first = static_cast<int>(RE::ActorValue::kOneHanded);
                    constexpr auto last = static_cast<int>(RE::ActorValue::kEnchanting);
                    for (int av = first; av <= last; ++av) {
                        pc->AsActorValueOwner()->SetBaseActorValue(static_cast<RE::ActorValue>(av), 100.0f);
                    }
                    status_message = "Set all skills to 100.";
                }

                if (option_button("Perk Points", "Add 10")) {
                    // The game stores the count in a signed byte.
                    auto& perk_count = pc->GetGameStatsData().perkCount;
                    perk_count = static_cast<std::int8_t>(std::min(static_cast<int>(perk_count) + 10, 127));
                    status_message = "Perk points: " + std::to_string(perk_count);
                }

                if (option_button("Test Spells and Shouts", "Add")) {
                    // Skyrim.esm form ids (python_scripts/spell_lists2), one or two spells per cast type
                    constexpr RE::FormID spells[] = {
                        0x0001C789, // Fireball: aimed
                        0x0002B96C, // Ice Spike: aimed
                        0x00012FCD, // Flames: aimed concentration
                        0x0002DD2A, // Sparks: aimed concentration
                        0x00035D7F, // Wall of Flames: aimed concentration, ground
                        0x0007A82B, // Fire Storm: ritual, self
                        0x0007E8E4, // Blizzard: ritual, self
                        0x0007E8E5, // Lightning Storm: ritual, aimed concentration
                        0x0002F3B8, // Fast Healing: self
                        0x000B62EF, // Close Wounds: self
                        0x00012FCC, // Healing: self concentration
                        0x0005AD5C, // Oakflesh: self buff
                        0x0004DEE8, // Courage: aimed buff
                        0x00012FD2, // Heal Other: aimed
                        0x0004D3F2, // Healing Hands: aimed concentration
                        0x0007E8DD, // Call to Arms: ritual, buff
                        0x00013018, // Lesser Ward: ward
                        0x000204C3, // Conjure Flame Atronach: target location
                    };
                    int added{ 0 };
                    for (const RE::FormID id : spells) {
                        auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(id);
                        if (spell && !pc->HasSpell(spell)) {
                            pc->AddSpell(spell);
                            ++added;
                        }
                    }
                    // Shouts with all three words learned and unlocked: Unrelenting Force, Fire Breath
                    constexpr RE::FormID shouts[] = { 0x00013E07, 0x0003F9EA };
                    int shouts_added{ 0 };
                    for (const RE::FormID id : shouts) {
                        auto* shout = RE::TESForm::LookupByID<RE::TESShout>(id);
                        if (!shout) {
                            continue;
                        }
                        if (pc->AddShout(shout)) {
                            ++shouts_added;
                        }
                        for (auto& variation : shout->variations) {
                            if (auto* word = variation.word) {
                                // what Game.TeachWord does, then unlock it as with a dragon soul
                                word->formFlags |= RE::TESForm::RecordFlags::kKnown;
                                pc->UnlockWord(word);
                            }
                        }
                    }
                    status_message = "Added " + std::to_string(added) + " spell(s), " + std::to_string(shouts_added) + " shout(s) with all words.";
                }

                if (option_button("Dual Casting Perks", "Add")) {
                    // Read from perkdata/dual_cast_perks.csv, a perk overhaul can point them at other perks
                    RE::BGSPerk* const perks[] = {
                        GameData::perk_alteration_dual_casting,
                        GameData::perk_conjuration_dual_casting,
                        GameData::perk_destruction_dual_casting,
                        GameData::perk_illusion_dual_casting,
                        GameData::perk_restoration_dual_casting,
                    };
                    int added{ 0 };
                    int missing{ 0 };
                    for (RE::BGSPerk* perk : perks) {
                        if (!perk) {
                            ++missing;
                        }
                        else if (!pc->HasPerk(perk)) {
                            pc->AddPerk(perk);
                            ++added;
                        }
                    }
                    status_message = "Added " + std::to_string(added) + " dual casting perk(s).";
                    if (missing > 0) {
                        status_message += " " + std::to_string(missing) + " not loaded from the perk config.";
                    }
                }

                // The transformation powers with the spells that go with them, as the old debug quest script added them
                const auto add_spells = [&](std::initializer_list<std::pair<RE::FormID, std::string_view>> ids, const char* what) {
                    auto* data_handler = RE::TESDataHandler::GetSingleton();
                    std::string added;
                    int missing{ 0 };
                    for (const auto& [id, plugin] : ids) {
                        auto* spell = data_handler ? data_handler->LookupForm<RE::SpellItem>(id, plugin) : nullptr;
                        if (!spell) {
                            ++missing;
                        }
                        else if (!pc->HasSpell(spell)) {
                            pc->AddSpell(spell);
                            added += (added.empty() ? "" : ", ") + std::string(spell->GetName());
                        }
                    }
                    status_message = std::string(what) + ": " + (added.empty() ? std::string("nothing new") : "added " + added) + ".";
                    if (missing > 0) {
                        status_message += " " + std::to_string(missing) + " spell(s) not found.";
                    }
                };
                // Perk points of the Dawnguard trees live in globals; all of the tree at once, as if every point was earned
                const auto max_tree_points = [&](const char* prefix) {
                    const auto global = [](const std::string& name) { return RE::TESForm::LookupByEditorID<RE::TESGlobal>(name); };
                    auto* points = global(std::string(prefix) + "PerkPoints");
                    auto* earned = global(std::string(prefix) + "TotalPerksEarned");
                    auto* max = global(std::string(prefix) + "MaxPerks");
                    if (!points) {
                        status_message += " Perk point global not found.";
                        return;
                    }
                    const float max_points = max ? max->value : 11.0f;
                    points->value = max_points;
                    if (earned) {
                        earned->value = max_points;
                    }
                    status_message += " Perk points: " + std::to_string(static_cast<int>(max_points)) + ".";
                };
                if (option_button("Werewolf", "Add Power")) {
                    add_spells({ { 0x092C48, "Skyrim.esm" }, { 0x0A1A3E, "Skyrim.esm" } }, "Werewolf");  // Beast Form + its companion spell
                    max_tree_points("DLC1Werewolf");
                }
                if (option_button("Vampire Lord", "Add Power")) {
                    add_spells({ { 0x00283B, "Dawnguard.esm" }, { 0x01462A, "Dawnguard.esm" } }, "Vampire Lord");  // Vampire Lord + its companion spell
                    max_tree_points("DLC1Vampire");
                }

                if (option_button("Horse", "Spawn")) {
                    // A generic, non-unique horse of the game: the first NPC of the horse race named "Horse"
                    RE::TESNPC* horse_base{ nullptr };
                    auto* horse_race = RE::TESForm::LookupByEditorID<RE::TESRace>("HorseRace");
                    if (auto* data_handler = RE::TESDataHandler::GetSingleton(); data_handler && horse_race) {
                        for (RE::TESNPC* npc : data_handler->GetFormArray<RE::TESNPC>()) {
                            if (npc && npc->GetRace() == horse_race && !npc->IsUnique() && std::string_view(npc->GetName()) == "Horse") {
                                horse_base = npc;
                                break;
                            }
                        }
                    }
                    if (!horse_base) {
                        status_message = "No horse found (race HorseRace).";
                    }
                    else if (auto horse = pc->PlaceObjectAtMe(horse_base, false)) {
                        // three metres in front of the player
                        const float yaw = pc->GetAngleZ();
                        const RE::NiPoint3 at = pc->GetPosition() + RE::NiPoint3(std::sin(yaw), std::cos(yaw), 0.0f) * 300.0f;
                        horse->SetPosition(at);
                        status_message = std::format("Spawned {} ({:08X}).", horse_base->GetName(), horse_base->GetFormID());
                    }
                    else {
                        status_message = "Could not place the horse.";
                    }
                }
            }
            end_options();

            // Not saved.
            if (begin_options("Cast Animation", 1)) {
                option_checkbox("Full Body Cast Pose When Standing", &casts::CastingController::cast_full_body_when_standing);
            }
            end_options();
            status_line();
        }

        class Tool : public FUCK::ITool
        {
        public:
            const char* Name() const override { return "Spell Hotbar 2"; }

            void Draw() override
            {
                if (!FUCK::BeginTabBar("##sh2_config")) {
                    return;
                }
                struct Tab {
                    const char* label;
                    void (*draw)();
                };
                static constexpr Tab tabs[] = {
                    { "Keybinds", draw_keybinds },
                    { "Settings", draw_settings },
                    { "Bars", draw_bars },
                    { "Perks", draw_perks },
                    { "Presets", draw_presets },
                    { "Windows", draw_windows },
                    { "Util", draw_util },
                };
                for (const Tab& tab : tabs) {
                    if (FUCK::BeginTabItem(tab.label)) {
                        tab.draw();
                        FUCK::EndTabItem();
                    }
                }
                if (k_testing_tab_enabled && FUCK::BeginTabItem("Testing")) {
                    draw_testing();
                    FUCK::EndTabItem();
                }
                FUCK::EndTabBar();
            }

            void OnClose() override
            {
                // A rebind left armed when the player leaves the tool would eat the next key.
                auto& capture = Mcp::bind_capture();
                if (capture.armed()) {
                    capture.cancel();
                }
            }
        };

        Tool* tool{ nullptr };
    }

    void register_tool()
    {
        if (!Flick::is_connected() || tool != nullptr) {
            return;
        }
        // Leaked on purpose: FLICK keeps the pointer for the life of the process.
        tool = new Tool();
        FUCK::RegisterTool(tool);
        logger::info("SH2 FLICK: registered the configuration tool (seven tabs) in the host's sidebar");
    }
}
