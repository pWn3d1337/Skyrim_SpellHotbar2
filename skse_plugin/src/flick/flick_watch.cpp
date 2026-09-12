#include "flick_watch.h"
#include "flick_images.h"
#include "flick_windows.h"
#include "ui/config_tool.h"

// This translation unit is built without the shared PCH (see CMakeLists, target sh2_flick), so it
// pulls its own prerequisites. FUCK_API.h's inline Connect() calls GetModuleHandleW,
// GetProcAddress and SKSE::log without including anything that declares them.
//
// Order is load-bearing: CommonLib's REX/W32/BASE.h hard-errors if <Windows.h> was included ahead
// of it ("Please move any Windows API includes after CommonLib"), so SKSE comes first.
#include "SKSE/SKSE.h"

#include <Windows.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <set>
#include <unordered_map>
#include <string>
#include <utility>
#include <algorithm>

#include "../../third_party/flick/FUCK_API.h"

namespace SpellHotbar::Flick {
    namespace {
        // Open windows, keyed by the pair FLICK identifies them with. A set rather than a counter
        // because the host dispatches on registration flushes too, and a double open must not
        // leave the count stuck above zero once the window closes.
        std::mutex open_lock;
        std::set<std::pair<std::string, std::string>> open_windows;

        // Read from the input thread; written from the render thread's listener callback.
        std::atomic_bool any_foreign_window_open{false};
        std::atomic_bool connected{false};

        FUCK::WindowEventListener window_listener;

        // Not every foreign window owns the screen. Menu Studio's action bar is the two round
        // corner buttons that sit over every vanilla menu: it opens once and never closes, so
        // counting it made Spell Hotbar 2 inert for the whole time the player was in a menu
        // ("'Menu Studio' / 'MS_ActionBar' (1 open)" with no matching close in the log).
        // FLICK's listener passes only plugin and window id, and
        // GetFlags() is available only for windows we register ourselves, so identity is the
        // only discriminator the API offers. Keep this list to persistent chrome.
        bool is_persistent_chrome(const std::string& a_plugin, const std::string& a_window)
        {
            return a_plugin == "Menu Studio" && a_window == "MS_ActionBar";
        }

        using FlickImages::glyph;
        using FlickImages::image_for;
        using FlickImages::unpack;

        // The in-menu dock as a FLICK guest window. It draws whatever DockFrame the plugin side
        // submitted last, and answers hover through FLICK's own mouse.
        //
        // Flags are Menu Studio's action bar's (menu-studio/src/ActionBar.cpp, GetFlags): the
        // one FLICK window in this load order that lives over every vanilla menu without
        // blanking one. FLICK swallows the entire input dispatch while a window WITHOUT
        // kPassInputToGame is up, so the flag is handed back whenever the mouse is off the dock
        // -- that is what keeps the Magic menu clickable around it. kIgnoreUserScale because
        // the frame is already in pixels.
        //
        // Every call here goes through FUCK::, never ImGui::. SH2 links its own ImGui with its
        // own GImGui; an ImGui:: call from inside FLICK's Draw() reads that uninitialised
        // context and CTDs.
        class DockWindow : public FUCK::IWindow
        {
        public:
            const char* Id() const override { return "SH2_MenuDock"; }
            const char* Title() const override { return "Spell Hotbar 2 dock"; }
            bool IsOpen() const override { return open_.load(std::memory_order_relaxed); }
            void SetOpen(bool a_open) override { open_.store(a_open, std::memory_order_relaxed); }

            FUCK::WindowFlags GetFlags() const override
            {
                using F = FUCK::WindowFlags;
                // Menu Studio's set. kNoBackground because the dock draws its own plate, tight
                // to its content: FLICK's window padding made the panel far larger than what
                // it held.
                F flags = F::kNoDecoration | F::kNoBackground | F::kAutoResize | F::kNoResize |
                          F::kCustomPosition | F::kNoMove | F::kIgnoreUserScale;
                const ImVec2 mouse = FUCK::GetMousePos();
                const bool over = mouse.x >= last_pos_.x && mouse.x <= last_pos_.x + last_size_.x &&
                                  mouse.y >= last_pos_.y && mouse.y <= last_pos_.y + last_size_.y;
                if (!over && !dragging_.load(std::memory_order_relaxed) && !FUCK::IsAnyItemActive() &&
                    !FUCK::IsPopupOpen(nullptr, FUCK::PopupFlags::kAnyPopup)) {
                    flags = flags | F::kPassInputToGame;
                }
                return flags;
            }

            ImVec2 GetDefaultPos() const override { return ImVec2(0.0f, 0.0f); }

            void submit(DockFrame&& a_frame)
            {
                std::scoped_lock guard(frame_lock_);
                frame_ = std::move(a_frame);
            }

            int hovered_slot() const { return hovered_.load(std::memory_order_relaxed); }

            DockActions take_actions()
            {
                std::scoped_lock guard(frame_lock_);
                DockActions out = actions_;
                actions_ = DockActions{};
                return out;
            }

            bool take_drag(float& a_dx, float& a_dy)
            {
                std::scoped_lock guard(frame_lock_);
                a_dx = drag_dx_;
                a_dy = drag_dy_;
                drag_dx_ = drag_dy_ = 0.0f;
                return a_dx != 0.0f || a_dy != 0.0f;
            }

            void Draw() override
            {
                DockFrame frame;
                {
                    std::scoped_lock guard(frame_lock_);
                    frame = frame_;
                }

                // Anchor and offsets, expressed against last frame's size (the window is
                // auto-sized, so this frame's is unknown until it has drawn). One frame of lag on
                // first show; Menu Studio's bar positions itself the same way.
                place(frame);
                last_pos_ = FUCK::GetWindowPos();
                last_size_ = FUCK::GetWindowSize();

                // Every size below is a fraction of the slot, and the same fractions live in
                // scripts/dock_mock.py, which renders this layout offline so it can be LOOKED AT
                // before it ships. Change them there first, look, then here.
                ImFont* font = FUCK::GetFont(FUCK::Font::kRegular);
                const float icon = std::max(frame.icon, 16.0f);
                const float text_px = icon * 0.32f;
                const float header_px = icon * 0.50f;
                // The row sets the rhythm: the arrows are slot-sized cells in it, and every
                // other gap is the slot spacing.
                const float arrow_w = icon;
                const float gap = std::max(frame.spacing, 2.0f);
                // Name to row is one spacing; the sides and the bottom get one and a half, so
                // the gear is not glued to the right edge.
                const float header_gap = gap;
                const float pad = gap;
                const float pad_x = gap * 1.5f;
                const float pad_bottom = gap * 1.5f;

                DockActions clicked;
                const float row_w = std::max(frame.width, 1.0f);
                const float row_h = std::max(frame.height, 1.0f);
                const float total_w = arrow_w + gap + row_w + gap + arrow_w;
                const float total_h = header_px + header_gap + row_h;
                // The plate is part of the layout, not an overhang: ImGui clips a window's draw
                // list to the window rect, so a plate drawn outside the reserved extent lost its
                // margin wherever FLICK's own padding was thinner than `pad`. Reserve it, then
                // inset the content.
                const ImVec2 plate0 = FUCK::GetCursorScreenPos();
                const ImVec2 plate1(plate0.x + total_w + pad_x * 2.0f, plate0.y + total_h + pad + pad_bottom);
                const ImVec2 top(plate0.x + pad_x, plate0.y + pad);

                FUCK::DrawRectFilled(plate0, plate1, ImVec4(0.055f, 0.055f, 0.063f, 0.92f), 4.0f);
                FUCK::DrawRect(plate0, plate1, ImVec4(0.27f, 0.27f, 0.30f, 1.0f), 4.0f, 1.0f);

                // A glyph with an invisible button under it, no frame: the click target is the
                // whole cell, the glyph is drawn inset by `a_inset` of it, brighter while hovered.
                const auto glyph_button = [&](const char* a_id, const char* a_stem, ImVec2 a_pos, float a_size,
                                              float a_inset, const char* a_tip) -> bool {
                    FUCK::SetCursorScreenPos(a_pos);
                    const bool pressed = FUCK::InvisibleButton(a_id, ImVec2(a_size, a_size));
                    const bool hovered = FUCK::IsItemHovered();
                    const ImVec2 p1(a_pos.x + a_size, a_pos.y + a_size);
                    if (FUCK::Image* g = glyph(a_stem); g != nullptr) {
                        const float inset = a_size * a_inset;
                        const ImVec4 tint = hovered ? ImVec4(1.0f, 1.0f, 0.8f, 1.0f) : ImVec4(0.92f, 0.92f, 0.92f, 1.0f);
                        FUCK::AddImage(g->GetID(), ImVec2(a_pos.x + inset, a_pos.y + inset),
                                       ImVec2(p1.x - inset, p1.y - inset), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tint);
                    }
                    if (hovered && a_tip != nullptr) {
                        FUCK::SetTooltip(a_tip);
                    }
                    return pressed;
                };

                // Top line: the bar's name centred, lock and gear at the right end.
                {
                    FUCK::PushFont(font, header_px);
                    const ImVec2 name_sz = FUCK::CalcTextSize(frame.header.c_str());
                    FUCK::SetCursorScreenPos(ImVec2(top.x + (total_w - name_sz.x) * 0.5f, top.y));
                    FUCK::TextUnformatted(frame.header.c_str());
                    FUCK::PopFont();

                    const float btn = header_px;
                    if (glyph_button("##lock", frame.locked ? "lock" : "unlock",
                                     ImVec2(top.x + total_w - btn * 2.0f - gap, top.y), btn, 0.0f,
                                     frame.locked ? "Unlock to drag the dock" : "Lock the dock in place")) {
                        clicked.toggle_lock = true;
                    }
                    if (frame.controls &&
                        glyph_button("##bind", "gear", ImVec2(top.x + total_w - btn, top.y), btn, 0.0f, "Bind menu")) {
                        clicked.open_bind_menu = true;
                    }
                }

                const float row_y = top.y + header_px + header_gap;
                const float arrow_y = row_y + (row_h - arrow_w) * 0.5f;

                if (frame.controls && glyph_button("##prev", "arrow_left", ImVec2(top.x, arrow_y), arrow_w, 0.2f, nullptr)) {
                    clicked.prev_bar = true;
                }

                const ImVec2 origin(top.x + arrow_w + gap, row_y);

                for (const DockImage& img : frame.images) {
                    FUCK::Image* tex = image_for(img.path);
                    if (tex == nullptr) {
                        continue;
                    }
                    FUCK::AddImage(tex->GetID(),
                                   ImVec2(origin.x + img.x0, origin.y + img.y0),
                                   ImVec2(origin.x + img.x1, origin.y + img.y1),
                                   ImVec2(img.u0, img.v0), ImVec2(img.u1, img.v1), unpack(img.col));
                }

                // FLICK's C-ABI has no draw-list text, so text is laid out: park the cursor and
                // emit. The Dummy below restores the layout extent afterwards.
                FUCK::PushFont(font, text_px);
                for (const DockText& t : frame.texts) {
                    ImVec2 at(origin.x + t.x, origin.y + t.y);
                    if (t.anchor != 0) {
                        const ImVec2 sz = FUCK::CalcTextSize(t.text.c_str());
                        at.x -= sz.x;
                        if (t.anchor == 2) {
                            at.y -= sz.y;
                        }
                    }
                    FUCK::SetCursorScreenPos(at);
                    FUCK::PushStyleColor(ImGuiCol_Text, unpack(t.col));
                    FUCK::TextUnformatted(t.text.c_str());
                    FUCK::PopStyleColor(1);
                }
                FUCK::PopFont();

                int hovered = -1;
                for (std::size_t i = 0; i < frame.slots.size(); ++i) {
                    const DockSlot& slot = frame.slots[i];
                    FUCK::SetCursorScreenPos(ImVec2(origin.x + slot.x, origin.y + slot.y));
                    FUCK::PushID(static_cast<int>(i));
                    FUCK::InvisibleButton("slot", ImVec2(slot.w, slot.h));
                    if (FUCK::IsItemHovered()) {
                        hovered = static_cast<int>(i);
                        if (!slot.tooltip.empty()) {
                            FUCK::SetTooltip(slot.tooltip.c_str());
                        }
                    }
                    FUCK::PopID();
                }
                hovered_.store(hovered, std::memory_order_relaxed);

                if (frame.controls && glyph_button("##next", "arrow_right", ImVec2(origin.x + row_w + gap, arrow_y), arrow_w, 0.2f, nullptr)) {
                    clicked.next_bar = true;
                }

                if (clicked.prev_bar || clicked.next_bar || clicked.open_bind_menu || clicked.toggle_lock) {
                    std::scoped_lock guard(frame_lock_);
                    actions_.prev_bar |= clicked.prev_bar;
                    actions_.next_bar |= clicked.next_bar;
                    actions_.open_bind_menu |= clicked.open_bind_menu;
                    actions_.toggle_lock |= clicked.toggle_lock;
                }

                // Drag-in-place, latched: a press anywhere on the dock starts it and it follows
                // the mouse until release, however fast the mouse moves. The un-latched version
                // lost the drag whenever the pointer outran the window between frames. Off while
                // locked.
                if (frame.locked) {
                    dragging_.store(false, std::memory_order_relaxed);
                }
                else if (!dragging_.load(std::memory_order_relaxed)) {
                    if (FUCK::IsWindowHovered() && FUCK::IsMouseClicked(0)) {
                        dragging_.store(true, std::memory_order_relaxed);
                    }
                }
                else if (!FUCK::IsMouseDown(0)) {
                    dragging_.store(false, std::memory_order_relaxed);
                }
                if (dragging_.load(std::memory_order_relaxed)) {
                    const ImVec2 d = FUCK::GetMouseDelta();
                    if (d.x != 0.0f || d.y != 0.0f) {
                        std::scoped_lock guard(frame_lock_);
                        drag_dx_ += d.x;
                        drag_dy_ += d.y;
                    }
                }

                // Size the window to the two rows.
                FUCK::SetCursorScreenPos(plate0);
                FUCK::Dummy(ImVec2(total_w + pad_x * 2.0f, total_h + pad + pad_bottom));

            }

        private:
            // Bars::anchor_point as an int: the same anchor enum, the same pivot per anchor as
            // the bar-position editor, so dragging the dock there moves this window.
            void place(const DockFrame& a_frame) const
            {
                const ImVec2 d = FUCK::GetDisplaySize();
                const float ox = a_frame.offset_x, oy = a_frame.offset_y;
                ImVec2 pos, pivot;
                switch (a_frame.anchor) {
                case 1:  pos = ImVec2(ox, d.y * 0.5f + oy);        pivot = ImVec2(0.0f, 0.5f); break;  // LEFT
                case 2:  pos = ImVec2(d.x * 0.5f + ox, oy);        pivot = ImVec2(0.5f, 0.0f); break;  // TOP
                case 3:  pos = ImVec2(d.x + ox, d.y * 0.5f + oy);  pivot = ImVec2(1.0f, 0.5f); break;  // RIGHT
                case 4:  pos = ImVec2(ox, d.y + oy);               pivot = ImVec2(0.0f, 1.0f); break;  // BOTTOM_LEFT
                case 5:  pos = ImVec2(ox, oy);                     pivot = ImVec2(0.0f, 0.0f); break;  // TOP_LEFT
                case 6:  pos = ImVec2(d.x + ox, d.y + oy);         pivot = ImVec2(1.0f, 1.0f); break;  // BOTTOM_RIGHT
                case 7:  pos = ImVec2(d.x + ox, oy);               pivot = ImVec2(1.0f, 0.0f); break;  // TOP_RIGHT
                case 8:  pos = ImVec2(d.x * 0.5f + ox, d.y * 0.5f + oy); pivot = ImVec2(0.5f, 0.5f); break;  // CENTER
                case 0:                                                                                  // BOTTOM
                default: pos = ImVec2(d.x * 0.5f + ox, d.y + oy);  pivot = ImVec2(0.5f, 1.0f); break;
                }
                FUCK::SetWindowPos(ImVec2(pos.x - pivot.x * last_size_.x, pos.y - pivot.y * last_size_.y),
                                   ImGuiCond_Always);
            }

            std::atomic_bool open_{false};
            std::atomic_bool dragging_{false};
            std::atomic_int  hovered_{-1};
            mutable std::mutex frame_lock_;
            DockFrame frame_;
            ImVec2 last_pos_{0.0f, 0.0f};
            ImVec2 last_size_{0.0f, 0.0f};
            float drag_dx_{0.0f}, drag_dy_{0.0f};  // guarded by frame_lock_
            DockActions actions_;                   // guarded by frame_lock_
        };

        DockWindow dock;
        std::atomic_bool dock_registered{false};


        void on_window_event(const char* a_plugin, const char* a_window, bool a_opening)
        {
            const std::string plugin = a_plugin ? a_plugin : "";
            const std::string window = a_window ? a_window : "";
            if (plugin == plugin_name) {
                return;
            }

            SKSE::log::debug("FLICK guest window {}: '{}' / '{}'{}, host IsMenuOpen() = {}",
                            a_opening ? "opened" : "closed", plugin, window,
                            is_persistent_chrome(plugin, window) ? " [ignored: persistent chrome]" : "",
                            FUCK::IsMenuOpen());

            if (is_persistent_chrome(plugin, window)) {
                return;
            }

            std::size_t remaining{0};
            {
                std::scoped_lock guard(open_lock);
                if (a_opening) {
                    open_windows.emplace(plugin, window);
                } else {
                    open_windows.erase({plugin, window});
                }
                remaining = open_windows.size();
                any_foreign_window_open.store(remaining != 0, std::memory_order_relaxed);
            }

            SKSE::log::debug("FLICK gate now counts {} foreign window(s)", remaining);
        }
    }

    void install()
    {
        if (connected.load(std::memory_order_relaxed)) {
            return;
        }

        // Connect() checks the host's API version against FUCK_API_VERSION in the vendored header
        // and fails closed on an older FLICK, which is what we want: the window listeners this
        // module depends on arrived in v4.
        if (!FUCK::Connect(plugin_name)) {
            SKSE::log::info(
                "FLICK (FUCK.dll) is absent or older than API {}; Spell Hotbar 2 cannot see other "
                "ImGui windows and will not go inert for them",
                FUCK_API_VERSION);
            return;
        }

        window_listener = FUCK::WindowEventListener(&on_window_event);
        connected.store(true, std::memory_order_relaxed);
        SKSE::log::info("Watching FLICK guest windows");
    }

    bool is_connected()
    {
        return connected.load(std::memory_order_relaxed);
    }

    void close_host_menu()
    {
        if (is_connected() && FUCK::IsMenuOpen()) {
            SKSE::log::info("SH2 FLICK: closing the host's config menu so a Spell Hotbar 2 window can open over the game");
            FUCK::SetMenuOpen(false);
        }
    }

    bool another_guest_owns_the_screen()
    {
        if (!connected.load(std::memory_order_relaxed)) {
            return false;
        }
        const bool foreign = any_foreign_window_open.load(std::memory_order_relaxed);

        const bool menu_flag = FUCK::IsMenuOpen();
        static std::atomic_int last_reported{-1};
        const int state = (foreign ? 1 : 0) | (menu_flag ? 2 : 0);
        if (last_reported.exchange(state, std::memory_order_relaxed) != state) {
            SKSE::log::debug("FLICK gate: foreign window open = {}, host IsMenuOpen() = {}", foreign, menu_flag);
        }

        // Both signals, because neither is sufficient alone. The window listener counts anything
        // a guest registers, including Menu Studio's permanent corner buttons, which is why the
        // chrome list above exists. IsMenuOpen() is the host's own "a guest menu owns the screen"
        // flag; it does not latch across ordinary menu use. Worst case is contained: this gate
        // reaches only the in-menu bar, never the HUD.
        return foreign || menu_flag;
    }

    void register_windows()
    {
        if (!connected.load(std::memory_order_relaxed) || dock_registered.load(std::memory_order_relaxed)) {
            return;
        }
        // kPostLoadGame (see plugin.cpp). kDataLoaded, where other guests register, and kPostLoad
        // both crashed with an EXCEPTION_ACCESS_VIOLATION in nvwgf2umx.dll during the load
        // screen; registering lazily from the render thread produced a window whose Draw()
        // never ran.
        FUCK::RegisterWindow(&dock);
        dock_registered.store(true, std::memory_order_relaxed);
        SKSE::log::info("SH2 dock: registered with FLICK");
    }

    bool hosts_dock()
    {
        return dock_registered.load(std::memory_order_relaxed);
    }

    void submit_dock(DockFrame a_frame)
    {
        if (!hosts_dock()) {
            return;
        }
        dock.submit(std::move(a_frame));
        if (!dock.IsOpen()) {
            dock.SetOpen(true);
            SKSE::log::info("SH2 dock: shown");
        }
    }

    void hide_dock()
    {
        if (hosts_dock() && dock.IsOpen()) {
            dock.SetOpen(false);
            SKSE::log::info("SH2 dock: hidden");
        }
    }

    int dock_hovered_slot()
    {
        return hosts_dock() ? dock.hovered_slot() : -1;
    }

    DockActions take_dock_actions()
    {
        return hosts_dock() ? dock.take_actions() : DockActions{};
    }

    bool take_dock_drag(float& a_dx, float& a_dy)
    {
        a_dx = a_dy = 0.0f;
        return hosts_dock() && dock.take_drag(a_dx, a_dy);
    }

    bool host_is_taking_keystrokes()
    {
        if (!connected.load(std::memory_order_relaxed)) {
            return false;
        }
        // Both read ImGui state that the render thread owns, so an answer can be a frame stale.
        // Stale in the true direction costs one ignored keypress; stale in the false direction
        // eats a letter out of somebody's search box, which is the bug. Erring true is correct.
        return FUCK::IsAnyItemActive() || FUCK::IsBinding();
    }

    void register_surfaces()
    {
        static std::atomic<bool> done{ false };
        if (!connected.load(std::memory_order_relaxed) || done.exchange(true)) {
            return;
        }
        register_windows();
        register_hud_windows();
        register_ui_windows(WindowSet{});
        FlickUi::ConfigTool::register_tool();
    }
}
