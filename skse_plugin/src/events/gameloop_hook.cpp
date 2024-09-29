#include "gameloop_hook.h"
#include "../logger/logger.h"
#include "../casts/casting_controller.h"
#include "../casts/spell_proc.h"
#include "../game_data/game_data.h"
#include "../bar/hotbars.h"

namespace SpellHotbar::events {

    bool was_blocking{ false };

	void GameLoopHook::Timinghook()
	{
        _Timinghook();
        auto ui = RE::UI::GetSingleton();
        if (ui && !ui->GameIsPaused()) {
            casts::CastingController::update_cast(deltaTime);
            casts::SpellProc::update_timer(deltaTime);
        }
        Bars::update_oblivion_bar_press_show_timer(deltaTime);
        auto pc = RE::PlayerCharacter::GetSingleton();
        if (pc) {
            bool blocking = pc->IsBlocking();
            if (blocking) {
                if (!was_blocking) {
                    GameData::block_timer = 0.0f;
                }
                else {
                    GameData::block_timer += deltaTime;
                }
            }
            else {
                if (was_blocking) {
                    //logger::info("Stopped Blocking after {}s", GameData::block_timer);
                    GameData::block_timer = 0.0f;
                }
            }
            was_blocking = blocking;


            auto caster_l = pc->GetMagicCaster(RE::MagicSystem::CastingSource::kLeftHand);
            auto caster_r = pc->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand);

            float ct_l = 0.0f;
            float ct_r = 0.0f;

            if (caster_l->currentSpell && caster_l->currentSpell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration) {
                ct_l = caster_l->castingTimer;
            }
            if (caster_r->currentSpell && caster_r->currentSpell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration) {
                ct_r = caster_r->castingTimer;
            }

            constexpr float dur = 4.0f;
            if(ct_l >= dur || ct_r >= dur)
            {
                casts::SpellProc::trigger_spellproc();
            }
        }
	}
}
