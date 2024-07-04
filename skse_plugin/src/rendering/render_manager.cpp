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

bool TextureImage::load(const std::string& path)
{
    return TextureLoader::fromFile(path.c_str(), &res, &width, &height);
}

void TextureImage::draw(float w, float h)
{
    ImGui::Image((void*)res, ImVec2(w, h));
}

void TextureImage::draw(float w, float h, float alpha)
{
    ImGui::Image((void*)res, ImVec2(w, h), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0, 1.0, 1.0, alpha));
}

void TextureImage::draw_on_top(ImVec2 pos, float w, float h, ImU32 col) {
    ImGui::GetWindowDrawList()->AddImage((void*)res, pos, ImVec2(pos.x + w, pos.y + h), ImVec2(0.0,0.0), ImVec2(1.0,1.0), col);
}

void TextureImage::draw()
{
    draw(static_cast<float>(width), static_cast<float>(height));
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
    ImGui::Image((void*)res, ImVec2(w, h), uv0, uv1); }

void SubTextureImage::draw(float w, float h, float alpha)
{ 
    ImGui::Image((void*)res, ImVec2(w, h), uv0, uv1, ImVec4(1.0f, 1.0f, 1.0f, alpha));
}

void SubTextureImage::draw_with_scale(float w, float h, ImU32 col, float scale) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(w, h));

    float dx = ((w * scale) - w) * 0.5f;
    float dy = ((h * scale) - h) * 0.5f;

    ImGui::GetWindowDrawList()->AddImage((void*)res, ImVec2(pos.x - dx, pos.y - dy), ImVec2(pos.x + w + dx, pos.y + h + dy), uv0, uv1,
                                         col);
}

void SubTextureImage::draw_on_top(ImVec2 pos, float w, float h, ImU32 col)
{
    ImGui::GetWindowDrawList()->AddImage((void*)res, pos, ImVec2(pos.x + w, pos.y + h), uv0, uv1, col);
}

ImFont* font_text = nullptr;
ImFont* font_symbols = nullptr;

float font_text_size{0};

std::vector<TextureImage> loaded_textures;
std::unordered_map<RE::FormID, SubTextureImage> spell_icons;
std::unordered_map<GameData::DefaultIconType, SubTextureImage> default_icons;
std::unordered_map<std::string, SubTextureImage> extra_icons;
std::vector<SubTextureImage> cooldown_icons;

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

bool RenderManager::current_inv_menu_tab_valid_for_hotbar()
{
    auto ui = RE::UI::GetSingleton();
    if (ui){
        auto* invMenu = static_cast<RE::InventoryMenu*>(ui->GetMenu(RE::InventoryMenu::MENU_NAME).get());
        if (invMenu) {
            //get current tab
            RE::GFxValue selection;
            invMenu->uiMovie->GetVariable(&selection, "_root.Menu_mc.inventoryLists.categoryList.selectedEntry.text");

            if (selection.GetType() == RE::GFxValue::ValueType::kString) {
                std::string tabtype = selection.GetString();
                return tabtype == tab_scrolls || tabtype == tab_potions || tabtype == tab_food;
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

bool RenderManager::should_block_game_cursor_inputs() { return show_drag_frame || SpellEditor::is_opened() || PotionEditor::is_opened(); }

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

bool RenderManager::should_block_game_key_inputs()
{
    return SpellEditor::is_opened() || PotionEditor::is_opened();
}

void RenderManager::close_key_blocking_frames()
{
    if (SpellEditor::is_opened()) {
        close_spell_editor();
    }
    if (PotionEditor::is_opened()) {
        close_potion_editor();
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
    float size_symbols = std::roundf(36.0f * scale_factor); 

    logger::info("Loading Fonts with sizes {}, {}", font_text_size, size_symbols);

    ImGuiIO& io = ImGui::GetIO();
    font_text = io.Fonts->AddFontFromFileTTF(
        ".\\data\\SKSE\\Plugins\\SpellHotbar\\fonts\\9_$ConsoleFont_Futura Condensed.ttf", font_text_size);

    font_symbols = io.Fonts->AddFontFromFileTTF(
        ".\\data\\SKSE\\Plugins\\SpellHotbar\\fonts\\2_$SkyrimSymbolsFont_SkyrimSymbols.ttf",
       size_symbols);

    CHECK_PTR(font_text);
    CHECK_PTR(font_symbols);
}

void RenderManager::load_gamedata_dependant_resources() {
    TextureCSVLoader::load_icons(std::filesystem::path(images_root_path));
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

    for (const auto& teximg : loaded_textures) {
        teximg.res->Release();
    }
    loaded_textures.clear();

    logger::info("Reloading Resources...");
    GameData::load_keynames_file();
    RenderManager::load_gamedata_dependant_resources();
}

void RenderManager::on_game_load()
{
}

TextureImage & RenderManager::load_texture(std::string path) {
    TextureImage tex_img;

    if (tex_img.load(path)) {
        loaded_textures.push_back(std::move(tex_img));
    }
    return loaded_textures.back();
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
    //TODO think about this casts
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

void RenderManager::draw_skill_in_editor(RE::FormID formID, ImVec2 pos, int size)
{
    SubTextureImage* img = get_tex_for_skill_internal(formID);
    if (img) {
        ImGui::GetWindowDrawList()->AddImage((void*)img->res, ImVec2(pos.x, pos.y), ImVec2(pos.x + size, pos.y + size), img->uv0, img->uv1,
            ImColor(1.0f, 1.0f, 1.0f, 1.0f));

        draw_slot_overlay(pos, size);
    }
}

void RenderManager::draw_default_icon_in_editor(GameData::DefaultIconType icon_type, ImVec2 pos, int size)
{
    if (default_icons.contains(icon_type)) {
        auto & img = default_icons.at(icon_type);
        ImGui::GetWindowDrawList()->AddImage((void*)img.res, ImVec2(pos.x, pos.y), ImVec2(pos.x + size, pos.y + size), img.uv0, img.uv1,
            ImColor(1.0f, 1.0f, 1.0f, 1.0f));

    }
    draw_slot_overlay(pos, size);
}

void RenderManager::draw_extra_icon_in_editor(const std::string& key, ImVec2 pos, int size)
{
    if (extra_icons.contains(key)) {
        auto& img = extra_icons.at(key);
        ImGui::GetWindowDrawList()->AddImage((void*)img.res, ImVec2(pos.x, pos.y), ImVec2(pos.x + size, pos.y + size), img.uv0, img.uv1,
            ImColor(1.0f, 1.0f, 1.0f, 1.0f));

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

void RenderManager::draw_highlight_overlay(ImVec2 pos, int size, ImU32 col)
{
    if (default_icons.contains(GameData::DefaultIconType::BAR_HIGHLIGHT)) {
        auto& overlay = default_icons.at(GameData::DefaultIconType::BAR_HIGHLIGHT);
        overlay.draw_on_top(pos, static_cast<float>(size), static_cast<float>(size), col);
    }
}

void RenderManager::draw_scaled_text(ImVec2 pos, ImU32 col, const char* text)
{
    float size = (Bars::slot_scale +0.25f)* font_text_size;
    ImGui::GetWindowDrawList()->AddText(font_text, size, pos, col, text);
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
    //use default spacing in menu
    ImGui::GetStyle().ItemSpacing = ImVec2(8, 4);

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

/*
* return screen_size_x, screen_size_y, window_width, window_height
*/
inline std::tuple<float, float, float, float> calculate_hud_window_size(int barsize, int row_len)
{
    auto& io = ImGui::GetIO();
    const float screen_size_x = io.DisplaySize.x, screen_size_y = io.DisplaySize.y;

    ImVec2 spacing = ImGui::GetStyle().ItemSpacing;

    spacing.x = Bars::slot_spacing;
    spacing.y = Bars::slot_spacing;
    ImGui::GetStyle().ItemSpacing = spacing;

    ImVec2 inner_spacing = ImGui::GetStyle().ItemInnerSpacing;
    ImVec2 frame_padding = ImGui::GetStyle().FramePadding;
    float slot_h = std::floor(get_hud_slot_height(screen_size_y));
    float window_height{ 0 };
    float window_width{ 0 };

    ImGui::PushFont(font_text);
    float font_height = ImGui::CalcTextSize("M").y + frame_padding.y; //spacing.y + inner_spacing.y;
    ImGui::PopFont();

    if (Bars::layout == Bars::bar_layout::CIRCLE && barsize >= 3) {
        window_width = (Bars::bar_circle_radius + 1.125f) * slot_h * 2.0f;
        window_height = font_height + window_width;
    }
    //else if (Bars::layout == Bars::bar_layout::CROSS && barsize >= 3) {
    // //TODO 
    //}
    else {
        int numrows = Bars::get_num_rows(barsize, row_len);
        //window_height = font_height + (slot_h + spacing.y) * (numrows);
        window_height = font_height + (slot_h + spacing.y) * static_cast<float>(numrows - 1) + slot_h + inner_spacing.y * 2 + frame_padding.y * 2;
        window_width = (slot_h + spacing.x) * static_cast<float>(row_len - 1) + slot_h + inner_spacing.x * 2 + frame_padding.x * 2;

    }
    ImGui::SetNextWindowSize(ImVec2(window_width, window_height));
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
    if (dragged_window == 1) {
        barsize = static_cast<uint8_t>(get_oblivion_bar_size());
        barrowlen = barsize;
        anchor = Bars::oblivion_bar_anchor_point;
        slot_spacing = &Bars::oblivion_slot_spacing;
        slot_scale = &Bars::oblivion_slot_scale;
        offset_x = Bars::oblivion_offset_x;
        offset_y = Bars::oblivion_offset_y;
    }

    auto [screen_size_x, screen_size_y, window_width, window_height] = calculate_hud_window_size(barsize, barrowlen);
    if (!drag_frame_initialized) {
        adjust_window_pos_to_anchor(screen_size_x, screen_size_y, window_width, window_height, anchor, offset_x, offset_y);
    }
    ImGui::SetNextWindowBgAlpha(0.65F);

    float alpha = 0.5f;

    const std::string text = "Drag Position, Mouse Wheel: Scale, ALT + Mouse Wheel: Spacing";
    SpellHotbar::Hotbar dummy(text, barsize);
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

    ImGui::SetItemUsingMouseWheel();    

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
    drawCenteredText(text.c_str());
    dummy.draw_in_hud(font_text, screen_size_x, screen_size_y, highlight_slot, get_highlight_factor(), key_modifier::none,
                    highlight_isred, alpha, 0.0f, 0);

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

    bool validTabActive = current_inv_menu_tab_valid_for_hotbar();

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

                // draw hotbar
                static constexpr ImGuiWindowFlags window_flag =
                    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs;  // ImGuiWindowFlags_NoBackground

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
                    else {
                        logger::trace("Error Rendering Table");
                    }
                    ImGui::SameLine();
                    ImGui::PushFont(font_symbols);
                    ImGui::Text("7");
                    ImGui::PopFont();

                    if (render_icons) {

                        bar.draw_in_menu(font_text, screen_size_x, screen_size_y, highlight_slot, get_highlight_factor(), mod);
                    }
                }
                else {
                    logger::error("Unknown Bar: {}", Bars::menu_bar_id);
                }
                ImGui::End();
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

                auto [screen_size_x, screen_size_y, window_width, window_height] = calculate_hud_window_size(Bars::barsize, Bars::bar_row_len);

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



                } else {
                    logger::error("Unknown Bar: {}", bar_id);
                }

                ImGui::End();
            }

            //logger::info("OblivionBarAlpha: {}", oblivion_bar_fade.get_bar_alpha());
            if (Input::is_oblivion_mode() && (should_show_oblivion || oblivion_bar_fade.is_hud_fading())) {
                int obl_bar_size = get_oblivion_bar_size();
                auto [screen_size_x, screen_size_y, window_width, window_height] = calculate_hud_window_size(obl_bar_size, obl_bar_size);

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