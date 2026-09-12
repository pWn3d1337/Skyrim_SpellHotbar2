#include "render_manager.h"
#include "../flick/flick_watch.h"
#include "../bar/hotbars.h"
#include "../bar/hotbar.h"
#include "../bar/oblivion_bar.h"

#include "../logger/logger.h"
#include "../storage/storage.h"
#include "../game_data/game_data.h"
#include "texture_csv_loader.h"
#include "../game_data/keynames_csv_loader.h"
#include <unordered_map>
#include "../input/input.h"
#include "../input/keybinds.h"
#include "../input/modes.h"
#include "ui_bridge.h"
#include "../flick/flick_windows.h"

#include <array>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace SpellHotbar {

    constexpr std::string_view images_root_path{ ".\\data\\SKSE\\Plugins\\SpellHotbar\\images\\" };

    namespace {
        std::uint32_t read_u32_le(const unsigned char* bytes)
        {
            return static_cast<std::uint32_t>(bytes[0]) |
                   (static_cast<std::uint32_t>(bytes[1]) << 8U) |
                   (static_cast<std::uint32_t>(bytes[2]) << 16U) |
                   (static_cast<std::uint32_t>(bytes[3]) << 24U);
        }

        bool read_texture_dimensions(const std::string& path, int& width, int& height)
        {
            if (std::filesystem::path(path).extension() == ".dds") {
                std::array<unsigned char, 20> header{};
                std::ifstream stream(path, std::ios::binary);
                if (stream.read(reinterpret_cast<char*>(header.data()), header.size()) &&
                    header[0] == 'D' && header[1] == 'D' && header[2] == 'S' && header[3] == ' ') {
                    height = static_cast<int>(read_u32_le(header.data() + 12));
                    width = static_cast<int>(read_u32_le(header.data() + 16));
                    return width > 0 && height > 0;
                }
                return false;
            }

            int channels = 0;
            return stbi_info(path.c_str(), &width, &height, &channels) != 0;
        }
    }


    TextureImage::TextureImage() : width(0), height(0)
    {
    }

    // Registers the atlas by path and reads its dimensions. Nothing here uploads a texture:
    // FLICK loads the file itself on the other side of the bridge (flick_images.h) and draws
    // from the UVs this side hands over, so all the plugin side ever needed was the size.
    bool TextureImage::load(const std::string& path)
    {
        source_path = path;
        if (!std::filesystem::exists(path)) {
            logger::error("Texture '{}' does not exist", path);
            return false;
        }

        if (!read_texture_dimensions(path, width, height)) {
            logger::warn("Could not read dimensions for texture '{}'", path);
            width = 1;
            height = 1;
        }
        return true;
    }

bool TextureImage::load_dds(const std::string& path)
{
    return load(path);
}

SubTextureImage::SubTextureImage(const TextureImage& other, ImVec2 uv0, ImVec2 uv1)
    :
    TextureImage(),
    uv0(uv0),
    uv1(uv1)
{
    this->width = other.width;
    this->height = other.height;
    //Keep the atlas path: the FLICK-hosted windows name textures by file.
    this->source_path = other.source_path;
}

std::vector<TextureImage> loaded_textures;
std::unordered_map<RE::FormID, SubTextureImage> spell_icons;
std::unordered_map<GameData::DefaultIconType, SubTextureImage> default_icons;
std::unordered_map<std::string, SubTextureImage> extra_icons;
std::vector<SubTextureImage> cooldown_icons;
std::vector<SubTextureImage> spellproc_overlay_icons;


// nested templates <3 
std::vector<std::tuple<std::string, std::vector<std::tuple<RE::FormID, std::string, SubTextureImage*>>>> editor_icon_list;

int highlight_slot = -1;
float highlight_time = 0.0F;
float highlight_dur_total = 1.0F;
bool highlight_isred = false;

bool menu_open = false;


// The engine's own backbuffer height, which is what ImGui's DisplaySize was under the plugin's
// own D3D hook. Read from the renderer rather than from FLICK so a co-save loads the same numbers
// whether or not the host is connected yet.
static float screen_height_px()
{
    const auto size = RE::BSGraphics::Renderer::GetScreenSize();
    return size.height > 0 ? static_cast<float>(size.height) : 1080.0f;
}

float SpellHotbar::RenderManager::scale_to_resolution(float normalized_value)
{
    return normalized_value * screen_height_px() / 1080.0f;
}
float SpellHotbar::RenderManager::scale_from_resolution(float scaled_value)
{
    return scaled_value / screen_height_px() * 1080.0f;
}

bool RenderManager::current_inv_menu_tab_valid_for_hotbar()
{
    auto ui = RE::UI::GetSingleton();
    /*if (ui != nullptr) {
        auto* invMenu = static_cast<RE::InventoryMenu*>(ui->GetMenu(RE::InventoryMenu::MENU_NAME).get());
        if (invMenu != nullptr && invMenu->uiMovie != nullptr && invMenu->uiMovie->IsAvailable("_root.Menu_mc.inventoryLists.categoryList.selectedIndex")) {
            //get current tab
            RE::GFxValue selection;
            //invMenu->uiMovie->GetVariable(&selection, "_root.Menu_mc.inventoryLists.categoryList.selectedEntry.text");
            invMenu->uiMovie->GetVariable(&selection, "_root.Menu_mc.inventoryLists.categoryList.selectedIndex");

            if (!selection.IsNull() && selection.IsNumber()) {
                int index = static_cast<int>(selection.GetNumber());
                return index >= 4 && index <= 6; //indices for scrolls, options and food
            }

        }
    }*/
    /*if (ui != nullptr) {
        auto* invMenu = static_cast<RE::InventoryMenu*>(ui->GetMenu(RE::InventoryMenu::MENU_NAME).get());
        if (invMenu != nullptr) {
            RE::GFxValue& root = invMenu->GetRuntimeData().root;

            if (root.GetType() == RE::GFxValue::ValueType::kDisplayObject && root.HasMember("inventoryLists")) {
                RE::GFxValue inventoryLists;
                root.GetMember("inventoryLists", &inventoryLists);

                if (inventoryLists.GetType() == RE::GFxValue::ValueType::kDisplayObject && inventoryLists.HasMember("categoryList")) {
                    RE::GFxValue categoryList;
                    inventoryLists.GetMember("categoryList", &categoryList);

                    if (categoryList.GetType() == RE::GFxValue::ValueType::kDisplayObject && categoryList.HasMember("selectedIndex")) {
                        RE::GFxValue selectedIndex;
                        categoryList.GetMember("selectedIndex", &selectedIndex);

                        if (selectedIndex.GetType() == RE::GFxValue::ValueType::kNumber) {
                            int index = static_cast<int>(selectedIndex.GetNumber());
                            return index >= 4 && index <= 6; //indices for scrolls, options and food
                        }
                    }
                }
            }
        }
    }
    return false;*/
    if (ui != nullptr) {
        auto* invMenu = static_cast<RE::InventoryMenu*>(ui->GetMenu(RE::InventoryMenu::MENU_NAME).get());
        return invMenu != nullptr;
    }

    return false;
}

bool RenderManager::current_selected_item_bindable()
{
    auto ui = RE::UI::GetSingleton();
    if (ui == nullptr) return false;

    auto* invMenu = static_cast<RE::InventoryMenu*>(ui->GetMenu(RE::InventoryMenu::MENU_NAME).get());

    if (!invMenu) return false; 

    RE::ItemList* item_list = invMenu->GetRuntimeData().itemList;
    if (item_list != nullptr) {
        RE::ItemList::Item* item = item_list->GetSelectedItem();
        if (item != nullptr && item->data.objDesc != nullptr) {
#undef GetObject // undefine stupid windows definition so GetObject() can be called
            RE::TESBoundObject* obj = item->data.objDesc->GetObject();
#ifdef UNICODE //redefine it
#define GetObject  GetObjectW
#else
#define GetObject  GetObjectA
#endif // !UNICODE
            if (obj != nullptr) {
                RE::FormID formID = obj->GetFormID();
                auto form = RE::TESForm::LookupByID(formID);
                if (form != nullptr) {
                    return Hotbar::is_valid_formtype_for_hotbar(form);
                }
            }
        }
    }
    return false;
}

bool RenderManager::should_overlay_be_rendered(GameData::DefaultIconType overlay)
{
    return overlay != GameData::DefaultIconType::UNKNOWN && overlay != GameData::DefaultIconType::NO_OVERLAY;
}

void update_highlight(float delta) {
    if (highlight_time > 0.0F) {
        highlight_time -= delta;
        if (highlight_time < 0.0F) {
            highlight_time = 0.0F;
            highlight_slot = -1;
            highlight_isred = false;
        }
    }
}

float get_highlight_factor()
{
    return highlight_time / highlight_dur_total;
}

void RenderManager::highlight_skill_slot(int id, float dur, bool error)
{ 
    highlight_slot = id;
    highlight_dur_total = dur;
    highlight_time = dur;
    highlight_isred = error;
}

enum class fade_type
{
    none,
    fade_in,
    fade_out
};

struct bar_fade {

    bar_fade() 
      : hud_fade_timer(1.0f),
        hud_fade_time_max(1.0f),
        hud_fade_type(fade_type::none),
        hud_fade_mod(key_modifier::none),
        last_should_show(false)
    {};

    void finish() {
        hud_fade_timer = 1.0f;
        hud_fade_time_max = 1.0f;
        hud_fade_type = fade_type::none;
        hud_fade_mod = key_modifier::none;
    }

    void update(float delta) {
        switch (hud_fade_type) {
        case fade_type::fade_in:
            hud_fade_timer += delta;
            if (hud_fade_timer >= hud_fade_time_max) {
                finish();
            }
            break;
        case fade_type::fade_out:
            hud_fade_timer -= delta;
            if (hud_fade_timer <= 0.0f) {
                finish();
            }
            break;
        default:
            break;
        }
    }

    void start_fade_in(float duration)
    {
        float new_val{ 0.0f };
        if (hud_fade_type == fade_type::fade_out) {
            float current_prog = hud_fade_timer / hud_fade_time_max;
            new_val = duration * current_prog;
        }

        hud_fade_type = fade_type::fade_in;
        hud_fade_timer = new_val;
        hud_fade_time_max = duration;
        hud_fade_mod = key_modifier::none;
    }

    void start_fade_out(float duration, key_modifier mod) {
        float new_val{ duration };
        if (hud_fade_type == fade_type::fade_in) {
            float current_prog = hud_fade_timer / hud_fade_time_max;
            new_val *= current_prog;
        }

        hud_fade_type = fade_type::fade_out;
        hud_fade_timer = new_val;
        hud_fade_time_max = duration;
        hud_fade_mod = mod;
    }

    bool is_hud_fading()
    {
        return hud_fade_type != fade_type::none;
    }

    bool is_hud_fading_out()
    {
        return hud_fade_type == fade_type::fade_out;
    }

    float get_bar_alpha()
    {
        return hud_fade_type == fade_type::none ? 1.0f : hud_fade_timer / hud_fade_time_max;
    }

    //Bar fade timers
    float hud_fade_timer;
    float hud_fade_time_max;
    fade_type hud_fade_type;
    key_modifier hud_fade_mod;
    bool last_should_show;
};

bar_fade main_bar_fade;
bar_fade oblivion_bar_fade;


key_modifier last_mod{ key_modifier::none };

void RenderManager::start_bar_dragging(int type)
{
    Flick::close_host_menu();
    Flick::open_bar_drag(type);
}

bool RenderManager::should_block_game_cursor_inputs() { return Flick::is_any_window_open(); }

void RenderManager::stop_bar_dragging()
{
    Flick::close_window(Flick::Window::bar_drag);
}

bool RenderManager::is_dragging_bar()
{
    return Flick::is_window_open(Flick::Window::bar_drag);
}

void RenderManager::open_spell_editor()
{
    Flick::close_host_menu();
    Flick::open_window(Flick::Window::spell_editor);
}

void RenderManager::close_spell_editor()
{
    Flick::close_window(Flick::Window::spell_editor);
}

void RenderManager::open_potion_editor()
{
    Flick::close_host_menu();
    Flick::open_window(Flick::Window::potion_editor);
}

void RenderManager::close_potion_editor()
{
    Flick::close_window(Flick::Window::potion_editor);
}

void RenderManager::open_advanced_binding_menu()
{
    // `menu_bar_id` starts at MAIN_BAR and was only re-synced inside the transform-bar branch of
    // the menu itself (Vampire Lord / Werewolf / custom), so an ordinary player always landed on
    // whatever bar was last selected. Open on the bar the player is actually holding; the combo
    // still lets them switch from there.
    const uint32_t live_bar = Bars::getCurrentHotbar_ingame();
    if (Bars::hotbars.contains(live_bar)) {
        Bars::menu_bar_id = live_bar;
    }
    // The vanilla menu underneath stays open and keeps its cursor. FLICK pauses the game for a
    // kPauseSoft window and pumps one cursor over everything.
    Flick::close_host_menu();
    Flick::open_window(Flick::Window::bind_menu);
}

bool RenderManager::is_bind_menu_opened()
{
    return Flick::is_window_open(Flick::Window::bind_menu);
}

bool RenderManager::should_block_game_key_inputs()
{
    return Flick::is_any_window_open();
}

void RenderManager::close_key_blocking_frames()
{
    Flick::close_all_windows();
}

bool RenderManager::has_custom_icon(RE::FormID form_id)
{
    return spell_icons.contains(form_id);
}

// text fade timers
float text_fade_timer{1.0f};
float text_fade_time_max{1.0f};
fade_type text_fade_type{fade_type::none};

uint32_t last_bar{0};

void start_text_fade(float time)
{
    text_fade_timer = time;
    text_fade_time_max = time;
}

void text_fade_check_last_bar(uint32_t bar) {
    if (last_bar != bar) {
        start_text_fade(2.5f);
    }
    last_bar = bar;
}

void text_fade_check_mod_change(key_modifier mod) {
    if (last_mod != mod) {
        start_text_fade(2.5f);
    }
}

void update_text_fade_timer(float delta) {
    if (text_fade_timer > delta) {
        text_fade_timer -= delta;
    } else {
        text_fade_timer = 0.0f;
    }
}

float get_text_fade_alpha() {
    if (Bars::text_show_setting == Bars::text_show_mode::never) {
        return 0.0f;
    } else if (Bars::text_show_setting == Bars::text_show_mode::always){
        return 1.0f;
    } else {
        if (text_fade_timer > 0.0f) {
            if (text_fade_timer <= (text_fade_time_max / 3.0f)) {
                    // Last third -> fade out
                    return text_fade_timer / (text_fade_time_max / 3.0f);

            } else if (text_fade_timer >= (text_fade_time_max * 2.0f / 3.0f)) {
                    // First third -> fade in
                    float f_max = (text_fade_time_max * 2.0f / 3.0f);
                    float f = text_fade_timer - f_max;
                    return 1.0f - (f / f_max);
            } else {
                    return 1.0f;  // middle third -> fully opaque
            }

        } else {
            return 0.0f;
        }
    }
}


void RenderManager::load_gamedata_dependant_resources() {
    TextureCSVLoader::load_icons(std::filesystem::path(images_root_path));
}

void RenderManager::load_fixed_textures() {
    // Nothing left. The frame background and the drawn cursor went with the ImGui windows:
    // FLICK draws the chrome and the host cursor is the cursor.
}

void RenderManager::reload_resouces() {

    //TODO fonts, reloading fonts in imgui is not that simple
    logger::info("Clearing all Resources...");

    GameData::key_names.clear();

    editor_icon_list.clear();
    spell_icons.clear();
    default_icons.clear();
    extra_icons.clear();
    cooldown_icons.clear();
    spellproc_overlay_icons.clear();

    loaded_textures.clear();

    logger::info("Reloading Resources...");
    GameData::load_keynames_file();
    RenderManager::load_fixed_textures();
    RenderManager::load_gamedata_dependant_resources();
}

void RenderManager::on_game_load()
{
}

TextureImage* RenderManager::load_texture(const std::string& path)
{
    TextureImage texture;
    if (!texture.load(path)) {
        return nullptr;
    }
    loaded_textures.push_back(std::move(texture));
    return &loaded_textures.back();
}

int RenderManager::load_texture_return_index(const std::string& path)
{
    if (!load_texture(path)) {
        return -1;
    }
    return static_cast<int>(loaded_textures.size()) - 1;
}

void RenderManager::add_spell_texture(TextureImage& main_texture, RE::FormID formID, ImVec2 uv0, ImVec2 uv1, const std::string& filename) {
    SubTextureImage tex_img(main_texture, uv0, uv1);
    auto it = spell_icons.find(formID);
    if (it != spell_icons.end()) {
        it->second = std::move(tex_img);
    } else {
        spell_icons.insert(std::make_pair(formID, std::move(tex_img)));
    }

    //add to editor_icon_list
    if (editor_icon_list.empty()) {
        editor_icon_list.emplace_back(std::make_tuple(filename, std::vector<std::tuple<RE::FormID, std::string, SubTextureImage*>>()));
    }
    else {
        auto lastfile = std::get<0>(editor_icon_list.back());
        if (lastfile != filename) {
            editor_icon_list.emplace_back(std::make_tuple(filename, std::vector<std::tuple<RE::FormID, std::string, SubTextureImage*>>()));
        }
    }
    auto& sub_vec = std::get<1>(editor_icon_list.back());
    sub_vec.emplace_back(std::make_tuple(formID, "", &spell_icons.at(formID)));
}

void RenderManager::add_default_icon(TextureImage& main_texture, GameData::DefaultIconType type, ImVec2 uv0, ImVec2 uv1, const std::string & icon_name)
{
    SubTextureImage tex_img(main_texture, uv0, uv1);
    default_icons.insert_or_assign(type, std::move(tex_img));

    //add to editor_icon_list
    if (editor_icon_list.empty()) {
        editor_icon_list.emplace_back(std::make_tuple("Default Icons", std::vector<std::tuple<RE::FormID, std::string, SubTextureImage*>>()));
    }
    else {
        auto lastfile = std::get<0>(editor_icon_list.back());
        if (lastfile != "Default Icons") {
            editor_icon_list.emplace_back(std::make_tuple("Default Icons", std::vector<std::tuple<RE::FormID, std::string, SubTextureImage*>>()));
        }
    }
    auto& sub_vec = std::get<1>(editor_icon_list.back());
    sub_vec.emplace_back(std::make_tuple(0, icon_name, &default_icons.at(type)));
}

void RenderManager::add_extra_icon(TextureImage& main_texture, const std::string& icon_name, ImVec2 uv0, ImVec2 uv1, const std::string& filename)
{
    SubTextureImage tex_img(main_texture, uv0, uv1);

    const std::string icon_key = filename + "_" + icon_name;
    extra_icons.insert_or_assign(icon_key, std::move(tex_img));

    //add to editor_icon_list
    if (editor_icon_list.empty()) {
        editor_icon_list.emplace_back(std::make_tuple(filename, std::vector<std::tuple<RE::FormID, std::string, SubTextureImage*>>()));
    }
    else {
        auto lastfile = std::get<0>(editor_icon_list.back());
        if (lastfile != filename) {
            editor_icon_list.emplace_back(std::make_tuple(filename, std::vector<std::tuple<RE::FormID, std::string, SubTextureImage*>>()));
        }
    }
    auto& sub_vec = std::get<1>(editor_icon_list.back());
    sub_vec.emplace_back(std::make_tuple(0, icon_key, &extra_icons.at(icon_key)));
}

void RenderManager::add_cooldown_icon(TextureImage& main_texture, ImVec2 uv0, ImVec2 uv1)
{
    cooldown_icons.emplace_back(main_texture, uv0, uv1);
}

void RenderManager::init_cooldown_icons(size_t amount) {
    cooldown_icons.clear();
    cooldown_icons.reserve(amount);
}

void RenderManager::add_spellproc_overlay_icon(TextureImage& main_texture, ImVec2 uv0, ImVec2 uv1)
{
    spellproc_overlay_icons.emplace_back(main_texture, uv0, uv1);
}

void RenderManager::init_spellproc_overlay_icons(size_t amount) {
    spellproc_overlay_icons.clear();
    spellproc_overlay_icons.reserve(amount);
}

inline SubTextureImage* lookup_default_icon(RE::FormID formID) {
    SubTextureImage* ret{ nullptr };
    // fallback icons
    GameData::DefaultIconType icon{ GameData::DefaultIconType::UNKNOWN };


    auto form = RE::TESForm::LookupByID(formID);

    if (form != nullptr) {

        if (form->GetFormType() == RE::FormType::AlchemyItem) {
            auto alch = form->As<RE::AlchemyItem>();
            if (alch && alch->IsFood()) {
                //check sound
                if (alch->data.consumptionSound == GameData::sound_ITMPotionUse) {
                    icon = GameData::DefaultIconType::GENERIC_FOOD_DRINK;
                }
                else if (alch->data.consumptionSound == GameData::sound_NPCHumanEatSoup) {
                    icon = GameData::DefaultIconType::GENERIC_FOOD_SOUP;
                }
                else {
                    icon = GameData::DefaultIconType::GENERIC_FOOD;
                }
            }
        }

        //No food icon found
        if (icon == GameData::DefaultIconType::UNKNOWN) {
            icon = GameData::get_fallback_icon_type(form);
        }
    }
    

    if (default_icons.contains(icon)) {
        ret = &default_icons.at(icon);
    }
    else {
        if (default_icons.contains(GameData::DefaultIconType::UNKNOWN)) {
            ret = &default_icons.at(GameData::DefaultIconType::UNKNOWN);
        }
    }
    return ret;
}

SubTextureImage* RenderManager::get_tex_for_skill_internal(RE::FormID formID)
{
    SubTextureImage* ret{ nullptr };
    if (formID != 0) {


        if (GameData::user_spell_cast_info.contains(formID)) {
            auto& dat = GameData::user_spell_cast_info.at(formID);
            if (dat.has_icon_data()) {

                if (dat.m_icon_form > 0) {

                    formID = dat.m_icon_form;
                    if (spell_icons.contains(formID)) {
                        return &spell_icons.at(formID);
                    }
                }
                else if (!dat.m_icon_str.empty()) {
                    if (TextureCSVLoader::default_icon_names.contains(dat.m_icon_str)) {
                        auto def_icon = TextureCSVLoader::default_icon_names.at(dat.m_icon_str);
                        return &default_icons.at(def_icon);
                    }
                    else if (extra_icons.contains(dat.m_icon_str)) {
                        return &extra_icons.at(dat.m_icon_str);
                    }
                }
            }
        }

        if (GameData::user_custom_entry_info.contains(formID)) {
            auto& dat = GameData::user_custom_entry_info.at(formID);
            if (dat.has_icon_data()) {
                if (dat.m_icon_form > 0) {

                    formID = dat.m_icon_form;
                    if (spell_icons.contains(formID)) {
                        return &spell_icons.at(formID);
                    }
                }
                else if (!dat.m_icon_str.empty()) {
                    if (TextureCSVLoader::default_icon_names.contains(dat.m_icon_str)) {
                        auto def_icon = TextureCSVLoader::default_icon_names.at(dat.m_icon_str);
                        return &default_icons.at(def_icon);
                    }
                    else if (extra_icons.contains(dat.m_icon_str)) {
                        return &extra_icons.at(dat.m_icon_str);
                    }
                }
            }
        }

        if (GameData::is_clear_spell(formID) && default_icons.contains(GameData::DefaultIconType::UNBIND_SLOT)) {
            ret = &default_icons.at(GameData::DefaultIconType::UNBIND_SLOT);
        }
        else if (GameData::is_toggle_dualcast_spell(formID) && default_icons.contains(GameData::DefaultIconType::DUAL_CAST) && default_icons.contains(GameData::DefaultIconType::SINGLE_CAST)) {
            GameData::DefaultIconType icon{ GameData::DefaultIconType::SINGLE_CAST };
            if (GameData::should_dual_cast()) {
                icon = GameData::DefaultIconType::DUAL_CAST;
            }
            ret = &default_icons.at(icon);
        }
        else if (spell_icons.contains(formID)) {
            ret = &spell_icons.at(formID);
        }
        else {
            ret = lookup_default_icon(formID);
        }
    }
    return ret;
}

void RenderManager::spell_slotted_draw_anim(int index)
{ 
    highlight_time = highlight_dur_total;
    highlight_slot = index;
}

ImU32 RenderManager::get_skill_color(const RE::TESForm* form) {
    ImU32 color = IM_COL32_WHITE;
    if (form != nullptr && form->GetFormType() == RE::FormType::AlchemyItem && form->IsDynamicForm()) {
        auto* alch_item = form->As<RE::AlchemyItem>();
        if (alch_item) {
            auto* effect = alch_item->GetCostliestEffectItem();
            if (effect) {
                color = Hotbar::calculate_potion_color(effect);
            }
        }
    }
    return color;
}

const SubTextureImage* RenderManager::resolve_skill_tex(RE::FormID formID)
{
    return get_tex_for_skill_internal(formID);
}

const SubTextureImage* RenderManager::named_icon_tex(const std::string& icon)
{
    if (TextureCSVLoader::default_icon_names.contains(icon)) {
        return default_icon_tex(TextureCSVLoader::default_icon_names.at(icon));
    }
    auto it = extra_icons.find(icon);
    return it == extra_icons.end() ? nullptr : &it->second;
}

const SubTextureImage* RenderManager::default_icon_tex(GameData::DefaultIconType type)
{
    auto it = default_icons.find(type);
    return it == default_icons.end() ? nullptr : &it->second;
}

const SubTextureImage* RenderManager::cooldown_tex(float cd)
{
    if (cooldown_icons.empty()) {
        return nullptr;
    }
    size_t i = static_cast<size_t>(std::round((cooldown_icons.size() - 1) * cd));
    return &cooldown_icons.at(std::clamp(i, static_cast<size_t>(0U), cooldown_icons.size() - 1U));
}

const SubTextureImage* RenderManager::spellproc_tex(float timer)
{
    if (spellproc_overlay_icons.empty()) {
        return nullptr;
    }
    const float prog = timer - std::floor(timer);  //run anim once per sec
    const size_t i = static_cast<size_t>(std::round((spellproc_overlay_icons.size() - 1) * prog));
    return &spellproc_overlay_icons.at(std::clamp(i, static_cast<size_t>(0U), spellproc_overlay_icons.size() - 1U));
}

const TextureImage* RenderManager::button_tex(int tex_index)
{
    if (tex_index < 0 || tex_index >= static_cast<int>(loaded_textures.size())) {
        return nullptr;
    }
    return &loaded_textures.at(tex_index);
}

inline
int wrap_index(int a, int n) {
    return ((a % n) + n) % n;
}

std::string RenderManager::get_skill_tooltip(const RE::TESForm* item) {
    std::string desc{ "" };
    if (item->GetFormType() == RE::FormType::Spell || item->GetFormType() == RE::FormType::Scroll)
    {
        const RE::SpellItem* spell = item->As<RE::SpellItem>();
        if (spell != nullptr) {
            RE::BSString buffer = "";
            RE::SpellItem* sp2 = const_cast<RE::SpellItem*>(spell);
            sp2->GetDescription(buffer, nullptr);
            std::stringstream desc_buf;
            bool first{ true };

            std::string b = buffer.c_str();
            if (!b.empty()) {
                desc_buf << b;
                first = false;
            }
            for (uint32_t e = 0; e < spell->effects.size(); e++) {
                auto eff = spell->effects[e];
                if (eff != nullptr && eff->baseEffect != nullptr) {
                    std::string eff_desc = eff->baseEffect->magicItemDescription.c_str();
                    if (!eff_desc.empty()) {
                        uint32_t dur = eff->GetDuration();
                        float mag = eff->GetMagnitude();
                        std::string eff_text = GameData::strip_tooltip(eff_desc, mag, dur);
                        if (!first) {
                            desc_buf << "\n";
                        }
                        else {
                            first = false;
                        }
                        desc_buf << eff_text;
                    }
                }
            }
            desc = desc_buf.str();
        }
    }
    else if (item->GetFormType() == RE::FormType::Shout) {
        const RE::TESShout* shout = item->As<RE::TESShout>();
        if (shout != nullptr) {
            RE::BSString buffer = "";
            RE::TESShout* sh2 = const_cast<RE::TESShout*>(shout);
            sh2->GetDescription(buffer, nullptr);
            desc = GameData::strip_tooltip(std::string(buffer.c_str()), 0.0f, 0);
        }
    }
    else if (item->GetFormType() == RE::FormType::AlchemyItem) {
        const RE::AlchemyItem* alch = item->As<RE::AlchemyItem>();
        std::stringstream desc_buf;
        bool first{ true };
        if (alch != nullptr) {
            for (uint32_t e = 0; e < alch->effects.size(); e++) {
                auto eff = alch->effects[e];
                if (eff != nullptr && eff->baseEffect != nullptr) {
                    std::string eff_desc = eff->baseEffect->magicItemDescription.c_str();
                    if (!eff_desc.empty()) {
                        if (!first) {
                            desc_buf << "\n";
                        }
                        else {
                            first = false;
                        }
                        uint32_t dur = eff->GetDuration();
                        float mag = eff->GetMagnitude();
                        std::string fixed_desc = GameData::strip_tooltip(eff_desc, mag, dur);

                        desc_buf << fixed_desc;
                    }
                }
            }
            desc = desc_buf.str();
        }
    }
    return desc;
}



//The per-frame HUD, built on demand for the FLICK HUD window. This was RenderManager::draw()
//under the plugin's own Present hook; now FLICK's SH2_Hud window asks for this frame's display
//lists from inside its own Draw(), on the render thread as before. The dock is handed over with
//submit_dock()/hide_dock() -- FLICK draws it in its own window over the vanilla menu.
UiBridge::HudFrame UiBridge::build_hud(float delta_seconds, float screen_size_x, float screen_size_y)
{
    HudFrame out;
    const float deltaTime = delta_seconds;
    update_highlight(deltaTime);
    main_bar_fade.update(deltaTime);
    if (Input::is_oblivion_mode()) {
        oblivion_bar_fade.update(deltaTime);
    }
    update_text_fade_timer(deltaTime);

    auto pc = RE::PlayerCharacter::GetSingleton();
    if (!pc || !pc->Is3DLoaded()) {
        Flick::hide_dock();
        return out;  // no player, no draw
    }

    auto ui = RE::UI::GetSingleton();
    if (!ui) {
        Flick::hide_dock();
        return out;
    }

    // check for ctrl/shift/alt modifiers
    key_modifier mod = Bars::get_current_modifier();

    auto* magMenu = static_cast<RE::MagicMenu*>(ui->GetMenu(RE::MagicMenu::MENU_NAME).get());
    auto* favMenu = static_cast<RE::FavoritesMenu*>(ui->GetMenu(RE::FavoritesMenu::MENU_NAME).get());

    // The dock shows whenever the Inventory menu is open. Gating it on the highlighted item
    // being slottable hid it on every weapon and armor, so most tabs never showed it.
    auto* invMenu = ui->GetMenu(RE::InventoryMenu::MENU_NAME).get();
    bool validTabActive = invMenu != nullptr;

    if (magMenu || validTabActive) {

        if (!menu_open) {
            // menu was open first time
            menu_open = true;
            Bars::menu_bar_id = Bars::getCurrentHotbar_ingame();
        }
        // An overlay is outside the menu. A FLICK window drawn over a vanilla
        // menu is not that menu, and RE::UI cannot see it -- which is how the in-menu bar ended
        // up painted across another guest's editor. This gate belongs to the IN-MENU bar only;
        // the gameplay HUD bar below is unaffected.
        const bool dock_visible = !Bars::disable_menu_rendering && !Flick::another_guest_owns_the_screen() &&
                                  !Flick::is_any_window_open() && !Bars::disable_menu_binding &&
                                  Flick::hosts_dock() && Bars::hotbars.contains(Bars::menu_bar_id);
        if (!dock_visible) {
            Flick::hide_dock();
        }
        else {
            bool render_icons = !Bars::disable_non_modifier_bar || Input::mod_1.isDown() || Input::mod_2.isDown() || Input::mod_3.isDown();

            //FLICK hosts the dock: build this frame's display list and hand it over.
            //FLICK draws it in its own window, over the vanilla menu, with its own mouse. Hover
            //comes back a frame later.
            auto& bar = Bars::hotbars.at(Bars::menu_bar_id);
            //Dragging the dock in place moves the same offsets the drag frame edits, so the
            //cosave persists it and the drag frame still agrees with it.
            float drag_dx{ 0.0f }, drag_dy{ 0.0f };
            if (Flick::take_dock_drag(drag_dx, drag_dy) && !Bars::menu_bar_locked) {
                Bars::menu_offset_x += drag_dx;
                Bars::menu_offset_y += drag_dy;
            }
            Flick::DockFrame frame;
            if (render_icons) {
                frame = bar.build_dock_frame(screen_size_x, screen_size_y, highlight_slot,
                                             get_highlight_factor(), mod, Flick::dock_hovered_slot());
            }
            //The dock's own arrows and bind button, now that it takes the mouse.
            const Flick::DockActions actions = Flick::take_dock_actions();
            if (actions.next_bar) {
                Bars::menu_bar_id = Bars::getNextMenuBar(Bars::menu_bar_id);
                RE::PlaySound(Input::sound_UISkillsForward);
            }
            if (actions.prev_bar) {
                Bars::menu_bar_id = Bars::getPreviousMenuBar(Bars::menu_bar_id);
                RE::PlaySound(Input::sound_UISkillsBackward);
            }
            if (actions.open_bind_menu) {
                RenderManager::open_advanced_binding_menu();
                RE::PlaySound(Input::sound_UISkillsForward);
            }
            if (actions.toggle_lock) {
                Bars::menu_bar_locked = !Bars::menu_bar_locked;
                RE::PlaySound(Input::sound_UISkillsForward);
            }
            frame.locked = Bars::menu_bar_locked;
            frame.header = Bars::hotbars.at(Bars::menu_bar_id).get_name();
            frame.icon = get_hud_slot_height(screen_size_y, Bars::menu_slot_scale);
            frame.spacing = Bars::menu_slot_spacing;
            frame.anchor = static_cast<int>(Bars::menu_bar_anchor_point);
            frame.offset_x = Bars::menu_offset_x;
            frame.offset_y = Bars::menu_offset_y;
            Flick::submit_dock(std::move(frame));
        }

    } else if (favMenu && GameData::hasFavMenuSlotBinding()) {
        // The vampire lord / werewolf bar in the Favorites menu: the dock again, on the
        // transform bar, without the paging arrows -- that bar is the only one the form has.
        const uint32_t bar_id = Bars::getCurrentHotbar_ingame();
        const bool dock_visible = !Bars::disable_menu_rendering && !Flick::another_guest_owns_the_screen() &&
                                  !Flick::is_any_window_open() && Flick::hosts_dock() && Bars::hotbars.contains(bar_id);
        if (!dock_visible) {
            Flick::hide_dock();
        }
        else {
            auto& bar = Bars::hotbars.at(bar_id);
            Flick::DockFrame frame = bar.build_dock_frame(screen_size_x, screen_size_y, highlight_slot,
                                                          get_highlight_factor(), mod, Flick::dock_hovered_slot());
            // The drag and the lock still work here; the arrows and the bind button do not
            // apply, so the actions are drained and dropped.
            float drag_dx{ 0.0f }, drag_dy{ 0.0f };
            if (Flick::take_dock_drag(drag_dx, drag_dy) && !Bars::menu_bar_locked) {
                Bars::menu_offset_x += drag_dx;
                Bars::menu_offset_y += drag_dy;
            }
            const Flick::DockActions actions = Flick::take_dock_actions();
            if (actions.toggle_lock) {
                Bars::menu_bar_locked = !Bars::menu_bar_locked;
                RE::PlaySound(Input::sound_UISkillsForward);
            }
            frame.controls = false;
            frame.locked = Bars::menu_bar_locked;
            frame.header = bar.get_name();
            frame.icon = get_hud_slot_height(screen_size_y, Bars::menu_slot_scale);
            frame.spacing = Bars::menu_slot_spacing;
            frame.anchor = static_cast<int>(Bars::menu_bar_anchor_point);
            frame.offset_x = Bars::menu_offset_x;
            frame.offset_y = Bars::menu_offset_y;
            Flick::submit_dock(std::move(frame));
        }
    } else {
        menu_open = false;
        Flick::hide_dock();

        auto [should_show, fade_dur] = GameData::shouldShowHUDBar();
        auto [should_show_oblivion, fade_dur_oblivion] = GameData::shouldShowOblivionHUDBar();
        if (!should_show && main_bar_fade.last_should_show) {
            //If a modifier caused a bar hide, fade out with previous mod
            key_modifier fademod = mod;
            if (mod != last_mod) {
                fademod = last_mod;
            }
            main_bar_fade.start_fade_out(fade_dur, fademod);
        } else if (should_show && !main_bar_fade.last_should_show) {
            main_bar_fade.start_fade_in(fade_dur);
        }
        main_bar_fade.last_should_show = should_show;

        //fading for oblivion bar
        if (Input::is_oblivion_mode()) {
            if (!should_show_oblivion && oblivion_bar_fade.last_should_show) {
                oblivion_bar_fade.start_fade_out(fade_dur_oblivion, key_modifier::none);
            }
            else if (should_show_oblivion && !oblivion_bar_fade.last_should_show) {
                oblivion_bar_fade.start_fade_in(fade_dur_oblivion);
            }
            oblivion_bar_fade.last_should_show = should_show_oblivion;
        }

        auto* hudMenu = static_cast<RE::HUDMenu*>(ui->GetMenu(RE::HUDMenu::MENU_NAME).get());
        float shout_cd_prog = 1.0f;
        float shout_cd_dur = 0.0;
        if (hudMenu) {
            shout_cd_prog = std::clamp(hudMenu->GetRuntimeData().shout->GetFillPct() * 0.01f, 0.0f, 1.0f);
            shout_cd_dur = hudMenu->GetRuntimeData().shout->cooldown;
        }

        if (should_show || main_bar_fade.is_hud_fading()) {
            uint32_t bar_id = Bars::getCurrentHotbar_ingame();
            if (Bars::hotbars.contains(bar_id)) {
                text_fade_check_last_bar(bar_id);

                const float alpha = main_bar_fade.get_bar_alpha();
                const float text_alpha = get_text_fade_alpha();
                auto& bar = Bars::hotbars.at(bar_id);

                key_modifier m = mod;
                if (main_bar_fade.is_hud_fading_out()) {
                    //If fading out, do not react to modifier changes visually
                    m = main_bar_fade.hud_fade_mod;
                }

                bar.build_hud_layer(out.main, screen_size_x, screen_size_y, highlight_slot, get_highlight_factor(), m,
                                    highlight_isred, alpha, shout_cd_prog, shout_cd_dur, text_alpha);
                out.main.alpha = alpha;
                out.main.text_alpha = text_alpha;
                out.main.anchor = static_cast<int>(Bars::bar_anchor_point);
                out.main.offset_x = Bars::offset_x;
                out.main.offset_y = Bars::offset_y;
            }
        }

        if (Input::is_oblivion_mode() && (should_show_oblivion || oblivion_bar_fade.is_hud_fading())) {
            const float alpha = oblivion_bar_fade.get_bar_alpha();
            GameData::oblivion_bar.build_hud_layer(out.oblivion, screen_size_x, screen_size_y, highlight_slot,
                                                   get_highlight_factor(), key_modifier::none, highlight_isred, alpha,
                                                   shout_cd_prog, shout_cd_dur);
            out.oblivion.anchor = static_cast<int>(Bars::oblivion_bar_anchor_point);
            out.oblivion.offset_x = Bars::oblivion_offset_x;
            out.oblivion.offset_y = Bars::oblivion_offset_y;
        }
    }
    last_mod = mod;
    return out;
}

// ---- The seam to the FLICK-hosted windows (ui_bridge.h). Plain types out; the FLICK side loads
// the atlas by path and draws from the UVs. ----
namespace UiBridge {
    namespace {
        std::optional<IconRef> ref_of(const SubTextureImage* a_img)
        {
            if (a_img == nullptr || a_img->source_path.empty()) {
                return std::nullopt;
            }
            IconRef ref;
            ref.path = a_img->source_path;
            ref.u0 = a_img->uv0.x;
            ref.v0 = a_img->uv0.y;
            ref.u1 = a_img->uv1.x;
            ref.v1 = a_img->uv1.y;
            ref.aspect = a_img->height > 0 ? static_cast<float>(a_img->width) / static_cast<float>(a_img->height) : 1.0f;
            return ref;
        }
    }

    std::optional<IconRef> skill_icon(RE::FormID a_form)
    {
        return ref_of(RenderManager::resolve_skill_tex(a_form));
    }

    std::optional<IconRef> named_icon(const std::string& a_icon)
    {
        return ref_of(RenderManager::named_icon_tex(a_icon));
    }

    std::optional<IconRef> default_icon(GameData::DefaultIconType a_type)
    {
        return ref_of(RenderManager::default_icon_tex(a_type));
    }

    std::optional<IconRef> extra_icon(const std::string& a_key)
    {
        auto it = extra_icons.find(a_key);
        return it == extra_icons.end() ? std::nullopt : ref_of(&it->second);
    }

    std::optional<IconRef> cooldown_icon(float a_cd)
    {
        return ref_of(RenderManager::cooldown_tex(a_cd));
    }

    std::optional<IconRef> key_icon(int a_texture_index)
    {
        const TextureImage* tex = RenderManager::button_tex(a_texture_index);
        if (tex == nullptr || tex->source_path.empty()) {
            return std::nullopt;
        }
        IconRef ref;
        ref.path = tex->source_path;
        ref.aspect = tex->height > 0 ? static_cast<float>(tex->width) / static_cast<float>(tex->height) : 1.0f;
        return ref;
    }

    unsigned skill_color(const RE::TESForm* a_form)
    {
        return static_cast<unsigned>(RenderManager::get_skill_color(a_form));
    }

    std::string skill_tooltip(const RE::TESForm* a_form)
    {
        return a_form == nullptr ? std::string() : RenderManager::get_skill_tooltip(a_form);
    }

    bool has_custom_icon(RE::FormID a_form)
    {
        return RenderManager::has_custom_icon(a_form);
    }

    bool should_overlay_be_rendered(GameData::DefaultIconType a_overlay)
    {
        return RenderManager::should_overlay_be_rendered(a_overlay);
    }

    void reload_resources()
    {
        RenderManager::reload_resouces();
    }

    std::vector<EditorIconGroup> editor_icon_groups()
    {
        std::vector<EditorIconGroup> groups;
        groups.reserve(editor_icon_list.size());
        for (const auto& [name, entries] : editor_icon_list) {
            EditorIconGroup group;
            group.name = name;
            group.entries.reserve(entries.size());
            for (const auto& [form, icon, _tex] : entries) {
                group.entries.push_back(EditorIconEntry{ form, icon });
            }
            groups.push_back(std::move(group));
        }
        return groups;
    }
}
}
