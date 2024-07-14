#include "gameloop_hook.h"
#include "../logger/logger.h"
#include "../casts/casting_controller.h"

namespace SpellHotbar::events {

	void GameLoopHook::Timinghook()
	{
        _Timinghook();
        auto ui = RE::UI::GetSingleton();
        if (ui && !ui->GameIsPaused()) {
            casts::CastingController::update_cast(deltaTime);
        }
	}
}
