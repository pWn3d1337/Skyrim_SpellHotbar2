#pragma once
#include "../logger/logger.h"

namespace SpellHotbar::events {
	//Credits to https://github.com/ersh1/OpenAnimationReplacer/

	static float& deltaTime = *(float*)REL::VariantID(523660, 410199, 0x30C3A08).address();

	class GameLoopHook {
	public:
		static void hook() {
			logger::info("Hooking Main Loop...");

			auto& trampoline = SKSE::GetTrampoline();
			const REL::Relocation<uintptr_t> mainHook{ REL::VariantID(35565, 36564, 0x5BAB10) };
			SKSE::AllocTrampoline(14);
			_Timinghook = trampoline.write_call<5>(mainHook.address() + REL::VariantOffset(0x748, 0xC26, 0x7EE).offset(), Timinghook);

			logger::info("...done");
		}

	private:
		static void Timinghook();

		static inline REL::Relocation<decltype(Timinghook)> _Timinghook;
	};


}