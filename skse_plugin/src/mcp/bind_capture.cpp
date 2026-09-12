#include "bind_capture.h"

namespace SpellHotbar::Mcp {
    BindCaptureState& bind_capture()
    {
        static BindCaptureState state;
        return state;
    }
}
