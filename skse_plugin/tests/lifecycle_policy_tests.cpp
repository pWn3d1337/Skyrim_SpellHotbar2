#include "lifecycle/first_run_policy.h"

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
    using SpellHotbar::Lifecycle::should_run_first_initialization;
    using SpellHotbar::Lifecycle::battlemage_tree_may_dispatch;
    using SpellHotbar::Lifecycle::classify_first_init_attempt;
    using SpellHotbar::Lifecycle::FirstInitAttempt;
    using SpellHotbar::Lifecycle::k_max_first_init_retries;

    require(should_run_first_initialization(true, false), "new game must initialize without settings");
    require(should_run_first_initialization(true, true), "new game must initialize even after another save");
    require(should_run_first_initialization(false, false), "legacy save without HOTB must initialize");
    require(!should_run_first_initialization(false, true), "existing settings must never be overwritten");

    require(!battlemage_tree_may_dispatch(false, false), "missing BattleMage plugin must not dispatch");
    require(!battlemage_tree_may_dispatch(false, true), "CSF without BattleMage plugin must not dispatch");
    require(!battlemage_tree_may_dispatch(true, false), "BattleMage plugin without CSF must not dispatch");
    require(battlemage_tree_may_dispatch(true, true), "BattleMage plugin and CSF may dispatch");

    require(
        classify_first_init_attempt(false, false, false, k_max_first_init_retries) == FirstInitAttempt::skipped,
        "unneeded init must skip");
    require(
        classify_first_init_attempt(true, true, true, k_max_first_init_retries) == FirstInitAttempt::skipped,
        "already initialized must skip");
    require(
        classify_first_init_attempt(true, false, true, 0) == FirstInitAttempt::complete,
        "player present must complete");
    require(
        classify_first_init_attempt(true, false, false, k_max_first_init_retries) == FirstInitAttempt::retry,
        "missing player must retry while budget remains");
    require(
        classify_first_init_attempt(true, false, false, 0) == FirstInitAttempt::give_up,
        "missing player with no retries must give up");
    require(k_max_first_init_retries > 0, "retry budget must be bounded and positive");
    return EXIT_SUCCESS;
}
