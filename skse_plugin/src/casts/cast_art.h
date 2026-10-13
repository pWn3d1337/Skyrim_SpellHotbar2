#pragma once
#include "../bar/hotbar.h"

/*
* Casting art of a hotbar cast: the art object the game shows in the hand for the spell (castingArt of its magic effect),
* attached to the magic node of the casting hand. The art meshes bring an own behavior graph that reacts to the
* animation events of a regular cast, they are sent to it from here.
*/
namespace SpellHotbar::casts::cast_art {

	void start(RE::PlayerCharacter* pc, const RE::SpellItem* spell, hand_mode used_hand);

	//The cast is past its release, the art plays its cast state
	void release();

	//Per frame. cast_active: a cast instance still exists, without one the art is removed.
	void update(RE::PlayerCharacter* pc, bool cast_active);

	void stop();
}
