#include "gameloop_hook.h"
#include "../logger/logger.h"
#include "../casts/casting_controller.h"

namespace SpellHotbar::events {

	void GameLoopHook::Timinghook()
	{
        auto ui = RE::UI::GetSingleton();
        if (!ui) {
            return;  // no ui reference, quit
        }
        if (!ui->GameIsPaused()) {
            //logger::info("GameTimeDelta: {}", deltaTime);
            casts::CastingController::update_cast(deltaTime);
        }
	}
}
