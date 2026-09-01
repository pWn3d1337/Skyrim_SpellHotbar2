#include "mcp_pages.h"

#include "bind_capture.h"
#include "mcp_preset_name.h"
#include "../bar/hotbar.h"
#include "../bar/hotbars.h"
#include "../game_data/game_data.h"
#include "../input/keybinds.h"
#include "../input/modes.h"
#include "../lifecycle/lifecycle.h"
#include "../rendering/render_manager.h"
#include "../storage/user_data_io.h"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace SpellHotbar::Mcp {
    namespace {
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
            "MCP keybind labels must cover IDs 0..22");

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

        bool combo(const char* label, int* value, const char* const* items, int count)
        {
            return ImGui::Combo(label, value, items, count);
        }

        void confirm_modal(const char* id, const char* message, const std::function<void()>& on_yes)
        {
            if (ImGui::BeginPopupModal(id, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
                ImGui::TextUnformatted(message);
                ImGui::Separator();
                if (ImGui::Button("Yes")) {
                    on_yes();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("No")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        bool global_toggle(const char* label, RE::TESGlobal* global)
        {
            if (global == nullptr) {
                ImGui::BeginDisabled();
                bool dummy = false;
                ImGui::Checkbox(label, &dummy);
                ImGui::EndDisabled();
                return false;
            }
            bool value = global->value != 0.0F;
            if (ImGui::Checkbox(label, &value)) {
                global->value = value ? 1.0F : 0.0F;
                return true;
            }
            return false;
        }

        bool global_slider(const char* label, RE::TESGlobal* global, float min, float max, const char* format)
        {
            if (global == nullptr) {
                ImGui::BeginDisabled();
                float dummy = min;
                ImGui::SliderFloat(label, &dummy, min, max, format);
                ImGui::EndDisabled();
                return false;
            }
            float value = global->value;
            if (ImGui::SliderFloat(label, &value, min, max, format)) {
                global->value = std::clamp(value, min, max);
                return true;
            }
            return false;
        }

        void draw_keybind_row(int id)
        {
            auto& capture = bind_capture();
            ImGui::PushID(id);
            const int code = Input::get_keybind(id);
            const std::string name = capture.armed() && capture.pending_id() == id
                                         ? "Press a key..."
                                         : GameData::get_key_text_long(code);
            ImGui::Text("%s: %s", k_keybind_labels[id], name.c_str());
            ImGui::SameLine();
            if (ImGui::Button("Rebind")) {
                capture.arm(id);
            }
            ImGui::SameLine();
            if (ImGui::Button("Unmap")) {
                Input::rebind_key(id, -1, false);
                if (capture.pending_id() == id) {
                    capture.cancel();
                }
            }
            ImGui::PopID();
        }

        void draw_keybinds()
        {
            ImGui::SeparatorText("Skill Bindings");
            for (int id = Input::keybind_id::spell_1; id <= Input::keybind_id::spell_12; ++id) {
                draw_keybind_row(id);
            }

            ImGui::SeparatorText("Menu Bindings");
            draw_keybind_row(Input::keybind_id::ui_next);
            draw_keybind_row(Input::keybind_id::ui_prev);
            draw_keybind_row(Input::keybind_id::open_advanced_bind_menu);

            ImGui::SeparatorText("Modifier Bindings");
            for (int id = Input::keybind_id::modifier_1; id <= Input::keybind_id::show_bar_mod; ++id) {
                draw_keybind_row(id);
            }

            ImGui::SeparatorText("Oblivion Mode Bindings");
            for (int id = Input::keybind_id::oblivion_cast; id <= Input::keybind_id::oblivion_show_bar_mod; ++id) {
                draw_keybind_row(id);
            }
        }

        void draw_settings()
        {
            ImGui::SeparatorText("Bar Configuration");
            int mode = Input::get_current_mode_index();
            if (combo("SpellHotbar Mode", &mode, k_input_modes, 3)) {
                Input::set_input_mode(std::clamp(mode, 0, 2));
            }
            ImGui::Checkbox("Disable Non-Modifier Bar", &Bars::disable_non_modifier_bar);
            int slots = Bars::barsize;
            if (ImGui::SliderInt("Slots per Bar", &slots, 1, static_cast<int>(max_bar_size))) {
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
            ImGui::Checkbox("Use Default bar when Sheathed", &Bars::use_default_bar_when_sheathed);
            ImGui::Checkbox("Disable Menu Rendering", &Bars::disable_menu_rendering);
            ImGui::Checkbox("Disable Menu Binding", &Bars::disable_menu_binding);
            bool key_icons = Bars::get_use_keybind_icons();
            if (ImGui::Checkbox("Use Key Icons", &key_icons)) {
                Bars::set_use_keybind_icons(key_icons);
            }

            ImGui::SeparatorText("Bar Positioning");
            int layout = static_cast<int>(Bars::layout);
            if (combo("Layout", &layout, k_layouts, 3)) {
                Bars::layout = Bars::bar_layout(std::clamp(layout, 0, 2));
            }
            int anchor = static_cast<int>(Bars::bar_anchor_point);
            if (combo("Anchor Point", &anchor, k_anchors, 9)) {
                Bars::bar_anchor_point = Bars::anchor_point(std::clamp(anchor, 0, 8));
            }
            ImGui::SliderFloat("Slot Scale", &Bars::slot_scale, 0.01F, 5.0F, "%.2f");
            float offset_x = RenderManager::scale_from_resolution(Bars::offset_x);
            if (ImGui::SliderFloat("Offset X", &offset_x, -2000.0F, 2000.0F, "%.0f")) {
                Bars::offset_x = RenderManager::scale_to_resolution(offset_x);
            }
            float spacing = RenderManager::scale_from_resolution(Bars::slot_spacing);
            if (ImGui::SliderFloat("Slot Spacing", &spacing, 0.0F, 50.0F, "%.0f")) {
                Bars::slot_spacing = RenderManager::scale_to_resolution(std::max(0.0F, spacing));
            }
            float offset_y = RenderManager::scale_from_resolution(Bars::offset_y);
            if (ImGui::SliderFloat("Offset Y", &offset_y, -2000.0F, 2000.0F, "%.0f")) {
                Bars::offset_y = RenderManager::scale_to_resolution(offset_y);
            }
            int row = Bars::bar_row_len;
            if (ImGui::SliderInt("Slots per Row", &row, 1, static_cast<int>(max_bar_size))) {
                Bars::bar_row_len = static_cast<uint8_t>(std::clamp(row, 1, static_cast<int>(max_bar_size)));
            }
            ImGui::SliderFloat("Circle Radius", &Bars::bar_circle_radius, 0.1F, 10.0F, "%.2f");
            ImGui::SliderFloat("Cross Distance", &Bars::bar_cross_distance, 0.0F, 1.0F, "%.3f");

            ImGui::SeparatorText("Gameplay");
            if (ImGui::SliderFloat("Potion GCD", &GameData::potion_gcd, 0.1F, 10.0F, "%.2f")) {
                GameData::potion_gcd = std::clamp(GameData::potion_gcd, 0.1F, 10.0F);
            }
            bool shout_cds = GameData::individual_shout_cooldowns;
            if (ImGui::Checkbox("Individual Shout Cooldowns", &shout_cds) &&
                shout_cds != GameData::individual_shout_cooldowns) {
                GameData::toggle_individual_shout_cooldowns();
            }

            ImGui::SeparatorText("Oblivion Mode Bar");
            ImGui::SliderFloat("Oblivion Slot Scale", &Bars::oblivion_slot_scale, 0.01F, 5.0F, "%.2f");
            float ob_x = RenderManager::scale_from_resolution(Bars::oblivion_offset_x);
            if (ImGui::SliderFloat("Oblivion Offset X", &ob_x, -2000.0F, 2000.0F, "%.0f")) {
                Bars::oblivion_offset_x = RenderManager::scale_to_resolution(ob_x);
            }
            ImGui::SliderFloat("Oblivion Slot Spacing", &Bars::oblivion_slot_spacing, 0.0F, 50.0F, "%.0f");
            float ob_y = RenderManager::scale_from_resolution(Bars::oblivion_offset_y);
            if (ImGui::SliderFloat("Oblivion Offset Y", &ob_y, -2000.0F, 2000.0F, "%.0f")) {
                Bars::oblivion_offset_y = RenderManager::scale_to_resolution(ob_y);
            }
            int ob_anchor = static_cast<int>(Bars::oblivion_bar_anchor_point);
            if (combo("Oblivion Anchor Point", &ob_anchor, k_anchors, 9)) {
                Bars::oblivion_bar_anchor_point = Bars::anchor_point(std::clamp(ob_anchor, 0, 8));
            }
            ImGui::Checkbox("Show Power", &Bars::oblivion_bar_show_power);
            int ob_show = static_cast<int>(Bars::oblivion_bar_show_setting);
            if (combo("Show Oblivion Bar", &ob_show, k_bar_show, 6)) {
                Bars::oblivion_bar_show_setting = Bars::bar_show_mode(std::clamp(ob_show, 0, 5));
            }
            ImGui::SliderFloat("Show Main Bar After", &Bars::oblivion_bar_held_show_time_threshold, 0.0F, 5.0F, "%.2f");
            ImGui::Checkbox("Vertical Oblivion bar", &Bars::oblivion_bar_vertical);
        }

        void draw_bars()
        {
            ImGui::TextUnformatted("The non-sneak main bar is always enabled.");
            for (const auto& bar : k_bars) {
                auto found = Bars::hotbars.find(bar.id);
                if (found == Bars::hotbars.end()) {
                    continue;
                }
                ImGui::PushID(static_cast<int>(bar.id));
                ImGui::SeparatorText(bar.label);
                bool enabled = found->second.is_enabled();
                if (ImGui::Checkbox("Enabled", &enabled)) {
                    found->second.set_enabled(enabled);
                }
                int inherit = found->second.get_inherit_mode();
                if (combo("Inherit Mode", &inherit, k_inherit, 3)) {
                    found->second.set_inherit_mode(inherit);
                }
                ImGui::PopID();
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

            const bool plugin_present = GameData::spellhotbar_battlemage_open_perks_power != nullptr;
            if (plugin_present) {
                if (ImGui::Button("Open BattleMage tree")) {
                    if (!Lifecycle::open_battlemage_tree()) {
                        status_message = "Custom Skills Framework is unavailable; the tree was not opened.";
                    } else {
                        status_message.clear();
                    }
                }
            } else {
                ImGui::TextUnformatted("SpellHotbar_BattleMage.esp is not loaded.");
            }
            if (!status_message.empty()) {
                ImGui::TextUnformatted(status_message.c_str());
            }
        }

        std::filesystem::path resolve_existing(const std::filesystem::path& user, const std::filesystem::path& mod)
        {
            if (std::filesystem::exists(user)) {
                return user;
            }
            return mod;
        }

        void draw_preset_list(
            const char* label,
            const std::vector<std::string>& names,
            const std::function<bool(const std::string&)>& loader)
        {
            bool any = false;
            for (const auto& name : names) {
                if (!is_listed_preset(name)) {
                    continue;
                }
                any = true;
                ImGui::PushID(name.c_str());
                if (ImGui::Button("Load")) {
                    if (loader(name)) {
                        status_message = "Loaded " + name;
                    } else {
                        status_message = "Failed to load " + name;
                    }
                }
                ImGui::SameLine();
                ImGui::TextUnformatted(name.c_str());
                ImGui::PopID();
            }
            if (!any) {
                ImGui::Text("No %s presets found.", label);
            }
        }

        void request_save(const char* popup, const std::filesystem::path& path, const std::function<void()>& save)
        {
            if (std::filesystem::exists(path)) {
                ImGui::OpenPopup(popup);
            } else {
                save();
            }
        }

        void draw_presets()
        {
            ImGui::SeparatorText("Configuration");
            ImGui::InputText("Save Config as...", config_save_name, sizeof(config_save_name));
            if (ImGui::Button("Save Config")) {
                if (!valid_preset_name(config_save_name)) {
                    status_message = "Invalid configuration filename.";
                } else {
                    const auto filename = with_json_extension(config_save_name);
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
            confirm_modal("Overwrite config?", "Overwrite the existing configuration file?", []() {
                if (Storage::IO::save_preset(with_json_extension(config_save_name))) {
                    status_message = "Saved configuration.";
                } else {
                    status_message = "Failed to save configuration.";
                }
            });
            draw_preset_list("config", Storage::IO::get_config_presets(), [](const std::string& name) {
                return Storage::IO::load_preset(name, true);
            });

            ImGui::SeparatorText("Bars");
            ImGui::InputText("Save Bars as...", bars_save_name, sizeof(bars_save_name));
            if (ImGui::Button("Save Bars")) {
                if (!valid_preset_name(bars_save_name)) {
                    status_message = "Invalid bars filename.";
                } else {
                    const auto path = Storage::IO::get_bars_user_dir() / with_json_extension(bars_save_name);
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
            confirm_modal("Overwrite bars?", "Overwrite the existing bars file?", []() {
                const auto path = Storage::IO::get_bars_user_dir() / with_json_extension(bars_save_name);
                std::filesystem::create_directories(path.parent_path());
                if (Bars::save_bars_to_json(path.string())) {
                    status_message = "Saved bars.";
                } else {
                    status_message = "Failed to save bars.";
                }
            });
            draw_preset_list("bar", Storage::IO::get_bar_presets(), [](const std::string& name) {
                const auto path = resolve_existing(
                    Storage::IO::get_bars_user_dir() / name, std::filesystem::path{k_bars_mod_dir} / name);
                return Bars::load_bars_from_json(path.string());
            });

            ImGui::SeparatorText("Icon Edits");
            ImGui::InputText("Save Icon Edits as...", icon_save_name, sizeof(icon_save_name));
            if (ImGui::Button("Save Icon Edits")) {
                if (!valid_preset_name(icon_save_name)) {
                    status_message = "Invalid icon-edit filename.";
                } else {
                    const auto path = Storage::IO::get_icon_edits_user_dir() / with_json_extension(icon_save_name);
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
            confirm_modal("Overwrite icon edits?", "Overwrite the existing icon-edit file?", []() {
                const auto path = Storage::IO::get_icon_edits_user_dir() / with_json_extension(icon_save_name);
                std::filesystem::create_directories(path.parent_path());
                if (GameData::save_icon_edits_to_json(path.string())) {
                    status_message = "Saved icon edits.";
                } else {
                    status_message = "Failed to save icon edits.";
                }
            });
            draw_preset_list("icon-edit", Storage::IO::get_icon_edits_presets(), [](const std::string& name) {
                const auto path = resolve_existing(
                    Storage::IO::get_icon_edits_user_dir() / name,
                    std::filesystem::path{k_icon_edits_mod_dir} / name);
                return GameData::load_icon_edits_from_json(path.string());
            });

            if (!status_message.empty()) {
                ImGui::TextUnformatted(status_message.c_str());
            }
        }

        void draw_bind_menu()
        {
            ImGui::TextUnformatted(
                "Slot known spells, shouts, potions, and powers onto the current hotbar.");
            if (ImGui::Button("Open Spell Bind Menu")) {
                RenderManager::open_advanced_binding_menu();
            }
        }

        void draw_spells()
        {
            if (ImGui::Button("Open Spell Editor")) {
                RenderManager::open_spell_editor();
            }
            if (ImGui::Button("Open Potion Editor")) {
                RenderManager::open_potion_editor();
            }
        }

        void draw_util()
        {
            if (ImGui::Button("Reload Resources")) {
                ImGui::OpenPopup("Reload resources?");
            }
            confirm_modal("Reload resources?", "Reload textures and fonts from disk?", []() {
                RenderManager::reload_resouces();
                status_message = "Reloaded resources.";
            });

            if (ImGui::Button("Reload Spell Data")) {
                ImGui::OpenPopup("Reload spell data?");
            }
            confirm_modal("Reload spell data?", "Reload spell data from CSV files?", []() {
                GameData::reload_data();
                status_message = "Reloaded spell data.";
            });

            if (ImGui::Button("Clear Bars")) {
                ImGui::OpenPopup("Clear bars?");
            }
            confirm_modal("Clear bars?", "Clear every hotbar slot, including the Oblivion bar?", []() {
                Bars::clear_bars();
                GameData::oblivion_bar.clear();
                status_message = "Cleared bars.";
            });

            if (ImGui::Button("Drag Main Bar")) {
                RenderManager::start_bar_dragging(0);
            }
            if (ImGui::Button("Drag Oblivion Mode Bar")) {
                RenderManager::start_bar_dragging(1);
            }
            bool unbind = Lifecycle::player_has_power(0);
            if (ImGui::Checkbox("Unbind Slot", &unbind) && unbind != Lifecycle::player_has_power(0)) {
                Lifecycle::toggle_player_power(0);
            }
            bool dual = Lifecycle::player_has_power(1);
            if (ImGui::Checkbox("Hotbar Dual Casting", &dual) && dual != Lifecycle::player_has_power(1)) {
                Lifecycle::toggle_player_power(1);
            }
            if (!status_message.empty()) {
                ImGui::TextUnformatted(status_message.c_str());
            }
        }

        void __stdcall render_keybinds() { draw_keybinds(); }
        void __stdcall render_bind_menu() { draw_bind_menu(); }
        void __stdcall render_settings() { draw_settings(); }
        void __stdcall render_bars() { draw_bars(); }
        void __stdcall render_perks() { draw_perks(); }
        void __stdcall render_presets() { draw_presets(); }
        void __stdcall render_spells() { draw_spells(); }
        void __stdcall render_util() { draw_util(); }
    }

    void register_pages()
    {
        SKSEMenuFramework::SetSection("Spell Hotbar 2");
        SKSEMenuFramework::AddSectionItem("Keybinds", render_keybinds);
        SKSEMenuFramework::AddSectionItem("Spell Bind Menu", render_bind_menu);
        SKSEMenuFramework::AddSectionItem("Settings", render_settings);
        SKSEMenuFramework::AddSectionItem("Bars", render_bars);
        SKSEMenuFramework::AddSectionItem("Perks", render_perks);
        SKSEMenuFramework::AddSectionItem("Presets", render_presets);
        SKSEMenuFramework::AddSectionItem("Spells", render_spells);
        SKSEMenuFramework::AddSectionItem("Util", render_util);
    }
}
