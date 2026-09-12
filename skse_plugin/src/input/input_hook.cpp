#include "input_hook.h"

#include "input.h"
#include "../logger/logger.h"

namespace SpellHotbar::Input {
    namespace {
        // From https://github.com/SlavicPotato/ied-dev and https://github.com/D7ry/wheeler: the
        // engine dispatches one frame's input as a linked list, so a filter that unlinks a node
        // hides that event from every handler downstream of this call.
        struct DispatchInputEventHook {
            static void thunk(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent** a_events)
            {
                if (a_events != nullptr) {
                    processAndFilter(a_events);
                }
                func(a_dispatcher, a_events);
            }
            static inline REL::Relocation<decltype(thunk)> func;
        };
    }

    void install_hook()
    {
        auto& trampoline = SKSE::GetTrampoline();
        const REL::Relocation<std::uintptr_t> caller{ RELOCATION_ID(67315, 68617) };
        DispatchInputEventHook::func = trampoline.write_call<5>(
            caller.address() + REL::VariantOffset(0x7B, 0x7B, 0).offset(), DispatchInputEventHook::thunk);
        logger::info("Hooked the input dispatch site (67315+0x7B)");
    }
}
