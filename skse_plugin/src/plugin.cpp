#include "logger/logger.h"
#include "papyrus_extensions/papyrus_functions.h"
#include "rendering/render_manager.h"
#include "storage/storage.h"
#include "game_data/game_data.h"
#include "bar/hotbars.h"
#include "input/input.h"
#include "events/eventlistener.h"
#include "events/animationeventhook.h"
#include "events/gameloop_hook.h"


constexpr uint32_t serializazion_id = 0xB8498471; //random generated 4byte

SKSEPluginLoad(const SKSE::LoadInterface * skse)
{
    SKSE::Init(skse);
    SpellHotbar::SetupLogger();
    logger::trace("SpellHotbar2 logger setup!");

    SpellHotbar::Bars::init();

    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        //logger::trace("Received SKSE Message {}", message->type);
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            SpellHotbar::GameData::onDataLoad();
            logger::info("SpellHotbar2 GameData loaded!");
        }
     });

    //Install animationeventhook
    //SpellHotbar::events::install(); //No need for now
    SpellHotbar::events::GameLoopHook::hook();

    auto event_listener = SpellHotbar::events::EventListener::GetSingleton();
    SKSE::GetActionEventSource()->AddEventSink(event_listener);
    auto eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
    eventSourceHolder->AddEventSink<RE::TESSpellCastEvent>(event_listener);
    eventSourceHolder->AddEventSink<RE::TESHitEvent>(event_listener);

    RE::CriticalHit::GetEventSource()->AddEventSink(event_listener);

    //SKSE::GetActionEventSource()->AddEventSink(event_listener);

    SKSE::GetPapyrusInterface()->Register(SpellHotbar::register_papyrus_functions);
    SpellHotbar::RenderManager::install();

    SKSE::AllocTrampoline(1 << 4);
    SpellHotbar::Input::install_hook();
    logger::info("SpellHotbar2 Papyrus DLL functions registered!");

    auto serialization = SKSE::GetSerializationInterface();
    serialization->SetUniqueID(serializazion_id);
    serialization->SetSaveCallback(SpellHotbar::Storage::SaveCallback);
    serialization->SetLoadCallback(SpellHotbar::Storage::LoadCallback);
    logger::info("SpellHotbar2 serialization registered!");

    return true;
}