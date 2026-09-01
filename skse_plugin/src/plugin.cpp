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
#include "lifecycle/lifecycle.h"
#include "smf/smf_guest.h"


constexpr uint32_t serializazion_id = 0xB8498471; //random generated 4byte

SKSEPluginLoad(const SKSE::LoadInterface * skse)
{
    SKSE::Init(skse);
    SpellHotbar::SetupLogger();
    logger::trace("SpellHotbar2 logger setup!");

    SpellHotbar::Bars::init();

    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kPostLoad) {
            SpellHotbar::SmfGuest::install();
        } else if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            SpellHotbar::RenderManager::load_fixed_textures();
            SpellHotbar::GameData::onDataLoad();
            logger::info("SpellHotbar2 GameData loaded!");
        } else if (message->type == SKSE::MessagingInterface::kNewGame) {
            SpellHotbar::Lifecycle::on_new_game();
        } else if (message->type == SKSE::MessagingInterface::kPostLoadGame) {
            SpellHotbar::Lifecycle::on_post_load_game();
        }
     });

    //Install animationeventhook
    //SpellHotbar::events::install(); no need
    SpellHotbar::events::GameLoopHook::hook();

    auto event_listener = SpellHotbar::events::EventListener::GetSingleton();
    SKSE::GetActionEventSource()->AddEventSink(event_listener);
    auto eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
    eventSourceHolder->AddEventSink<RE::TESSpellCastEvent>(event_listener);
    eventSourceHolder->AddEventSink<RE::TESHitEvent>(event_listener);
    eventSourceHolder->AddEventSink<RE::TESEquipEvent>(event_listener);
    //eventSourceHolder->AddEventSink<RE::TESPlayerBowShotEvent>(event_listener);

    RE::CriticalHit::GetEventSource()->AddEventSink(event_listener);

    //SKSE::GetActionEventSource()->AddEventSink(event_listener);

    SKSE::GetPapyrusInterface()->Register(SpellHotbar::register_papyrus_functions);

    SKSE::AllocTrampoline(1 << 4);
    logger::info("SpellHotbar2 Papyrus DLL functions registered!");

    auto serialization = SKSE::GetSerializationInterface();
    serialization->SetUniqueID(serializazion_id);
    serialization->SetSaveCallback(SpellHotbar::Storage::SaveCallback);
    serialization->SetLoadCallback(SpellHotbar::Storage::LoadCallback);
    serialization->SetRevertCallback(SpellHotbar::Storage::RevertCallback);
    logger::info("SpellHotbar2 serialization registered!");

    return true;
}
