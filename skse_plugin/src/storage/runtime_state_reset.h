#pragma once

#include <utility>

namespace SpellHotbar::Storage {
    template <class ResetBars, class ResetInput, class ResetGameData, class ResetLifecycle>
    void reset_runtime_state(
        ResetBars&& reset_bars,
        ResetInput&& reset_input,
        ResetGameData&& reset_game_data,
        ResetLifecycle&& reset_lifecycle)
    {
        std::forward<ResetBars>(reset_bars)();
        std::forward<ResetInput>(reset_input)();
        std::forward<ResetGameData>(reset_game_data)();
        std::forward<ResetLifecycle>(reset_lifecycle)();
    }
}
