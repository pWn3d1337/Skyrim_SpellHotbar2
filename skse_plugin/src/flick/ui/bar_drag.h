#pragma once

// The bar-position editor: a preview of the HUD bar, the Oblivion bar or the in-menu dock that
// the player drags into place, with the mouse wheel for slot scale and ALT + wheel for spacing.
// Its position IS the setting: on close the offset it was dragged by lands in Bars::. A second,
// small window beside it lists the controls. FLICK-side only.
namespace SpellHotbar::FlickUi::BarDrag {
    bool is_opened();
    // 0 the HUD bar, 1 the Oblivion bar, 2 the in-menu dock.
    void show(int a_type);
    void hide();

    const char* title();
    // The preview window. Sizes itself to the bar and places itself from the bar's anchor and
    // offsets on first show; afterwards it goes where it is dragged.
    void draw();
    // The controls hint. Its own window so it stays put while the preview moves.
    void draw_info();

    FUCK::WindowFlags flags();
    FUCK::WindowFlags info_flags();
}
