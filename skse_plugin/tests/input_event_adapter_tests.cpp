#include "input/input_event_adapter.h"

#include <cstdlib>
#include <iostream>

namespace {
    struct Event {
        Event* next{ nullptr };
        int id{ 0 };
    };

    using Adapter = SpellHotbar::Input::InputEventAdapter<Event>;
    using Decision = SpellHotbar::Input::InputEventDecision<Event>;

    void require(bool condition, const char* message)
    {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    void returns_the_domain_capture_decision()
    {
        Adapter adapter;
        Event event{ .id = 1 };

        const bool captured = adapter.process(&event, [](Event*) {
            return Decision{ .capture = true };
        });

        require(captured, "captured input must be removed by the SMF host");
    }

    void splices_an_injected_event_immediately_after_its_source()
    {
        Adapter adapter;
        Event tail{ .id = 3 };
        Event source{ .next = &tail, .id = 1 };
        Event injected{ .id = 2 };

        const bool captured = adapter.process(&source, [&injected](Event*) {
            return Decision{ .capture = false, .injected = &injected };
        });

        require(!captured, "injection must not change the source capture decision");
        require(source.next == &injected, "synthetic shout input must follow its source event");
        require(injected.next == &tail, "synthetic shout input must preserve the remaining host queue");
    }

    void forwards_injected_shout_down_and_up_without_processing_them_again()
    {
        Adapter adapter;
        Event source_down{ .id = 1 };
        Event shout_down{ .id = 2 };
        Event source_up{ .id = 3 };
        Event shout_up{ .id = 4 };
        int domain_calls = 0;

        adapter.process(&source_down, [&](Event*) {
            ++domain_calls;
            return Decision{ .injected = &shout_down };
        });
        const bool down_captured = adapter.process(&shout_down, [&](Event*) {
            ++domain_calls;
            return Decision{ .capture = true };
        });

        adapter.process(&source_up, [&](Event*) {
            ++domain_calls;
            return Decision{ .injected = &shout_up };
        });
        const bool up_captured = adapter.process(&shout_up, [&](Event*) {
            ++domain_calls;
            return Decision{ .capture = true };
        });

        require(!down_captured && !up_captured, "synthetic shout down/up input must reach Skyrim");
        require(domain_calls == 2, "synthetic shout down/up input must not recursively trigger Spell Hotbar");
    }
}

int main()
{
    returns_the_domain_capture_decision();
    splices_an_injected_event_immediately_after_its_source();
    forwards_injected_shout_down_and_up_without_processing_them_again();
    return EXIT_SUCCESS;
}
