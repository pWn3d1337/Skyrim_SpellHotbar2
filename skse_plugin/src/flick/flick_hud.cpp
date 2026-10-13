#include "flick_watch.h"
#include "flick_images.h"
#include "../rendering/ui_bridge.h"
#include "../bar/hud_geometry.h"


// The gameplay HUD bars as FLICK guest windows. FLICK is the only ImGui in the plugin's life,
// so the bar is a window like the dock: chromeless, backgroundless, always open, and
// kPassInputToGame so the game never loses a click or a key to it. FUCK-Man renders a
// passthrough window every frame it renders anything, blocks no input for it and forces no
// cursor (FUCKMan::IsInputBlocked, ShouldRenderPassthrough), which is exactly the HUD contract.
//
// Every call here goes through FUCK::, never ImGui:: (flick_pch.h).
namespace SpellHotbar::Flick {
    namespace {
        using FlickImages::image_for;
        using FlickImages::unpack;

        // The frame the plugin side built for this render pass. SH2_Hud registers first and
        // asks for it in its Draw(); SH2_OblivionHud, registered after it, draws the second
        // layer of the same frame. Both run on FLICK's render pass in registration order.
        UiBridge::HudFrame current_frame;

        // measured_slot_shift(): the HUD bar, the Oblivion bar, the dock. Render thread only.
        std::array<SlotShift, 3> slot_shifts{};

        class HudLayerWindow : public FUCK::IWindow
        {
        public:
            HudLayerWindow(const char* a_id, const char* a_title, bool a_builds_frame)
                : id_(a_id), title_(a_title), builds_frame_(a_builds_frame)
            {}

            const char* Id() const override { return id_; }
            const char* Title() const override { return title_; }
            bool IsOpen() const override { return true; }
            void SetOpen(bool) override {}

            FUCK::WindowFlags GetFlags() const override
            {
                using F = FUCK::WindowFlags;
                return F::kNoDecoration | F::kNoBackground | F::kAutoResize | F::kNoResize | F::kCustomPosition |
                       F::kNoMove | F::kIgnoreUserScale | F::kPassInputToGame;
            }

            ImVec2 GetDefaultPos() const override { return ImVec2(0.0f, 0.0f); }

            void Draw() override
            {
                if (builds_frame_) {
                    const ImVec2 display = FUCK::GetDisplaySize();
                    current_frame = UiBridge::build_hud(FUCK::GetDeltaTime(), display.x, display.y);
                }
                const HudLayer& layer = builds_frame_ ? current_frame.main : current_frame.oblivion;

                // ImGui clips a window's draw list to its rect inset by the style's
                // WindowBorderSize on every side (InnerClipRect), and FLICK's theme sets that to
                // 5.3 px with zero WindowPadding. Content laid out from the window edge
                // therefore lost its outer five pixels: the first slot's left bevel, the last
                // slot's right bevel, every slot's bottom bevel and the count text -- "missing
                // borders". The dock never showed it because its plate
                // insets the content by its own padding. So the content sits `margin` inside
                // the window and the window is that much larger; place() subtracts the margin,
                // so the slots land exactly where the anchor puts them.
                margin_ = std::ceil(FUCK::GetStyleVar(ImGuiStyleVar_WindowBorderSize)) + 1.0f;

                // Anchor and offsets against last frame's size, as the dock does: the window is
                // auto-sized, so this frame's extent is known only once it has drawn.
                place(layer);
                last_size_ = FUCK::GetWindowSize();

                if (!layer.visible) {
                    FUCK::Dummy(ImVec2(1.0f, 1.0f));
                    return;
                }

                const ImVec2 corner = FUCK::GetCursorScreenPos();
                const ImVec2 origin(corner.x + margin_, corner.y + margin_);
                ImFont* font = FUCK::GetFont(FUCK::Font::kRegular);

                // How far the content landed from where place() aimed it, past ImGui's
                // truncation of the window position, for the bar-position editor.
                {
                    const ImVec2 d = FUCK::GetDisplaySize();
                    const HudGeometry::Point aim = HudGeometry::anchored_corner(
                        layer.anchor, d.x, d.y, layer.offset_x, layer.offset_y, layer.width, layer.height);
                    record_slot_shift(builds_frame_ ? 0 : 1, origin.x - (std::trunc(aim.x - margin_) + margin_),
                                      origin.y - (std::trunc(aim.y - margin_) + margin_));
                }

                for (const DockImage& img : layer.images) {
                    FUCK::Image* tex = image_for(img.path);
                    if (tex == nullptr) {
                        continue;
                    }
                    FUCK::AddImage(tex->GetID(),
                                   ImVec2(origin.x + img.x0, origin.y + img.y0),
                                   ImVec2(origin.x + img.x1, origin.y + img.y1),
                                   ImVec2(img.u0, img.v0), ImVec2(img.u1, img.v1), unpack(img.col));
                }

                for (const DockRect& r : layer.rects) {
                    const ImVec2 r0(origin.x + r.x0, origin.y + r.y0);
                    const ImVec2 r1(origin.x + r.x1, origin.y + r.y1);
                    if (r.thickness > 0.0f) {
                        FUCK::DrawRect(r0, r1, unpack(r.col), 0.0f, r.thickness);
                    } else {
                        FUCK::DrawRectFilled(r0, r1, unpack(r.col), 0.0f);
                    }
                }

                // FLICK's C-ABI has no draw-list text, so text is laid out: park the cursor and
                // emit. The Dummy below restores the layout extent afterwards.
                for (const DockText& t : layer.texts) {
                    // A text at alpha 0 is not drawn at all. FLICK paints its text with a dark
                    // shadow that ignores the colour's alpha byte, so the bar name -- faded out
                    // to alpha 0 after its 2.5 s -- kept showing as a black word.
                    if ((t.col & 0xFF000000u) == 0u) {
                        continue;
                    }
                    const float px = t.px > 0.0f ? t.px : std::max(layer.text_px, 6.0f);
                    FUCK::PushFont(font, px);
                    ImVec2 at(origin.x + t.x, origin.y + t.y);
                    if (t.anchor != 0) {
                        const ImVec2 sz = FUCK::CalcTextSize(t.text.c_str());
                        if (t.anchor == 3) {
                            at.x -= sz.x * 0.5f;
                        } else {
                            at.x -= sz.x;
                            if (t.anchor == 2) {
                                at.y -= sz.y;
                            }
                        }
                    }
                    FUCK::SetCursorScreenPos(at);
                    FUCK::PushStyleColor(ImGuiCol_Text, unpack(t.col));
                    FUCK::TextUnformatted(t.text.c_str());
                    FUCK::PopStyleColor(1);
                    FUCK::PopFont();
                }

                FUCK::SetCursorScreenPos(corner);
                FUCK::Dummy(ImVec2(std::max(layer.width, 1.0f) + margin_ * 2.0f,
                                   std::max(layer.height, 1.0f) + margin_ * 2.0f));
            }

        private:
            // Bars::anchor_point through HudGeometry, the table the dock and the bar-position
            // editor use too: the anchor names the screen point and the pivot of the window
            // that sits on it.
            void place(const HudLayer& a_layer) const
            {
                const ImVec2 d = FUCK::GetDisplaySize();
                // The plugin side reports the content extent it is about to draw, so the pivot
                // can use it directly and the window lands on its anchor on the first frame.
                // The window is the content plus the clip margin on each side; the CONTENT is
                // what sits on the anchor, so the margin comes off the window position.
                const ImVec2 size = a_layer.visible ? ImVec2(a_layer.width, a_layer.height) : last_size_;
                const float m = a_layer.visible ? margin_ : 0.0f;  // last_size_ already holds the margin
                const HudGeometry::Point at = HudGeometry::anchored_corner(a_layer.anchor, d.x, d.y, a_layer.offset_x,
                                                                           a_layer.offset_y, size.x, size.y);
                FUCK::SetWindowPos(ImVec2(at.x - m, at.y - m), ImGuiCond_Always);
            }

            const char* id_;
            const char* title_;
            bool builds_frame_;
            mutable ImVec2 last_size_{ 0.0f, 0.0f };
            mutable float margin_{ 0.0f };
        };

        HudLayerWindow* hud_window{ nullptr };
        HudLayerWindow* oblivion_window{ nullptr };
    }

    SlotShift measured_slot_shift(int a_bar)
    {
        return a_bar >= 0 && a_bar < static_cast<int>(slot_shifts.size()) ? slot_shifts[a_bar] : SlotShift{};
    }

    void record_slot_shift(int a_bar, float a_x, float a_y)
    {
        if (a_bar >= 0 && a_bar < static_cast<int>(slot_shifts.size())) {
            slot_shifts[a_bar] = SlotShift{ a_x, a_y };
        }
    }

    void register_hud_windows()
    {
        if (!is_connected() || hud_window != nullptr) {
            return;
        }
        // Leaked on purpose, like the dock: FLICK keeps the pointer for the life of the process.
        hud_window = new HudLayerWindow("SH2_Hud", "Spell Hotbar 2", true);
        oblivion_window = new HudLayerWindow("SH2_OblivionHud", "Spell Hotbar 2 Oblivion bar", false);
        FUCK::RegisterWindow(hud_window);
        FUCK::RegisterWindow(oblivion_window);
        logger::info("SH2 FLICK: registered the HUD windows SH2_Hud and SH2_OblivionHud");
    }
}
