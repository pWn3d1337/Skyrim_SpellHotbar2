#include "render_manager.h"
#include "texture_loader.h"
#include "../bar/hotbars.h"
#include "../bar/hotbar.h"
#include "../bar/oblivion_bar.h"

#include "../logger/logger.h"
#include "../storage/storage.h"

#include <d3d11.h>
#include <d3d11_3.h>

#include <dxgi.h>
#include "../game_data/game_data.h"
#include "texture_csv_loader.h"
#include "../game_data/keynames_csv_loader.h"
#include <unordered_map>
#include "../input/input.h"
#include "../input/keybinds.h"
#include "../input/modes.h"
#include "spell_editor.h"
#include "potion_editor.h"
#include "bar_dragging_config_window.h"
#include "advanced_bind_menu.h"

#include <imgui_internal.h>

// Hook render stuff for imgui, mostly copied from wheeler
namespace stl {
    using namespace SKSE::stl;

    template <class T>
    void write_thunk_call() {
        auto& trampoline = SKSE::GetTrampoline();
        const REL::Relocation<std::uintptr_t> hook{T::id, T::offset};
        T::func = trampoline.write_call<5>(hook.address(), T::thunk);
    }
}

namespace SpellHotbar {

    constexpr std::string_view images_root_path{ ".\\data\\SKSE\\Plugins\\SpellHotbar\\images\\" };
    constexpr std::string_view texture_frame_bg_path{ ".\\data\\SKSE\\Plugins\\SpellHotbar\\images\\inv_bg.dds" };
    constexpr std::string_view texture_cursor_path{ ".\\data\\SKSE\\Plugins\\SpellHotbar\\images\\cursor.dds" };

    void apply_imgui_style() {
        //set imgui style

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        //Set all roundings to 0
        style.WindowRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.GrabRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.PopupRounding = 0.0f;
        style.TabRounding = 0.0f;
        
        constexpr ImVec4 color_black = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
        constexpr ImVec4 color_dark_gray = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        constexpr ImVec4 color_very_dark_gray = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);
        constexpr ImVec4 color_light_gray = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        constexpr ImVec4 color_medium_gray = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        constexpr ImVec4 color_highlight = ImVec4(0.6f, 0.6f, 0.6f, 1.00f);

        colors[ImGuiCol_TitleBg] = color_black;
        colors[ImGuiCol_TitleBgActive] = color_dark_gray;
        colors[ImGuiCol_TitleBgCollapsed] = color_light_gray;

        colors[ImGuiCol_Button] = color_dark_gray;
        colors[ImGuiCol_ButtonHovered] = color_light_gray;
        colors[ImGuiCol_ButtonActive] = color_medium_gray;

        colors[ImGuiCol_FrameBg] = color_dark_gray;
        colors[ImGuiCol_FrameBgActive] = color_medium_gray;
        colors[ImGuiCol_FrameBgHovered] = color_highlight;

        colors[ImGuiCol_CheckMark] = color_light_gray;

        colors[ImGuiCol_Header] = color_medium_gray;
        colors[ImGuiCol_HeaderHovered] = color_highlight;
        colors[ImGuiCol_HeaderActive] = color_light_gray;

        colors[ImGuiCol_DragDropTarget] = color_highlight;

        colors[ImGuiCol_Separator] = color_medium_gray;
        colors[ImGuiCol_SeparatorHovered] = color_light_gray;
        colors[ImGuiCol_SeparatorActive] = color_highlight;

        colors[ImGuiCol_TableRowBg] = color_black;
        colors[ImGuiCol_TableRowBgAlt] = color_very_dark_gray;

        colors[ImGuiCol_SliderGrab] = color_light_gray;
        colors[ImGuiCol_SliderGrabActive] = color_highlight;
    } 


    TextureImage::TextureImage() : res(nullptr), width(0), height(0)
    {
    }

    bool TextureImage::load(const std::string& path)
{
    if (path.ends_with("dds")) {
        return TextureLoader::fromDDSFile(path, &res, &width, &height);
    }
    else {
        return TextureLoader::fromFile(path.c_str(), &res, &width, &height);
    }
}

bool TextureImage::load_dds(const std::string& path)
{
    return TextureLoader::fromDDSFile(path, &res, &width, &height);
}

void TextureImage::draw(float w, float h)
{
    ImGui::Image(get_res(), ImVec2(w, h));
}

void TextureImage::draw(float w, float h, float alpha)
{
    ImGui::Image(get_res(), ImVec2(w, h), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0, 1.0, 1.0, alpha));
}

void TextureImage::draw_on_top(ImVec2 pos, float w, float h, ImU32 col) {
    ImGui::GetWindowDrawList()->AddImage(get_res(), pos, ImVec2(pos.x + w, pos.y + h), ImVec2(0.0, 0.0), ImVec2(1.0, 1.0), col);
}

void TextureImage::draw()
{
    draw(static_cast<float>(width), static_cast<float>(height));
}

ImTextureID TextureImage::get_res()
{
    return (ImTextureID)res;
}

SubTextureImage::SubTextureImage(const TextureImage& other, ImVec2 uv0, ImVec2 uv1)
    :
    TextureImage(),
    uv0(uv0),
    uv1(uv1)
{
    this->res = other.res;
    this->width = other.width;
    this->height = other.height;
}

void SubTextureImage::draw(float w, float h)
{
    ImGui::Image(get_res(), ImVec2(w, h), uv0, uv1);
}
void SubTextureImage::draw(float w, float h, float alpha)
{ 
    ImGui::Image(get_res(), ImVec2(w, h), uv0, uv1, ImVec4(1.0f, 1.0f, 1.0f, alpha));
}

void SubTextureImage::draw_with_scale(float w, float h, ImU32 col, float scale) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(w, h));

    float dx = ((w * scale) - w) * 0.5f;
    float dy = ((h * scale) - h) * 0.5f;

    ImGui::GetWindowDrawList()->AddImage(get_res(), ImVec2(pos.x - dx, pos.y - dy), ImVec2(pos.x + w + dx, pos.y + h + dy), uv0, uv1,
                                         col);
}

void SubTextureImage::draw_on_top(ImVec2 pos, float w, float h, ImU32 col)
{
    ImGui::GetWindowDrawList()->AddImage(get_res(), pos, ImVec2(pos.x + w, pos.y + h), uv0, uv1, col);
}

void SubTextureImage::draw_with_scale_at(ImVec2 pos, float w, float h, ImU32 col, float scale) {
    float dx = ((w * scale) - w) * 0.5f;
    float dy = ((h * scale) - h) * 0.5f;

    ImGui::GetWindowDrawList()->AddImage(get_res(), ImVec2(pos.x - dx, pos.y - dy), ImVec2(pos.x + w + dx, pos.y + h + dy), uv0, uv1,
        col);
}

ImFont* font_text = nullptr;
ImFont* font_text_title = nullptr;
ImFont* font_text_big = nullptr;
ImFont* font_symbols = nullptr;

float font_text_size{0};

std::vector<TextureImage> loaded_textures;
std::unordered_map<RE::FormID, SubTextureImage> spell_icons;
std::unordered_map<GameData::DefaultIconType, SubTextureImage> default_icons;
std::unordered_map<std::string, SubTextureImage> extra_icons;
std::vector<SubTextureImage> cooldown_icons;
std::vector<SubTextureImage> spellproc_overlay_icons;

long frame_bg_texture_index{ -1 };
long cursor_texture_index{ -1 };

// nested templates <3 
std::vector<std::tuple<std::string, std::vector<std::tuple<RE::FormID, std::string, SubTextureImage*>>>> editor_icon_list;

int highlight_slot = -1;
float highlight_time = 0.0F;
float highlight_dur_total = 1.0F;
bool highlight_isred = false;

bool menu_open = false;

bool show_drag_frame = false;
bool drag_frame_initialized = false;
ImVec2 drag_window_pos = ImVec2(0,0);
float drag_window_width = 0.0f;
float drag_window_height = 0.0f;
ImVec2 drag_window_start_pos = ImVec2(0, 0);
float drag_window_start_width = 0.0f;
float drag_window_start_height = 0.0f;
int dragged_window = 0;

float SpellHotbar::RenderManager::scale_to_resolution(float normalized_value)
{
    auto & io = ImGui::GetIO();
    return normalized_value * io.DisplaySize.y / 1080.0f;
}
float SpellHotbar::RenderManager::scale_from_resolution(float scaled_value)
{
    auto& io = ImGui::GetIO();
    return scaled_value / io.DisplaySize.y * 1080.0f;
}

void RenderManager::draw_custom_mouse_cursor(float cursor_size) {
    if (cursor_texture_index >= 0 && cursor_texture_index < loaded_textures.size()) {
        auto& io = ImGui::GetIO();
        io.MouseDrawCursor = false;
        auto res = loaded_textures[cursor_texture_index].get_res();
        float scale_factor = io.DisplaySize.y / 1080.0f;
        // draw texture centered, default cursor is only in bottom right quarter.
        float half_draw_size = cursor_size * scale_factor;
        ImVec2 mouse_pos = ImGui::GetMousePos();
        ImVec2 p1 = ImVec2(mouse_pos.x - half_draw_size, mouse_pos.y - half_draw_size);
        ImVec2 p2 = ImVec2(p1.x + half_draw_size*2.0f, p1.y + half_draw_size*2.0f);
        ImGui::GetForegroundDrawList()->AddImage(res, p1, p2);
    }
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
    show_drag_frame = true;
    drag_frame_initialized = false;

    drag_window_pos = ImVec2(0, 0);
    drag_window_width = 0.0f;
    drag_window_height = 0.0f;
    drag_window_start_pos = ImVec2(0, 0);
    drag_window_start_width = 0;
    drag_window_start_height = 0;
    dragged_window = type;
}

bool RenderManager::should_block_game_cursor_inputs() { return show_drag_frame || SpellEditor::is_opened() || PotionEditor::is_opened() || BindMenu::is_opened(); }

void RenderManager::stop_bar_dragging()
{ 
    show_drag_frame = false;
}

bool RenderManager::is_dragging_bar()
{
    return show_drag_frame;
}

void RenderManager::open_spell_editor()
{
    SpellEditor::show();
}

void RenderManager::close_spell_editor()
{
    SpellEditor::hide();
}

void RenderManager::open_potion_editor()
{
    PotionEditor::show();
}

void RenderManager::close_potion_editor()
{
    PotionEditor::hide();
}

void RenderManager::open_advanced_binding_menu()
{
    if (!BindMenu::is_opened()) {
        BindMenu::show();
    }
}

bool RenderManager::is_bind_menu_opened()
{
    return BindMenu::is_opened();
}

void RenderManager::ImGui_push_title_style()
{
    constexpr ImVec4 col_transparent(0.0f, 0.0f, 0.0f, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_TitleBg, col_transparent);
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, col_transparent);
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, col_transparent);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f)); //Center
    ImGui::PushFont(font_text_title);
}

void RenderManager::ImGui_pop_title_style()
{
    ImGui::PopFont();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
}

std::tuple<float, float, float> RenderManager::calculate_frame_size(float screen_percent, float aspect_ratio)
{
    auto& io = ImGui::GetIO();
    const float screen_size_x = io.DisplaySize.x, screen_size_y = io.DisplaySize.y;

    float frame_height = screen_size_y * screen_percent;
    float frame_width = frame_height * aspect_ratio;

    ImGui::SetNextWindowSize(ImVec2(frame_width, frame_height));
    ImGui::SetNextWindowPos(ImVec2((screen_size_x - frame_width) * 0.5f, (screen_size_y - frame_height) * 0.5f));

    return std::make_tuple(screen_size_x, screen_size_y, frame_width);
}

void RenderManager::set_large_font()
{
    ImGui::PushFont(font_text_big);
}
void RenderManager::revert_font()
{
    ImGui::PopFont();
}

bool RenderManager::should_block_game_key_inputs()
{
    return SpellEditor::is_opened() || PotionEditor::is_opened() || BindMenu::is_opened();
}

void RenderManager::close_key_blocking_frames()
{
    if (SpellEditor::is_opened()) {
        close_spell_editor();
    }
    if (PotionEditor::is_opened()) {
        close_potion_editor();
    }
    if (BindMenu::is_opened()) {
        BindMenu::hide();
    }
}

bool RenderManager::has_custom_icon(RE::FormID form_id)
{
    return spell_icons.contains(form_id);
}

std::vector<std::tuple<std::string, std::vector<std::tuple<RE::FormID, std::string, SubTextureImage*>>>>& RenderManager::get_editor_icon_list()
{
    return editor_icon_list;
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


template <typename T>
void _check_ptr(T* ptr, std::string name) {
    if (ptr == nullptr) {
        logger::error("Error loading {}", name);
    }
}
#define CHECK_PTR(a) _check_ptr(a, #a) //this passes both as var and string(name of var)

void load_font_resources(float window_height) {
    //calculate required font size, defaults are for 1080p
    float scale_factor = window_height / 1080.0f;

    font_text_size = std::roundf(24.0f * scale_factor);
    float font_text_title_size = std::round(font_text_size * 1.5f);
    float font_text_big_size = std::round(font_text_size * 1.25f);
    float size_symbols = std::roundf(36.0f * scale_factor); 

    logger::info("Loading Fonts with sizes {}, {}", font_text_size, size_symbols);

    ImGuiIO& io = ImGui::GetIO();

    std::string_view text_font_folder(".\\data\\SKSE\\Plugins\\SpellHotbar\\fonts");
    //look for files named text_font-codepage.ttf, if no "-" found in the filename, use default.
    //first file starting with 'text_font' and ending in .ttf will be used
    std::string text_font_name = "text_font.ttf";
    const ImWchar* glyph_range = 0;
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(text_font_folder))) {
        if (entry.is_regular_file()) {
            std::string str_filename = entry.path().filename().string();
            if (str_filename.ends_with(".ttf") && str_filename.starts_with("text_font")) {
                size_t ind = str_filename.find_last_of('-');
                text_font_name = entry.path().filename().string();
                if (ind != std::string::npos) {
                    ind += 1; //text starts after -
                    std::string glyph_range_text = text_font_name.substr(ind, text_font_name.length() - ind - 4);
                    logger::info("loading glyph ranges for '{}'", glyph_range_text);
                    if (glyph_range_text == "chinese") {
                        glyph_range = ImGui::GetIO().Fonts->GetGlyphRangesChineseFull();
                    }
                    else if (glyph_range_text == "cyrillic") {
                        glyph_range = ImGui::GetIO().Fonts->GetGlyphRangesCyrillic();
                    }
                    else if (glyph_range_text == "greek") {
                        glyph_range = ImGui::GetIO().Fonts->GetGlyphRangesGreek();
                    }
                    else if (glyph_range_text == "japanese") {
                        glyph_range = ImGui::GetIO().Fonts->GetGlyphRangesJapanese();
                    }
                    else if (glyph_range_text == "korean") {
                        glyph_range = ImGui::GetIO().Fonts->GetGlyphRangesKorean();
                    }
                    else if (glyph_range_text == "thai") {
                        glyph_range = ImGui::GetIO().Fonts->GetGlyphRangesThai();
                    }
                    else if (glyph_range_text == "vietnamese") {
                        glyph_range = ImGui::GetIO().Fonts->GetGlyphRangesVietnamese();
                    }
                    else {
                        logger::error("Unkown glyph range in text_font filename: '{}'", glyph_range_text);
                    }
                }
                break;
            }
        }
    }
    auto font_path = std::filesystem::path(text_font_folder) / text_font_name;
    font_text = io.Fonts->AddFontFromFileTTF(
        font_path.string().c_str(), font_text_size, NULL, glyph_range);


    if (glyph_range == ImGui::GetIO().Fonts->GetGlyphRangesChineseFull()) {
        // cannot load cn font 3 times, there will be a crash (https://github.com/pWn3d1337/Skyrim_SpellHotbar2/issues/38)
        font_text_title = font_text;
        font_text_big = font_text;
    }
    else {
        font_text_title = io.Fonts->AddFontFromFileTTF(
            font_path.string().c_str(), font_text_title_size, NULL, glyph_range);

        font_text_big = io.Fonts->AddFontFromFileTTF(
            font_path.string().c_str(), font_text_big_size, NULL, glyph_range);
    }

    font_symbols = io.Fonts->AddFontFromFileTTF(
        ".\\data\\SKSE\\Plugins\\SpellHotbar\\fonts\\skyrim_symbols_font.ttf",
       size_symbols);

    CHECK_PTR(font_text);
    CHECK_PTR(font_text_title);
    CHECK_PTR(font_text_big);
    CHECK_PTR(font_symbols);
}

void RenderManager::load_gamedata_dependant_resources() {
    TextureCSVLoader::load_icons(std::filesystem::path(images_root_path));
}

void RenderManager::load_fixed_textures() {
    if (std::filesystem::exists(std::filesystem::path(texture_frame_bg_path))) {
        RenderManager::load_texture(std::string(texture_frame_bg_path));
        frame_bg_texture_index = static_cast<long>(loaded_textures.size()) - 1;
    }
    else {
        logger::error("Could not Load texture {}", texture_frame_bg_path);
    }

    if (std::filesystem::exists(std::filesystem::path(texture_cursor_path))) {
        RenderManager::load_texture(std::string(texture_cursor_path));
        cursor_texture_index = static_cast<long>(loaded_textures.size()) - 1;
    }
    else {
        logger::error("Could not Load texture {}", texture_cursor_path);
    }
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

    cursor_texture_index = -1;
    frame_bg_texture_index = -1;
    for (const auto& teximg : loaded_textures) {
        teximg.res->Release();
    }
    loaded_textures.clear();

    logger::info("Reloading Resources...");
    GameData::load_keynames_file();
    RenderManager::load_fixed_textures();
    RenderManager::load_gamedata_dependant_resources();
}

void RenderManager::on_game_load()
{
}

TextureImage & RenderManager::load_texture(const std::string path) {
    TextureImage tex_img;

    if (tex_img.load(path)) {
        loaded_textures.push_back(std::move(tex_img));
    }
    return loaded_textures.back();
}

int RenderManager::load_texture_return_index(const std::string path)
{
    TextureImage tex_img;

    if (tex_img.load(path)) {
        loaded_textures.push_back(std::move(tex_img));
        return static_cast<int>(loaded_textures.size()) - 1;
    }
    else {
        return -1;
    }
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

LRESULT RenderManager::WndProcHook::thunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto& io = ImGui::GetIO();
    if (uMsg == WM_KILLFOCUS) {
        io.ClearInputCharacters();
        io.ClearInputKeys();
    }

    return func(hWnd, uMsg, wParam, lParam);
}

void RenderManager::D3DInitHook::thunk() {
    func();

    logger::info("RenderManager: Initializing...");
    auto renderer = RE::BSGraphics::Renderer::GetSingleton();
    if (!renderer){
        logger::error("Cannot find renderer. Initialization failed!");
        return;
    }
    auto render_data = renderer->GetRendererData();
    if (!render_data) {
        logger::error("Cannot get renderer data. Initialization failed!");
        return;
    }

    logger::info("Getting swapchain...");
    auto render_window = renderer->GetCurrentRenderWindow();
    if (!render_window) {
        logger::error("Cannot get render_window. Initialization failed!");
        return;
    }
    auto swapchain = render_window->swapChain;
    if (!swapchain) {
        logger::error("Cannot get swapChain. Initialization failed!");
        return;
    }

    logger::info("Getting swapchain desc...");
    REX::W32::DXGI_SWAP_CHAIN_DESC sd{};
    if (swapchain->GetDesc(std::addressof(sd)) < 0) {
        logger::error("IDXGISwapChain::GetDesc failed.");
        return;
    }

    device = render_data->forwarder;
    context = render_data->context;

    logger::info("Initializing ImGui...");
    ImGui::CreateContext();

    //auto& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    if (!ImGui_ImplWin32_Init(sd.outputWindow)) {
        logger::error("ImGui initialization failed (Win32)");
        return;
    } else {
        //ImGui_ImplWin32_EnableAlphaCompositing(sd.outputWindow);
    }
    if (!ImGui_ImplDX11_Init((ID3D11Device*)device, (ID3D11DeviceContext*)context)) {
        logger::error("ImGui initialization failed (DX11)");
        return;
    }

    logger::info("...ImGui Initialized");

    initialized.store(true);

    WndProcHook::func = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrA(sd.outputWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcHook::thunk)));
    if (!WndProcHook::func) logger::error("SetWindowLongPtrA failed!");

    logger::trace("Loading fonts...");
    //get window size for font from HWND since imgui does not know size yet
    REX::W32::RECT rect;
    if (REX::W32::GetWindowRect(sd.outputWindow, &rect)) {
        int height = rect.y2 - rect.y1;
        load_font_resources(static_cast<float>(height));
    } else {
        logger::error("Could not get window size for font loading");
    }
    load_fixed_textures();

    logger::info("RenderManager: Initialized");
}

void RenderManager::DXGIPresentHook::thunk(std::uint32_t a_p1) {
    func(a_p1);

    if (!D3DInitHook::initialized.load()) return;

    // start imgui
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // My stuff
    RenderManager::draw();

    // end imgui
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void RenderManager::MessageCallback(SKSE::MessagingInterface::Message* msg)
{
    if (msg->type == SKSE::MessagingInterface::kDataLoaded && D3DInitHook::initialized) {
        auto& io = ImGui::GetIO();
        io.MouseDrawCursor = false;
        io.WantSetMousePos = false;
    }
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

bool RenderManager::install() {
    auto g_message = SKSE::GetMessagingInterface();
    if (!g_message) {
        logger::error("Messaging Interface Not Found!");
        return false;
    }

    g_message->RegisterListener(MessageCallback);

    SKSE::AllocTrampoline(14 * 2);

    stl::write_thunk_call<D3DInitHook>();
    stl::write_thunk_call<DXGIPresentHook>();

    return true;
}

void RenderManager::spell_slotted_draw_anim(int index)
{ 
    highlight_time = highlight_dur_total;
    highlight_slot = index;
}

void RenderManager::draw_bg(int size, float alpha)
{
    if (default_icons.contains(GameData::DefaultIconType::BAR_EMPTY)) {
        auto& sub_image = default_icons.at(GameData::DefaultIconType::BAR_EMPTY);
        sub_image.draw(static_cast<float>(size), static_cast<float>(size), alpha);
    } else {
        ImGui::Dummy(ImVec2(static_cast<float>(size), static_cast<float>(size)));
    }
}

void RenderManager::draw_frame_bg(bool *show_frame) {
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::Begin("##background_image", show_frame, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoInputs);
    RenderManager::draw_frame_bg_texture(ImGui::GetWindowSize().x, ImGui::GetWindowSize().y, 1.0f);
    ImGui::PopStyleVar(3);
    ImGui::End();
}

void RenderManager::draw_frame_bg_texture(float size_x, float size_y, float alpha)
{
    constexpr float tex_size = 256.0f;
    constexpr float segment_size = tex_size / 3.0f;

    constexpr float one_third = 1.0f / 3.0f;
    constexpr float two_third = 1.0f - one_third;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    if (frame_bg_texture_index < 0 || frame_bg_texture_index >= loaded_textures.size()) return;
    auto res = loaded_textures[frame_bg_texture_index].get_res();

    if (size_x > tex_size && size_y > tex_size) {
        int num_segments_x = static_cast<int>(size_x / segment_size);
        int num_segments_y = static_cast<int>(size_y / segment_size);

        auto col = ImColor(1.0f, 1.0f, 1.0f, alpha);
        for (int y = 0; y < num_segments_y; y++) for (int x=0; x < num_segments_x; x++)
        {
            ImVec2 p = ImVec2(pos.x + x * segment_size, pos.y + y * segment_size);

            if (y == 0 && x == 0) {
                ImGui::GetWindowDrawList()->AddImage(res, p, ImVec2(p.x + segment_size, p.y + segment_size),
                    ImVec2(0.0f,0.0f), ImVec2(one_third, one_third), col);
            }
            else if (y == 0 && x == num_segments_x - 1) {

                ImGui::GetWindowDrawList()->AddImage(res, ImVec2(pos.x + size_x -segment_size, p.y), ImVec2(pos.x + size_x, p.y + segment_size),
                    ImVec2(two_third, 0.0f), ImVec2(1.0f, one_third), col);

                //draw partial segment
                float partial_size = pos.x + size_x - segment_size - p.x;
                float partial_percent = partial_size / segment_size * one_third;

                ImGui::GetWindowDrawList()->AddImage(res, ImVec2(p.x, p.y), ImVec2(p.x + partial_size, p.y + segment_size),
                    ImVec2(one_third, 0.0f), ImVec2(one_third+partial_percent, one_third), col);

            }
            else if (y == num_segments_y - 1  && x == 0) {

                ImGui::GetWindowDrawList()->AddImage(res, ImVec2(p.x, pos.y + size_y - segment_size), ImVec2(p.x + segment_size, pos.y + size_y),
                    ImVec2(0.0f, two_third), ImVec2(one_third, 1.0f), col);

                //draw partial segment
                float partial_size = pos.y + size_y - segment_size - p.y;
                float partial_percent = partial_size / segment_size * one_third;

                ImGui::GetWindowDrawList()->AddImage(res, ImVec2(p.x, p.y), ImVec2(p.x + segment_size, p.y + partial_size),
                    ImVec2(0.0f, one_third+partial_percent), ImVec2(one_third, two_third), col);
            }
            else if (y == num_segments_y - 1 && x == num_segments_x - 1) {

                ImGui::GetWindowDrawList()->AddImage(res, ImVec2(pos.x + size_x - segment_size, pos.y + size_y - segment_size), ImVec2(pos.x + size_x, pos.y + size_y),
                    ImVec2(two_third, two_third), ImVec2(1.0f, 1.0f), col);

                //draw partial segments
                float partial_size_x = pos.x + size_x -segment_size - p.x;
                float partial_percent_x = partial_size_x / segment_size * one_third;
                float partial_size_y = pos.y + size_y -segment_size - p.y;
                float partial_percent_y = partial_size_y / segment_size * one_third;

                //top
                ImGui::GetWindowDrawList()->AddImage(res, ImVec2(pos.x + size_x - segment_size, p.y), ImVec2(pos.x + size_x, p.y + partial_size_y),
                    ImVec2(two_third, one_third+partial_percent_y), ImVec2(1.0f, two_third), col);

                //left
                ImGui::GetWindowDrawList()->AddImage(res, ImVec2(p.x, pos.y + size_y - segment_size), ImVec2(p.x + partial_size_x, pos.y + size_y),
                    ImVec2(one_third+ partial_percent_x, two_third), ImVec2(two_third, 1.0f), col);

                //top-left
                ImGui::GetWindowDrawList()->AddImage(res, ImVec2(p.x, p.y), ImVec2(pos.x + size_x - segment_size, pos.y + size_y - segment_size),
                    ImVec2(one_third + partial_percent_x, one_third + partial_percent_y), ImVec2(two_third, two_third), col);

            }
            else if (x == 0) {

                ImGui::GetWindowDrawList()->AddImage(res, ImVec2(p.x, p.y), ImVec2(p.x + segment_size, p.y+segment_size),
                    ImVec2(0.0f, one_third), ImVec2(one_third, two_third), col);
            }
            else if (x == num_segments_x - 1) {

                ImGui::GetWindowDrawList()->AddImage(res, ImVec2(pos.x + size_x - segment_size, p.y), ImVec2(pos.x + size_x, p.y + segment_size),
                    ImVec2(two_third, one_third), ImVec2(1.0f, two_third), col);

                //draw partial segment
                float partial_size = pos.x + size_x -segment_size - p.x;
                float partial_percent = partial_size / segment_size * one_third;

                ImGui::GetWindowDrawList()->AddImage(res, ImVec2(p.x, p.y), ImVec2(p.x + partial_size, p.y + segment_size),
                    ImVec2(one_third, one_third), ImVec2(one_third + partial_percent, two_third), col);
            }
            else if (y == 0) {

                ImGui::GetWindowDrawList()->AddImage(res, ImVec2(p.x, p.y), ImVec2(p.x + segment_size, p.y + segment_size),
                    ImVec2(one_third, 0.0f), ImVec2(two_third, one_third), col);
            }
            else if (y == num_segments_y - 1) {

                ImGui::GetWindowDrawList()->AddImage(res, ImVec2(p.x, pos.y + size_y - segment_size), ImVec2(p.x + segment_size, pos.y + size_y),
                    ImVec2(one_third, two_third), ImVec2(two_third, 1.0f), col);

                //draw partial segment
                float partial_size = pos.y + size_y -segment_size - p.y;
                float partial_percent = partial_size / segment_size * one_third;

                ImGui::GetWindowDrawList()->AddImage(res, ImVec2(p.x, p.y), ImVec2(p.x + segment_size, p.y + partial_size),
                    ImVec2(one_third, one_third + partial_percent), ImVec2(two_third, two_third), col);
            }
            else
            {
                ImGui::GetWindowDrawList()->AddImage(res, p, ImVec2(p.x+segment_size, p.y+segment_size),
                    ImVec2(one_third, one_third), ImVec2(two_third, two_third), col);
            }
        
        }
    
    }
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

bool RenderManager::draw_skill(RE::FormID formID, int size, ImU32 col) {
    constexpr float scale = 1.0f;
    SubTextureImage* img = get_tex_for_skill_internal(formID);
    if (img) {
        img->draw_with_scale(static_cast<float>(size), static_cast<float>(size), col, scale);
        return true;
    }
    else {
        return false;
    }
}

bool RenderManager::draw_skill_in_editor(RE::FormID formID, ImVec2 pos, int size, ImU32 col)
{
    SubTextureImage* img = get_tex_for_skill_internal(formID);
    if (img != nullptr) {
        ImGui::GetWindowDrawList()->AddImage(img->get_res(), ImVec2(pos.x, pos.y), ImVec2(pos.x + size, pos.y + size),
            img->uv0, img->uv1, col);

        draw_slot_overlay(pos, size);
        return true;
    }
    else {
        return false;
    }
}

void RenderManager::draw_default_icon_in_editor(GameData::DefaultIconType icon_type, ImVec2 pos, int size, ImU32 col)
{
    if (default_icons.contains(icon_type)) {
        auto & img = default_icons.at(icon_type);
        ImGui::GetWindowDrawList()->AddImage(img.get_res(), ImVec2(pos.x, pos.y), ImVec2(pos.x + size, pos.y + size),
            img.uv0, img.uv1, col);

    }
    draw_slot_overlay(pos, size);
}

void RenderManager::draw_extra_icon_in_editor(const std::string& key, ImVec2 pos, int size, ImU32 col)
{
    if (extra_icons.contains(key)) {
        auto& img = extra_icons.at(key);
        ImGui::GetWindowDrawList()->AddImage(img.get_res(), ImVec2(pos.x, pos.y), ImVec2(pos.x + size, pos.y + size),
            img.uv0, img.uv1, col);
    }
    draw_slot_overlay(pos, size);
}

void RenderManager::draw_slot_overlay(ImVec2 pos, int size, ImU32 col)
{
    if (default_icons.contains(GameData::DefaultIconType::BAR_OVERLAY)) {
        auto& overlay = default_icons.at(GameData::DefaultIconType::BAR_OVERLAY);
        overlay.draw_on_top(pos, static_cast<float>(size), static_cast<float>(size), col);
    } 
}

void RenderManager::draw_cd_overlay(ImVec2 pos, int size, float cd, ImU32 col) {
    if (cooldown_icons.size() > 0) {
        size_t i = static_cast<size_t>(std::round((cooldown_icons.size()-1) * cd));
        size_t index = std::clamp(i, static_cast<size_t>(0U), static_cast<size_t>(cooldown_icons.size()-1U));
        cooldown_icons.at(index).draw_on_top(pos, static_cast<float>(size), static_cast<float>(size), col);
    }
}

void RenderManager::draw_spellproc_overlay(ImVec2 pos, int size, float timer, float total, float alpha) {
    if (spellproc_overlay_icons.size() > 0) {
        float prog = timer - std::floor(timer); //run anim once per sec
        size_t i = static_cast<size_t>(std::round((spellproc_overlay_icons.size() - 1) * prog));
        size_t index = std::clamp(i, static_cast<size_t>(0U), static_cast<size_t>(spellproc_overlay_icons.size() - 1U));

        //fade out
        constexpr float fade_out_time = 1.5f;
        if (timer > (total - fade_out_time)) {
            float p = std::clamp(timer - (total - fade_out_time) / fade_out_time, 0.0f, 1.0f);
            alpha *= p;
        }
        spellproc_overlay_icons.at(index).draw_on_top(pos, static_cast<float>(size), static_cast<float>(size), IM_COL32(255, 255, 255, alpha*255));
    }
}

void RenderManager::draw_highlight_overlay(ImVec2 pos, int size, ImU32 col)
{
    if (default_icons.contains(GameData::DefaultIconType::BAR_HIGHLIGHT)) {
        auto& overlay = default_icons.at(GameData::DefaultIconType::BAR_HIGHLIGHT);
        overlay.draw_on_top(pos, static_cast<float>(size), static_cast<float>(size), col);
    }
}

void RenderManager::draw_button_icon(ImVec2 pos, int tex_index, int tex_index_modifier, int icon_size, ImU32 col)
{
    if (tex_index_modifier < 0) {
        if (tex_index >= 0 && tex_index < loaded_textures.size()) {
            auto& texture = loaded_textures.at(tex_index);
            float sizef = static_cast<float>(icon_size);
            float s =sizef * 0.5f;
            float aspect = static_cast<float>(texture.width) / static_cast<float>(texture.height);
            
            float total_width = aspect * s;

            ImVec2 draw_pos = ImVec2(pos.x + icon_size * 0.5f - total_width*0.5f, pos.y + sizef* keybind_icon_pos_factor);

            texture.draw_on_top(draw_pos, total_width, s, col);
        }
    }
    else {
        if (tex_index >= 0 && tex_index < loaded_textures.size() && tex_index_modifier >= 0 && tex_index_modifier < loaded_textures.size()) {
            
            auto& texture_key = loaded_textures.at(tex_index);
            auto& texture_mod = loaded_textures.at(tex_index_modifier);
            
            float sizef = static_cast<float>(icon_size);
            float target_height = sizef * 0.5f;
            float aspect_key = static_cast<float>(texture_key.width) / static_cast<float>(texture_key.height);
            float aspect_mod = static_cast<float>(texture_mod.width) / static_cast<float>(texture_mod.height);

            float total_width = target_height * aspect_key + target_height * aspect_mod;
            float max_width = static_cast<float>(icon_size) * 0.95f;
            if (total_width >  max_width) {
                target_height *= max_width / total_width;
                total_width = target_height * aspect_key + target_height * aspect_mod;
            }

            ImVec2 draw_pos = ImVec2(pos.x + icon_size * 0.5f - total_width * 0.5f, pos.y + sizef * keybind_icon_pos_factor);
            texture_mod.draw_on_top(draw_pos, target_height * aspect_mod, target_height, col);
            texture_key.draw_on_top(ImVec2(draw_pos.x + target_height * aspect_mod, draw_pos.y), target_height * aspect_key, target_height, col);
        }
    }
}

void RenderManager::draw_button_icon_menu(ImVec2 pos, int tex_index, int tex_index_modifier, int size, ImU32 col)
{
    if (tex_index_modifier < 0) {
        if (tex_index >= 0 && tex_index < loaded_textures.size()) {
            auto& texture = loaded_textures.at(tex_index);
            float sizef = static_cast<float>(size);
            float aspect = static_cast<float>(texture.width) / static_cast<float>(texture.height);

            texture.draw_on_top(pos, sizef*aspect, sizef, col);
        }
    }
    else {
        if (tex_index >= 0 && tex_index < loaded_textures.size() && tex_index_modifier >= 0 && tex_index_modifier < loaded_textures.size()) {

            auto& texture_key = loaded_textures.at(tex_index);
            auto& texture_mod = loaded_textures.at(tex_index_modifier);

            float sizef = static_cast<float>(size);
            float aspect_key = static_cast<float>(texture_key.width) / static_cast<float>(texture_key.height);
            float aspect_mod = static_cast<float>(texture_mod.width) / static_cast<float>(texture_mod.height);
            texture_mod.draw_on_top(pos, sizef * aspect_mod, sizef, col);
            texture_key.draw_on_top(ImVec2(pos.x + sizef * aspect_mod, pos.y), sizef * aspect_key, sizef, col);
        }
    }
}

float RenderManager::get_button_icons_length(int tex_index_key, int tex_index_mod)
{
    float ret{ 0.0f };
    if (tex_index_key >= 0 && tex_index_key < loaded_textures.size()) {
        auto& texture_key = loaded_textures.at(tex_index_key);
        ret += texture_key.width / texture_key.height;
    }
    if (tex_index_mod >= 0 && tex_index_mod < loaded_textures.size()) {
        auto& texture_mod = loaded_textures.at(tex_index_mod);
        ret += texture_mod.width / texture_mod.height;
    }
    return ret;
}

void RenderManager::draw_scaled_text(ImVec2 pos, ImU32 col, const char* text)
{
    float size = get_scaled_text_size_multiplier() * font_text_size;
    ImGui::GetWindowDrawList()->AddText(font_text, size, pos, col, text);
}

float RenderManager::get_scaled_text_size_multiplier()
{
    return (Bars::slot_scale + 0.25f);
}

void RenderManager::draw_icon_overlay(ImVec2 pos, int size, GameData::DefaultIconType type, ImU32 col)
{
    if (default_icons.contains(type)) {
        auto& overlay = default_icons.at(type);
        overlay.draw_on_top(pos, static_cast<float>(size), static_cast<float>(size), col);
    }
}

void TextCenterHorizontal(std::string text) {
    float font_size = ImGui::GetFontSize() * text.size() / 2;
    ImGui::SameLine(ImGui::GetWindowSize().x / 2 - font_size + (font_size / 2));
    ImGui::Text(text.c_str());
}

void drawCenteredText(std::string text, float alpha = 1.0F) {
    auto windowWidth = ImGui::GetWindowSize().x;
    auto textWidth = ImGui::CalcTextSize(text.c_str()).x;

    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::TextColored(ImColor(1.0f, 1.0f, 1.0f, alpha), text.c_str());
}

inline bool is_ultrawide(const float & screen_size_x, const float & screen_size_y) {
    //16:9 is ~1.777, 21:9 is ~2.333
    return screen_size_x / screen_size_y >= 2.0f;
}

/**
* Return screen_size_x, screen_size_y, window_width
*/
inline std::tuple<float, float, float> calculate_menu_window_size(bool include_icon_height = true)
{
    const ImVec2 default_spacing = ImVec2(8, 4);
    //use default spacing in menu
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, default_spacing);

    const float screen_size_x = ImGui::GetIO().DisplaySize.x, screen_size_y = ImGui::GetIO().DisplaySize.y;

    float icons_height{ 0.0f };
    if (include_icon_height) {
        icons_height = (get_slot_height(screen_size_y) + ImGui::GetStyle().ItemSpacing.y) * static_cast<float>(Bars::barsize);
    }

    ImGui::PushFont(font_text);
    float text_height_offset = ImGui::CalcTextSize("M").y + ImGui::GetStyle().ItemSpacing.y * 6;
    ImGui::PopFont();

    float window_height = icons_height + text_height_offset;
    float window_width {0.0f};
    if (is_ultrawide(screen_size_x, screen_size_y)) {
        window_width = screen_size_x * 0.35f;
        ImGui::SetNextWindowPos(ImVec2(screen_size_x * 0.5f, 0.0f));
    } else {
        window_width = screen_size_x * 0.3f;
        ImGui::SetNextWindowPos(ImVec2(screen_size_x * 0.5755f, 0.0f));
    }
    ImGui::SetNextWindowSize(ImVec2(window_width, window_height));
    ImGui::SetNextWindowBgAlpha(0.65F);
    ImGui::PopStyleVar();
    return std::make_tuple(screen_size_x, screen_size_y, window_width);
}

inline int get_oblivion_bar_size() {
    int barsize = 1;
    if (Input::key_oblivion_potion.isValidBound()) {
        barsize++;
    }
    if (Bars::oblivion_bar_show_power) {
        barsize++;
    }
    return barsize;
}
inline int get_oblivion_bar_row_length() {
    int row_len{ 1 };
    if (!Bars::oblivion_bar_vertical) {
        row_len = get_oblivion_bar_size();
    }
    return row_len;
}

/*
* return screen_size_x, screen_size_y, window_width, window_height
*/
inline std::tuple<float, float, float, float> calculate_hud_window_size(int barsize, int row_len, Bars::bar_layout layout, float bar_slot_spacing, float bar_slot_scale, bool include_bar_text)
{
    auto& io = ImGui::GetIO();
    const float screen_size_x = io.DisplaySize.x, screen_size_y = io.DisplaySize.y;


    ImVec2 spacing(bar_slot_spacing, bar_slot_spacing);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, spacing);

    ImVec2 inner_spacing = ImGui::GetStyle().ItemInnerSpacing;
    ImVec2 frame_padding = ImGui::GetStyle().FramePadding;
    float slot_h = std::floor(get_hud_slot_height(screen_size_y, bar_slot_scale));
    float window_height{ 0 };
    float window_width{ 0 };

    ImGui::PushFont(font_text);
    float font_height = include_bar_text ? ImGui::CalcTextSize("M").y + frame_padding.y: 0.0f; //spacing.y + inner_spacing.y;
    ImGui::PopFont();

    if (layout == Bars::bar_layout::CIRCLE && barsize >= 3) {
        window_width = (Bars::bar_circle_radius + 1.125f) * slot_h * 2.0f;
        window_height = font_height + window_width + inner_spacing.y;
    }
    else if (layout == Bars::bar_layout::CROSS && barsize >= 4) {
        int numcrosses = static_cast<int>(std::ceil(static_cast<float>(barsize) / 4.0f));
        //3 rows, 3 icons len per cross
        window_height = font_height + (slot_h + spacing.y) * 2.0f + slot_h + inner_spacing.y * 2 + frame_padding.y * 2;
        window_width = (slot_h + spacing.x) * (numcrosses*3 -1) + slot_h + inner_spacing.x * 2 + frame_padding.x * 2;
        window_width += (screen_size_x * Bars::bar_cross_distance) * (numcrosses-1);
    }
    else {
        int numrows = Bars::get_num_rows(barsize, row_len);
        //window_height = font_height + (slot_h + spacing.y) * (numrows);
        window_height = font_height + (slot_h + spacing.y) * static_cast<float>(numrows - 1) + slot_h + inner_spacing.y * 2 + frame_padding.y * 2;
        window_width = (slot_h + spacing.x) * static_cast<float>(row_len - 1) + slot_h + inner_spacing.x * 2 + frame_padding.x * 2;

        //Add height for extra rows
        if (Bars::use_keybind_icons()) {
            window_height += slot_h * keybind_icon_pos_factor * (numrows - 1);
        }
    }

    //add extra height for keybind buttons
    if (Bars::use_keybind_icons()) {
        window_height += slot_h * keybind_icon_pos_factor;
    }

    ImGui::SetNextWindowSize(ImVec2(window_width, window_height));
    ImGui::PopStyleVar();
    return std::make_tuple(screen_size_x, screen_size_y, window_width, window_height);
}

void adjust_window_pos_to_anchor(float screen_size_x, float screen_size_y, float window_width, float window_height, Bars::anchor_point anchor, float offset_x, float offset_y)
{
    switch (anchor)
    {
    case SpellHotbar::Bars::anchor_point::LEFT:
        ImGui::SetNextWindowPos(ImVec2(offset_x,
                                       screen_size_y * 0.5f - window_height * 0.5f + offset_y));
        break;
    case SpellHotbar::Bars::anchor_point::TOP:
        ImGui::SetNextWindowPos(ImVec2(screen_size_x * 0.5f - window_width * 0.5f + offset_x,
                                       offset_y));
        break;
    case SpellHotbar::Bars::anchor_point::RIGHT:
        ImGui::SetNextWindowPos(ImVec2(screen_size_x - window_width + offset_x,
                                       screen_size_y * 0.5f - window_height * 0.5f + offset_y));
        break;
    case SpellHotbar::Bars::anchor_point::BOTTOM_LEFT:
        ImGui::SetNextWindowPos(ImVec2(offset_x,
                                       screen_size_y - window_height + offset_y));
        break;
    case SpellHotbar::Bars::anchor_point::TOP_LEFT:
        ImGui::SetNextWindowPos(ImVec2(offset_x,
                                       offset_y));
        break;
    case SpellHotbar::Bars::anchor_point::BOTTOM_RIGHT:
        ImGui::SetNextWindowPos(ImVec2(screen_size_x - window_width + offset_x,
                                       screen_size_y - window_height + offset_y));
        break;
    case SpellHotbar::Bars::anchor_point::TOP_RIGHT:
        ImGui::SetNextWindowPos(ImVec2(screen_size_x - window_width + offset_x,
                                       offset_y));
        break;
    case SpellHotbar::Bars::anchor_point::CENTER:
        ImGui::SetNextWindowPos(ImVec2(screen_size_x * 0.5f - window_width * 0.5f + offset_x,
                                       screen_size_y * 0.5f - window_height * 0.5f + offset_y));
        break;
    case SpellHotbar::Bars::anchor_point::BOTTOM:
    default:
        ImGui::SetNextWindowPos(ImVec2(screen_size_x * 0.5f - window_width * 0.5f + offset_x,
                                       screen_size_y        - window_height       + offset_y));
        break;
    }
}

inline
int wrap_index(int a, int n) {
    return ((a % n) + n) % n;
}

void draw_drag_menu() {
    static constexpr ImGuiWindowFlags window_flag = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
                                                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse;
    //static constexpr ImGuiWindowFlags window_flag = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs; // | ImGuiWindowFlags_NoBackground;

    auto& io = ImGui::GetIO();
    io.MouseDrawCursor = true;

    uint8_t barsize = Bars::barsize;
    uint8_t barrowlen = Bars::bar_row_len;
    Bars::anchor_point anchor = Bars::bar_anchor_point;
    float* slot_spacing = &Bars::slot_spacing;
    if (Bars::layout == Bars::bar_layout::CIRCLE && barsize >= 3) {
        slot_spacing = &Bars::bar_circle_radius;
    }

    float* slot_scale = &Bars::slot_scale;
    float offset_x = Bars::offset_x;
    float offset_y = Bars::offset_y;
    bool include_bar_text{ true };
    Bars::bar_layout used_layout = Bars::layout;
    if (dragged_window == 1) {
        barsize = static_cast<uint8_t>(get_oblivion_bar_size());
        barrowlen = static_cast<uint8_t>(get_oblivion_bar_row_length());
        anchor = Bars::oblivion_bar_anchor_point;
        slot_spacing = &Bars::oblivion_slot_spacing;
        slot_scale = &Bars::oblivion_slot_scale;
        offset_x = Bars::oblivion_offset_x;
        offset_y = Bars::oblivion_offset_y;
        used_layout = Bars::bar_layout::BARS;
        include_bar_text = false;
    }
    auto [screen_size_x, screen_size_y, window_width, window_height] = calculate_hud_window_size(barsize, barrowlen, used_layout, *slot_spacing, *slot_scale, include_bar_text);

    if (!drag_frame_initialized) {
        adjust_window_pos_to_anchor(screen_size_x, screen_size_y, window_width, window_height, anchor, offset_x, offset_y);
    }
    ImGui::SetNextWindowBgAlpha(0.65F);

    float alpha = 0.5f;

    //const std::string text = "Drag Position, Mouse Wheel: Scale, ALT + Mouse Wheel: Spacing";
    ImGui::Begin("Drag Bar", &show_drag_frame, window_flag);

    drag_window_pos = ImGui::GetWindowPos();
    drag_window_width = ImGui::GetWindowWidth();
    drag_window_height = ImGui::GetWindowHeight();
    if (!drag_frame_initialized) {
        drag_window_start_pos = drag_window_pos;
        drag_window_start_width = drag_window_width;
        drag_window_start_height = drag_window_height;
        drag_frame_initialized = true;
    }

    ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);

    float constexpr scale_diff = 0.05f;

    float spacing_diff = RenderManager::scale_to_resolution(1.0f);
    float max_spacing = RenderManager::scale_to_resolution(50.0f);
    float min_spacing = 0.0f;
    //use radius instead of spacing on circle mode
    if (Bars::layout == Bars::bar_layout::CIRCLE && barsize >= 3) {
        spacing_diff = 0.1f;
        max_spacing = 10.0f;
        min_spacing = 0.1f;
    }

    //if (ImGui::IsItemHovered()) {
    if (io.MouseWheel < 0) {
        if (Input::mod_alt.isDown()) {
            *slot_spacing -= spacing_diff;
            if (*slot_spacing < min_spacing) {
                *slot_spacing = min_spacing;
            }
        }
        else {
            if (*slot_scale > 0.1f) {
                *slot_scale -= scale_diff;
            }
        }
    } else if (io.MouseWheel > 0) {
        if (Input::mod_alt.isDown()) {
             *slot_spacing += spacing_diff;
            if (*slot_spacing > max_spacing) {
                *slot_spacing = max_spacing;
            }
        }
        else {
            if (*slot_scale < 10.0f) {
                *slot_scale += scale_diff;
            }
        }
    }
    if (dragged_window == 1)
    {
        SpellHotbar::Bars::OblivionBar dummy;
        dummy.draw_in_hud(font_text, screen_size_x, screen_size_y, highlight_slot, get_highlight_factor(), key_modifier::none,
            highlight_isred, alpha, 0.0f, 0);
    }
    else {
        const std::string text = translate("$BAR_TEXT");
        SpellHotbar::Hotbar dummy(text, barsize);
        drawCenteredText(text.c_str());
        dummy.draw_in_hud(font_text, screen_size_x, screen_size_y, highlight_slot, get_highlight_factor(), key_modifier::none,
            highlight_isred, alpha, 0.0f, 0);
    }
    BarDraggingConfigWindow::draw_info();
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

void RenderManager::show_skill_tooltip(const RE::TESForm* item, float offset_x) {
    static RE::FormID last_tooltip = 0;
    static std::string desc = "";

    if (item != nullptr) {
        std::string title = item->GetName();

        if (last_tooltip != item->GetFormID()) {
            desc = RenderManager::get_skill_tooltip(item);
        }
        last_tooltip = item->GetFormID();

        show_tooltip(title, desc, offset_x);
    }
}

void RenderManager::show_tooltip(const std::string & title, const std::string & desc, float offset_x) {
    if (ImGui::BeginItemTooltip())
    {
        float scalef = ImGui::GetIO().DisplaySize.y / 1080.0f;
        ImVec2 left_offset = ImVec2(offset_x * scalef, 0);

        ImGui::Dummy(left_offset); ImGui::SameLine();
        RenderManager::set_large_font();
        ImGui::Text(title.c_str());
        RenderManager::revert_font();

        if (!desc.empty()) {
            ImGui::Dummy(left_offset); ImGui::SameLine();
            float text_wrap = ImGui::GetIO().DisplaySize.x * 0.35f;
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + text_wrap);
            ImGui::TextUnformatted(desc.c_str());
            ImGui::PopTextWrapPos();
        }
        ImGui::EndTooltip();
    }
}


//Draw Custom stuff 
void RenderManager::draw() {
    float deltaTime = ImGui::GetIO().DeltaTime;
    update_highlight(deltaTime);
    main_bar_fade.update(deltaTime);
    if (Input::is_oblivion_mode()) {
        oblivion_bar_fade.update(deltaTime);
    }
    update_text_fade_timer(deltaTime);

    auto pc = RE::PlayerCharacter::GetSingleton();
    if (!pc || !pc->Is3DLoaded()) {
        return;  // no player, no draw
    }

    // begin draw
    auto ui = RE::UI::GetSingleton();
    if (!ui) {
        return;  // no ui reference, no draw
    }

    // check for ctrl/shift/alt modifiers
    key_modifier mod = Bars::get_current_modifier();

    auto* magMenu = static_cast<RE::MagicMenu*>(ui->GetMenu(RE::MagicMenu::MENU_NAME).get());
    auto* favMenu = static_cast<RE::FavoritesMenu*>(ui->GetMenu(RE::FavoritesMenu::MENU_NAME).get());

    bool validTabActive = current_selected_item_bindable(); //current_inv_menu_tab_valid_for_hotbar();

    apply_imgui_style();

    if (show_drag_frame) {
        // show frame to drag the bar
        draw_drag_menu();
        ImGui::End();
    }
    else if (SpellEditor::is_opened()) {
        SpellEditor::renderEditor();
    }
    else if (PotionEditor::is_opened()) {
        PotionEditor::renderEditor();
    }
    else if (BindMenu::is_opened()) {
        BindMenu::drawFrame(font_text, font_text_big, font_text_title);
    }
    else
    {
        if (drag_frame_initialized) {

            //update bar position after drag has finished
            float width_diff = (drag_window_width - drag_window_start_width) * 0.5f;
            float height_diff = (drag_window_height - drag_window_start_height);

            if (dragged_window == 1) {
                Bars::oblivion_offset_x += (drag_window_pos.x - drag_window_start_pos.x) + width_diff;
                Bars::oblivion_offset_y += (drag_window_pos.y - drag_window_start_pos.y) + height_diff;
            }
            else {
                Bars::offset_x += (drag_window_pos.x - drag_window_start_pos.x) + width_diff;
                Bars::offset_y += (drag_window_pos.y - drag_window_start_pos.y) + height_diff;
            }
            drag_window_pos = {0, 0};
            drag_window_start_pos = {0, 0};
            drag_window_width = 0.0f;
            drag_window_height = 0.0f;
            drag_window_start_width = 0.0f;
            drag_window_start_height = 0.0f;

            drag_frame_initialized = false;
            dragged_window = 0;
        }

        auto& io = ImGui::GetIO();
        io.MouseDrawCursor = false;
        //io.WantCaptureMouse = false;

        if (magMenu || validTabActive) {

            if (!menu_open) {
                // menu was open first time
                menu_open = true;
                Bars::menu_bar_id = Bars::getCurrentHotbar_ingame();
            }
            if (!Bars::disable_menu_rendering) {
                
                static constexpr ImGuiWindowFlags window_flag =
                    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs;  // ImGuiWindowFlags_NoBackground

                if (!Bars::disable_menu_binding) {
                    // draw hotbar

                    bool render_icons = !Bars::disable_non_modifier_bar || Input::mod_1.isDown() || Input::mod_2.isDown() || Input::mod_3.isDown();

                    auto [screen_size_x, screen_size_y, window_width] = calculate_menu_window_size(render_icons);

                    ImGui::Begin("SpellHotbar", nullptr, window_flag);
                    if (Bars::hotbars.contains(Bars::menu_bar_id)) {
                        auto& bar = Bars::hotbars.at(Bars::menu_bar_id);

                        auto& prev_bar = Bars::hotbars.at(SpellHotbar::Bars::getPreviousMenuBar(SpellHotbar::Bars::menu_bar_id));
                        auto& next_bar = Bars::hotbars.at(SpellHotbar::Bars::getNextMenuBar(SpellHotbar::Bars::menu_bar_id));

                        ImGui::PushFont(font_symbols);
                        ImGui::Text("6");
                        ImGui::PopFont();
                        ImGui::SameLine();

                        bool table_ok = ImGui::BeginTable("SpellHotbarNavigation", 3, 0, ImVec2(window_width * 0.85f, 0.0f));
                        if (table_ok) {
                            ImGui::TableNextColumn();

                            ImGui::PushFont(font_text);
                            ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Button, ImColor(0, 0, 0, 0).Value);
                            ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, ImColor(192, 192, 192).Value);

                            ImGui::Button(prev_bar.get_name().c_str(), ImVec2(-FLT_MIN, 0.0f));
                            ImGui::TableNextColumn();

                            ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, ImColor(255, 255, 255).Value);
                            ImGui::Button(bar.get_name().c_str(), ImVec2(-FLT_MIN, 0.0f));
                            ImGui::PopStyleColor();
                            ImGui::TableNextColumn();

                            ImGui::Button(next_bar.get_name().c_str(), ImVec2(-FLT_MIN, 0.0f));

                            ImGui::PopStyleColor();
                            ImGui::PopStyleColor();
                            ImGui::PopFont();
                            ImGui::EndTable();

                        }
                        ImGui::SameLine();
                        ImGui::PushFont(font_symbols);
                        ImGui::Text("7");
                        ImGui::PopFont();

                        if (render_icons) {

                            bar.draw_in_menu(font_text, screen_size_x, screen_size_y, highlight_slot, get_highlight_factor(), mod);
                        }
                    }
                    ImGui::End();
                }
                else {
                    //Draw Hint to open Bind Menu
                    if (Input::key_open_advanced_bind_menu.isValidBound()) {
                        calculate_menu_window_size(false);
                        ImGui::Begin("SpellHotbar", nullptr, window_flag);
                        int code = Input::key_open_advanced_bind_menu.get_dx_scancode();
                        std::string hint_text = "Press '" + GameData::get_key_text_long(code) + "' to open SpellHotbar Binding Menu";
                        ImGui::TextUnformatted(hint_text.c_str());
                        ImGui::End();
                    }
                }
            }

        } else if (favMenu && GameData::hasFavMenuSlotBinding()) {
            // draw vampire lord / werewolf bind menu
            if (!Bars::disable_menu_rendering) {
                uint32_t bar_id = Bars::getCurrentHotbar_ingame();
                if (Bars::hotbars.contains(bar_id)) {
                    auto& bar = Bars::hotbars.at(bar_id);

                    static constexpr ImGuiWindowFlags window_flag =
                        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs;  // ImGuiWindowFlags_NoBackground

                    auto [screen_size_x, screen_size_y, window_width] = calculate_menu_window_size();

                    ImGui::Begin("SpellHotbar", nullptr, window_flag);

                    bar.draw_in_menu(font_text, screen_size_x, screen_size_y, highlight_slot, get_highlight_factor(), mod);

                    ImGui::End();
                }
            }
        } else {
            menu_open = false;

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

            static constexpr ImGuiWindowFlags window_flag =
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground;

            if (should_show || main_bar_fade.is_hud_fading()) {

                auto [screen_size_x, screen_size_y, window_width, window_height] = calculate_hud_window_size(Bars::barsize, Bars::bar_row_len, Bars::layout, Bars::slot_spacing, Bars::slot_scale, true);

                adjust_window_pos_to_anchor(screen_size_x, screen_size_y, window_width, window_height, Bars::bar_anchor_point, Bars::offset_x, Bars::offset_y);

                ImGui::SetNextWindowBgAlpha(0.65F);

                ImGui::Begin("SpellHotbarHUD", nullptr, window_flag);

                uint32_t bar_id = Bars::getCurrentHotbar_ingame();
                if (Bars::hotbars.contains(bar_id)) {
                    text_fade_check_last_bar(bar_id);
                    //text_fade_check_mod_change(mod);

                    float alpha = main_bar_fade.get_bar_alpha();
                    float text_alpha = get_text_fade_alpha();
                    auto& bar = Bars::hotbars.at(bar_id);
                    ImGui::PushFont(font_text);

                    key_modifier m = mod;
                    if (main_bar_fade.is_hud_fading_out()) {
                        //If fading out, do not react to modifier changes visually
                        m = main_bar_fade.hud_fade_mod;
                    }

                    drawCenteredText(bar.get_name(), alpha* text_alpha);
                    ImGui::PopFont();

                    bar.draw_in_hud(font_text, screen_size_x, screen_size_y, highlight_slot, get_highlight_factor(), m,
                                    highlight_isred, alpha, shout_cd_prog, shout_cd_dur);



                }
                ImGui::End();
            }

            //logger::info("OblivionBarAlpha: {}", oblivion_bar_fade.get_bar_alpha());
            if (Input::is_oblivion_mode() && (should_show_oblivion || oblivion_bar_fade.is_hud_fading())) {
                int obl_bar_size = get_oblivion_bar_size();
                int obl_bar_row = get_oblivion_bar_row_length();
                auto [screen_size_x, screen_size_y, window_width, window_height] = calculate_hud_window_size(obl_bar_size, obl_bar_row, Bars::bar_layout::BARS, Bars::oblivion_slot_spacing, Bars::oblivion_slot_scale, false);

                adjust_window_pos_to_anchor(screen_size_x, screen_size_y, window_width, window_height, Bars::oblivion_bar_anchor_point, Bars::oblivion_offset_x, Bars::oblivion_offset_y);

                ImGui::Begin("SpellHotbarOblivionHUD", nullptr, window_flag);

                float alpha = oblivion_bar_fade.get_bar_alpha();
                GameData::oblivion_bar.draw_in_hud(font_text, screen_size_x, screen_size_y, highlight_slot, get_highlight_factor(), key_modifier::none,
                    highlight_isred, alpha, shout_cd_prog, shout_cd_dur);

                ImGui::End();
            }

        }
    }
    last_mod = mod;
}
}