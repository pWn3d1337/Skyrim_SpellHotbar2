#include "cast_art.h"
#include "../logger/logger.h"

namespace SpellHotbar::casts::cast_art {

	namespace {
		constexpr std::string_view magic_node_left{ "NPC L MagicNode [LMag]"sv };
		constexpr std::string_view magic_node_right{ "NPC R MagicNode [RMag]"sv };

		//Variables of the art behaviors (meshes/magic/magicbehaviors), they select the hand the events are meant for
		constexpr std::string_view var_left_hand{ "bLeftHand"sv };
		constexpr std::string_view var_right_hand{ "bRightHand"sv };
		constexpr std::string_view var_casting_dual{ "isCastingDual"sv };

		struct ArtEvent {
			std::string name;
			//only sent if the event before it was refused by the art graph
			bool fallback{ false };
			//the art is removed if its graph does not know this event
			bool required{ false };
		};

		struct HandArt {
			RE::NiPointer<RE::ModelReferenceEffect> effect;
			bool right{ false };
			bool variables_set{ false };
			bool scaled{ false };
			//events the art graph still has to get, in order. An event sent before the graph is loaded is lost.
			std::vector<ArtEvent> pending;
			//frames the first pending event was refused
			int refused{ 0 };
		};

		std::vector<HandArt> arts;

		//state of the current cast, needed to create the art again after a change of the point of view
		const RE::SpellItem* current_spell{ nullptr };
		hand_mode current_hand{ hand_mode::left_hand };
		bool released{ false };
		bool first_person{ false };

		//Dual cast of a one handed spell: the left hand shows the (scaled) art, the art of the right hand goes into its quiet dual state
		bool is_dual_cast()
		{
			return current_hand == hand_mode::dual_hand && !current_spell->IsTwoHanded();
		}

		bool is_first_person()
		{
			auto camera = RE::PlayerCamera::GetSingleton();
			return camera && camera->IsInFirstPerson();
		}

		const RE::EffectSetting* get_art_effect(const RE::SpellItem* spell)
		{
			const RE::Effect* costliest = spell->GetCostliestEffectItem();
			if (costliest && costliest->baseEffect && costliest->baseEffect->data.castingArt) {
				return costliest->baseEffect;
			}
			for (const auto* effect : spell->effects) {
				if (effect && effect->baseEffect && effect->baseEffect->data.castingArt) {
					return effect->baseEffect;
				}
			}
			return nullptr;
		}

		RE::NiAVObject* get_magic_node(RE::PlayerCharacter* pc, bool right)
		{
			if (auto root = pc->Get3D(first_person)) {
				if (auto node = root->GetObjectByName(right ? magic_node_right : magic_node_left)) {
					return node;
				}
			}
			auto caster = pc->GetMagicCaster(right ? RE::MagicSystem::CastingSource::kRightHand : RE::MagicSystem::CastingSource::kLeftHand);
			return caster ? caster->GetMagicNode() : nullptr;
		}

		//frames an event is offered to a loaded art graph before it is dropped, not every art behavior knows every event
		constexpr int max_refused_frames{ 30 };

		//Animation events of a regular cast that start the charge of this spell.
		//The ritual and dual cast events only exist in some art behaviors, the event of the hand is the fallback.
		void add_start_events(std::vector<ArtEvent>& events, const RE::SpellItem* spell, hand_mode used_hand, bool right)
		{
			const bool concentration = spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;
			const bool self = spell->GetDelivery() == RE::MagicSystem::Delivery::kSelf;
			const std::string type = std::string(self ? "Self" : "Aimed") + (concentration ? "ConcentrationStart" : "Start");

			bool has_special{ true };
			if (spell->IsTwoHanded()) {
				events.push_back({ concentration ? "RitualSpellAimConcentrationStart" : "RitualSpellStart" });
			}
			else if (used_hand == hand_mode::dual_hand) {
				if (right) {
					//an art behavior without dual state would play a second full effect in the right hand, the game shows none there
					events.push_back({ "DualMagic_Spell" + type, false, true });
					return;
				}
				events.push_back({ "DualMagic_Spell" + type });
			}
			else {
				has_special = false;
			}
			events.push_back({ (right ? "MRh_Spell" : "MLh_Spell") + type, has_special });
		}

		std::string get_release_event(bool right)
		{
			return right ? "MRh_SpellRelease_Event" : "MLh_SpellRelease_Event";
		}

		//Effects of the given art on the player
		std::vector<RE::ModelReferenceEffect*> find_effects(RE::PlayerCharacter* pc, const RE::BGSArtObject* art_object)
		{
			std::vector<RE::ModelReferenceEffect*> found;
			if (auto process_lists = RE::ProcessLists::GetSingleton()) {
				process_lists->ForEachModelEffect([&](RE::ModelReferenceEffect* model_effect) {
					if (model_effect->artObject == art_object && !model_effect->finished && model_effect->target.get().get() == pc) {
						found.push_back(model_effect);
					}
					return RE::BSContainer::ForEachResult::kContinue;
				});
			}
			return found;
		}

		//ApplyArtObject does not return the effect it creates (the value is not a pointer), it is the one that is new in the list afterwards
		RE::ModelReferenceEffect* apply_art(RE::PlayerCharacter* pc, RE::BGSArtObject* art_object, RE::NiAVObject* node)
		{
			const auto before = find_effects(pc, art_object);
			//no duration, the art is removed when the cast ends
			pc->ApplyArtObject(art_object, -1.0f, nullptr, false, false, node, false);
			for (auto model_effect : find_effects(pc, art_object)) {
				if (std::find(before.begin(), before.end(), model_effect) == before.end()) {
					return model_effect;
				}
			}
			return nullptr;
		}

		void add_art(RE::PlayerCharacter* pc, const RE::EffectSetting* effect, bool right)
		{
			auto node = get_magic_node(pc, right);
			if (!node) {
				logger::warn("cast art: no magic node for the {} hand", right ? "right" : "left");
				return;
			}
			auto model_effect = apply_art(pc, effect->data.castingArt, node);
			if (!model_effect) {
				logger::warn("cast art: could not apply art {:08x}", effect->data.castingArt->GetFormID());
				return;
			}
			HandArt art;
			art.effect.reset(model_effect);
			art.right = right;
			add_start_events(art.pending, current_spell, current_hand, right);
			if (released && !(right && is_dual_cast())) {
				art.pending.push_back({ get_release_event(right) });
			}
			arts.push_back(std::move(art));
		}

		void create(RE::PlayerCharacter* pc)
		{
			const RE::EffectSetting* effect = get_art_effect(current_spell);
			if (!effect) {
				return;
			}
			first_person = is_first_person();
			if (current_hand == hand_mode::left_hand || current_hand == hand_mode::dual_hand) {
				add_art(pc, effect, false);
			}
			if (current_hand == hand_mode::right_hand || current_hand == hand_mode::dual_hand) {
				add_art(pc, effect, true);
			}
		}

		void remove_arts()
		{
			for (auto& art : arts) {
				if (art.effect) {
					art.effect->finished = true;
				}
			}
			arts.clear();
		}

		void update_art(HandArt& art)
		{
			auto effect = art.effect.get();
			//the mesh is cloned in the background, the graph comes with it
			if (!effect->Get3D() || !effect->animationGraphManager) {
				return;
			}

			if (!art.variables_set) {
				art.variables_set = true;
				effect->SetGraphVariableBool(var_left_hand, !art.right);
				effect->SetGraphVariableBool(var_right_hand, art.right);
				effect->SetGraphVariableBool(var_casting_dual, is_dual_cast());
			}

			if (!art.scaled) {
				art.scaled = true;
				//vanilla scales the art of a dual cast
				if (is_dual_cast() && !art.right && !current_spell->GetNoDualCastModifications()) {
					if (const RE::EffectSetting* art_effect = get_art_effect(current_spell); art_effect && art_effect->data.dualCastScale > 0.0f) {
						effect->Get3D()->local.scale *= art_effect->data.dualCastScale;
					}
				}
			}

			while (!art.pending.empty()) {
				const bool accepted = effect->NotifyAnimationGraph(art.pending.front().name);
				//refused: graph not loaded yet or the event is unknown to this art behavior
				if (!accepted && ++art.refused < max_refused_frames) {
					break;
				}
				art.refused = 0;
				if (!accepted && art.pending.front().required) {
					effect->finished = true;
					art.effect.reset();
					art.pending.clear();
					return;
				}
				art.pending.erase(art.pending.begin());
				//the fallback of an accepted event is not needed, the one of a dropped event is tried next
				if (accepted) {
					while (!art.pending.empty() && art.pending.front().fallback) {
						art.pending.erase(art.pending.begin());
					}
				}
			}
		}
	}

	void start(RE::PlayerCharacter* pc, const RE::SpellItem* spell, hand_mode used_hand)
	{
		remove_arts();
		current_spell = spell;
		current_hand = used_hand;
		released = false;
		if (pc && spell) {
			create(pc);
		}
	}

	void release()
	{
		if (!released) {
			released = true;
			for (auto& art : arts) {
				if (art.effect && !(art.right && is_dual_cast())) {
					art.pending.push_back({ get_release_event(art.right) });
				}
			}
		}
	}

	void update(RE::PlayerCharacter* pc, bool cast_active)
	{
		if (!cast_active) {
			if (current_spell) {
				stop();
			}
			return;
		}
		if (!current_spell || !pc) {
			return;
		}

		//the magic node belongs to one of the two skeletons of the player
		if (!arts.empty() && is_first_person() != first_person) {
			remove_arts();
			create(pc);
		}

		for (auto& art : arts) {
			if (art.effect) {
				update_art(art);
			}
		}
	}

	void stop()
	{
		remove_arts();
		current_spell = nullptr;
		released = false;
	}
}
