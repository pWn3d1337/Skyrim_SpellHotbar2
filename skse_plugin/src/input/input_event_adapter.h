#pragma once

#include <utility>

namespace SpellHotbar::Input {
    template <class Event>
    struct InputEventDecision {
        bool capture{ false };
        Event* injected{ nullptr };
    };

    /**
     * Adapts Spell Hotbar's input decision to SKSE Menu Framework's per-event
     * callback contract. Injected events are spliced into the host-owned list
     * and skipped once when the host reaches them, so a synthetic shout event
     * reaches Skyrim without recursively triggering Spell Hotbar.
     */
    template <class Event>
    class InputEventAdapter {
    public:
        template <class Processor>
        bool process(Event* event, Processor&& processor)
        {
            if (!event) {
                return false;
            }

            if (event == ignored_injected_event_) {
                ignored_injected_event_ = nullptr;
                return false;
            }

            const auto decision = std::forward<Processor>(processor)(event);
            if (decision.injected && decision.injected != event) {
                decision.injected->next = event->next;
                event->next = decision.injected;
                ignored_injected_event_ = decision.injected;
            }

            return decision.capture;
        }

    private:
        Event* ignored_injected_event_{ nullptr };
    };
}
