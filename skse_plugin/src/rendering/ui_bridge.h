#pragma once

// What the FLICK-hosted windows need from the plugin side, with no ImGui type in it.
//
// The FLICK side compiles apart from the rest of the plugin (see flick/flick_pch.h). The
// texture registry, the tooltip text and the icon catalogue all live on the plugin side, in
// render_manager.cpp, behind a header the FLICK side may not include. This header is the seam:
// every function here is implemented in render_manager.cpp and answers in plain types --
// paths, UVs, packed colours, strings. The FLICK side loads the atlas itself (flick_images.h)
// and draws from these coordinates.
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "../game_data/game_data.h"
#include "../flick/flick_watch.h"

namespace SpellHotbar::UiBridge {
    // ---- The gameplay HUD ----
    //
    // One frame of the HUD bars as display lists. FLICK's SH2_Hud window calls this from its
    // Draw(), once per rendered frame, with FLICK's delta time and display size; the plugin side
    // advances its fades and highlight timers on that call, exactly as RenderManager::draw() did
    // under the plugin's own Present hook. A layer that is not visible draws nothing.
    struct HudFrame {
        Flick::HudLayer main;
        Flick::HudLayer oblivion;
    };
    HudFrame build_hud(float delta_seconds, float screen_size_x, float screen_size_y);

    // Reload textures and key names from disk (the config tool's "Reload Resources").
    void reload_resources();

    // One sub-rectangle of an atlas SH2 loaded. `path` is the atlas as SH2 spelled it when it
    // loaded it; `aspect` is the whole texture's width over height, for the key glyphs, which
    // are not square.
    struct IconRef {
        std::string path;
        float u0{ 0.0f }, v0{ 0.0f }, u1{ 1.0f }, v1{ 1.0f };
        float aspect{ 1.0f };
    };

    // Each mirrors the RenderManager::draw_* helper of the same name, answering "which texture
    // and which UVs" instead of drawing. nullopt when there is nothing to draw.
    std::optional<IconRef> skill_icon(RE::FormID a_form);
    // A default icon by its CSV name or an extra icon by its "<file>_<name>" key, the two
    // spellings a per-save icon override (m_icon_str) can carry.
    std::optional<IconRef> named_icon(const std::string& a_icon);
    std::optional<IconRef> default_icon(GameData::DefaultIconType a_type);
    std::optional<IconRef> extra_icon(const std::string& a_key);
    std::optional<IconRef> cooldown_icon(float a_cd);
    // A key glyph by its index in the texture registry (GameData::get_keybind_icon_index).
    std::optional<IconRef> key_icon(int a_texture_index);

    // IM_COL32-packed tint for a form: the brewed-potion colour, white otherwise.
    unsigned skill_color(const RE::TESForm* a_form);

    std::string skill_tooltip(const RE::TESForm* a_form);
    bool has_custom_icon(RE::FormID a_form);
    bool should_overlay_be_rendered(GameData::DefaultIconType a_overlay);

    // The icon picker's catalogue: every atlas, and for each the icons in it, either by the
    // form they belong to or by the extra/default icon name. Same order as the plugin-side list.
    struct EditorIconEntry {
        RE::FormID form{ 0 };
        std::string icon;
    };
    struct EditorIconGroup {
        std::string name;
        std::vector<EditorIconEntry> entries;
    };
    std::vector<EditorIconGroup> editor_icon_groups();
}
