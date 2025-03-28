#pragma once
#include "../logger/logger.h"
#include "../bar/hotbars.h"
#include "../game_data/game_data.h"

#include <d3d11.h>
#include <dxgi.h>

#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

namespace SpellHotbar {

    struct TextureImage {
        REX::W32::ID3D11ShaderResourceView* res;
        int width;
        int height;

        TextureImage();
        virtual ~TextureImage() = default;

        bool load(const std::string& path);
        bool load_dds(const std::string& path);

        virtual void draw(float w, float h);
        virtual void draw(float w, float h, float alpha);
        virtual void draw_on_top(ImVec2 pos, float w, float h, ImU32 col);

        void draw();
        ImTextureID get_res();
    };

    struct SubTextureImage : public TextureImage {
        // Copy texture from TextureImage
        SubTextureImage(const TextureImage& other, ImVec2 uv0, ImVec2 uv1);

        virtual ~SubTextureImage() = default;

        virtual void draw(float w, float h) override;
        virtual void draw(float w, float h, float alpha) override;
        virtual void draw_with_scale(float w, float h, ImU32 col, float scale);
        virtual void draw_on_top(ImVec2 pos, float w, float h, ImU32 col) override;
        virtual void draw_with_scale_at(ImVec2 pos, float w, float h, ImU32 col, float scale);

        ImVec2 uv0;
        ImVec2 uv1;
    };

    inline float get_slot_height(float screensize_y) { return screensize_y / 26.0f; }

    inline float get_hud_slot_height(float screensize_y, float bar_slot_scale) { return (screensize_y / 13.0f) * bar_slot_scale; }

    inline constexpr float keybind_icon_pos_factor = 0.8f;

    // Hook render stuff for imgui, mostly copied from wheeler
    class RenderManager {

         struct WndProcHook {
            static LRESULT thunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
            static inline WNDPROC func;
        };

        struct D3DInitHook {
            static void thunk();
            static inline REL::Relocation<decltype(thunk)> func;

            static constexpr auto id = REL::RelocationID(75595, 77226);
            static constexpr auto offset = REL::VariantOffset(0x9, 0x275, 0x00);  // VR unknown

            static inline std::atomic<bool> initialized = false;
        };

        struct DXGIPresentHook {
            static void thunk(std::uint32_t a_p1);
            static inline REL::Relocation<decltype(thunk)> func;

            static constexpr auto id = REL::RelocationID(75461, 77246);
            static constexpr auto offset = REL::Offset(0x9);
        };

    private:
        // not instantiable
        RenderManager() = delete;

        static void draw();
        static void MessageCallback(SKSE::MessagingInterface::Message* msg);

        static SubTextureImage* get_tex_for_skill_internal(RE::FormID formID);

    public:
        static bool install();
        static inline REX::W32::ID3D11Device* device = nullptr;
        static inline REX::W32::ID3D11DeviceContext* context = nullptr;
        static void spell_slotted_draw_anim(int index);

        static void load_gamedata_dependant_resources();
        /*
        * load textures independent of gamedata
        */
        static void load_fixed_textures();
        static void reload_resouces();

        static void on_game_load();

        static TextureImage & load_texture(const std::string path);
        static int load_texture_return_index(const std::string path);

        static void add_spell_texture(TextureImage & main_texture, RE::FormID formID, ImVec2 uv0, ImVec2 uv1, const std::string& filename);
        static void add_default_icon(TextureImage & main_texture, GameData::DefaultIconType type, ImVec2 uv0, ImVec2 uv1, const std::string& icon_name);
        static void add_extra_icon(TextureImage& main_texture, const std::string& icon_name, ImVec2 uv0, ImVec2 uv1, const std::string& filename);
        static void add_cooldown_icon(TextureImage& main_texture, ImVec2 uv0, ImVec2 uv1);
        static void add_spellproc_overlay_icon(TextureImage& main_texture, ImVec2 uv0, ImVec2 uv1);
        static void init_cooldown_icons(size_t amount);
        static void init_spellproc_overlay_icons(size_t amount);

        static void draw_bg(int size, float alpha = 1.0f);
        static void draw_frame_bg_texture(float size_x, float size_y, float alpha = 1.0f);
        static void draw_frame_bg(bool* show_frame);
        static bool draw_skill(RE::FormID formID, int size, ImU32 col = IM_COL32_WHITE);
        static bool draw_skill_in_editor(RE::FormID formID, ImVec2 pos, int size, ImU32 col = IM_COL32_WHITE);
        static void draw_default_icon_in_editor(GameData::DefaultIconType icon_type, ImVec2 pos, int size, ImU32 col = IM_COL32_WHITE);
        static void draw_extra_icon_in_editor(const std::string &key, ImVec2 pos, int size, ImU32 col = IM_COL32_WHITE);
        static void draw_slot_overlay(ImVec2 pos, int size, ImU32 col = IM_COL32_WHITE);
        static void draw_cd_overlay(ImVec2 pos, int size, float cd, ImU32 col);
        static void draw_spellproc_overlay(ImVec2 pos, int size, float timer, float total, float alpha);
        static void draw_highlight_overlay(ImVec2 pos, int size, ImU32 col);
        static void draw_button_icon(ImVec2 pos, int tex_index, int tex_index_modifier, int size, ImU32 col = IM_COL32_WHITE);
        static void draw_button_icon_menu(ImVec2 pos, int tex_index, int tex_index_modifier, int size, ImU32 col = IM_COL32_WHITE);
        /**
        * Returns the "Aspect Ration" of button icons, -1 index will be ignored
        */
        static float get_button_icons_length(int tex_index_key, int tex_index_mod);
        static void draw_scaled_text(ImVec2 pos, ImU32 col, const char* text);
        static float get_scaled_text_size_multiplier();
        static void draw_icon_overlay(ImVec2 pos, int size, GameData::DefaultIconType type, ImU32 col);

        static void highlight_skill_slot(int id, float dur = 1.0F, bool error = false);

        static void start_bar_dragging(int type);
        static bool should_block_game_cursor_inputs();
        static void stop_bar_dragging();
        static bool is_dragging_bar();

        static void open_spell_editor();
        static void close_spell_editor();
        static bool should_block_game_key_inputs();
        static void close_key_blocking_frames();

        static bool has_custom_icon(RE::FormID form_id);

        static std::vector<std::tuple<std::string, std::vector<std::tuple<RE::FormID, std::string, SubTextureImage*>>>>& get_editor_icon_list();

        static float scale_to_resolution(float normalized_value);
        static float scale_from_resolution(float scaled_value);
        
        static bool current_inv_menu_tab_valid_for_hotbar();
        static bool current_selected_item_bindable();

        static bool should_overlay_be_rendered(GameData::DefaultIconType overlay);

        static void open_potion_editor();
        static void close_potion_editor();

        static void open_advanced_binding_menu();
        static bool is_bind_menu_opened();

        static void ImGui_push_title_style();
        static void ImGui_pop_title_style();
        /**
        * Set Size of next frame by x% of screen height, width will be height * aspect_ratio
        * return screen_size_x, screen_size_y, frame_width
        */
        static std::tuple<float, float, float> calculate_frame_size(float screen_percent, float aspect_ratio = 16.0f/9.0f);

        /**
        * push the 25% larger font
        */
        static void set_large_font();
        /**
        * pop the last font change
        */
        static void revert_font();

        /*
        * Draw custom mouse cursor
        */
        static void draw_custom_mouse_cursor(float cursor_size = 64.0f);

        static std::string get_skill_tooltip(const RE::TESForm* item);
        static void show_skill_tooltip(const RE::TESForm* item, float offset_x = 32.0f);
        static void show_tooltip(const std::string & title, const std::string & desc, float offset_x = 32.0f);

        /*returns a color depending on most expensive effect for self brewed potions, white otherwise */
        static ImU32 get_skill_color(const RE::TESForm* form);
    };
}