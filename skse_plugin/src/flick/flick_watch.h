#pragma once

// SH2's connection to FLICK (`FUCK.dll`), the guest-window watch and the in-menu dock. The bind
// menu, the editors and the bar drag are FLICK windows too; they live in flick_windows.h. The
// watch exists for one reason: `RE::UI` cannot see a FLICK window at all, so SH2 has no other
// way to know that another guest's editor is open over the Inventory menu. Without it the bar
// painted across that editor and a bind key ate letters typed into its search box.
//
// Deliberately free of ImGui types: the FLICK side compiles as its own CMake target (sh2_flick,
// see flick_pch.h) and hands back plain types.
#include <string>
#include <vector>

namespace SpellHotbar::Flick {
    // The name SH2 registers its own windows under. Windows carrying it are ours and never count
    // as another guest owning the screen.
    inline constexpr const char* plugin_name = "Spell Hotbar 2";

    // Resolve `FUCK.dll` and start watching guest windows. Call at kPostLoad, once every SKSE
    // plugin is loaded. FLICK being absent is normal: every query below then reports false and
    // SH2 behaves exactly as it did before.
    void install();

    bool is_connected();

    // Close FLICK's own config menu (F7). A Spell Hotbar 2 window opened from a config page
    // would otherwise sit under the menu that opened it.
    void close_host_menu();

    // The HUD windows (flick_hud.cpp): the gameplay bar and the Oblivion-mode bar. Call at
    // kPostLoadGame like register_windows(). Idempotent.
    void register_hud_windows();

    // True while any FLICK surface other than SH2's own owns the screen -- another guest's window
    // (Fitting Room, Menu Studio) or FLICK's own F7 config menu.
    //
    // Note for the port: `FUCK::IsMenuOpen()` alone is NOT this answer. It maps to
    // `FUCKMan::IsOpen()`, which is the F7 menu and nothing else, and is blind to other guests'
    // windows -- the exact case that caused both bugs.
    bool another_guest_owns_the_screen();

    // Register the dock. Call at kPostLoadGame, the first save load: registering at kPostLoad or
    // kDataLoaded crashed with an access violation on an nvwgf2umx.dll worker thread during the
    // load screen, and a lazy registration from the render thread never got its Draw() called.
    // Idempotent.
    void register_windows();

    // Every FLICK surface at once: the dock, the HUD bars, the windows and the config tool.
    // Idempotent. plugin.cpp calls it at kPostLoadGame; the game loop calls it once the player's
    // 3D is loaded, which is the same moment for a new game or a console `coc`, where SKSE sends
    // no kPostLoadGame at all and the UI would otherwise wait for the first save load.
    void register_surfaces();

    // ---- The in-menu dock, hosted on FLICK ----
    //
    // The dock is a FLICK window because that is how a Spell Hotbar 2 surface can take the mouse
    // over a vanilla menu: FLICK pumps its own cursor, so a hosted button over the Magic menu
    // reports hover and clicks with the menu still rendering underneath.
    //
    // This translation unit cannot see SH2's bars, forms or textures (it compiles apart, without
    // the PCH -- see CMakeLists), so the plugin side builds a display list in plain types each
    // frame and hands it over. Coordinates are pixels relative to the dock's content origin.
    // Textures are named by file: FLICK only draws images it loaded itself, and its loader reads
    // PNG, not DDS, so each atlas has a PNG sibling written by scripts/dds_to_png.py and the
    // FLICK side loads that. Colours are ImGui's packed ABGR (IM_COL32).
    struct DockImage {
        std::string path;  // the atlas file SH2 loaded, as SH2 spelled it; FLICK loads the PNG sibling
        float x0, y0, x1, y1;
        float u0, v0, u1, v1;
        unsigned col;
    };
    struct DockText {
        float x, y;
        int anchor;  // 0 top-left, 1 top-right, 2 bottom-right, 3 top-centre
        std::string text;
        unsigned col;
        float px{0.0f};  // font size in pixels; 0 takes the window's default for its kind of text
    };
    struct DockSlot {
        float x, y, w, h;
        std::string tooltip;  // shown while the mouse is over this rect; empty shows nothing
    };
    struct DockFrame {
        std::string header;  // the bar's name, between the paging arrows
        std::string footer;  // unused since the bind button; kept so a hint can return
        float width{0.0f}, height{0.0f};  // the slot grid's extent
        float icon{0.0f};   // slot size in pixels; the dock's text is sized from it
        float spacing{8.0f}; // slot spacing; every other gap in the dock is this or twice this
        bool locked{false}; // drag-in-place off; the lock glyph shows closed
        bool controls{true}; // the paging arrows and the bind button; off for the transform bar in the Favorites menu
        std::vector<DockImage> images;
        std::vector<DockText> texts;
        std::vector<DockSlot> slots;
        int anchor{0};  // Bars::anchor_point, as an int so this header stays free of SH2 types
        float offset_x{0.0f}, offset_y{0.0f};
    };

    // ---- The gameplay HUD bars, hosted on FLICK ----
    //
    // The bar is a FLICK window too: chromeless, backgroundless, kPassInputToGame, always open,
    // anchored like the bar always was. FLICK draws it every frame it draws anything, and the
    // plugin side builds the display list on demand from inside that window's Draw()
    // (UiBridge::build_hud), so
    // there is no handoff and no frame of lag between the game state and the pixels.
    struct HudLayer {
        bool visible{false};
        float width{0.0f}, height{0.0f};  // content extent, pixels
        int anchor{0};  // Bars::anchor_point as an int, same table as DockFrame::anchor
        float offset_x{0.0f}, offset_y{0.0f};
        float text_px{0.0f};  // default size for texts whose px is 0
        float alpha{1.0f}, text_alpha{1.0f};  // the fades this frame was built with, for the probe log
        std::vector<DockImage> images;
        std::vector<DockText> texts;
    };

    // True when FLICK is connected and the dock window is registered, i.e. the plugin side
    // should build a DockFrame.
    bool hosts_dock();

    // Hand the dock this frame's display list and show it. Hide it with hide_dock(); every path
    // that stops drawing the dock must call that, or the window stays on screen.
    void submit_dock(DockFrame a_frame);
    void hide_dock();

    // The slot index the mouse was over on the dock's last Draw(), or -1. One frame stale by
    // construction; the plugin side uses it for the highlight tint.
    int dock_hovered_slot();

    // Drag-in-place. Holding the left button on the dock and moving the mouse accumulates a
    // pixel delta here; the plugin side takes it, adds it to Bars::menu_offset_x/y (which the
    // cosave persists) and the next frame's DockFrame carries the moved offsets back. Returns
    // false when nothing was dragged.
    bool take_dock_drag(float& a_dx, float& a_dy);

    // Clicks on the dock's own controls since the last call: the paging arrows, the gear that
    // opens the bind window, and the lock. Now that the dock takes the mouse these replace the
    // key hints the ImGui window printed.
    struct DockActions {
        bool prev_bar{false};
        bool next_bar{false};
        bool open_bind_menu{false};
        bool toggle_lock{false};
    };
    DockActions take_dock_actions();

    // True while a FLICK widget holds keyboard focus, or the host is capturing a rebind. FLICK's
    // text input never calls `AllowTextInput`, so `ControlMap::textEntryCount` stays 0 and SH2's
    // existing typing guard never fires. This is that guard's replacement for FLICK's fields.
    bool host_is_taking_keystrokes();
}
