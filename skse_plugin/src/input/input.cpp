#include "input.h"
#include "keybinds.h"
#include "../logger/logger.h"
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "../rendering/render_manager.h"
#include "../casts/casting_controller.h"
#include "../storage/storage.h"
#include "keycode_helper.h"
#include "modes.h"

namespace {

    // from https://github.com/SlavicPotato/ied-dev / https://github.com/D7ry/wheeler/

    /// Hooks the input event dispatching function, this function dispatches a linked list of input events
    /// to other input event handlers, hence by modifying the linked list we can filter out unwanted input events.
    class OnInputEventDispatch {
    public:
        static void Install() {
            auto& trampoline = SKSE::GetTrampoline();
            REL::Relocation<uintptr_t> caller{RELOCATION_ID(67315, 68617)};
            _DispatchInputEvent = trampoline.write_call<5>(
                caller.address() + REL::VariantOffset(0x7B, 0x7B, 0).offset(), DispatchInputEvent);
        }

    private:
        static void DispatchInputEvent(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent** a_evns) {
            if (!a_evns) {
                _DispatchInputEvent(a_dispatcher, a_evns);
                return;
            }

            SpellHotbar::Input::processAndFilter(a_evns);

            _DispatchInputEvent(a_dispatcher, a_evns);
        }
        static inline REL::Relocation<decltype(DispatchInputEvent)> _DispatchInputEvent;
    };


}
namespace SpellHotbar::Input {

    void install_hook() { OnInputEventDispatch::Install(); }

    void processAndFilter(RE::InputEvent** a_event)
    {
        //based on wheeler
        if (!a_event) {
            return;
        }

        //don't react to inputs outside of the game:
        auto pc = RE::PlayerCharacter::GetSingleton();
        if (!pc || !pc->Is3DLoaded()) {
            return;  // no player, no draw
        }


        RE::InputEvent* event = *a_event;
        RE::InputEvent* prev = nullptr;

        RE::InputEvent* addEvent = nullptr;

        /*auto controlmap = RE::ControlMap::GetSingleton();
        uint32_t shoutkey = 0;
        if (controlmap) {
             shoutkey = controlmap->GetMappedKey("Shout", RE::INPUT_DEVICE::kKeyboard);
        }*/

        auto [shoutKeyDev, shoutKey] = get_shout_key_and_device();


        while (event != nullptr) {
            bool captureEvent = false; //Capure this event? (not forward to game)
            auto eventType = event->eventType;

            if (event->eventType == RE::INPUT_EVENT_TYPE::kMouseMove) {
                if (RenderManager::should_block_game_cursor_inputs()) {
                    //RE::MouseMoveEvent* mouseMove = static_cast<RE::MouseMoveEvent*>(event);
                    captureEvent = true;
                }
            }
            else if (event->eventType == RE::INPUT_EVENT_TYPE::kThumbstick) {
                if (RenderManager::should_block_game_cursor_inputs()) {
                    RE::ThumbstickEvent* thumbstick = static_cast<RE::ThumbstickEvent*>(event);
                    if (thumbstick->IsRight()) {
                        captureEvent = true;
                    }
                }
            }
            else if (event->eventType == RE::INPUT_EVENT_TYPE::kChar) {
                if (!captureEvent && RenderManager::should_block_game_key_inputs()) {
                    auto ch_event = event->AsCharEvent();
                    
                    auto& io = ImGui::GetIO();
                    io.AddInputCharacter(ch_event->keycode);

                }
            } else if (event->eventType == RE::INPUT_EVENT_TYPE::kButton) {
                RE::ButtonEvent* bEvent = event->AsButtonEvent();
                if (bEvent) {

                    auto& io = ImGui::GetIO();

                    //update imgui mods
                    if (bEvent->GetIDCode() == 29 || bEvent->GetIDCode() == 157) {
                        io.AddKeyEvent(ImGuiKey_ModCtrl, bEvent->IsPressed());
                    }
                    if (bEvent->GetIDCode() == 42 || bEvent->GetIDCode() == 54) {
                        io.AddKeyEvent(ImGuiKey_ModShift, bEvent->IsPressed());
                        //mod_shift.update(bEvent);
                    }
                    if (bEvent->GetIDCode() == 56 || bEvent->GetIDCode() == 184) {
                        io.AddKeyEvent(ImGuiKey_ModAlt, bEvent->IsPressed());
                        mod_alt.update(bEvent);
                    }
                    mod_1.update(bEvent);
                    mod_2.update(bEvent);
                    mod_3.update(bEvent);
                    mod_dual_cast.update(bEvent);
                    mod_show_bar.update(bEvent);

                    //update all keybind states
                    for (size_t i = 0; i < key_spells.size(); ++i) {
                        key_spells[i].update(bEvent);
                    }
                    key_oblivion_cast.update(bEvent);
                    key_oblivion_potion.update(bEvent);

                    if (bEvent->GetDevice() == RE::INPUT_DEVICE::kMouse) {
                        // credits open animation replacer
                        //forward the mouse inputs to ImGUI
                        if (bEvent->GetIDCode() > 7) {
                            io.AddMouseWheelEvent(0, bEvent->value * (bEvent->GetIDCode() == 8 ? 1 : -1));
                        } else {
                            if (bEvent->GetIDCode() > 5) bEvent->idCode = 5;
                            io.AddMouseButtonEvent(bEvent->idCode, bEvent->IsPressed());
                        }
                        if (RenderManager::should_block_game_cursor_inputs()) {
                            captureEvent = true;
                        }
                    }

                    if (!captureEvent && RenderManager::is_dragging_bar() && bEvent->GetDevice() == RE::INPUT_DEVICE::kKeyboard && bEvent->GetIDCode() == 1) {
                        RenderManager::stop_bar_dragging();
                        captureEvent = true;
                    }

                    //Block control inputs when a special frame is opened (SpellEditor)
                    if (!captureEvent && RenderManager::should_block_game_key_inputs()) {
                        if (bEvent->GetDevice() == RE::INPUT_DEVICE::kKeyboard || bEvent->GetDevice() == RE::INPUT_DEVICE::kGamepad) {
                            captureEvent = true;

                            if (bEvent->GetDevice() == RE::INPUT_DEVICE::kKeyboard && bEvent->GetIDCode() == 1 && bEvent->IsDown()) {
                                //Close Frames when ESC is pressed
                                RenderManager::close_key_blocking_frames();
                            }
                            else {
                                int dx_code = input_to_dx_scancode(bEvent->GetDevice(), static_cast<uint8_t>(bEvent->GetIDCode()));
                                if (dx_code >= 0 && dx_code < dx_to_imgui.size()) {
                                    ImGuiKey key = dx_to_imgui[dx_code];
                                    if (key != ImGuiKey_None) {
                                        io.AddKeyEvent(key, bEvent->IsPressed());
                                    }
                                }
                            }
                        }
                    }

                    if (!captureEvent && (bEvent->GetDevice() == RE::INPUT_DEVICE::kKeyboard || bEvent->GetDevice() == RE::INPUT_DEVICE::kGamepad || bEvent->GetDevice() == RE::INPUT_DEVICE::kMouse)) {

                        if (casts::CastingController::is_movement_blocking_cast() && in_ingame_state()) {
                            auto cm = RE::ControlMap::GetSingleton();
                            if (cm) {
                                RE::INPUT_DEVICE device = bEvent->GetDevice();
                                uint32_t key_forward = cm->GetMappedKey("Forward"sv, device);
                                uint32_t key_back= cm->GetMappedKey("Back"sv, device);
                                uint32_t key_left = cm->GetMappedKey("Strafe Left"sv, device);
                                uint32_t key_right = cm->GetMappedKey("Strafe Right"sv, device);

                                if (bEvent->GetIDCode() == key_forward || bEvent->GetIDCode() == key_back || bEvent->GetIDCode() == key_left || bEvent->GetIDCode() == key_right) {
                                    if (!bEvent->IsUp()) {
                                        captureEvent = true;
                                    }
                                }

                            }
                        }
                    }

                    if (!captureEvent) {
                        bool handled{ false };
                        if (!Bars::disable_non_modifier_bar || Input::mod_1.isDown() || Input::mod_2.isDown() || Input::mod_3.isDown()) {

                            for (size_t i = 0; i < key_spells.size() && !handled; ++i) {
                                const auto& bind = key_spells[i];

                                if (bind.matches(bEvent))
                                {
                                    if (in_binding_menu())
                                    {
                                        if (bEvent->IsDown()) {
                                            handled = true;
                                            RE::TESForm* form = get_current_selected_spell_in_menu();
                                            if (form) {
                                                slot_spell(form, i);
                                            }
                                        }
                                    }
                                    else if (in_ingame_state())
                                    {
                                        if (bEvent->IsDown()) {
                                            handled = true;
                                            auto skill = GameData::get_current_spell_info_in_slot(i);
                                            if (GameData::isVampireLord() &&
                                                GameData::global_vampire_lord_equip_mode && GameData::global_vampire_lord_equip_mode->value > 0.0f &&
                                                !Input::is_equip_mode())
                                            {
                                                //If Vampire Lord and using Not Equipmode -> use special VL mode (equip spells & cast powers) instead
                                                //The global can turn of this behaviour
                                                InputModeVampireLord::getSingleton()->process_input(skill, addEvent, i, bind, shoutKeyDev, shoutKey);
                                            }
                                            else if (InputModeBase::current_mode) {
                                                InputModeBase::current_mode->process_input(skill, addEvent, i, bind, shoutKeyDev, shoutKey);
                                            }
                                        }
                                        else if (bEvent->IsUp()) {
                                            handled = true;
                                            //check for release of power/shout key release event
                                            if (casts::CastingController::is_currently_using_power()) {

                                                float ct = casts::CastingController::get_current_casttime();
                                                if (!addEvent) {
                                                    addEvent = RE::ButtonEvent::Create(shoutKeyDev, "Shout", shoutKey, 0.0f, ct); //default shout key
                                                }
                                            }
                                        }
                                        else if (bEvent->IsRepeating()) {
                                            //Check in Oblivion Mode for holding slot key down to show bar.
                                            InputModeBase::current_mode->process_key_update(bind, i, bEvent->HeldDuration());
                                        }
                                        if (handled && (mod_1.isDown() || mod_2.isDown() || mod_3.isDown())) {
                                            //Do not forward keypress to game if modifier was used, this allows easy double binding with modifiers
                                            captureEvent = true;
                                        }
                                    }
                                }
                            }
                        }
                        if (!handled && Input::is_oblivion_mode() && in_ingame_state())
                        {
                            bool cast = key_oblivion_cast.matches(bEvent);
                            bool potion = key_oblivion_potion.matches(bEvent);
                            if (cast || potion)
                            {
                                if (bEvent->IsDown()) {
                                    size_t index = keybind_id::oblivion_potion;
                                    if (cast) {
                                        index = keybind_id::oblivion_cast;
                                    }
                                    handled = true;
                                    auto skill = GameData::get_current_spell_info_in_slot(index);

                                    if (InputModeBase::current_mode) {
                                        if (cast) {
                                            InputModeBase::current_mode->process_input(skill, addEvent, index, key_oblivion_cast, shoutKeyDev, shoutKey);
                                        }
                                        else if (potion) {
                                            InputModeBase::current_mode->process_input(skill, addEvent, index, key_oblivion_potion, shoutKeyDev, shoutKey);
                                        }
                                    }
                                }
                            }
                        }

                        if (!handled && in_binding_menu())
                        {
                            if (key_next.matches(bEvent) && bEvent->IsDown()) {
                                handled = true;
                                Bars::menu_bar_id = Bars::getNextMenuBar(Bars::menu_bar_id);
                                RE::PlaySound(sound_UISkillsForward);
                            }
                            else if (key_prev.matches(bEvent) && bEvent->IsDown()) {
                                handled = true;
                                Bars::menu_bar_id = Bars::getPreviousMenuBar(Bars::menu_bar_id);
                                RE::PlaySound(sound_UISkillsBackward);
                            }
                        }

                    }

                }
            }

            RE::InputEvent* nextEvent = event->next;
            if (captureEvent) {
                //remove event out of queue
                if (prev != nullptr) {
                    prev->next = nextEvent;
                } else {
                    *a_event = nextEvent;
                }
            } else {
                prev = event;
            }
            event = nextEvent;
        }
        if (addEvent) {
            if (prev) {
                prev->next = addEvent;
            }
            else {
                *a_event = addEvent;
            }
        }
    }

    KeyModifier::KeyModifier(RE::INPUT_DEVICE device, uint8_t code1, uint8_t code2)
        : input_device(device), keycode(code1), keycode2(code2), isDown1(false), isDown2(false)
    {}

    void KeyModifier::update(RE::ButtonEvent* bEvent) {
        if (bEvent->GetDevice() == input_device) {
            if (keycode > 0 && bEvent->idCode == keycode)
            {
                isDown1 = bEvent->IsPressed();
            }
            else if (keycode2 > 0 && bEvent->idCode == keycode2)
            {
                isDown2 = bEvent->IsPressed();
            }
        }
    }
    void KeyModifier::rebind(int dx_scancode)
    {
        auto [device, code] = dx_scan_code_to_input(dx_scancode);
        input_device = device;
        keycode = code;
        keycode2 = code;

        if (input_device == RE::INPUT_DEVICE::kKeyboard) {
            //ctrl
            if (keycode == 29 || keycode == 157) {
                keycode = 29;
                keycode2 = 157;
            }
            //shift
            else if (keycode == 42 || keycode == 54) {
                keycode = 42;
                keycode2 = 54;
            }
            //alt
            else if (keycode == 56 || keycode == 184) {
                keycode = 56;
                keycode2 = 184;
            }
        }
        isDown1 = false;
        isDown2 = false;
    }

    int KeyModifier::get_dx_scancode()
    {
        return input_to_dx_scancode(input_device, keycode);
    }

    int KeyModifier::get_dx_scancode2()
    {
        return input_to_dx_scancode(input_device, keycode2);
    }

    bool KeyModifier::isValidBound()
    {
        return input_device == RE::INPUT_DEVICE::kKeyboard || input_device == RE::INPUT_DEVICE::kMouse || input_device == RE::INPUT_DEVICE::kGamepad;
    }


    KeyBind::KeyBind(RE::INPUT_DEVICE device, uint8_t code) : input_device(device), keycode(code), m_isDown(false)
    {
    }

    bool KeyBind::matches(RE::ButtonEvent* bEvent) const
    {
        return bEvent->GetDevice() == input_device && bEvent->GetIDCode() == keycode;
    }

    int KeyBind::get_dx_scancode() const
    {
        return input_to_dx_scancode(input_device, keycode);
    }

    void KeyBind::assign_from_dx_scancode(int code)
    {
        auto [dev, key] = dx_scan_code_to_input(code);
        input_device = dev;
        keycode = key;
    }

    void KeyBind::update(RE::ButtonEvent* bEvent)
    {
        if (matches(bEvent)) {
            m_isDown = bEvent->IsPressed();
        }
    }

    void KeyBind::unbind()
    {
        input_device = RE::INPUT_DEVICE::kNone;
        keycode = 0Ui8;
        m_isDown = false;
    }

    bool in_ingame_state() {
        const auto ui = RE::UI::GetSingleton();
        
        if (!ui || ui->GameIsPaused() || !ui->IsCursorHiddenWhenTopmost() || !ui->IsShowingMenus() || !ui->GetMenu<RE::HUDMenu>() || ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME) || ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME))
        {
            return false;
        }
        else {
            return true;
        }
    }

    std::tuple<RE::INPUT_DEVICE, uint8_t> dx_scan_code_to_input(int dx_scancode)
    {
        RE::INPUT_DEVICE input_device{ RE::INPUT_DEVICE::kNone };
        uint8_t keycode{ 0Ui8 };
        if (dx_scancode < 0)
        {
            input_device = RE::INPUT_DEVICE::kNone;
            keycode = 0Ui8;
        }
        else if (dx_scancode < 256) {
            input_device = RE::INPUT_DEVICE::kKeyboard;
            keycode = static_cast<uint8_t>(dx_scancode);
        }
        else if (dx_scancode < 266) {
            input_device = RE::INPUT_DEVICE::kMouse;
            keycode = static_cast<uint8_t>(dx_scancode - 256);
        }
        else {
            input_device = RE::INPUT_DEVICE::kGamepad;
            keycode = static_cast<uint8_t>(dx_scancode - 266);
        }

        return std::make_tuple(input_device, keycode);
    }

    int input_to_dx_scancode(RE::INPUT_DEVICE device, uint8_t code)
    {
        if (device == RE::INPUT_DEVICE::kNone) {
            return -1;
        }

        int offset{ 0 };
        if (device == RE::INPUT_DEVICE::kMouse) {
            offset = 256;
        }
        else if (device == RE::INPUT_DEVICE::kGamepad) {
            offset = 266;
        }
        return static_cast<int>(code) + offset;
    }

    std::tuple<RE::INPUT_DEVICE, uint8_t> get_shout_key_and_device()
    {
        RE::INPUT_DEVICE dev{ RE::INPUT_DEVICE::kNone };

        auto controlmap = RE::ControlMap::GetSingleton();
        uint32_t shoutkey = 0U;
        if (controlmap) {
            shoutkey = controlmap->GetMappedKey("Shout", RE::INPUT_DEVICE::kKeyboard);
            if (shoutkey >= 255) {
                shoutkey = controlmap->GetMappedKey("Shout", RE::INPUT_DEVICE::kMouse);

                if (shoutkey >= 255) {
                    shoutkey = controlmap->GetMappedKey("Shout", RE::INPUT_DEVICE::kGamepad);

                    if (shoutkey >= 255) {
                        shoutkey = 0;
                    }
                    else {
                        dev = RE::INPUT_DEVICE::kGamepad;
                    }
                }
                else {
                    dev = RE::INPUT_DEVICE::kMouse;
                }
            }
            else {
                dev = RE::INPUT_DEVICE::kKeyboard;
            }
        }
        return std::make_tuple(dev, static_cast<uint8_t>(shoutkey));
    }

    int get_shout_key_dxcode()
    {
       auto [dev, code] = get_shout_key_and_device();
       return input_to_dx_scancode(dev, code);
    }

    bool allowed_to_instantcast(RE::FormID skill)
    {
        auto pc = RE::PlayerCharacter::GetSingleton();
        if (!pc || !pc->Is3DLoaded()) {
            return false;
        }

        if (GameData::is_skill_on_cd(skill)) {
            return false;
        }

        const auto* control_map = RE::ControlMap::GetSingleton();
        if (!control_map || !control_map->IsMovementControlsEnabled()) // || !control_map->IsFightingControlsEnabled()) this is not working in 1170 and probably not needed anyway
        {
            return false;
        }

        return !pc->IsOnMount();
    }

    bool allowed_to_cast(RE::FormID skill)
    {
        auto pc = RE::PlayerCharacter::GetSingleton();
        if (allowed_to_instantcast(skill) && pc) {
            auto as = pc->AsActorState();

            bool inJumpState{ false };
            //bool bowDrawn{ false };
            pc->GetGraphVariableBool("bInJumpState"sv, inJumpState);
            //pc->GetGraphVariableBool("bInJumpState"sv, bowDrawn); //TODO look for bow anim

            return !(as->IsSprinting() || as->IsSwimming() || inJumpState); //|| bowDrawn);
        }
        else return false;
    }

    RE::TESForm* get_current_selected_spell_in_menu()
    {
        RE::UI* ui = RE::UI::GetSingleton();
        if (!ui) return nullptr;
        if (!SpellHotbar::GameData::hasFavMenuSlotBinding()) {
            // totally stolen from Wheeler
            auto* magMenu = static_cast<RE::MagicMenu*>(ui->GetMenu(RE::MagicMenu::MENU_NAME).get());
            auto* invMenu = static_cast<RE::InventoryMenu*>(ui->GetMenu(RE::InventoryMenu::MENU_NAME).get());
            bool valid_tab = false;
            
            if (invMenu) {
                valid_tab = RenderManager::current_inv_menu_tab_valid_for_hotbar();
            };
            if (!magMenu && !valid_tab) return nullptr;

            RE::GFxValue selection;
            if (magMenu) {
                magMenu->uiMovie->GetVariable(&selection, "_root.Menu_mc.inventoryLists.itemList.selectedEntry.formId");
            }
            else if (invMenu) {
                invMenu->uiMovie->GetVariable(&selection, "_root.Menu_mc.inventoryLists.itemList.selectedEntry.formId");
            }
            if (selection.GetType() == RE::GFxValue::ValueType::kNumber) {
                RE::FormID formID = static_cast<std::uint32_t>(selection.GetNumber());
                return RE::TESForm::LookupByID(formID);
            }
        }
        else {
            auto* favMenu = static_cast<RE::FavoritesMenu*>(ui->GetMenu(RE::FavoritesMenu::MENU_NAME).get());
            if (!favMenu) return nullptr;

            auto& root = favMenu->GetRuntimeData().root;

            if (root.GetType() == RE::GFxValue::ValueType::kDisplayObject && root.HasMember("itemList")) {
                RE::GFxValue itemList;
                root.GetMember("itemList", &itemList);

                if (itemList.GetType() == RE::GFxValue::ValueType::kDisplayObject && itemList.HasMember("selectedEntry")) {
                    RE::GFxValue selectedEntry;
                    itemList.GetMember("selectedEntry", &selectedEntry);

                    if (selectedEntry.GetType() == RE::GFxValue::ValueType::kObject && selectedEntry.HasMember("formId")) {
                        RE::GFxValue formId;
                        selectedEntry.GetMember("formId", &formId);

                        if (formId.GetType() == RE::GFxValue::ValueType::kNumber) {
                            RE::FormID formID = static_cast<std::uint32_t>(formId.GetNumber());
                            return RE::TESForm::LookupByID(formID);
                        }
                    }
                }
            }
        }
        return nullptr;
    }

    bool slot_spell(RE::TESForm* form, size_t index)
    {
        if (form != nullptr)
        {
            SpellHotbar::Storage::menu_slot_type slot_type{ Storage::menu_slot_type::magic_menu };
            if (GameData::isVampireLord()) {
                slot_type = Storage::menu_slot_type::vampire_lord;
            }
            else if (GameData::isWerewolf()) {
                slot_type = Storage::menu_slot_type::werewolf;
            }
            else if (SpellHotbar::GameData::isCustomTransform()) {
                auto casttype = SpellHotbar::GameData::getCustomTransformCasttype();
                if (casttype == SpellHotbar::GameData::custom_transform_spell_type::fav_menu ||
                    casttype == SpellHotbar::GameData::custom_transform_spell_type::fav_menu_switch) {
                    slot_type = SpellHotbar::Storage::menu_slot_type::custom_favmenu;
                }
            }
            return SpellHotbar::Storage::slotSpell(form->GetFormID(), index, slot_type);
        }
        return false;
    }

    bool in_binding_menu()
    {
        auto ui = RE::UI::GetSingleton();
        if (!ui) {
            return false;
        }
        const auto* control_map = RE::ControlMap::GetSingleton();
        if (!control_map) {
        }
        if (control_map && (control_map->textEntryCount > 0))
        {
            return false;
        }

        if (GameData::hasFavMenuSlotBinding())
        {
            return ui->GetMenu(RE::FavoritesMenu::MENU_NAME).get() != nullptr;
        }
        else 
        {
            auto* magMenu = ui->GetMenu(RE::MagicMenu::MENU_NAME).get();
            if (magMenu) {
                return true;
            }
            else {
                return RenderManager::current_inv_menu_tab_valid_for_hotbar();
            }
        }
    }
}
