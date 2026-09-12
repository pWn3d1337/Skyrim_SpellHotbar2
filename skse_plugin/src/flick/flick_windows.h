#pragma once

// Spell Hotbar 2's windows as FLICK guests. One tabbed window carries
// Spellbind and, on an Advanced tab, the spell and potion editors; the bar-position editor is
// its own chromeless preview. This header is free of ImGui types on purpose -- the plugin side
// (render_manager.cpp, input.cpp, plugin.cpp) includes it
// to open and close windows and to ask whether one is up; the content is drawn on the FLICK
// side, in flick/ui/, and never leaks a type across.
//
// `Window` names a SURFACE the caller wants on screen, not a FUCK::IWindow: the first three are
// tabs of the one window, and opening one selects its tab. Each surface's state lives in its own
// model in flick/ui/; this module is the single place that turns those into IWindows. The host
// owns window position and chrome; SH2 owns the content.
#include <cstdint>

namespace SpellHotbar::Flick {
    enum class Window : std::uint8_t {
        bind_menu = 0,
        spell_editor,
        potion_editor,
        bar_drag,
        count
    };

    // Which surfaces to register. Everything by default; a tab that is off is not drawn, and the
    // tabbed window registers when any of its tabs is on.
    struct WindowSet {
        bool bind_menu{ true };
        bool spell_editor{ true };
        bool potion_editor{ true };
        bool bar_drag{ true };
    };

    // Register the windows with FLICK. Call at kPostLoadGame and nowhere earlier: registering
    // during the initial load crashed on an nvwgf2umx.dll worker thread (see flick_watch.h).
    // Idempotent, since that message fires on every save load.
    void register_ui_windows(const WindowSet& a_set);

    // True when FLICK is connected and this window was registered.
    bool hosts_window(Window a_window);

    // Open one surface: the tabbed window on that tab, or the bar drag. The other kind closes
    // first (they are modal to each other, as the ImGui windows were) and the in-menu dock hides.
    // Returns false when the surface is not hosted.
    bool open_window(Window a_window);
    // The bar-position editor needs to know which bar: 0 the HUD bar, 1 the Oblivion bar, 2
    // the in-menu dock. Same modal rule as open_window.
    bool open_bar_drag(int a_type);
    void close_window(Window a_window);
    void close_all_windows();

    bool is_window_open(Window a_window);
    bool is_any_window_open();
}
