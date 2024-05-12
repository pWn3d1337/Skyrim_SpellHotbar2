#pragma once
namespace SpellHotbar::events {
    class EventListener : public RE::BSTEventSink<RE::TESSpellCastEvent>, public RE::BSTEventSink<SKSE::ActionEvent>
    {
    public:

        static EventListener* GetSingleton();

        EventListener(const EventListener&) = delete;
        EventListener& operator=(const EventListener) = delete;
    
        RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* event,
                                              RE::BSTEventSource<RE::TESSpellCastEvent>*);

        RE::BSEventNotifyControl ProcessEvent(const SKSE::ActionEvent* event,
            RE::BSTEventSource<SKSE::ActionEvent>*);

    private:
        EventListener() = default;

    };
}