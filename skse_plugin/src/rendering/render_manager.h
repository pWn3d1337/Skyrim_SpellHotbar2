#pragma once
#ifdef SH2_FLICK_TU
#error "render_manager.h is the plugin side of the texture registry; a FLICK translation unit reaches it through rendering/ui_bridge.h"
#endif
#include "../logger/logger.h"
#include "../bar/hotbars.h"
#include "../game_data/game_data.h"

namespace SpellHotbar {

    // An atlas SH2 knows by path and size. Nothing on this side draws it: FLICK loads the file
    // itself and draws from the UVs (flick_images.h), so this is a registry entry, not a texture.
    struct TextureImage {
        int width;
        int height;
        std::string source_path;

        TextureImage();
        virtual ~TextureImage() = default;

        bool load(const std::string& path);
        bool load_dds(const std::string& path);
    };

    struct SubTextureImage : public TextureImage {
        // A sub-rectangle of an atlas, by UV.
        SubTextureImage(const TextureImage& other, ImVec2 uv0, ImVec2 uv1);

        virtual ~SubTextureImage() = default;

        ImVec2 uv0;
        ImVec2 uv1;
    };

    inline float get_slot_height(float screensize_y) { return screensize_y / 26.0f; }

    inline float get_hud_slot_height(float screensize_y, float bar_slot_scale) { return (screensize_y / 13.0f) * bar_slot_scale; }

    inline constexpr float keybind_icon_pos_factor = 0.8f;

    class RenderManager {
    private:
        // not instantiable
        RenderManager() = delete;

        static SubTextureImage* get_tex_for_skill_internal(RE::FormID formID);

    public:
        static void spell_slotted_draw_anim(int index);

        static void load_gamedata_dependant_resources();
        /*
        * load textures independent of gamedata
        */
        static void load_fixed_textures();
        static void reload_resouces();

        static void on_game_load();

        static TextureImage* load_texture(const std::string& path);
        static int load_texture_return_index(const std::string& path);

        static void add_spell_texture(TextureImage & main_texture, RE::FormID formID, ImVec2 uv0, ImVec2 uv1, const std::string& filename);
        static void add_default_icon(TextureImage & main_texture, GameData::DefaultIconType type, ImVec2 uv0, ImVec2 uv1, const std::string& icon_name);
        static void add_extra_icon(TextureImage& main_texture, const std::string& icon_name, ImVec2 uv0, ImVec2 uv1, const std::string& filename);
        static void add_cooldown_icon(TextureImage& main_texture, ImVec2 uv0, ImVec2 uv1);
        static void add_spellproc_overlay_icon(TextureImage& main_texture, ImVec2 uv0, ImVec2 uv1);
        static void init_cooldown_icons(size_t amount);
        static void init_spellproc_overlay_icons(size_t amount);

        // Texture lookups for the FLICK-hosted windows. The dock and the HUD are drawn by FLICK
        // from a display list, so these answer "which texture and which UVs" instead of drawing.
        // Each returns nullptr when there is nothing to draw, mirroring the draw_* helper it
        // replaces.
        static const SubTextureImage* resolve_skill_tex(RE::FormID formID);
        // A default icon by its CSV name, or an extra icon by its "<file>_<name>" key: the two
        // spellings a per-save icon override (m_icon_str) can carry.
        static const SubTextureImage* named_icon_tex(const std::string& icon);
        static const SubTextureImage* default_icon_tex(GameData::DefaultIconType type);
        static const SubTextureImage* cooldown_tex(float cd);
        // The spell-proc animation frame for `timer` seconds into the proc: one cycle per second.
        static const SubTextureImage* spellproc_tex(float timer);
        static const TextureImage* button_tex(int tex_index);

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

        static float scale_to_resolution(float normalized_value);
        static float scale_from_resolution(float scaled_value);
        
        static bool current_inv_menu_tab_valid_for_hotbar();
        static bool current_selected_item_bindable();

        static bool should_overlay_be_rendered(GameData::DefaultIconType overlay);

        static void open_potion_editor();
        static void close_potion_editor();

        static void open_advanced_binding_menu();
        static bool is_bind_menu_opened();

        static std::string get_skill_tooltip(const RE::TESForm* item);

        /*returns a color depending on most expensive effect for self brewed potions, white otherwise */
        static ImU32 get_skill_color(const RE::TESForm* form);
    };
}
