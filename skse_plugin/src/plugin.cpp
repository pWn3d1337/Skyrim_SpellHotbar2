#include "flick/flick_watch.h"
#include "logger/logger.h"
#include "papyrus_extensions/papyrus_functions.h"
#include "rendering/render_manager.h"
#include "storage/storage.h"
#include "game_data/game_data.h"
#include "bar/hotbars.h"
#include "input/input.h"
#include "input/input_hook.h"
#include "events/eventlistener.h"
#include "events/animationeventhook.h"
#include "events/gameloop_hook.h"
#include "lifecycle/lifecycle.h"


constexpr uint32_t serializazion_id = 0xB8498471; //random generated 4byte

SKSEPluginLoad(const SKSE::LoadInterface * skse)
{
    SKSE::Init(skse);
    SpellHotbar::SetupLogger();
    logger::trace("SpellHotbar2 logger setup!");

    SpellHotbar::Bars::init();

    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        //logger::trace("Received SKSE Message {}", message->type);
        if (message->type == SKSE::MessagingInterface::kPostLoad) {
            //FLICK is the UI host: the windows, the dock, the HUD bars and the config tool are
            //all its guests. Connect here, once every plugin DLL is loaded; register nothing
            //yet (see kPostLoadGame).
            SpellHotbar::Flick::install();
            //The input hook goes in HERE rather than in SKSEPluginLoad so that it is written
            //after FLICK's on the same call site, which makes SH2's thunk the outer one: it sees
            //every event raw, before the host zeroes the ones it blocks. Bind capture depends
            //on that order (input/input_hook.h).
            SpellHotbar::Input::install_hook();
        }
        else if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            SpellHotbar::RenderManager::load_fixed_textures();
            SpellHotbar::GameData::onDataLoad();
            logger::info("SpellHotbar2 GameData loaded!");
        }
        else if (message->type == SKSE::MessagingInterface::kPostLoadGame) {
            //The FLICK surfaces register HERE, on the first save load, not during the initial
            //load: registering at kPostLoad or kDataLoaded crashed on a driver worker thread
            //(flick_watch.h). Idempotent, so later save loads are no-ops. A new game sends no
            //kPostLoadGame; the game loop hook covers that (gameloop_hook.cpp).
            SpellHotbar::Flick::register_surfaces();
            SpellHotbar::Lifecycle::on_post_load_game();
        }
        else if (message->type == SKSE::MessagingInterface::kNewGame) {
            SpellHotbar::Lifecycle::on_new_game();
        }
     });

    //Must precede every hook install below; a trampoline allocated after a write_call would
    //make that hook fail silently at startup.
    SKSE::AllocTrampoline(1 << 6);

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
    logger::info("SpellHotbar2 Papyrus DLL functions registered!");

    auto serialization = SKSE::GetSerializationInterface();
    serialization->SetUniqueID(serializazion_id);
    serialization->SetSaveCallback(SpellHotbar::Storage::SaveCallback);
    serialization->SetLoadCallback(SpellHotbar::Storage::LoadCallback);
    serialization->SetRevertCallback(SpellHotbar::Storage::RevertCallback);
    logger::info("SpellHotbar2 serialization registered!");

    return true;
}