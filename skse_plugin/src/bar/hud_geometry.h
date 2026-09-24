#pragma once
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

// Where the bars' slots go, in pixels, and where the bars' windows sit on screen. Plain math
// with no RE, ImGui or render_manager dependency, so the plugin side that builds the bars and
// the FLICK side that draws the bar-position editor share one copy: the editor's preview has to
// land exactly where the bar will, and two copies of this drifted apart.
namespace SpellHotbar::HudGeometry {
    inline constexpr float keybind_icon_pos_factor = 0.8f;

    inline float hud_slot_height(float a_screen_y, float a_slot_scale) { return (a_screen_y / 13.0f) * a_slot_scale; }

    // The slot size the bars draw at: whole pixels.
    inline float hud_icon(float a_screen_y, float a_slot_scale)
    {
        return static_cast<float>(static_cast<int>(hud_slot_height(a_screen_y, a_slot_scale)));
    }

    struct Point {
        float x{ 0.0f }, y{ 0.0f };
    };

    // A bar's slots, top-left corner of each, relative to the content origin, and the content
    // extent. `name_px` is the bar name's font size and 0 for a bar without one.
    struct BarLayout {
        float icon{ 0.0f };
        float width{ 0.0f }, height{ 0.0f };
        float name_px{ 0.0f };
        std::vector<Point> slots;
    };

    // Same values as Bars::bar_layout, so a cast converts.
    enum class Shape { rows = 0, circle = 1, cross = 2 };

    // The HUD bar (Hotbar::build_hud_layer): its name over the slots, then a grid, a circle or crosses.
    inline BarLayout hud_bar(float a_screen_x, float a_screen_y, int a_barsize, int a_row_len, Shape a_shape,
                             float a_slot_scale, float a_spacing, float a_circle_radius, float a_cross_distance,
                             bool a_keybind_icons)
    {
        BarLayout out;
        const float icon = hud_icon(a_screen_y, a_slot_scale);
        out.icon = icon;
        // The ImGui bar's sizes: the loaded font drew at 28 px on a 1440 px tall display.
        out.name_px = a_screen_y * (28.0f / 1440.0f);
        const float top = out.name_px + icon * 0.05f;

        float grid_w{ 0.0f }, grid_h{ 0.0f };
        if (a_shape == Shape::circle && a_barsize >= 3) {
            grid_w = (a_circle_radius + 1.125f) * icon * 2.0f;
            grid_h = grid_w;
            const float c = (grid_w - icon) * 0.5f;
            const double angle_rad = (2.0 * std::numbers::pi) / static_cast<double>(a_barsize);
            const float s = std::sinf(static_cast<float>(angle_rad));
            const float co = std::cosf(static_cast<float>(angle_rad));
            Point offset{ 0.0f, -(a_circle_radius + 0.5f) * icon };
            for (int i = 0; i < a_barsize; i++) {
                out.slots.push_back({ c + offset.x, top + c + offset.y });
                offset = Point{ offset.x * co - offset.y * s, offset.x * s + offset.y * co };
            }
        }
        else if (a_shape == Shape::cross && a_barsize >= 4) {
            const int numcrosses = static_cast<int>(std::ceil(static_cast<float>(a_barsize) / 4.0f));
            const float cross_gap = a_screen_x * a_cross_distance;
            const float pitch = icon + a_spacing;
            for (int cross = 0; cross < numcrosses; cross++) {
                const float base_x = cross * (3.0f * pitch + cross_gap);
                out.slots.push_back({ base_x + pitch, top });                 // top
                out.slots.push_back({ base_x, top + pitch });                 // left
                out.slots.push_back({ base_x + 2.0f * pitch, top + pitch });  // right
                out.slots.push_back({ base_x + pitch, top + 2.0f * pitch });  // bottom
            }
            grid_w = numcrosses * 3.0f * pitch - a_spacing + cross_gap * (numcrosses - 1);
            grid_h = 3.0f * pitch - a_spacing;
        }
        else {
            const int row_len = std::max(1, a_row_len);
            const float key_strip = a_keybind_icons ? icon * 0.35f * keybind_icon_pos_factor : 0.0f;
            const float row_pitch = icon + a_spacing + key_strip;
            int rows = 0;
            for (int i = 0; i < a_barsize; i++) {
                const int col_i = i % row_len, row_i = i / row_len;
                rows = row_i + 1;
                out.slots.push_back({ col_i * (icon + a_spacing), top + row_i * row_pitch });
            }
            grid_w = std::min(row_len, a_barsize) * (icon + a_spacing) - a_spacing;
            grid_h = rows * row_pitch - a_spacing;
            if (a_keybind_icons) {
                // The key glyph hangs below the slot (drawn at 0.8 of it, half a slot tall).
                grid_h += icon * 0.3f;
            }
        }
        out.width = grid_w;
        out.height = top + grid_h;
        return out;
    }

    // The Oblivion-mode bar (OblivionBar::build_hud_layer): `a_count` slots in a row or a column, no name.
    inline BarLayout oblivion_bar(float a_screen_y, int a_count, bool a_vertical, float a_slot_scale, float a_spacing,
                                  bool a_keybind_icons)
    {
        BarLayout out;
        const float icon = hud_icon(a_screen_y, a_slot_scale);
        out.icon = icon;
        const float pitch = icon + a_spacing;
        for (int n = 0; n < a_count; n++) {
            out.slots.push_back({ a_vertical ? 0.0f : n * pitch, a_vertical ? n * pitch : 0.0f });
        }
        const float extent = a_count * pitch - a_spacing;
        out.width = a_vertical ? icon : extent;
        out.height = a_vertical ? extent : icon;
        if (a_keybind_icons) {
            out.height += icon * 0.3f;
        }
        return out;
    }

    // The in-menu dock's slot row (Hotbar::build_dock_frame): a plain row that wraps at 60% of the display.
    inline BarLayout dock_row(float a_screen_x, float a_screen_y, int a_barsize, float a_slot_scale, float a_spacing,
                              bool a_keybind_icons)
    {
        BarLayout out;
        const float icon = hud_icon(a_screen_y, a_slot_scale);
        out.icon = icon;
        int row_len = static_cast<int>(std::floor((a_screen_x * 0.60f + a_spacing) / (icon + a_spacing)));
        row_len = std::clamp(row_len, 1, std::max(a_barsize, 1));
        const float key_strip_h = a_keybind_icons ? icon * 0.5f : 0.0f;
        const float row_h = icon + key_strip_h + a_spacing;
        for (int i = 0; i < a_barsize; i++) {
            out.slots.push_back({ (i % row_len) * (icon + a_spacing), (i / row_len) * row_h });
        }
        const int rows = (a_barsize + row_len - 1) / row_len;
        out.width = row_len * icon + (row_len - 1) * a_spacing;
        out.height = rows * (icon + key_strip_h) + (rows - 1) * a_spacing;
        return out;
    }

    // The dock's plate around its slot row (DockWindow::Draw): the name line on top, a slot-sized
    // arrow cell either side of the row, and padding. Every size is a fraction of the slot.
    struct DockPlate {
        float icon{ 0.0f };
        float text_px{ 0.0f }, header_px{ 0.0f };
        float arrow_w{ 0.0f }, gap{ 0.0f };
        float header_gap{ 0.0f }, pad{ 0.0f }, pad_x{ 0.0f }, pad_bottom{ 0.0f };
        float row_w{ 0.0f }, row_h{ 0.0f };
        float total_w{ 0.0f }, total_h{ 0.0f };  // inside the padding
        float plate_w{ 0.0f }, plate_h{ 0.0f };  // the whole window
        Point row_origin;                        // the slot row, relative to the plate's corner
    };

    // `a_icon` is DockFrame::icon, the unrounded slot height; the scripts/dock_mock.py fractions.
    inline DockPlate dock_plate(float a_icon, float a_spacing, float a_row_w, float a_row_h)
    {
        DockPlate p;
        p.icon = std::max(a_icon, 16.0f);
        p.text_px = p.icon * 0.32f;
        p.header_px = p.icon * 0.50f;
        p.arrow_w = p.icon;
        p.gap = std::max(a_spacing, 2.0f);
        p.header_gap = p.gap;
        p.pad = p.gap;
        p.pad_x = p.gap * 1.5f;
        p.pad_bottom = p.gap * 1.5f;
        p.row_w = std::max(a_row_w, 1.0f);
        p.row_h = std::max(a_row_h, 1.0f);
        p.total_w = p.arrow_w + p.gap + p.row_w + p.gap + p.arrow_w;
        p.total_h = p.header_px + p.header_gap + p.row_h;
        p.plate_w = p.total_w + p.pad_x * 2.0f;
        p.plate_h = p.total_h + p.pad + p.pad_bottom;
        p.row_origin = Point{ p.pad_x + p.arrow_w + p.gap, p.pad + p.header_px + p.header_gap };
        return p;
    }

    // Bars::anchor_point, as an int: the screen point the anchor names (offsets included) and the
    // pivot of the box that sits on it. The box's top-left is pos - pivot * size.
    struct Placement {
        Point pos, pivot;
    };

    inline Placement anchor_placement(int a_anchor, float a_display_x, float a_display_y, float a_ox, float a_oy)
    {
        const float dx = a_display_x, dy = a_display_y;
        switch (a_anchor) {
        case 1:  return { { a_ox, dy * 0.5f + a_oy },             { 0.0f, 0.5f } };  // LEFT
        case 2:  return { { dx * 0.5f + a_ox, a_oy },             { 0.5f, 0.0f } };  // TOP
        case 3:  return { { dx + a_ox, dy * 0.5f + a_oy },        { 1.0f, 0.5f } };  // RIGHT
        case 4:  return { { a_ox, dy + a_oy },                    { 0.0f, 1.0f } };  // BOTTOM_LEFT
        case 5:  return { { a_ox, a_oy },                         { 0.0f, 0.0f } };  // TOP_LEFT
        case 6:  return { { dx + a_ox, dy + a_oy },               { 1.0f, 1.0f } };  // BOTTOM_RIGHT
        case 7:  return { { dx + a_ox, a_oy },                    { 1.0f, 0.0f } };  // TOP_RIGHT
        case 8:  return { { dx * 0.5f + a_ox, dy * 0.5f + a_oy }, { 0.5f, 0.5f } };  // CENTER
        case 0:                                                                      // BOTTOM
        default: return { { dx * 0.5f + a_ox, dy + a_oy },        { 0.5f, 1.0f } };
        }
    }

    // The top-left of a box of the given size placed on its anchor.
    inline Point anchored_corner(int a_anchor, float a_display_x, float a_display_y, float a_ox, float a_oy,
                                 float a_w, float a_h)
    {
        const Placement p = anchor_placement(a_anchor, a_display_x, a_display_y, a_ox, a_oy);
        return { p.pos.x - p.pivot.x * a_w, p.pos.y - p.pivot.y * a_h };
    }
}
