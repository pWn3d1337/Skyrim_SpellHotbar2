#pragma once

namespace SpellHotbar::SmfGuest {
    enum class Window : std::uint8_t {
        spell_editor = 0,
        potion_editor,
        bar_dragging,
        bind_menu,
        count
    };

    // Register SH2 as an SMF guest. Missing host/API exports leave every UI
    // surface inert; there is deliberately no private-renderer fallback.
    bool install();
    bool is_ready();

    bool open_window(Window window);
    void close_window(Window window);
    void close_all_windows();
    bool is_window_open(Window window);
    bool is_any_window_open();
}
