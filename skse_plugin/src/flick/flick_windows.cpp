#include "flick_windows.h"

#include "flick_images.h"
#include "flick_watch.h"
#include "ui/bar_drag.h"
#include "ui/bind_menu.h"
#include "ui/potion_editor.h"
#include "ui/spell_editor.h"
#include "../game_data/localization.h"

namespace SpellHotbar::Flick {
    namespace {
        // Which tabs were registered. A tab that is off is not drawn and cannot be opened; the
        // window itself registers when any tab is on.
        WindowSet enabled{};

        // ---- The one SH2 window: Spellbind on one tab, the editors on
        // an Advanced tab. The models keep their own open flags; the window owns the tab. ----

        std::atomic_bool main_open{ false };
        // The tab a caller asked for. Consumed by the next Draw(), which selects it for one frame
        // and then lets the tab bar remember the player's own clicks.
        std::atomic<int> requested_tab{ -1 };
        std::atomic<int> requested_editor{ -1 };

        constexpr int tab_spellbind = 0;
        constexpr int tab_advanced = 1;
        constexpr int editor_spells = 0;
        constexpr int editor_potions = 1;

        int active_tab{ tab_spellbind };
        int active_editor{ editor_spells };

        void hide_all_models()
        {
            FlickUi::BindMenu::hide();
            FlickUi::SpellEditor::hide();
            FlickUi::PotionEditor::hide();
        }

        // Keep exactly the model on the visible tab shown. Switching tabs closes the one that
        // left the screen, so its list reloads fresh when the player comes back, exactly as the
        // separate ImGui windows did on reopen.
        void sync_models_to_tabs()
        {
            const bool want_bind = active_tab == tab_spellbind && enabled.bind_menu;
            const bool want_spells = active_tab == tab_advanced && active_editor == editor_spells && enabled.spell_editor;
            const bool want_potions = active_tab == tab_advanced && active_editor == editor_potions && enabled.potion_editor;
            if (want_bind != FlickUi::BindMenu::is_opened()) {
                want_bind ? FlickUi::BindMenu::show() : FlickUi::BindMenu::hide();
            }
            if (want_spells != FlickUi::SpellEditor::is_opened()) {
                want_spells ? FlickUi::SpellEditor::show() : FlickUi::SpellEditor::hide();
            }
            if (want_potions != FlickUi::PotionEditor::is_opened()) {
                want_potions ? FlickUi::PotionEditor::show() : FlickUi::PotionEditor::hide();
            }
        }

        class MainWindow : public FUCK::IWindow
        {
        public:
            const char* Id() const override { return "SH2_Main"; }
            const char* Title() const override
            {
                if (active_tab == tab_spellbind) {
                    return FlickUi::BindMenu::title();
                }
                return active_editor == editor_spells ? FlickUi::SpellEditor::title() : FlickUi::PotionEditor::title();
            }
            bool IsOpen() const override { return main_open.load(std::memory_order_relaxed); }
            void SetOpen(bool a_open) override
            {
                if (!a_open) {
                    hide_all_models();
                    main_open.store(false, std::memory_order_relaxed);
                    drawn_since_open = false;
                    logger::info("SH2 window: closed");
                    return;
                }
                main_open.store(true, std::memory_order_relaxed);
            }

            bool drawn_since_open{ false };
            // A blocking window: game time frozen, Escape closes it, the host draws chrome and
            // remembers where the player put it. No kCloseOnGameMenu: Spellbind opens WHILE the
            // Magic menu is open and must not hide for it. No kPassInputToGame: while this is up
            // the vanilla menu underneath gets nothing, which is what "modal" means here.
            FUCK::WindowFlags GetFlags() const override
            {
                using F = FUCK::WindowFlags;
                // kNoDecoration: the host's title bar is its own window above the guest's,
                // painted with the theme's translucent TitleBg, and no guest draw reaches it, so
                // it stays see-through over an opaque plate. The header is drawn here instead,
                // on the plate, in Draw().
                return F::kPauseSoft | F::kCloseOnEsc | F::kNoDecoration;
            }
            // The content is twelve key slots tall, so the default is sized to that: 70% of the
            // screen height and 1.6 times that across, clamped to the display. The grip below
            // lets the player pull it either way, and the host remembers where they settle.
            // In the host's units: it multiplies a guest's size by its global scale before
            // placing it (the saved state for the 3096x1296 window read 2578x1069, and that
            // default overflowed the screen and was pinned to the corner). Divide it out so the
            // window lands at the fraction of the screen named here.
            ImVec2 GetDefaultSize() const override
            {
                const ImVec2 d = FUCK::GetDisplaySize();
                const float h = d.y * 0.7f;
                const float k = std::max(FUCK::GetGlobalScale(), 0.01f);
                return ImVec2(std::min(d.x * 0.9f, h * 1.6f) / k, h / k);
            }
            ImVec2 GetDefaultPos() const override
            {
                const ImVec2 d = FUCK::GetDisplaySize();
                const ImVec2 s = GetDefaultSize();
                return ImVec2((d.x - s.x) * 0.5f, (d.y - s.y) * 0.5f);
            }

            // The window is opaque on purpose. The host paints the frame with its theme's
            // WindowBg before Draw() runs, and that colour
            // carries an alpha below 1, so the Magic menu and the world bleed through the table.
            // No flag turns that off; the dock's answer is the same one used here -- paint a
            // plate in the theme's own colour at alpha 1 under everything the tabs draw. The
            // rect runs from the top of the content area (the title bar stays the host's) to the
            // window's bottom-right corner; the window draw list clips it to the window rect.
            static void paint_opaque_plate(bool a_log)
            {
                const ImVec2 pos = FUCK::GetWindowPos();
                const ImVec2 size = FUCK::GetWindowSize();
                const ImVec2 cursor = FUCK::GetCursorScreenPos();
                ImVec4 bg = FUCK::GetStyleColorVec4(ImGuiCol_WindowBg);
                bg.w = 1.0f;
                // From the window's own top edge: a plate that starts at the content cursor
                // leaves the title and tab strip see-through. ImGui clips this draw list to the
                // content area, so a host title bar survives.
                FUCK::DrawRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bg, 0.0f);
                if (a_log) {
                    logger::info("SH2 window: plate pos=({:.0f},{:.0f}) size=({:.0f},{:.0f}) cursor=({:.0f},{:.0f}) bg=({:.2f},{:.2f},{:.2f},{:.2f})",
                                 pos.x, pos.y, size.x, size.y, cursor.x, cursor.y, bg.x, bg.y, bg.z, FUCK::GetStyleColorVec4(ImGuiCol_WindowBg).w);
                }
            }

            // The title and close control the host would have drawn, now on the plate.
            // Returns false once the X closed the window: Draw() must stop there, or the tab
            // below re-shows the models SetOpen(false) just hid. The close control follows the
            // dock's pattern: a glyph over an invisible button, no frame, brighter while hovered.
            // The row is one text line tall; the glyph sits at its right.
            bool draw_header()
            {
                const ImVec2 row = FUCK::GetCursorScreenPos();
                const float h = FUCK::GetTextLineHeight();
                const float avail = FUCK::GetContentRegionAvail().x;
                FUCK::TextUnformatted(Title());

                const float size = h * 1.25f;
                const ImVec2 at(row.x + avail - size, row.y + (h - size) * 0.5f);
                FUCK::SetCursorScreenPos(at);
                const bool pressed = FUCK::InvisibleButton("##sh2_close", ImVec2(size, size));
                const bool hovered = FUCK::IsItemHovered();
                if (FUCK::Image* g = FlickImages::glyph("close"); g != nullptr) {
                    const float inset = size * 0.2f;
                    const ImVec4 tint = hovered ? ImVec4(1.0f, 1.0f, 0.8f, 1.0f) : ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
                    FUCK::AddImage(g->GetID(), ImVec2(at.x + inset, at.y + inset),
                                   ImVec2(at.x + size - inset, at.y + size - inset), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tint);
                }
                if (hovered) {
                    FUCK::SetTooltip("Close");
                }
                if (pressed) {
                    SetOpen(false);
                    return false;
                }
                FUCK::SetCursorScreenPos(ImVec2(row.x, row.y + h));
                FUCK::Separator();
                return true;
            }

            void Draw() override
            {
                // One line per open, so a log proves the host called Draw() and not only that
                // SH2 asked for the window.
                const bool first_frame = !drawn_since_open;
                if (first_frame) {
                    drawn_since_open = true;
                    logger::info("SH2 window: first frame drawn on tab {}; scale resolution={:.3f} user={:.3f} global={:.3f}",
                                 active_tab == tab_spellbind ? "spellbind" : "advanced",
                                 FUCK::GetResolutionScale(), FUCK::GetUserScale(), FUCK::GetGlobalScale());
                }
                paint_opaque_plate(first_frame);
                if (!draw_header()) {
                    return;
                }
                const int want_tab = requested_tab.exchange(-1, std::memory_order_relaxed);
                const int want_editor = requested_editor.exchange(-1, std::memory_order_relaxed);
                if (want_editor >= 0) {
                    active_editor = want_editor;
                }

                if (FUCK::BeginTabBar("##sh2_tabs")) {
                    if (enabled.bind_menu) {
                        const int flags = want_tab == tab_spellbind ? ImGuiTabItemFlags_SetSelected : 0;
                        if (FUCK::BeginTabItem(translate_c("$TAB_SPELLBIND"), flags)) {
                            active_tab = tab_spellbind;
                            sync_models_to_tabs();
                            FlickUi::BindMenu::draw();
                            FUCK::EndTabItem();
                        }
                    }
                    if (enabled.spell_editor || enabled.potion_editor) {
                        const int flags = want_tab == tab_advanced ? ImGuiTabItemFlags_SetSelected : 0;
                        if (FUCK::BeginTabItem(translate_c("$TAB_ADVANCED"), flags)) {
                            active_tab = tab_advanced;
                            draw_advanced(want_editor);
                            FUCK::EndTabItem();
                        }
                    }
                    FUCK::EndTabBar();
                }
                draw_resize_grip();
            }

        private:
            // The grip kNoDecoration took away. FLICK's flag is ImGui's NoDecoration, which
            // bundles NoResize with NoTitleBar, and there is no title-bar-only flag. So the grip
            // is drawn on the plate like the close glyph: an invisible button in the bottom-right
            // corner,
            // and while it is held the window's far corner follows the mouse. The grab offset
            // is kept so the corner does not jump to the pointer on the first frame.
            ImVec2 grip_offset{ 0.0f, 0.0f };
            bool grip_held{ false };
            void draw_resize_grip()
            {
                const ImVec2 pos = FUCK::GetWindowPos();
                const ImVec2 size = FUCK::GetWindowSize();
                const ImVec2 pad = FUCK::GetStyleVarVec(ImGuiStyleVar_WindowPadding);
                const float g = FUCK::GetTextLineHeight() * 1.25f;
                // Half the padding in from the corner keeps the button inside the clip rect,
                // which is where ImGui hit-tests it.
                const ImVec2 corner(pos.x + size.x - pad.x * 0.5f, pos.y + size.y - pad.y * 0.5f);
                const ImVec2 at(corner.x - g, corner.y - g);
                const ImVec2 saved = FUCK::GetCursorScreenPos();
                FUCK::SetCursorScreenPos(at);
                FUCK::InvisibleButton("##sh2_grip", ImVec2(g, g));
                const bool hovered = FUCK::IsItemHovered();
                const bool active = FUCK::IsItemActive();
                if (active) {
                    const ImVec2 m = FUCK::GetMousePos();
                    if (!grip_held) {
                        grip_held = true;
                        grip_offset = ImVec2(pos.x + size.x - m.x, pos.y + size.y - m.y);
                    }
                    constexpr float min_w = 640.0f;
                    constexpr float min_h = 480.0f;
                    const ImVec2 want(std::max(min_w, m.x + grip_offset.x - pos.x),
                                      std::max(min_h, m.y + grip_offset.y - pos.y));
                    FUCK::SetWindowSize(want);
                } else {
                    grip_held = false;
                }
                const ImVec4 tint = (hovered || active) ? ImVec4(1.0f, 1.0f, 0.8f, 1.0f) : ImVec4(0.75f, 0.75f, 0.75f, 0.6f);
                const float inset = g * 0.2f;
                FUCK::DrawTriangleFilled(ImVec2(corner.x - inset, at.y + inset),
                                         ImVec2(corner.x - inset, corner.y - inset),
                                         ImVec2(at.x + inset, corner.y - inset), tint);
                if (hovered) {
                    FUCK::SetTooltip("Resize");
                }
                FUCK::SetCursorScreenPos(saved);
            }

            void draw_advanced(int a_want_editor)
            {
                if (FUCK::BeginTabBar("##sh2_editors")) {
                    if (enabled.spell_editor) {
                        const int flags = a_want_editor == editor_spells ? ImGuiTabItemFlags_SetSelected : 0;
                        if (FUCK::BeginTabItem(translate_c("$SPELL_EDITOR"), flags)) {
                            active_editor = editor_spells;
                            sync_models_to_tabs();
                            FlickUi::SpellEditor::draw();
                            FUCK::EndTabItem();
                        }
                    }
                    if (enabled.potion_editor) {
                        const int flags = a_want_editor == editor_potions ? ImGuiTabItemFlags_SetSelected : 0;
                        if (FUCK::BeginTabItem(translate_c("$POTION_EDITOR"), flags)) {
                            active_editor = editor_potions;
                            sync_models_to_tabs();
                            FlickUi::PotionEditor::draw();
                            FUCK::EndTabItem();
                        }
                    }
                    FUCK::EndTabBar();
                }
            }
        };

        // ---- The bar-position editor: a preview that places itself (kCustomPosition) because
        // its position is the setting being edited, and a hint window beside it. ----

        class BarDragWindow : public FUCK::IWindow
        {
        public:
            const char* Id() const override { return "SH2_BarDrag"; }
            const char* Title() const override { return FlickUi::BarDrag::title(); }
            bool IsOpen() const override { return FlickUi::BarDrag::is_opened(); }
            void SetOpen(bool a_open) override
            {
                if (!a_open) {
                    FlickUi::BarDrag::hide();
                }
                // Opening goes through open_bar_drag(type); the host never opens this one.
            }
            FUCK::WindowFlags GetFlags() const override { return FlickUi::BarDrag::flags(); }
            void Draw() override { FlickUi::BarDrag::draw(); }
        };

        class BarDragInfoWindow : public FUCK::IWindow
        {
        public:
            const char* Id() const override { return "SH2_BarDragInfo"; }
            const char* Title() const override { return "Spell Hotbar 2 bar controls"; }
            bool IsOpen() const override { return FlickUi::BarDrag::is_opened(); }
            void SetOpen(bool a_open) override
            {
                if (!a_open) {
                    FlickUi::BarDrag::hide();
                }
            }
            FUCK::WindowFlags GetFlags() const override { return FlickUi::BarDrag::info_flags(); }
            ImVec2 GetDefaultPos() const override
            {
                const ImVec2 d = FUCK::GetDisplaySize();
                return ImVec2(d.x * 0.62f, d.y * 0.25f);
            }
            void Draw() override { FlickUi::BarDrag::draw_info(); }
        };

        MainWindow main_window;
        BarDragWindow bar_drag_window;
        BarDragInfoWindow bar_drag_info_window;

        std::atomic_bool main_registered{ false };
        std::atomic_bool bar_drag_registered{ false };

        const char* name(Window a_window)
        {
            switch (a_window) {
            case Window::bind_menu:
                return "bind menu";
            case Window::spell_editor:
                return "spell editor";
            case Window::potion_editor:
                return "potion editor";
            case Window::bar_drag:
                return "bar-position editor";
            default:
                return "?";
            }
        }

        bool prepare_open(Window a_window)
        {
            if (!hosts_window(a_window)) {
                logger::error("Cannot open the {} because FLICK is absent or the window is switched off", name(a_window));
                return false;
            }
            // The dock is a FLICK window that keeps its last frame until told otherwise, and
            // the HUD frame may not be built again before this window is up. Hide it here, not
            // next frame.
            hide_dock();
            logger::info("SH2 window: opening the {}", name(a_window));
            return true;
        }

        void open_main(int a_tab, int a_editor)
        {
            FlickUi::BarDrag::hide();
            requested_tab.store(a_tab, std::memory_order_relaxed);
            requested_editor.store(a_editor, std::memory_order_relaxed);
            active_tab = a_tab;
            if (a_editor >= 0) {
                active_editor = a_editor;
            }
            sync_models_to_tabs();
            main_window.drawn_since_open = false;
            main_open.store(true, std::memory_order_relaxed);
        }
    }

    void register_ui_windows(const WindowSet& a_set)
    {
        if (!is_connected()) {
            return;
        }
        enabled = a_set;
        const bool any_tab = a_set.bind_menu || a_set.spell_editor || a_set.potion_editor;
        if (any_tab && !main_registered.load(std::memory_order_relaxed)) {
            FUCK::RegisterWindow(&main_window);
            main_registered.store(true, std::memory_order_relaxed);
            logger::info("SH2 window '{}': registered with FLICK (spellbind {}, spell editor {}, potion editor {})",
                         main_window.Id(), a_set.bind_menu ? "on" : "off", a_set.spell_editor ? "on" : "off",
                         a_set.potion_editor ? "on" : "off");
        }
        if (a_set.bar_drag && !bar_drag_registered.load(std::memory_order_relaxed)) {
            FUCK::RegisterWindow(&bar_drag_window);
            FUCK::RegisterWindow(&bar_drag_info_window);
            bar_drag_registered.store(true, std::memory_order_relaxed);
            logger::info("SH2 window '{}': registered with FLICK", bar_drag_window.Id());
        }
    }

    bool hosts_window(Window a_window)
    {
        if (!is_connected()) {
            return false;
        }
        const bool main = main_registered.load(std::memory_order_relaxed);
        switch (a_window) {
        case Window::bind_menu:
            return main && enabled.bind_menu;
        case Window::spell_editor:
            return main && enabled.spell_editor;
        case Window::potion_editor:
            return main && enabled.potion_editor;
        case Window::bar_drag:
            return bar_drag_registered.load(std::memory_order_relaxed);
        default:
            return false;
        }
    }

    bool open_window(Window a_window)
    {
        if (a_window == Window::bar_drag) {
            return open_bar_drag(0);
        }
        if (!prepare_open(a_window)) {
            return false;
        }
        switch (a_window) {
        case Window::bind_menu:
            open_main(tab_spellbind, -1);
            break;
        case Window::spell_editor:
            open_main(tab_advanced, editor_spells);
            break;
        case Window::potion_editor:
            open_main(tab_advanced, editor_potions);
            break;
        default:
            return false;
        }
        return true;
    }

    bool open_bar_drag(int a_type)
    {
        if (!prepare_open(Window::bar_drag)) {
            return false;
        }
        if (main_open.load(std::memory_order_relaxed)) {
            hide_all_models();
            main_open.store(false, std::memory_order_relaxed);
        }
        FlickUi::BarDrag::show(a_type);
        return true;
    }

    void close_window(Window a_window)
    {
        if (!is_window_open(a_window)) {
            return;
        }
        if (a_window == Window::bar_drag) {
            FlickUi::BarDrag::hide();
        } else {
            hide_all_models();
            main_open.store(false, std::memory_order_relaxed);
        }
        logger::info("SH2 window: closed the {}", name(a_window));
    }

    void close_all_windows()
    {
        for (std::size_t i = 0; i < static_cast<std::size_t>(Window::count); ++i) {
            close_window(static_cast<Window>(i));
        }
    }

    bool is_window_open(Window a_window)
    {
        if (!hosts_window(a_window)) {
            return false;
        }
        const bool main = main_open.load(std::memory_order_relaxed);
        switch (a_window) {
        case Window::bind_menu:
            return main && FlickUi::BindMenu::is_opened();
        case Window::spell_editor:
            return main && FlickUi::SpellEditor::is_opened();
        case Window::potion_editor:
            return main && FlickUi::PotionEditor::is_opened();
        case Window::bar_drag:
            return FlickUi::BarDrag::is_opened();
        default:
            return false;
        }
    }

    bool is_any_window_open()
    {
        return (main_registered.load(std::memory_order_relaxed) && main_open.load(std::memory_order_relaxed)) ||
               (bar_drag_registered.load(std::memory_order_relaxed) && FlickUi::BarDrag::is_opened());
    }
}
