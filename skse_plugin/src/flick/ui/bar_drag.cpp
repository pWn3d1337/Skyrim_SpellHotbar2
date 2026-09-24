#include "bar_drag.h"

#include "widgets.h"
#include "../../bar/hotbars.h"
#include "../../bar/hud_geometry.h"
#include "../../game_data/game_data.h"
#include "../../game_data/localization.h"
#include "../../input/keybinds.h"
#include "../flick_watch.h"

// The bar-position editor, ported from RenderManager::draw_drag_menu plus
// rendering/bar_dragging_config_window.cpp to FLICK.
//
// The preview has to sit exactly where the bar will, so it takes every size and slot position
// from HudGeometry -- the same functions the HUD bar, the Oblivion bar and the dock are built
// from -- and places itself with the same anchor table, the same pivot and (for the two HUD
// bars) the same clip margin as flick_hud.cpp.
//
// The host does not move this window: kCustomPosition says the position IS the setting. What the
// editor tracks is the bar's anchor point (the screen point the anchor names, offsets included);
// the window follows it through the pivot every frame, so the wheel scales the preview around its
// anchor the way the bar scales. A press drags the point by a latched mouse delta -- the same
// hand-rolled drag the in-menu dock uses (flick_watch.cpp), which survives a mouse that outruns the
// window between frames. On hide() the point, minus the anchor's own screen point, IS the offset
// pair, whatever the anchor and however the size changed on the way.
namespace SpellHotbar::FlickUi::BarDrag {
    namespace {
        using HudGeometry::Point;

        std::atomic_bool show_frame{ false };
        int drag_type{ 0 };

        bool initialized{ false };
        bool dragging{ false };
        Point anchor_pos;  // the bar's anchor point on screen, offsets included
        Point anchor_base; // the point the anchor names with zero offsets, as of the last frame

        std::string window_title;

        // RenderManager::scale_to_resolution.
        float scale_to_resolution(float a_normalized) { return a_normalized * FUCK::GetDisplaySize().y / 1080.0f; }

        int oblivion_bar_size()
        {
            int barsize = 1;
            if (Input::key_oblivion_potion.isValidBound()) {
                barsize++;
            }
            if (Bars::oblivion_bar_show_power) {
                barsize++;
            }
            return barsize;
        }

        // The settings of the bar being dragged, resolved once per frame.
        struct DragParams {
            Bars::anchor_point anchor{ Bars::anchor_point::BOTTOM };
            float* slot_spacing{ nullptr };
            float* slot_scale{ nullptr };
            float* offset_x{ nullptr };
            float* offset_y{ nullptr };
            bool circle_spacing{ false };  // the wheel edits the radius, not the spacing
        };

        DragParams resolve_params()
        {
            DragParams p;
            if (drag_type == 1) {
                p.anchor = Bars::oblivion_bar_anchor_point;
                p.slot_spacing = &Bars::oblivion_slot_spacing;
                p.slot_scale = &Bars::oblivion_slot_scale;
                p.offset_x = &Bars::oblivion_offset_x;
                p.offset_y = &Bars::oblivion_offset_y;
            }
            else if (drag_type == 2) {
                p.anchor = Bars::menu_bar_anchor_point;
                p.slot_spacing = &Bars::menu_slot_spacing;
                p.slot_scale = &Bars::menu_slot_scale;
                p.offset_x = &Bars::menu_offset_x;
                p.offset_y = &Bars::menu_offset_y;
            }
            else {
                p.anchor = Bars::bar_anchor_point;
                p.slot_spacing = &Bars::slot_spacing;
                p.slot_scale = &Bars::slot_scale;
                p.offset_x = &Bars::offset_x;
                p.offset_y = &Bars::offset_y;
                if (Bars::layout == Bars::bar_layout::CIRCLE && Bars::barsize >= 3) {
                    p.slot_spacing = &Bars::bar_circle_radius;
                    p.circle_spacing = true;
                }
            }
            return p;
        }

        // How a preview slot's key is drawn: the HUD bars' way or the dock's.
        struct SlotStyle {
            float text_px{ 0.0f };
            float text_dx{ 0.05f }, text_dy{ 0.0125f };  // key text inset, fractions of the slot
            bool dock_keys{ false };                     // key glyphs left-aligned under the slot
        };

        // The preview: its extent, where the slots go inside it, and whether the dock's plate
        // or the HUD bar's name goes around them.
        struct Preview {
            float width{ 0.0f }, height{ 0.0f };
            float margin{ 0.0f };  // clip margin around the content, as flick_hud.cpp adds it
            HudGeometry::BarLayout slots;
            Point slots_origin;
            SlotStyle style;
            bool has_name{ false };
            bool has_plate{ false };
            HudGeometry::DockPlate plate;
        };

        Preview build_preview(const DragParams& a_p)
        {
            Preview v;
            const ImVec2 display = FUCK::GetDisplaySize();
            const bool keybind_icons = Bars::use_keybind_icons();
            // flick_hud.cpp: the HUD windows sit this far outside their content so ImGui's
            // border clip does not eat the slots' edges.
            const float hud_margin = std::ceil(FUCK::GetStyleVar(ImGuiStyleVar_WindowBorderSize)) + 1.0f;

            if (drag_type == 1) {
                v.slots = HudGeometry::oblivion_bar(display.y, oblivion_bar_size(), Bars::oblivion_bar_vertical,
                                                    *a_p.slot_scale, *a_p.slot_spacing, keybind_icons);
                v.width = v.slots.width;
                v.height = v.slots.height;
                v.margin = hud_margin;
                v.style.text_px = v.slots.icon * 0.24f;  // OblivionBar::build_hud_layer
            }
            else if (drag_type == 2) {
                v.slots = HudGeometry::dock_row(display.x, display.y, static_cast<int>(Bars::barsize), *a_p.slot_scale,
                                                *a_p.slot_spacing, keybind_icons);
                v.plate = HudGeometry::dock_plate(HudGeometry::hud_slot_height(display.y, *a_p.slot_scale),
                                                  *a_p.slot_spacing, v.slots.width, v.slots.height);
                v.width = v.plate.plate_w;
                v.height = v.plate.plate_h;
                v.slots_origin = v.plate.row_origin;
                v.has_plate = true;
                // Hotbar::build_dock_frame's deeper inset, at DockWindow's text size.
                v.style.text_px = v.plate.text_px;
                v.style.text_dx = 0.07f;
                v.style.text_dy = 0.06f;
                v.style.dock_keys = true;
            }
            else {
                v.slots = HudGeometry::hud_bar(display.x, display.y, static_cast<int>(Bars::barsize),
                                               static_cast<int>(Bars::bar_row_len),
                                               static_cast<HudGeometry::Shape>(Bars::layout), Bars::slot_scale,
                                               Bars::slot_spacing, Bars::bar_circle_radius, Bars::bar_cross_distance,
                                               keybind_icons);
                v.width = v.slots.width;
                v.height = v.slots.height;
                v.margin = hud_margin;
                v.has_name = true;
                v.style.text_px = v.slots.name_px * (Bars::slot_scale + 0.25f);  // Hotbar::build_hud_layer
            }
            return v;
        }

        // The keybind id a preview slot shows. The HUD bar's slots are the spell keys in order;
        // the Oblivion bar's three are its own keys, in the order that bar draws them.
        int slot_keybind_id(int a_slot)
        {
            if (drag_type != 1) {
                return a_slot;
            }
            if (a_slot == 0) {
                return static_cast<int>(Input::keybind_id::oblivion_cast);
            }
            if (a_slot == 1 && Input::key_oblivion_potion.isValidBound()) {
                return static_cast<int>(Input::keybind_id::oblivion_potion);
            }
            return static_cast<int>(Input::keybind_id::dummy_key_vanilla_shout);
        }

        // One empty slot frame plus its key, as Hotbar::push_single_skill (or, for the dock,
        // Hotbar::build_dock_frame) paints an empty slot.
        void draw_preview_slot(ImVec2 a_pos, float a_size, int a_slot, const SlotStyle& a_style)
        {
            draw_default_icon_at(GameData::DefaultIconType::BAR_EMPTY, a_pos, a_size);

            const int key_id = slot_keybind_id(a_slot);
            if (!Bars::use_keybind_icons()) {
                const std::string key_text = GameData::get_keybind_text(key_id, key_modifier::none);
                text_at(ImVec2(a_pos.x + a_size * a_style.text_dx, a_pos.y + a_size * a_style.text_dy), col_white,
                        key_text.c_str(), std::max(a_style.text_px, 6.0f));
                return;
            }

            const auto [icon_main, icon_mod] = GameData::get_keybind_icon_index(key_id, key_modifier::none);
            if (a_style.dock_keys) {
                draw_key_icons_at(ImVec2(a_pos.x, a_pos.y + a_size), icon_main, icon_mod, a_size * 0.5f);
                return;
            }
            // Half a slot tall, centred under the slot at 0.8 of its height; with a modifier
            // glyph beside it the pair is squeezed to 0.95 of the slot width.
            const float len = key_icons_length(icon_main, icon_mod);
            float key_h = a_size * 0.5f;
            if (icon_mod >= 0 && len * key_h > a_size * 0.95f && len > 0.0f) {
                key_h = a_size * 0.95f / len;
            }
            draw_key_icons_at(ImVec2(a_pos.x + a_size * 0.5f - key_h * len * 0.5f,
                                     a_pos.y + a_size * HudGeometry::keybind_icon_pos_factor),
                              icon_main, icon_mod, key_h);
        }

        // The mouse wheel: scale on its own, spacing (or the circle's radius) with ALT.
        void apply_wheel(const DragParams& a_p)
        {
            const float wheel = FUCK::GetMouseWheel();
            if (wheel == 0.0f) {
                return;
            }
            constexpr float scale_diff = 0.05f;
            float spacing_diff = scale_to_resolution(1.0f);
            float max_spacing = scale_to_resolution(50.0f);
            float min_spacing = 0.0f;
            //use radius instead of spacing on circle mode
            if (a_p.circle_spacing) {
                spacing_diff = 0.1f;
                max_spacing = 10.0f;
                min_spacing = 0.1f;
            }

            const bool alt = FUCK::IsModifierPressed(FUCK::Modifier::kAlt);
            if (wheel < 0.0f) {
                if (alt) {
                    *a_p.slot_spacing = std::max(*a_p.slot_spacing - spacing_diff, min_spacing);
                }
                else if (*a_p.slot_scale > 0.1f) {
                    *a_p.slot_scale -= scale_diff;
                }
            }
            else {
                if (alt) {
                    *a_p.slot_spacing = std::min(*a_p.slot_spacing + spacing_diff, max_spacing);
                }
                else if (*a_p.slot_scale < 10.0f) {
                    *a_p.slot_scale += scale_diff;
                }
            }
        }
    }

    bool is_opened() { return show_frame.load(std::memory_order_relaxed); }

    void show(int a_type)
    {
        drag_type = a_type;
        initialized = false;
        dragging = false;
        anchor_pos = anchor_base = Point{};
        window_title = translate("$BAR_DRAG_TITLE");
        show_frame.store(true, std::memory_order_relaxed);
    }

    void hide()
    {
        if (initialized) {
            //the bar's offsets are its anchor point relative to the point the anchor names
            const DragParams p = resolve_params();
            *p.offset_x = anchor_pos.x - anchor_base.x;
            *p.offset_y = anchor_pos.y - anchor_base.y;
        }

        initialized = false;
        dragging = false;
        anchor_pos = anchor_base = Point{};
        drag_type = 0;
        show_frame.store(false, std::memory_order_relaxed);
    }

    const char* title()
    {
        return window_title.empty() ? "Spell Hotbar 2 bar" : window_title.c_str();
    }

    void draw()
    {
        const DragParams p = resolve_params();
        apply_wheel(p);
        const Preview v = build_preview(p);
        const ImVec2 display = FUCK::GetDisplaySize();
        const int anchor = static_cast<int>(p.anchor);

        // The anchor point starts where the bar's offsets put it and afterwards goes where the
        // drag takes it.
        if (!initialized) {
            anchor_pos = HudGeometry::anchor_placement(anchor, display.x, display.y, *p.offset_x, *p.offset_y).pos;
            initialized = true;
        }

        // Drag-in-place, latched: a press anywhere on the preview starts it and it follows the
        // mouse until release, however fast the mouse moves (flick_watch.cpp says why latched).
        if (!dragging) {
            if (FUCK::IsWindowHovered() && FUCK::IsMouseClicked(0)) {
                dragging = true;
            }
        }
        else if (!FUCK::IsMouseDown(0)) {
            dragging = false;
        }
        if (dragging) {
            const ImVec2 d = FUCK::GetMouseDelta();
            anchor_pos = Point{ anchor_pos.x + d.x, anchor_pos.y + d.y };
        }

        // Where the bar would put its content for this anchor point: through the pivot, with the
        // window position truncated to whole pixels as ImGui does for the bar's window, plus the
        // shift the bar measured between that aim and where its slots really landed
        // (Flick::measured_slot_shift). What must match are the slots, so the preview draws them at
        // that absolute point and then moves its own window, whatever its padding, to cover them.
        const HudGeometry::Placement base = HudGeometry::anchor_placement(anchor, display.x, display.y, 0.0f, 0.0f);
        anchor_base = base.pos;
        const Point content{ anchor_pos.x - base.pivot.x * v.width, anchor_pos.y - base.pivot.y * v.height };
        const Flick::SlotShift shift = Flick::measured_slot_shift(drag_type);
        const ImVec2 origin(std::trunc(content.x - v.margin) + v.margin + shift.x,
                            std::trunc(content.y - v.margin) + v.margin + shift.y);

        const ImVec2 win = FUCK::GetWindowPos();
        const ImVec2 cursor = FUCK::GetCursorScreenPos();
        FUCK::SetWindowPos(ImVec2(origin.x - v.margin - (cursor.x - win.x), origin.y - v.margin - (cursor.y - win.y)),
                           ImGuiCond_Always);
        const ImVec2 corner = FUCK::GetCursorScreenPos();

        if (v.has_plate) {
            // DockWindow's plate, and its name line with the bar text in it.
            const ImVec2 plate1(origin.x + v.width, origin.y + v.height);
            FUCK::DrawRectFilled(origin, plate1, ImVec4(0.055f, 0.055f, 0.063f, 0.92f), 4.0f);
            FUCK::DrawRect(origin, plate1, ImVec4(0.27f, 0.27f, 0.30f, 1.0f), 4.0f, 1.0f);
            const std::string bar_text = translate("$BAR_TEXT");
            FUCK::PushFont(FUCK::GetFont(FUCK::Font::kRegular), v.plate.header_px);
            const ImVec2 sz = FUCK::CalcTextSize(bar_text.c_str());
            FUCK::PopFont();
            text_at(ImVec2(origin.x + v.plate.pad_x + (v.plate.total_w - sz.x) * 0.5f, origin.y + v.plate.pad),
                    col_white, bar_text.c_str(), v.plate.header_px);
        }
        else if (v.has_name) {
            // Where Hotbar::build_hud_layer puts the bar's name: centred over the slots, at the top.
            const std::string bar_text = translate("$BAR_TEXT");
            FUCK::PushFont(FUCK::GetFont(FUCK::Font::kRegular), v.slots.name_px);
            const ImVec2 sz = FUCK::CalcTextSize(bar_text.c_str());
            FUCK::PopFont();
            text_at(ImVec2(origin.x + v.width * 0.5f - sz.x * 0.5f, origin.y), col_white, bar_text.c_str(),
                    v.slots.name_px);
        }

        for (size_t i = 0; i < v.slots.slots.size(); i++) {
            const Point& s = v.slots.slots[i];
            draw_preview_slot(ImVec2(origin.x + v.slots_origin.x + s.x, origin.y + v.slots_origin.y + s.y), v.slots.icon,
                              static_cast<int>(i), v.style);
        }

        // Reserve the extent the slots were drawn into: they go straight to the draw list, so
        // without this the window has no content and collapses under kAutoResize.
        FUCK::SetCursorScreenPos(corner);
        FUCK::Dummy(ImVec2(std::max(v.width, 1.0f) + v.margin * 2.0f, std::max(v.height, 1.0f) + v.margin * 2.0f));
    }

    void draw_info()
    {
        FUCK::TextUnformatted(translate_c("$BAR_DRAG_INFO_ESC"));
        FUCK::TextUnformatted(translate_c("$BAR_DRAG_INFO_TAB"));
        FUCK::TextUnformatted(translate_c("$BAR_DRAG_INFO_WHEEL"));
        FUCK::TextUnformatted(translate_c("$BAR_DRAG_INFO_ALT"));
    }

    FUCK::WindowFlags flags()
    {
        using F = FUCK::WindowFlags;
        // kNoDecoration: the whole preview is the drag handle, and a title bar over a bar
        // preview would sit where the bar's own name goes. kIgnoreUserScale: the bars ignore
        // FLICK's UI scale, so a scaled preview would not match them.
        return F::kCustomPosition | F::kNoResize | F::kAutoResize | F::kCloseOnEsc | F::kPauseSoft |
               F::kNoDecoration | F::kIgnoreUserScale;
    }

    FUCK::WindowFlags info_flags()
    {
        using F = FUCK::WindowFlags;
        return F::kAutoResize | F::kNoResize | F::kCloseOnEsc;
    }
}
