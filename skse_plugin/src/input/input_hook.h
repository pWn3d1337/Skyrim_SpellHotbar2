#pragma once

// Spell Hotbar 2's input hook: the dispatch call site at RELOCATION_ID(67315, 68617)+0x7B, the
// one wheeler / IED hook and the one FLICK hooks too. Two write_call<5> writes on one site
// chain: the later install runs first and calls the earlier thunk as its original, so SH2 and
// FLICK both see every frame's events in either load order. Installing at kPostLoad puts SH2
// ahead of FLICK, which is what lets a key typed into the config tool's Rebind reach the
// capture before the host zeroes it.
namespace SpellHotbar::Input {
    // Install the dispatch hook. Needs the SKSE trampoline allocated (plugin.cpp does that once,
    // before every hook install). Call once, at kPostLoad.
    void install_hook();
}
