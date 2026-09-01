#pragma once

namespace SpellHotbar::Mcp {
    inline constexpr int k_first_keybind_id = 0;
    inline constexpr int k_last_keybind_id = 22;

    enum class CaptureApply : unsigned char {
        ignored,
        cancelled,
        rebound
    };

    class BindCaptureState {
    public:
        bool arm(int key_id)
        {
            if (key_id < k_first_keybind_id || key_id > k_last_keybind_id) {
                return false;
            }
            pending_id_ = key_id;
            return true;
        }

        void cancel() { pending_id_ = -1; }

        bool armed() const { return pending_id_ >= 0; }

        int pending_id() const { return pending_id_; }

        bool consume_escape()
        {
            if (!armed()) {
                return false;
            }
            cancel();
            return true;
        }

        bool consume_rebind()
        {
            if (!armed()) {
                return false;
            }
            cancel();
            return true;
        }

        CaptureApply apply_down_edge(bool is_escape)
        {
            if (!armed()) {
                return CaptureApply::ignored;
            }
            if (is_escape) {
                consume_escape();
                return CaptureApply::cancelled;
            }
            consume_rebind();
            return CaptureApply::rebound;
        }

    private:
        int pending_id_{-1};
    };

    BindCaptureState& bind_capture();
}
