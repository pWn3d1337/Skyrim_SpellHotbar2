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

        bool combo(const char* label, int* value, const char* const* items, int count)
        {
            return FUCK::Combo(label, value, items, count);
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
                checkbox(label, &dummy);
                FUCK::EndDisabled();
                return false;
            }
            bool value = global->value != 0.0F;
            if (checkbox(label, &value)) {
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
                FUCK::SliderFloat(label, &dummy, min, max, format);
                FUCK::EndDisabled();
                return false;
            }
            float value = global->value;
            if (FUCK::SliderFloat(label, &value, min, max, format)) {
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
            FUCK::Text("%s: %s", k_keybind_labels[id], name.c_str());
            FUCK::SameLine();
            if (FUCK::Button("Rebind")) {
                capture.arm(id);
            }
            FUCK::SameLine();
            if (FUCK::Button("Unmap")) {
                Input::rebind_key(id, -1, false);
                if (capture.pending_id() == id) {
                    capture.cancel();
                }
            }
            FUCK::PopID();
        }

        void draw_keybinds()
        {
            FUCK::SeparatorText("Skill Bindings");
            for (int id = Input::keybind_id::spell_1; id <= Input::keybind_id::spell_12; ++id) {
                draw_keybind_row(id);
            }

            FUCK::SeparatorText("Menu Bindings");
            draw_keybind_row(Input::keybind_id::ui_next);
            draw_keybind_row(Input::keybind_id::ui_prev);
            draw_keybind_row(Input::keybind_id::open_advanced_bind_menu);

            FUCK::SeparatorText("Modifier Bindings");
            for (int id = Input::keybind_id::modifier_1; id <= Input::keybind_id::show_bar_mod; ++id) {
                draw_keybind_row(id);
            }

            FUCK::SeparatorText("Oblivion Mode Bindings");
            for (int id = Input::keybind_id::oblivion_cast; id <= Input::keybind_id::oblivion_show_bar_mod; ++id) {
                draw_keybind_row(id);
            }
        }

        void draw_settings()
        {
            FUCK::SeparatorText("Bar Configuration");
            int mode = Input::get_current_mode_index();
            if (combo("SpellHotbar Mode", &mode, k_input_modes, 3)) {
                Input::set_input_mode(std::clamp(mode, 0, 2));
            }
            checkbox("Disable Non-Modifier Bar", &Bars::disable_non_modifier_bar);
            int slots = Bars::barsize;
            if (FUCK::SliderInt("Slots per Bar", &slots, 1, static_cast<int>(max_bar_size))) {
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
            checkbox("Use Default bar when Sheathed", &Bars::use_default_bar_when_sheathed);
            checkbox("Disable Menu Rendering", &Bars::disable_menu_rendering);
            checkbox("Disable Menu Binding", &Bars::disable_menu_binding);
            bool key_icons = Bars::get_use_keybind_icons();
            if (checkbox("Use Key Icons", &key_icons)) {
                Bars::set_use_keybind_icons(key_icons);
            }

            FUCK::SeparatorText("Bar Positioning");
            int layout = static_cast<int>(Bars::layout);
            if (combo("Layout", &layout, k_layouts, 3)) {
                Bars::layout = Bars::bar_layout(std::clamp(layout, 0, 2));
            }
            int anchor = static_cast<int>(Bars::bar_anchor_point);
            if (combo("Anchor Point", &anchor, k_anchors, 9)) {
                Bars::bar_anchor_point = Bars::anchor_point(std::clamp(anchor, 0, 8));
            }
            FUCK::SliderFloat("Slot Scale", &Bars::slot_scale, 0.01F, 5.0F, "%.2f");
            float offset_x = from_resolution(Bars::offset_x);
            if (FUCK::SliderFloat("Offset X", &offset_x, -2000.0F, 2000.0F, "%.0f")) {
                Bars::offset_x = to_resolution(offset_x);
            }
            float spacing = from_resolution(Bars::slot_spacing);
            if (FUCK::SliderFloat("Slot Spacing", &spacing, 0.0F, 50.0F, "%.0f")) {
                Bars::slot_spacing = to_resolution(std::max(0.0F, spacing));
            }
            float offset_y = from_resolution(Bars::offset_y);
            if (FUCK::SliderFloat("Offset Y", &offset_y, -2000.0F, 2000.0F, "%.0f")) {
                Bars::offset_y = to_resolution(offset_y);
            }
            int row = Bars::bar_row_len;
            if (FUCK::SliderInt("Slots per Row", &row, 1, static_cast<int>(max_bar_size))) {
                Bars::bar_row_len = static_cast<uint8_t>(std::clamp(row, 1, static_cast<int>(max_bar_size)));
            }
            FUCK::SliderFloat("Circle Radius", &Bars::bar_circle_radius, 0.1F, 10.0F, "%.2f");
            FUCK::SliderFloat("Cross Distance", &Bars::bar_cross_distance, 0.0F, 1.0F, "%.3f");

            FUCK::SeparatorText("Gameplay");
            if (FUCK::SliderFloat("Potion GCD", &GameData::potion_gcd, 0.1F, 10.0F, "%.2f")) {
                GameData::potion_gcd = std::clamp(GameData::potion_gcd, 0.1F, 10.0F);
            }
            bool shout_cds = GameData::individual_shout_cooldowns;
            if (checkbox("Individual Shout Cooldowns", &shout_cds) &&
                shout_cds != GameData::individual_shout_cooldowns) {
                GameData::toggle_individual_shout_cooldowns();
            }
            checkbox("Dual Casting Blocks Movement", &GameData::dual_cast_blocks_movement);

            FUCK::SeparatorText("Oblivion Mode Bar");
            FUCK::SliderFloat("Oblivion Slot Scale", &Bars::oblivion_slot_scale, 0.01F, 5.0F, "%.2f");
            float ob_x = from_resolution(Bars::oblivion_offset_x);
            if (FUCK::SliderFloat("Oblivion Offset X", &ob_x, -2000.0F, 2000.0F, "%.0f")) {
                Bars::oblivion_offset_x = to_resolution(ob_x);
            }
            FUCK::SliderFloat("Oblivion Slot Spacing", &Bars::oblivion_slot_spacing, 0.0F, 50.0F, "%.0f");
            float ob_y = from_resolution(Bars::oblivion_offset_y);
            if (FUCK::SliderFloat("Oblivion Offset Y", &ob_y, -2000.0F, 2000.0F, "%.0f")) {
                Bars::oblivion_offset_y = to_resolution(ob_y);
            }
            int ob_anchor = static_cast<int>(Bars::oblivion_bar_anchor_point);
            if (combo("Oblivion Anchor Point", &ob_anchor, k_anchors, 9)) {
                Bars::oblivion_bar_anchor_point = Bars::anchor_point(std::clamp(ob_anchor, 0, 8));
            }
            checkbox("Show Power", &Bars::oblivion_bar_show_power);
            int ob_show = static_cast<int>(Bars::oblivion_bar_show_setting);
            if (combo("Show Oblivion Bar", &ob_show, k_bar_show, 6)) {
                Bars::oblivion_bar_show_setting = Bars::bar_show_mode(std::clamp(ob_show, 0, 5));
            }
            FUCK::SliderFloat("Show Main Bar After", &Bars::oblivion_bar_held_show_time_threshold, 0.0F, 5.0F, "%.2f");
            checkbox("Vertical Oblivion bar", &Bars::oblivion_bar_vertical);
        }

        void draw_bars()
        {
            FUCK::TextUnformatted("The non-sneak main bar is always enabled.");
            for (const auto& bar : k_bars) {
                auto found = Bars::hotbars.find(bar.id);
                if (found == Bars::hotbars.end()) {
                    continue;
                }
                FUCK::PushID(static_cast<int>(bar.id));
                FUCK::SeparatorText(bar.label);
                bool enabled = found->second.is_enabled();
                if (checkbox("Enabled", &enabled)) {
                    found->second.set_enabled(enabled);
                }
                int inherit = found->second.get_inherit_mode();
                if (combo("Inherit Mode", &inherit, k_inherit, 3)) {
                    found->second.set_inherit_mode(inherit);
                }
                FUCK::PopID();
            }
        }

        void draw_perks()
        {
            global_toggle("Disable Perk Requirements", GameData::global_spellhotbar_perks_override);
            global_toggle("Require Half-Cost Perk", GameData::global_spellhotbar_perks_require_halfcostperk);
            global_slider("Timed Block Window", GameData::global_spellhotbar_perks_timed_block_window, 0.0F, 5.0F, "%.2f");
            global_slider("Block Proc Chance", GameData::global_spellhotbar_perks_block_trigger_chance, 0.0F, 1.0F, "%.2f");
            global_slider(
                "Power Attack Proc Chance",
                GameData::global_spellhotbar_perks_power_attack_trigger_chance,
                0.0F,
                1.0F,
                "%.2f");
            global_slider(
                "Sneak Attack Proc Chance",
                GameData::global_spellhotbar_perks_sneak_attack_trigger_chance,
                0.0F,
                1.0F,
                "%.2f");
            global_slider("Crit Proc Chance", GameData::global_spellhotbar_perks_crit_trigger_chance, 0.0F, 1.0F, "%.2f");
            global_slider("Proc Cooldown", GameData::global_spellhotbar_perks_proc_cooldown, 0.0F, 60.0F, "%.2f");

            const bool plugin_present = GameData::battlemage_plugin_loaded;
            if (plugin_present) {
                if (FUCK::Button("Open BattleMage tree")) {
                    if (!Lifecycle::open_battlemage_tree()) {
                        status_message = "Custom Skills Framework is unavailable; the tree was not opened.";
                    } else {
                        status_message.clear();
                    }
                }
            } else {
                FUCK::TextUnformatted("SpellHotbar_BattleMage.esp is not loaded.");
            }
            if (!status_message.empty()) {
                FUCK::TextUnformatted(status_message.c_str());
            }
        }

        std::filesystem::path resolve_existing(const std::filesystem::path& user, const std::filesystem::path& mod)
        {
            if (std::filesystem::exists(user)) {
                return user;
            }
            return mod;
        }

        // `heading` is the translated "$MCM_PROMPT_*_LOAD" prompt the MCM used over this list.
        void draw_preset_list(
            const char* label,
            const std::string& heading,
            const std::vector<std::string>& names,
            const std::function<bool(const std::string&)>& loader)
        {
            FUCK::TextUnformatted(heading.c_str());
            bool any = false;
            for (const auto& name : names) {
                if (!Mcp::is_listed_preset(name)) {
                    continue;
                }
                any = true;
                FUCK::PushID(name.c_str());
                if (FUCK::Button("Load")) {
                    if (loader(name)) {
                        status_message = "Loaded " + name;
                    } else {
                        status_message = "Failed to load " + name;
                    }
                }
                FUCK::SameLine();
                FUCK::TextUnformatted(name.c_str());
                FUCK::PopID();
            }
            if (!any) {
                FUCK::Text("No %s presets found.", label);
            }
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

        void draw_presets()
        {
            FUCK::SeparatorText("Configuration");
            FUCK::InputText(translate("$MCM_PROMPT_SETTINGS_SAVE").c_str(), config_save_name, sizeof(config_save_name));
            if (FUCK::Button("Save Config")) {
                if (!Mcp::valid_preset_name(config_save_name)) {
                    status_message = "Invalid configuration filename.";
                } else {
                    const auto filename = Mcp::with_json_extension(config_save_name);
                    const auto path = Storage::IO::get_preset_user_dir() / filename;
                    request_save("Overwrite config?", path, [filename]() {
                        if (Storage::IO::save_preset(filename)) {
                            status_message = "Saved configuration.";
                        } else {
                            status_message = "Failed to save configuration.";
                        }
                    });
                }
            }
            confirm_modal("Overwrite config?", overwrite_prompt(config_save_name).c_str(), []() {
                if (Storage::IO::save_preset(Mcp::with_json_extension(config_save_name))) {
                    status_message = "Saved configuration.";
                } else {
                    status_message = "Failed to save configuration.";
                }
            });
            draw_preset_list("config", translate("$MCM_PROMPT_SETTINGS_LOAD"), Storage::IO::get_config_presets(), [](const std::string& name) {
                return Storage::IO::load_preset(name, true);
            });

            FUCK::SeparatorText("Bars");
            FUCK::InputText(translate("$MCM_PROMPT_BAR_SAVE").c_str(), bars_save_name, sizeof(bars_save_name));
            if (FUCK::Button("Save Bars")) {
                if (!Mcp::valid_preset_name(bars_save_name)) {
                    status_message = "Invalid bars filename.";
                } else {
                    const auto path = Storage::IO::get_bars_user_dir() / Mcp::with_json_extension(bars_save_name);
                    request_save("Overwrite bars?", path, [path]() {
                        std::filesystem::create_directories(path.parent_path());
                        if (Bars::save_bars_to_json(path.string())) {
                            status_message = "Saved bars.";
                        } else {
                            status_message = "Failed to save bars.";
                        }
                    });
                }
            }
            confirm_modal("Overwrite bars?", overwrite_prompt(bars_save_name).c_str(), []() {
                const auto path = Storage::IO::get_bars_user_dir() / Mcp::with_json_extension(bars_save_name);
                std::filesystem::create_directories(path.parent_path());
                if (Bars::save_bars_to_json(path.string())) {
                    status_message = "Saved bars.";
                } else {
                    status_message = "Failed to save bars.";
                }
            });
            draw_preset_list("bar", translate("$MCM_PROMPT_BAR_LOAD"), Storage::IO::get_bar_presets(), [](const std::string& name) {
                const auto path = resolve_existing(
                    Storage::IO::get_bars_user_dir() / name, std::filesystem::path{k_bars_mod_dir} / name);
                return Bars::load_bars_from_json(path.string());
            });

            FUCK::SeparatorText("Icon Edits");
            FUCK::InputText(translate("$MCM_PROMPT_ICON_EDITS_SAVE").c_str(), icon_save_name, sizeof(icon_save_name));
            if (FUCK::Button("Save Icon Edits")) {
                if (!Mcp::valid_preset_name(icon_save_name)) {
                    status_message = "Invalid icon-edit filename.";
                } else {
                    const auto path = Storage::IO::get_icon_edits_user_dir() / Mcp::with_json_extension(icon_save_name);
                    request_save("Overwrite icon edits?", path, [path]() {
                        std::filesystem::create_directories(path.parent_path());
                        if (GameData::save_icon_edits_to_json(path.string())) {
                            status_message = "Saved icon edits.";
                        } else {
                            status_message = "Failed to save icon edits.";
                        }
                    });
                }
            }
            confirm_modal("Overwrite icon edits?", overwrite_prompt(icon_save_name).c_str(), []() {
                const auto path = Storage::IO::get_icon_edits_user_dir() / Mcp::with_json_extension(icon_save_name);
                std::filesystem::create_directories(path.parent_path());
                if (GameData::save_icon_edits_to_json(path.string())) {
                    status_message = "Saved icon edits.";
                } else {
                    status_message = "Failed to save icon edits.";
                }
            });
            draw_preset_list("icon-edit", translate("$MCM_PROMPT_ICON_EDITS_LOAD"), Storage::IO::get_icon_edits_presets(), [](const std::string& name) {
                const auto path = resolve_existing(
                    Storage::IO::get_icon_edits_user_dir() / name,
                    std::filesystem::path{k_icon_edits_mod_dir} / name);
                return GameData::load_icon_edits_from_json(path.string());
            });

            if (!status_message.empty()) {
                FUCK::TextUnformatted(status_message.c_str());
            }
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
            FUCK::SeparatorText("Spell Bind Menu");
            FUCK::TextUnformatted("Slot known spells, shouts, potions, and powers onto the current hotbar.");
            if (FUCK::Button("Open Spell Bind Menu")) {
                open_from_menu(Flick::Window::bind_menu);
            }
            FUCK::SeparatorText("Editors");
            if (FUCK::Button("Open Spell Editor")) {
                open_from_menu(Flick::Window::spell_editor);
            }
            if (FUCK::Button("Open Potion Editor")) {
                open_from_menu(Flick::Window::potion_editor);
            }
            FUCK::SeparatorText("Bar Positions");
            if (FUCK::Button("Drag Main Bar")) {
                Flick::close_host_menu();
                Flick::open_bar_drag(0);
            }
            if (FUCK::Button("Drag Oblivion Mode Bar")) {
                Flick::close_host_menu();
                Flick::open_bar_drag(1);
            }
            if (FUCK::Button("Drag In-Menu Dock")) {
                Flick::close_host_menu();
                Flick::open_bar_drag(2);
            }
        }

        void draw_util()
        {
            if (FUCK::Button("Reload Resources")) {
                FUCK::OpenPopup("Reload resources?");
            }
            confirm_modal("Reload resources?", "Reload textures and key names from disk?", []() {
                UiBridge::reload_resources();
                status_message = "Reloaded resources.";
            });

            if (FUCK::Button("Reload Spell Data")) {
                FUCK::OpenPopup("Reload spell data?");
            }
            confirm_modal("Reload spell data?", "Reload spell data from CSV files?", []() {
                GameData::reload_data();
                status_message = "Reloaded spell data.";
            });

            if (FUCK::Button("Clear Bars")) {
                FUCK::OpenPopup("Clear bars?");
            }
            confirm_modal("Clear bars?", "Clear every hotbar slot, including the Oblivion bar?", []() {
                Bars::clear_bars();
                GameData::oblivion_bar.clear();
                status_message = "Cleared bars.";
            });

            bool unbind = Lifecycle::player_has_power(0);
            if (checkbox("Unbind Slot", &unbind) && unbind != Lifecycle::player_has_power(0)) {
                Lifecycle::toggle_player_power(0);
            }
            bool dual = Lifecycle::player_has_power(1);
            if (checkbox("Hotbar Dual Casting", &dual) && dual != Lifecycle::player_has_power(1)) {
                Lifecycle::toggle_player_power(1);
            }
            if (!status_message.empty()) {
                FUCK::TextUnformatted(status_message.c_str());
            }
        }

        // Cheats that shorten test setups, the tab only exists while k_testing_tab_enabled is set.
        void draw_testing()
        {
            auto* pc = RE::PlayerCharacter::GetSingleton();
            if (!pc) {
                return;
            }

            if (FUCK::Button("Set All Skills to 100")) {
                constexpr auto first = static_cast<int>(RE::ActorValue::kOneHanded);
                constexpr auto last = static_cast<int>(RE::ActorValue::kEnchanting);
                for (int av = first; av <= last; ++av) {
                    pc->AsActorValueOwner()->SetBaseActorValue(static_cast<RE::ActorValue>(av), 100.0f);
                }
                status_message = "Set all skills to 100.";
            }

            if (FUCK::Button("Add 10 Perk Points")) {
                // The game stores the count in a signed byte.
                auto& perk_count = pc->GetGameStatsData().perkCount;
                perk_count = static_cast<std::int8_t>(std::min(static_cast<int>(perk_count) + 10, 127));
                status_message = "Perk points: " + std::to_string(perk_count);
            }

            if (FUCK::Button("Add Spells")) {
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
                status_message = "Added " + std::to_string(added) + " spell(s).";
            }

            if (FUCK::Button("Add Dual Casting Perks")) {
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

            // Not saved.
            FUCK::SeparatorText("Cast Animation");
            checkbox("Full Body Cast Pose When Standing", &casts::CastingController::cast_full_body_when_standing);

            if (!status_message.empty()) {
                FUCK::TextUnformatted(status_message.c_str());
            }
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
