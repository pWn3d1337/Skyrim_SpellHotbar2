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

        TextureImage() = default;
        virtual ~TextureImage() = default;

        bool load(const std::string& path);
        bool load_dds(const std::string& path);

        virtual void draw(float w, float h);
        virtual void draw(float w, float h, float alpha);
        virtual void draw_on_top(ImVec2 pos, float w, float h, ImU32 col);

        void draw();
    };

    struct SubTextureImage : public TextureImage {
        // Copy texture from TextureImage
        SubTextureImage(const TextureImage& other, ImVec2 uv0, ImVec2 uv1);

        virtual ~SubTextureImage() = default;

        virtual void draw(float w, float h) override;
        virtual void draw(float w, float h, float alpha) override;
        virtual void draw_with_scale(float w, float h, ImU32 col, float scale);
        virtual void draw_on_top(ImVec2 pos, float w, float h, ImU32 col) override;

        ImVec2 uv0;
        ImVec2 uv1;
    };

    inline float get_slot_height(float screensize_y) { return screensize_y / 26.0f; }

    inline float get_hud_slot_height(float screensize_y, float bar_slot_scale) { return (screensize_y / 13.0f) * bar_slot_scale; }

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
        static void reload_resouces();

        static void on_game_load();

        static TextureImage & load_texture(std::string path);

        static void add_spell_texture(TextureImage & main_texture, RE::FormID formID, ImVec2 uv0, ImVec2 uv1, const std::string& filename);
        static void add_default_icon(TextureImage & main_texture, GameData::DefaultIconType type, ImVec2 uv0, ImVec2 uv1, const std::string& icon_name);
        static void add_extra_icon(TextureImage& main_texture, const std::string& icon_name, ImVec2 uv0, ImVec2 uv1, const std::string& filename);
        static void add_cooldown_icon(TextureImage& main_texture, ImVec2 uv0, ImVec2 uv1);
        static void add_spellproc_overlay_icon(TextureImage& main_texture, ImVec2 uv0, ImVec2 uv1);
        static void init_cooldown_icons(size_t amount);
        static void init_spellproc_overlay_icons(size_t amount);

        static void draw_bg(int size, float alpha = 1.0f);
        static bool draw_skill(RE::FormID formID, int size, ImU32 col = IM_COL32_WHITE);
        static void draw_skill_in_editor(RE::FormID formID, ImVec2 pos, int size);
        static void draw_default_icon_in_editor(GameData::DefaultIconType icon_type, ImVec2 pos, int size);
        static void draw_extra_icon_in_editor(const std::string &key, ImVec2 pos, int size);
        static void draw_slot_overlay(ImVec2 pos, int size, ImU32 col = IM_COL32_WHITE);
        static void draw_cd_overlay(ImVec2 pos, int size, float cd, ImU32 col);
        static void draw_spellproc_overlay(ImVec2 pos, int size, float timer, float total, float alpha);
        static void draw_highlight_overlay(ImVec2 pos, int size, ImU32 col);
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

        static bool should_overlay_be_rendered(GameData::DefaultIconType overlay);

        static void open_potion_editor();
        static void close_potion_editor();
    };
}