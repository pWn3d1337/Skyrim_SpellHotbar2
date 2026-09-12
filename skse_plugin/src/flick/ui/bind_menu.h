#pragma once

// The Spellbind window: every known spell, power, shout, scroll, potion, ability and action in
// a filterable, sortable table on the left, the selected bar's slots on the right, drag and
// drop between them. FLICK-side only.
namespace SpellHotbar::FlickUi::BindMenu {
    bool is_opened();
    void show();
    void hide();

    // The window title FLICK shows: the bind menu's, or the editor's while one is open inside.
    const char* title();

    // One frame of content. Runs inside the FUCK::IWindow::Draw() flick_windows.cpp owns.
    void draw();
}
