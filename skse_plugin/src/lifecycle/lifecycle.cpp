#include "lifecycle.h"

#include "first_run_policy.h"
#include "../game_data/game_data.h"
#include "../input/input.h"
#include "../logger/logger.h"
#include "../storage/storage.h"
#include "../storage/user_data_io.h"

namespace SpellHotbar::Lifecycle {
    namespace {
        // Vanilla SOUN 000362B6 MAGCloakIn and 0003966B MAGCloakOut — the same
        // descriptors the retired Papyrus SoundToggleOn/SoundToggleOff properties used.
        constexpr const char* k_dual_cast_on_sound = "MAGCloakIn";
        constexpr const char* k_dual_cast_off_sound = "MAGCloakOut";

        bool initialized_this_game{ false };
        bool pending_first_init{ false };
        bool pending_is_new_game{ false };
        int first_init_retries_remaining{ 0 };

        RE::SpellItem* power_for_type(int type)
        {
            switch (type) {
            case 0:
                return GameData::spellhotbar_unbind_slot;
            case 1:
                return GameData::spellhotbar_toggle_dualcast;
            default:
                return nullptr;
            }
        }

        bool has_papyrus_static_function(
            RE::BSScript::IVirtualMachine* vm,
            const RE::BSFixedString& class_name,
            const RE::BSFixedString& function_name)
        {
            RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> type_info;
            if (vm == nullptr || !vm->GetScriptObjectType(class_name, type_info) || !type_info ||
                type_info->linkedValid != RE::BSScript::ObjectTypeInfo::LinkValidState::kLinkedValid) {
                return false;
            }

            const auto* functions = type_info->GetGlobalFuncIter();
            for (std::uint32_t i = 0; i < type_info->GetNumGlobalFuncs(); ++i) {
                const auto& function = functions[i].func;
                if (function && function->GetIsStatic() && function->GetName() == function_name) {
                    return true;
                }
            }
            return false;
        }

        void remove_legacy_battlemage_power()
        {
            auto* player = RE::PlayerCharacter::GetSingleton();
            auto* legacy_power = GameData::spellhotbar_battlemage_open_perks_power;
            if (player != nullptr && legacy_power != nullptr && player->HasSpell(legacy_power)) {
                player->RemoveSpell(legacy_power);
                logger::info("Removed legacy BattleMage opener power; the tree is available from the Mod Control Panel");
            }
        }

        void grant_powers(RE::PlayerCharacter* player)
        {
            for (int type = 0; type < 2; ++type) {
                auto* power = power_for_type(type);
                if (power != nullptr && !player->HasSpell(power)) {
                    player->AddSpell(power);
                }
            }
        }

        void finish_first_initialization(RE::PlayerCharacter* player)
        {
            grant_powers(player);

            const bool profile_loaded = Storage::IO::load_preset("auto_profile.json", false);
            const std::filesystem::path auto_edits{
                "Data/SKSE/Plugins/SpellHotbar/icon_edits/auto_edits.json"
            };
            const bool edits_loaded = std::filesystem::exists(auto_edits) &&
                                      GameData::load_icon_edits_from_json(auto_edits.string());

            initialized_this_game = true;
            pending_first_init = false;
            first_init_retries_remaining = 0;
            logger::info(
                "SpellHotbar2 first initialization complete (auto profile: {}, auto icon edits: {})",
                profile_loaded,
                edits_loaded);
        }

        void begin_first_initialization(bool is_new_game)
        {
            auto* player = RE::PlayerCharacter::GetSingleton();
            const auto attempt = classify_first_init_attempt(
                true,
                initialized_this_game,
                player != nullptr,
                k_max_first_init_retries);

            if (attempt == FirstInitAttempt::complete) {
                finish_first_initialization(player);
                return;
            }

            if (attempt == FirstInitAttempt::retry) {
                pending_first_init = true;
                pending_is_new_game = is_new_game;
                first_init_retries_remaining = k_max_first_init_retries;
                logger::warn("SpellHotbar2 first initialization deferred: player is unavailable; retrying on the main loop");
            }
        }
    }

    void reset()
    {
        initialized_this_game = false;
        pending_first_init = false;
        pending_is_new_game = false;
        first_init_retries_remaining = 0;
    }

    void on_new_game()
    {
        remove_legacy_battlemage_power();
        if (should_run_first_initialization(true, Storage::loaded_existing_settings())) {
            begin_first_initialization(true);
        }
    }

    void on_post_load_game()
    {
        remove_legacy_battlemage_power();
        if (should_run_first_initialization(false, Storage::loaded_existing_settings())) {
            begin_first_initialization(false);
        }
    }

    void try_pending_first_initialization()
    {
        if (!pending_first_init) {
            return;
        }

        auto* player = RE::PlayerCharacter::GetSingleton();
        const auto attempt = classify_first_init_attempt(
            true,
            initialized_this_game,
            player != nullptr,
            first_init_retries_remaining);

        if (attempt == FirstInitAttempt::complete) {
            finish_first_initialization(player);
            return;
        }

        if (attempt == FirstInitAttempt::retry) {
            --first_init_retries_remaining;
            return;
        }

        pending_first_init = false;
        logger::error(
            "SpellHotbar2 first initialization abandoned after {} retries (new game: {})",
            k_max_first_init_retries,
            pending_is_new_game);
    }

    bool player_has_power(int type)
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        auto* power = power_for_type(type);
        return player != nullptr && power != nullptr && player->HasSpell(power);
    }

    bool toggle_player_power(int type)
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        auto* power = power_for_type(type);
        if (player == nullptr || power == nullptr) {
            return false;
        }

        if (player->HasSpell(power)) {
            player->RemoveSpell(power);
        } else {
            player->AddSpell(power);
        }
        return player->HasSpell(power);
    }

    bool open_battlemage_tree()
    {
        const bool plugin_loaded = GameData::spellhotbar_battlemage_open_perks_power != nullptr;
        if (!plugin_loaded) {
            logger::warn("Cannot open BattleMage tree: SpellHotbar_BattleMage.esp is not loaded");
            return false;
        }

        auto* skyrim_vm = RE::SkyrimVM::GetSingleton();
        if (skyrim_vm == nullptr || skyrim_vm->impl == nullptr) {
            logger::error("Cannot open BattleMage tree: Papyrus VM is unavailable");
            return false;
        }

        static const RE::BSFixedString class_name{ "CustomSkills" };
        static const RE::BSFixedString function_name{ "OpenCustomSkillMenu" };
        const bool csf_present = has_papyrus_static_function(skyrim_vm->impl.get(), class_name, function_name);
        if (!battlemage_tree_may_dispatch(plugin_loaded, csf_present)) {
            logger::error("Cannot open BattleMage tree: CustomSkills.OpenCustomSkillMenu is unavailable");
            return false;
        }

        auto* args = RE::MakeFunctionArguments(std::string{ "SpellHotbar_Battlemage" });
        RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> result;
        const bool dispatched = skyrim_vm->impl->DispatchStaticCall(class_name, function_name, args, result);
        if (!dispatched) {
            logger::error("Cannot open BattleMage tree: CustomSkills.OpenCustomSkillMenu is unavailable");
        }
        return dispatched;
    }

    bool process_spell_cast(RE::FormID spell)
    {
        auto* toggle_spell = GameData::spellhotbar_toggle_dualcast;
        auto* toggle_global = GameData::global_spellhotbar_use_dual_casting;
        if (toggle_spell == nullptr || toggle_global == nullptr || spell != toggle_spell->GetFormID()) {
            return false;
        }

        const bool enabled = toggle_global->value == 0.0F;
        toggle_global->value = enabled ? 1.0F : 0.0F;
        RE::PlaySound(enabled ? k_dual_cast_on_sound : k_dual_cast_off_sound);
        return true;
    }
}
