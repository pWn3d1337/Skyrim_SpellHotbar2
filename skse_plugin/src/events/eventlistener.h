#pragma once
namespace SpellHotbar::events {
    class EventListener : public RE::BSTEventSink<RE::TESSpellCastEvent>, public RE::BSTEventSink<SKSE::ActionEvent>, public RE::BSTEventSink<RE::TESHitEvent>,
        public RE::BSTEventSink<RE::CriticalHit::Event> //, public RE::BSTEventSink<RE::TESPlayerBowShotEvent>
    {
    public:

        static EventListener* GetSingleton();

        EventListener(const EventListener&) = delete;
        EventListener& operator=(const EventListener) = delete;
    
        RE::BSEventNotifyControl ProcessEvent(const RE::TESSpellCastEvent* event,
                                              RE::BSTEventSource<RE::TESSpellCastEvent>*);

        RE::BSEventNotifyControl ProcessEvent(const SKSE::ActionEvent* event,
            RE::BSTEventSource<SKSE::ActionEvent>*);

        RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent* event,
            RE::BSTEventSource<RE::TESHitEvent>*);

        RE::BSEventNotifyControl ProcessEvent(const RE::CriticalHit::Event* event, RE::BSTEventSource<RE::CriticalHit::Event>*);

        //RE::BSEventNotifyControl ProcessEvent(const RE::TESPlayerBowShotEvent* event, RE::BSTEventSource<RE::TESPlayerBowShotEvent>*);

    private:
        EventListener() = default;

    };
}