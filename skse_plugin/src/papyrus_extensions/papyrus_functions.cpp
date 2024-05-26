#include "papyrus_functions.h"
#include "../storage/storage.h"
#include "../logger/logger.h"
#include "../bar/hotbars.h"
#include "../bar/hotbar.h"
#include "../game_data/game_data.h"
#include "../rendering/render_manager.h"
#include "../casts/casting_controller.h"
#include "../input/keybinds.h"
#include "../storage/user_data_io.h"

int get_number_of_slots(RE::StaticFunctionTag*)
{
    return static_cast<int>(SpellHotbar::Bars::barsize);
}

int set_number_of_slots(RE::StaticFunctionTag*, int num)
{
    SpellHotbar::Bars::barsize = static_cast<uint8_t>(std::clamp(num, 1, static_cast<int>(SpellHotbar::max_bar_size)));
    return static_cast<int>(SpellHotbar::Bars::barsize);
}

float get_slot_scale(RE::StaticFunctionTag*)
{
    return SpellHotbar::Bars::slot_scale;
}

float set_slot_scale(RE::StaticFunctionTag*, float scale)
{
    return SpellHotbar::Bars::slot_scale = scale;
}

float get_offset_x(RE::StaticFunctionTag*, bool rescale)
{
    if (rescale) {
        return SpellHotbar::RenderManager::scale_from_resolution(SpellHotbar::Bars::offset_x);
    }
    else {
        return SpellHotbar::Bars::offset_x;
    }
}
float set_offset_x(RE::StaticFunctionTag*, float value, bool rescale)
{
    if (rescale) {
        SpellHotbar::Bars::offset_x = SpellHotbar::RenderManager::scale_to_resolution(value);
    }
    else {
        SpellHotbar::Bars::offset_x = value;
    }
    return SpellHotbar::Bars::offset_x;
}

float get_offset_y(RE::StaticFunctionTag*, bool rescale)
{
    if (rescale) {
        return SpellHotbar::RenderManager::scale_from_resolution(SpellHotbar::Bars::offset_y);
    }
    else {
        return SpellHotbar::Bars::offset_y;
    }
}
float set_offset_y(RE::StaticFunctionTag*, float value, bool rescale)
{
    if (rescale) {
        SpellHotbar::Bars::offset_y = SpellHotbar::RenderManager::scale_to_resolution(value);
    }
    else {
        SpellHotbar::Bars::offset_y = value;
    }
    return SpellHotbar::Bars::offset_y;
}

bool is_transformed_with_fav_menu_binding(RE::StaticFunctionTag*) {
    if (SpellHotbar::GameData::isCustomTransform()) {
        auto type = SpellHotbar::GameData::getCustomTransformCasttype();
        return type == SpellHotbar::GameData::custom_transform_spell_type::fav_menu ||
            type == SpellHotbar::GameData::custom_transform_spell_type::fav_menu_switch;
    }
    return false;
}

bool toggle_bar_enabled(RE::StaticFunctionTag*, int barID)
{
    bool is_enabled_after{false};
    if (SpellHotbar::Bars::hotbars.contains(barID)) {
        is_enabled_after = !SpellHotbar::Bars::hotbars.at(barID).is_enabled();
        SpellHotbar::Bars::hotbars.at(barID).set_enabled(is_enabled_after);
    } else {
        logger::warn("toggle_bar_enabled: Unknown bar id: {}", barID);
    }
    return is_enabled_after;
}
bool get_bar_enabled(RE::StaticFunctionTag*, int barID) {
    bool ret{false};
    if (SpellHotbar::Bars::hotbars.contains(barID)) {
        ret = SpellHotbar::Bars::hotbars.at(barID).is_enabled();
    } else {
        logger::warn("get_bar_enabled: Unknown bar id: {}", barID);
    }
    return ret;
}

int get_inherit_mode(RE::StaticFunctionTag*, int barID)
{
    if (SpellHotbar::Bars::hotbars.contains(barID))
    {
        return SpellHotbar::Bars::hotbars.at(barID).get_inherit_mode();
    }
    logger::warn("get_inherit_mode: Unknown bar id: {}", barID);
    return 0;
}

int set_inherit_mode(RE::StaticFunctionTag*, int barID, int inherit_mode)
{
    if (SpellHotbar::Bars::hotbars.contains(barID)) {
        return SpellHotbar::Bars::hotbars.at(barID).set_inherit_mode(inherit_mode);
    }
    logger::warn("set_inherit_mode: Unknown bar id: {}", barID);
    return 0;
}

int set_hud_bar_show_mode(RE::StaticFunctionTag*, int show_mode)
{
    SpellHotbar::Bars::bar_show_setting = SpellHotbar::Bars::bar_show_mode(std::clamp(show_mode, 0, 5));
    return static_cast<int>(SpellHotbar::Bars::bar_show_setting);
}

int get_hud_bar_show_mode(RE::StaticFunctionTag*)
{
    return static_cast<int>(SpellHotbar::Bars::bar_show_setting);
}

void reload_resources(RE::StaticFunctionTag*)
{ 
    SpellHotbar::RenderManager::reload_resouces();
}

void reload_spelldata(RE::StaticFunctionTag*)
{
    SpellHotbar::GameData::reload_data();
}

inline void _set_spell_casttime(RE::TESForm* form, float& out_casttime)
{
    if (form->GetFormType() == RE::FormType::Spell) {
        RE::SpellItem* spell = form->As<RE::SpellItem>();
        if (spell->GetChargeTime() > 0.01f) {
            out_casttime = spell->GetChargeTime();
        }
    }
}

void show_drag_bar(RE::StaticFunctionTag*)
{
    SpellHotbar::RenderManager::start_bar_dragging();
}

int set_hud_bar_show_mode_vampire_lord(RE::StaticFunctionTag*, int show_mode) {
    SpellHotbar::Bars::bar_show_setting_vampire_lord = SpellHotbar::Bars::bar_show_mode(std::clamp(show_mode, 0, 2));
    return static_cast<int>(SpellHotbar::Bars::bar_show_setting_vampire_lord);
}

int get_hud_bar_show_mode_vampire_lord(RE::StaticFunctionTag*) {
    return static_cast<int>(SpellHotbar::Bars::bar_show_setting_vampire_lord);
}

int set_hud_bar_show_mode_werewolf(RE::StaticFunctionTag*, int show_mode) {
    SpellHotbar::Bars::bar_show_setting_werewolf = SpellHotbar::Bars::bar_show_mode(std::clamp(show_mode, 0, 2));
    return static_cast<int>(SpellHotbar::Bars::bar_show_setting_werewolf);
}

int get_hud_bar_show_mode_werewolf(RE::StaticFunctionTag*) {
    return static_cast<int>(SpellHotbar::Bars::bar_show_setting_werewolf);
}

bool save_bars_to_file(RE::StaticFunctionTag*, std::string filename)
{ 
    return SpellHotbar::Bars::save_bars_to_json(filename);
}

bool file_exists(RE::StaticFunctionTag*, std::string filepath)
{ 
    return std::filesystem::exists(filepath);
}

bool load_bars_from_file(RE::StaticFunctionTag*, std::string filename_mod_dir, std::string filename_user_dir)
{
    if (std::filesystem::exists(filename_user_dir)) {
        return SpellHotbar::Bars::load_bars_from_json(filename_user_dir);

    } else if (std::filesystem::exists(filename_mod_dir)) {
        return SpellHotbar::Bars::load_bars_from_json(filename_mod_dir);

    } else {
        logger::error("Could not load bars, both '{}' and '{}' do not exist!", filename_user_dir, filename_mod_dir);
        return false;
    }
}

void clear_bars(RE::StaticFunctionTag*)
{
    SpellHotbar::Bars::clear_bars();
}

float get_slot_spacing(RE::StaticFunctionTag*)
{
    return static_cast<float>(SpellHotbar::Bars::slot_spacing);
}

float set_slot_spacing(RE::StaticFunctionTag*, float spacing)
{
    SpellHotbar::Bars::slot_spacing = std::max(0, static_cast<int>(spacing));
    return static_cast<float>(SpellHotbar::Bars::slot_spacing);
}

int get_text_show_mode(RE::StaticFunctionTag*)
{ 
    return static_cast<int>(SpellHotbar::Bars::text_show_setting);
}

int set_text_show_mode(RE::StaticFunctionTag*, int value)
{ 
    SpellHotbar::Bars::text_show_setting = SpellHotbar::Bars::text_show_mode(static_cast<uint8_t>(std::clamp(value, 0, 2)));
    return static_cast<int>(SpellHotbar::Bars::text_show_setting);
}

bool is_default_bar_when_sheated(RE::StaticFunctionTag*) {
    return SpellHotbar::Bars::use_default_bar_when_sheathed;
}

bool toggle_default_bar_when_sheathed(RE::StaticFunctionTag*) {
    return SpellHotbar::Bars::use_default_bar_when_sheathed = !SpellHotbar::Bars::use_default_bar_when_sheathed;
}

bool is_disable_menu_rendering(RE::StaticFunctionTag*) {
    return SpellHotbar::Bars::disable_menu_rendering;
}

bool toggle_disable_menu_rendering(RE::StaticFunctionTag*) {
    return SpellHotbar::Bars::disable_menu_rendering = !SpellHotbar::Bars::disable_menu_rendering;
}

int set_key_bind(RE::StaticFunctionTag*, int key, int code) {
    int bind{ 0 };
    if (key >= 0 && key < SpellHotbar::Input::keybind_id::num_keys) {
        bind = SpellHotbar::Input::rebind_key(key, code);
    }
    return bind;
}
int get_key_bind(RE::StaticFunctionTag*, int key) {
    int bind{ 0 };
    if (key >= 0 && key < SpellHotbar::Input::keybind_id::num_keys) {
        bind = SpellHotbar::Input::get_keybind(key);
    }
    return bind;
}

bool is_non_mod_bar_disabled(RE::StaticFunctionTag*) {
    return SpellHotbar::Bars::disable_non_modifier_bar;
}

bool toggle_disable_non_mod_bar(RE::StaticFunctionTag*) {
    return SpellHotbar::Bars::disable_non_modifier_bar = !SpellHotbar::Bars::disable_non_modifier_bar;
}

void open_spell_editor(RE::StaticFunctionTag*)
{
    SpellHotbar::RenderManager::open_spell_editor();
}

bool load_config(RE::StaticFunctionTag*, std::string filename, bool include_user_dir)
{
    return SpellHotbar::Storage::IO::load_preset(filename, include_user_dir);
}

bool save_config(RE::StaticFunctionTag*, std::string filename)
{
    return SpellHotbar::Storage::IO::save_preset(filename);
}

std::vector<std::string> get_config_presets(RE::StaticFunctionTag*) {
    return SpellHotbar::Storage::IO::get_config_presets();
}

std::vector<std::string> get_bars_presets(RE::StaticFunctionTag*) {
    return SpellHotbar::Storage::IO::get_bar_presets();
}

int get_bar_anchor_point(RE::StaticFunctionTag*) {
    return static_cast<int>(SpellHotbar::Bars::bar_anchor_point);
}

int set_bar_anchor_point(RE::StaticFunctionTag*, int value) {
    SpellHotbar::Bars::bar_anchor_point = SpellHotbar::Bars::anchor_point(std::clamp(value, 0, static_cast<int>(SpellHotbar::Bars::anchor_point::CENTER)));
    return static_cast<int>(SpellHotbar::Bars::bar_anchor_point);
}

bool SpellHotbar::register_papyrus_functions(RE::BSScript::IVirtualMachine* vm) {
    vm->RegisterFunction("getNumberOfSlots", "SpellHotbar", get_number_of_slots);
    vm->RegisterFunction("setNumberOfSlots", "SpellHotbar", set_number_of_slots);
    vm->RegisterFunction("setSlotScale", "SpellHotbar", set_slot_scale);
    vm->RegisterFunction("getSlotScale", "SpellHotbar", get_slot_scale);
    vm->RegisterFunction("setOffsetX", "SpellHotbar", set_offset_x);
    vm->RegisterFunction("getOffsetX", "SpellHotbar", get_offset_x);
    vm->RegisterFunction("setOffsetY", "SpellHotbar", set_offset_y);
    vm->RegisterFunction("getOffsetY", "SpellHotbar", get_offset_y);
    vm->RegisterFunction("toggleBarEnabled", "SpellHotbar", toggle_bar_enabled);
    vm->RegisterFunction("getBarEnabled", "SpellHotbar", get_bar_enabled);
    vm->RegisterFunction("getInheritMode", "SpellHotbar", get_inherit_mode);
    vm->RegisterFunction("setInheritMode", "SpellHotbar", set_inherit_mode);
    vm->RegisterFunction("setHudBarShowMode", "SpellHotbar", set_hud_bar_show_mode);
    vm->RegisterFunction("getHudBarShowMode", "SpellHotbar", get_hud_bar_show_mode);
    vm->RegisterFunction("reloadResources", "SpellHotbar", reload_resources);
    vm->RegisterFunction("reloadData", "SpellHotbar", reload_spelldata);
    vm->RegisterFunction("showDragBar", "SpellHotbar", show_drag_bar);
    vm->RegisterFunction("setHudBarShowModeVampireLord", "SpellHotbar", set_hud_bar_show_mode_vampire_lord);
    vm->RegisterFunction("getHudBarShowModeVampireLord", "SpellHotbar", get_hud_bar_show_mode_vampire_lord);
    vm->RegisterFunction("setHudBarShowModeWerewolf", "SpellHotbar", set_hud_bar_show_mode_werewolf);
    vm->RegisterFunction("getHudBarShowModeWerewolf", "SpellHotbar", get_hud_bar_show_mode_werewolf);
    vm->RegisterFunction("saveBarsToFile", "SpellHotbar", save_bars_to_file);
    vm->RegisterFunction("fileExists", "SpellHotbar", file_exists);
    vm->RegisterFunction("loadBarsFromFile", "SpellHotbar", load_bars_from_file);
    vm->RegisterFunction("clearBars", "SpellHotbar", clear_bars);
    vm->RegisterFunction("setSlotSpacing", "SpellHotbar", set_slot_spacing);
    vm->RegisterFunction("getSlotSpacing", "SpellHotbar", get_slot_spacing);
    vm->RegisterFunction("getTextShowMode", "SpellHotbar", get_text_show_mode);
    vm->RegisterFunction("setTextShowMode", "SpellHotbar", set_text_show_mode);
    vm->RegisterFunction("isTransformedFavMenuBind", "SpellHotbar", is_transformed_with_fav_menu_binding);
    vm->RegisterFunction("isDefaultBarWhenSheathed", "SpellHotbar", is_default_bar_when_sheated);
    vm->RegisterFunction("toggleDefaultBarWhenSheathed", "SpellHotbar", toggle_default_bar_when_sheathed);
    vm->RegisterFunction("isDisableMenuRendering", "SpellHotbar", is_disable_menu_rendering);
    vm->RegisterFunction("toggleDisableMenuRendering", "SpellHotbar", toggle_disable_menu_rendering);
    vm->RegisterFunction("SetKeyBind", "SpellHotbar", set_key_bind);
    vm->RegisterFunction("GetKeyBind", "SpellHotbar", get_key_bind);
    vm->RegisterFunction("isNonModBarDisabled", "SpellHotbar", is_non_mod_bar_disabled);
    vm->RegisterFunction("toggleDisableNonModBar", "SpellHotbar", toggle_disable_non_mod_bar);
    vm->RegisterFunction("openSpellEditor", "SpellHotbar", open_spell_editor);
    vm->RegisterFunction("loadConfig", "SpellHotbar", load_config);
    vm->RegisterFunction("saveConfig", "SpellHotbar", save_config);
    vm->RegisterFunction("getConfigPresets", "SpellHotbar", get_config_presets);
    vm->RegisterFunction("getBarsPresets", "SpellHotbar", get_bars_presets);
    vm->RegisterFunction("getBarAnchorPoint", "SpellHotbar", get_bar_anchor_point);
    vm->RegisterFunction("setBarAnchorPoint", "SpellHotbar", set_bar_anchor_point);

    return true;
}