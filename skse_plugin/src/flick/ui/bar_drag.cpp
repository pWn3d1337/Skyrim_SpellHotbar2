#include "bar_drag.h"

#include "widgets.h"
#include "../../bar/hotbars.h"
#include "../../game_data/game_data.h"
#include "../../game_data/localization.h"
#include "../../input/keybinds.h"

// The bar-position editor, ported from RenderManager::draw_drag_menu plus
// rendering/bar_dragging_config_window.cpp to FLICK.
//
// The ImGui version was a window the player dragged by its body, and the delta between
// where it opened and where it closed became the bar's offset. FLICK keeps the contract and
// changes who moves the window: the host does not, because kCustomPosition says the position IS
// the setting. So the preview places itself on the first frame from the bar's anchor and
// offsets, then drags itself by a latched mouse delta -- the same hand-rolled drag the in-menu
// dock uses (flick_watch.cpp), which survives a mouse that outruns the window between frames.
// On hide() the accumulated delta lands in the Bars:: offset pair with the old formula.
//
// The preview is drawn here rather than through Hotbar::build_hud_layer: an empty slot grid
// needs none of the per-slot state that builds, and the layout math below is the same math the
// ImGui window used, in FUCK:: terms.
namespace SpellHotbar::FlickUi::BarDrag {
    namespace {
        std::atomic_bool show_frame{ false };
        int drag_type{ 0 };

        bool initialized{ false };
        bool dragging{ false };
        ImVec2 window_pos{ 0.0f, 0.0f };
        ImVec2 start_pos{ 0.0f, 0.0f };
        float window_width{ 0.0f }, window_height{ 0.0f };
        float start_width{ 0.0f }, start_height{ 0.0f };

        std::string window_title;

        // render_manager.h is off limits to a FLICK TU (flick_pch.h), so its two layout
        // constants live here too. They must stay in step with that header.
        constexpr float keybind_icon_pos_factor = 0.8f;
        float hud_slot_height(float a_screen_y, float a_slot_scale) { return (a_screen_y / 13.0f) * a_slot_scale; }

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

        int oblivion_bar_row_length()
        {
            return Bars::oblivion_bar_vertical ? 1 : oblivion_bar_size();
        }

        // Everything the preview needs about the bar being dragged, resolved once per frame.
        struct DragParams {
            int barsize{ 1 };
            int row_len{ 1 };
            Bars::anchor_point anchor{ Bars::anchor_point::BOTTOM };
            Bars::bar_layout layout{ Bars::bar_layout::BARS };
            float* slot_spacing{ nullptr };
            float* slot_scale{ nullptr };
            float offset_x{ 0.0f };
            float offset_y{ 0.0f };
            bool include_bar_text{ true };
            bool circle_spacing{ false };  // the wheel edits the radius, not the spacing
        };

        DragParams resolve_params()
        {
            DragParams p;
            p.barsize = Bars::barsize;
            p.row_len = Bars::bar_row_len;
            p.anchor = Bars::bar_anchor_point;
            p.layout = Bars::layout;
            p.slot_spacing = &Bars::slot_spacing;
            p.slot_scale = &Bars::slot_scale;
            p.offset_x = Bars::offset_x;
            p.offset_y = Bars::offset_y;
            if (Bars::layout == Bars::bar_layout::CIRCLE && p.barsize >= 3) {
                p.slot_spacing = &Bars::bar_circle_radius;
                p.circle_spacing = true;
            }

            if (drag_type == 1) {
                p.barsize = oblivion_bar_size();
                p.row_len = oblivion_bar_row_length();
                p.anchor = Bars::oblivion_bar_anchor_point;
                p.slot_spacing = &Bars::oblivion_slot_spacing;
                p.slot_scale = &Bars::oblivion_slot_scale;
                p.offset_x = Bars::oblivion_offset_x;
                p.offset_y = Bars::oblivion_offset_y;
                p.layout = Bars::bar_layout::BARS;
                p.include_bar_text = false;
                p.circle_spacing = false;
            }
            else if (drag_type == 2) {
                // The in-menu dock. It is a plain row whatever the HUD bar's layout
                // is, and it wraps on the same 60% trigger the dock itself uses, so what the
                // player drags here is what they will see over the Magic menu.
                p.anchor = Bars::menu_bar_anchor_point;
                p.slot_spacing = &Bars::menu_slot_spacing;
                p.slot_scale = &Bars::menu_slot_scale;
                p.offset_x = Bars::menu_offset_x;
                p.offset_y = Bars::menu_offset_y;
                p.layout = Bars::bar_layout::BARS;
                p.circle_spacing = false;
                const ImVec2 display = FUCK::GetDisplaySize();
                const float dock_slot_h = hud_slot_height(display.y, *p.slot_scale);
                const int fit = static_cast<int>(std::floor((display.x * 0.60f + *p.slot_spacing) /
                                                            (dock_slot_h + *p.slot_spacing)));
                p.row_len = std::clamp(fit, 1, p.barsize);
            }
            p.row_len = std::max(p.row_len, 1);
            return p;
        }

        struct Metrics {
            float slot_h{ 0.0f };
            float font_height{ 0.0f };
            ImVec2 spacing{ 0.0f, 0.0f };
            ImVec2 inner{ 0.0f, 0.0f };
            ImVec2 padding{ 0.0f, 0.0f };
            float width{ 0.0f };
            float height{ 0.0f };
            int numcrosses{ 1 };
            float cross_gap{ 0.0f };
        };

        // RenderManager::calculate_hud_window_size, in FUCK:: terms and returning the size
        // instead of pushing it at the next window.
        Metrics measure(const DragParams& a_p)
        {
            Metrics m;
            const ImVec2 display = FUCK::GetDisplaySize();
            m.spacing = ImVec2(*a_p.slot_spacing, *a_p.slot_spacing);
            m.inner = FUCK::GetStyleVarVec(ImGuiStyleVar_ItemInnerSpacing);
            m.padding = FUCK::GetStyleVarVec(ImGuiStyleVar_FramePadding);
            m.slot_h = std::floor(hud_slot_height(display.y, *a_p.slot_scale));
            m.font_height = a_p.include_bar_text ? FUCK::CalcTextSize("M").y + m.padding.y : 0.0f;

            if (a_p.layout == Bars::bar_layout::CIRCLE && a_p.barsize >= 3) {
                m.width = (Bars::bar_circle_radius + 1.125f) * m.slot_h * 2.0f;
                m.height = m.font_height + m.width + m.inner.y;
            }
            else if (a_p.layout == Bars::bar_layout::CROSS && a_p.barsize >= 4) {
                m.numcrosses = static_cast<int>(std::ceil(static_cast<float>(a_p.barsize) / 4.0f));
                //3 rows, 3 icons len per cross
                m.height = m.font_height + (m.slot_h + m.spacing.y) * 2.0f + m.slot_h + m.inner.y * 2.0f + m.padding.y * 2.0f;
                m.width = (m.slot_h + m.spacing.x) * static_cast<float>(m.numcrosses * 3 - 1) + m.slot_h +
                          m.inner.x * 2.0f + m.padding.x * 2.0f;
                m.cross_gap = display.x * Bars::bar_cross_distance;
                m.width += m.cross_gap * static_cast<float>(m.numcrosses - 1);
            }
            else {
                const int numrows = Bars::get_num_rows(a_p.barsize, a_p.row_len);
                m.height = m.font_height + (m.slot_h + m.spacing.y) * static_cast<float>(numrows - 1) + m.slot_h +
                           m.inner.y * 2.0f + m.padding.y * 2.0f;
                m.width = (m.slot_h + m.spacing.x) * static_cast<float>(a_p.row_len - 1) + m.slot_h +
                          m.inner.x * 2.0f + m.padding.x * 2.0f;

                //Add height for extra rows
                if (Bars::use_keybind_icons()) {
                    m.height += m.slot_h * keybind_icon_pos_factor * static_cast<float>(numrows - 1);
                }
            }

            //add extra height for keybind buttons
            if (Bars::use_keybind_icons()) {
                m.height += m.slot_h * keybind_icon_pos_factor;
            }
            return m;
        }

        // RenderManager::adjust_window_pos_to_anchor, returning the position.
        ImVec2 anchored_pos(const DragParams& a_p, const Metrics& a_m)
        {
            const ImVec2 d = FUCK::GetDisplaySize();
            const float w = a_m.width, h = a_m.height;
            const float ox = a_p.offset_x, oy = a_p.offset_y;
            switch (a_p.anchor) {
            case Bars::anchor_point::LEFT:
                return ImVec2(ox, d.y * 0.5f - h * 0.5f + oy);
            case Bars::anchor_point::TOP:
                return ImVec2(d.x * 0.5f - w * 0.5f + ox, oy);
            case Bars::anchor_point::RIGHT:
                return ImVec2(d.x - w + ox, d.y * 0.5f - h * 0.5f + oy);
            case Bars::anchor_point::BOTTOM_LEFT:
                return ImVec2(ox, d.y - h + oy);
            case Bars::anchor_point::TOP_LEFT:
                return ImVec2(ox, oy);
            case Bars::anchor_point::BOTTOM_RIGHT:
                return ImVec2(d.x - w + ox, d.y - h + oy);
            case Bars::anchor_point::TOP_RIGHT:
                return ImVec2(d.x - w + ox, oy);
            case Bars::anchor_point::CENTER:
                return ImVec2(d.x * 0.5f - w * 0.5f + ox, d.y * 0.5f - h * 0.5f + oy);
            case Bars::anchor_point::BOTTOM:
            default:
                return ImVec2(d.x * 0.5f - w * 0.5f + ox, d.y - h + oy);
            }
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

        // One empty slot frame plus its key, as Hotbar::draw_single_skill paints an empty slot.
        void draw_preview_slot(ImVec2 a_pos, float a_size, int a_slot)
        {
            draw_default_icon_at(GameData::DefaultIconType::BAR_EMPTY, a_pos, a_size);

            const int key_id = slot_keybind_id(a_slot);
            if (!Bars::use_keybind_icons()) {
                const std::string key_text = GameData::get_keybind_text(key_id, key_modifier::none);
                text_at(ImVec2(a_pos.x + a_size * 0.05f, a_pos.y + a_size * 0.0125f), col_white, key_text.c_str());
            }
            else {
                const auto [icon_main, icon_mod] = GameData::get_keybind_icon_index(key_id, key_modifier::none);
                const float key_h = a_size * 0.35f;
                const float len = key_icons_length(icon_main, icon_mod);
                draw_key_icons_at(ImVec2(a_pos.x + a_size * 0.5f - key_h * len * 0.5f,
                                         a_pos.y + a_size * keybind_icon_pos_factor),
                                  icon_main, icon_mod, key_h);
            }
        }

        // The grid, circle or cross the HUD bar lays its slots out in (Hotbar::draw_in_hud).
        void draw_preview_slots(const DragParams& a_p, const Metrics& a_m, ImVec2 a_origin)
        {
            const float slot = a_m.slot_h;
            const ImVec2 top(a_origin.x, a_origin.y + a_m.font_height);

            if (a_p.layout == Bars::bar_layout::CIRCLE && a_p.barsize >= 3) {
                const double angle_rad = (2.0 * std::numbers::pi) / static_cast<double>(a_p.barsize);
                const float s = std::sinf(static_cast<float>(angle_rad));
                const float c = std::cosf(static_cast<float>(angle_rad));
                const float cc = (a_m.width - slot) * 0.5f;
                const ImVec2 center(top.x + cc - a_m.inner.x - a_m.padding.x, top.y + cc);
                ImVec2 offset(0.0f, -(Bars::bar_circle_radius + 0.5f) * slot);
                for (int i = 0; i < a_p.barsize; i++) {
                    draw_preview_slot(ImVec2(center.x + offset.x, center.y + offset.y), slot, i);
                    offset = ImVec2(offset.x * c - offset.y * s, offset.x * s + offset.y * c);
                }
                return;
            }

            if (a_p.layout == Bars::bar_layout::CROSS && a_p.barsize >= 4) {
                // Per cross: index 0 top-centre, 1 left, 2 right, 3 bottom -- the shape
                // Hotbar::draw_in_hud builds out of dummies and SameLine.
                const float step_x = slot + a_m.spacing.x;
                const float step_y = slot + a_m.spacing.y;
                for (int cross = 0; cross < a_m.numcrosses; cross++) {
                    const float base_x = top.x + static_cast<float>(cross) * (step_x * 3.0f + a_m.cross_gap);
                    const int ind = cross * 4;
                    const auto cell = [&](int a_col, int a_row, int a_index) {
                        if (a_index >= a_p.barsize) {
                            return;
                        }
                        draw_preview_slot(ImVec2(base_x + static_cast<float>(a_col) * step_x,
                                                 top.y + static_cast<float>(a_row) * step_y),
                                          slot, a_index);
                    };
                    cell(1, 0, ind);
                    cell(0, 1, ind + 1);
                    cell(2, 1, ind + 2);
                    cell(1, 2, ind + 3);
                }
                return;
            }

            const float row_step = slot + a_m.spacing.y +
                                   (Bars::use_keybind_icons() ? slot * 0.35f * keybind_icon_pos_factor : 0.0f);
            for (int i = 0; i < a_p.barsize; i++) {
                const int col = i % a_p.row_len;
                const int row = i / a_p.row_len;
                draw_preview_slot(ImVec2(top.x + static_cast<float>(col) * (slot + a_m.spacing.x),
                                         top.y + static_cast<float>(row) * row_step),
                                  slot, i);
            }
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
        window_pos = start_pos = ImVec2(0.0f, 0.0f);
        window_width = window_height = start_width = start_height = 0.0f;
        window_title = translate("$BAR_DRAG_TITLE");
        show_frame.store(true, std::memory_order_relaxed);
    }

    void hide()
    {
        if (initialized) {
            //update bar position after drag has finished
            const float width_diff = (window_width - start_width) * 0.5f;
            const float height_diff = (window_height - start_height);
            const float dx = (window_pos.x - start_pos.x) + width_diff;
            const float dy = (window_pos.y - start_pos.y) + height_diff;

            if (drag_type == 1) {
                Bars::oblivion_offset_x += dx;
                Bars::oblivion_offset_y += dy;
            }
            else if (drag_type == 2) {
                Bars::menu_offset_x += dx;
                Bars::menu_offset_y += dy;
            }
            else {
                Bars::offset_x += dx;
                Bars::offset_y += dy;
            }
        }

        initialized = false;
        dragging = false;
        window_pos = start_pos = ImVec2(0.0f, 0.0f);
        window_width = window_height = start_width = start_height = 0.0f;
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
        const Metrics m = measure(p);

        // The window is its own setting, so it sizes to the bar and places itself: on the first
        // frame from the anchor and the stored offsets, afterwards wherever the drag left it.
        FUCK::SetWindowSize(ImVec2(m.width, m.height), ImGuiCond_Always);
        if (!initialized) {
            const ImVec2 pos = anchored_pos(p, m);
            FUCK::SetWindowPos(pos, ImGuiCond_Always);
            window_pos = start_pos = pos;
            window_width = start_width = m.width;
            window_height = start_height = m.height;
            initialized = true;
        }
        else {
            window_pos = FUCK::GetWindowPos();
            window_width = m.width;
            window_height = m.height;
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
            if (d.x != 0.0f || d.y != 0.0f) {
                window_pos = ImVec2(window_pos.x + d.x, window_pos.y + d.y);
                FUCK::SetWindowPos(window_pos, ImGuiCond_Always);
            }
        }

        const ImVec2 origin = FUCK::GetCursorScreenPos();
        if (p.include_bar_text) {
            const std::string bar_text = translate("$BAR_TEXT");
            const ImVec2 sz = FUCK::CalcTextSize(bar_text.c_str());
            text_at(ImVec2(origin.x + (m.width - sz.x) * 0.5f, origin.y), col_white, bar_text.c_str());
        }
        draw_preview_slots(p, m, origin);

        // Reserve the extent the slots were drawn into: they go straight to the draw list, so
        // without this the window has no content and collapses under kAutoResize.
        FUCK::Dummy(ImVec2(m.width, m.height));
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
        // preview would sit where the bar's own name goes.
        return F::kCustomPosition | F::kNoResize | F::kAutoResize | F::kCloseOnEsc | F::kPauseSoft |
               F::kNoDecoration;
    }

    FUCK::WindowFlags info_flags()
    {
        using F = FUCK::WindowFlags;
        return F::kAutoResize | F::kNoResize | F::kCloseOnEsc;
    }
}
