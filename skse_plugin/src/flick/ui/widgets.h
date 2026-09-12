#pragma once

// Drawing vocabulary for Spell Hotbar 2's FLICK windows: the RenderManager::draw_* helpers the
// ImGui windows used, rewritten against FUCK::, plus the handful of ImGui widgets FLICK's
// C-ABI does not export (radio button, list clipper, item tooltip, indexed combo).
//
// FLICK-side only. Everything here runs inside a FUCK::IWindow::Draw().
#include "../../rendering/ui_bridge.h"

namespace SpellHotbar::FlickUi {
    inline constexpr unsigned col_white = 0xFFFFFFFFu;

    // IM_COL32(r, g, b, a) without IM_COL32: ImGui's packed ABGR.
    constexpr unsigned rgba(int r, int g, int b, int a = 255)
    {
        return (static_cast<unsigned>(a) << 24) | (static_cast<unsigned>(b) << 16) |
               (static_cast<unsigned>(g) << 8) | static_cast<unsigned>(r);
    }

    // 1080p-relative pixel scale TIMES the player's FLICK UI Size slider. FUCK::Scale is the
    // resolution alone; UIScale folds in the slider, which is what every host window sizes by.
    inline float scale(float a_px) { return FUCK::UIScale(a_px); }

    // ---- Icons. `_at` variants draw to the window draw list at a screen position and emit no
    // layout item; the callers reserve the space themselves (InvisibleButton, Dummy). `_item`
    // variants emit a layout item of the given size and draw into it. ----

    bool add_icon(const std::optional<UiBridge::IconRef>& a_ref, ImVec2 a_p0, ImVec2 a_p1, unsigned a_col = col_white);
    bool icon_item(const std::optional<UiBridge::IconRef>& a_ref, float a_size, unsigned a_col = col_white);

    // Skill icon (or nothing) plus the slot overlay, as draw_skill_in_editor.
    bool draw_skill_at(RE::FormID a_form, ImVec2 a_pos, float a_size, unsigned a_col = col_white);
    bool draw_skill_item(RE::FormID a_form, float a_size, unsigned a_col = col_white);
    // A per-save icon override (m_icon_form / m_icon_str): another form's icon, or a default or
    // extra icon by name. Exactly one of the two is set.
    bool draw_custom_icon_at(RE::FormID a_icon_form, const std::string& a_icon, ImVec2 a_pos, float a_size, unsigned a_col = col_white);
    void draw_default_icon_at(GameData::DefaultIconType a_type, ImVec2 a_pos, float a_size, unsigned a_col = col_white);
    void draw_extra_icon_at(const std::string& a_key, ImVec2 a_pos, float a_size, unsigned a_col = col_white);
    void draw_slot_overlay(ImVec2 a_pos, float a_size, unsigned a_col = col_white);
    void draw_cd_overlay(ImVec2 a_pos, float a_size, float a_cd, unsigned a_col = col_white);
    void draw_highlight_overlay(ImVec2 a_pos, float a_size, unsigned a_col);
    void draw_icon_overlay(ImVec2 a_pos, float a_size, GameData::DefaultIconType a_type, unsigned a_col = col_white);
    // Key glyph(s) at natural aspect, `a_height` tall: modifier first, then the key.
    void draw_key_icons_at(ImVec2 a_pos, int a_tex_key, int a_tex_mod, float a_height, unsigned a_col = col_white);
    // Sum of the glyphs' width/height aspects, so callers can reserve `length * height`.
    float key_icons_length(int a_tex_key, int a_tex_mod);

    // ---- Text placed at a screen position. FLICK's C-ABI has no draw-list text, so text is
    // laid out: park the cursor, emit, and put the cursor back where it was. Safe between
    // items; do not call between an item and the SameLine() that follows it. ----
    void text_at(ImVec2 a_pos, unsigned a_col, const char* a_text, float a_font_size = 0.0f);

    // ---- Widgets ----

    // ImGui::RadioButton: a circle, filled when `*a_v == a_value`, then the label.
    bool radio_button(const char* a_label, int* a_v, int a_value);

    // The item tooltip the bind menu and editors show: title in the large font, wrapped body.
    // Call right after the item; shows only while it is hovered.
    void item_tooltip(const std::string& a_title, const std::string& a_desc);
    void skill_tooltip(const RE::TESForm* a_form);

    // ImGui::BeginCombo/Selectable/EndCombo collapsed to an index over a list.
    bool combo(const char* a_label, int* a_index, const std::vector<std::string>& a_items);

    // ImGui::Checkbox as it looks in ImGui: the box, then the label, left-aligned.
    bool checkbox(const char* a_label, bool* a_v);

    // ImGuiListClipper for a table with ScrollY. `begin()` reads the table's scroll and size and
    // emits one spacer row for everything above the visible band; the caller draws rows
    // [first, last); `end()` emits the spacer below. Row height must be what the caller's rows
    // really measure, or the list scrolls in the wrong place.
    struct RowClipper {
        int first{ 0 };
        int last{ 0 };
        float row_h{ 0.0f };
        int count{ 0 };

        static RowClipper begin(int a_count, float a_row_h);
        void end() const;
    };

    // The bind menu's tab strip button: empty slot frame, an icon over it, dimmed while not
    // selected, highlighted while hovered, with a tooltip. Returns true on click.
    bool tab_button(const char* a_id, bool a_selected, float a_size, const char* a_tooltip,
                    const std::function<void(ImVec2, float)>& a_draw_icon);

    // The icon picker every editor shares: one collapsing header per atlas, a wrapped grid of
    // icon buttons under it. Returns true when the player picked one; `a_form` / `a_icon`
    // receive the pick (exactly one of them non-empty).
    bool icon_picker(const char* a_id, RE::FormID& a_form, std::string& a_icon, unsigned a_tint = col_white);

    // The large font, for titles and the Save/Cancel row.
    void push_large_font();
    void pop_font();
}
