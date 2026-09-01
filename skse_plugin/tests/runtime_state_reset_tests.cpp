#include "storage/runtime_state_reset.h"

#include <cstdlib>
#include <iostream>

namespace {
    void require(bool condition, const char* message)
    {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
}

int main()
{
    int bars_resets = 0;
    int input_resets = 0;
    int game_data_resets = 0;
    int lifecycle_resets = 0;

    SpellHotbar::Storage::reset_runtime_state(
        [&] { ++bars_resets; },
        [&] { ++input_resets; },
        [&] { ++game_data_resets; },
        [&] { ++lifecycle_resets; });

    require(bars_resets == 1, "bar state must reset exactly once");
    require(input_resets == 1, "input state must reset exactly once");
    require(game_data_resets == 1, "game data must reset exactly once");
    require(lifecycle_resets == 1, "lifecycle state must reset exactly once");
    return EXIT_SUCCESS;
}
