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

    inline constexpr std::tuple<uint32_t, RE::INPUT_DEVICE> get_device_and_input(const RE::ButtonEvent* bEvent) {
        uint32_t key = 0;
        RE::INPUT_DEVICE dev = RE::INPUT_DEVICE::kNone;
        if (bEvent->GetDevice() == RE::INPUT_DEVICE::kKeyboard) {
            key = bEvent->GetIDCode();
            dev = RE::INPUT_DEVICE::kKeyboard;
        }
        else if (bEvent->GetDevice() == RE::INPUT_DEVICE::kMouse) {
            key = bEvent->GetIDCode();
            dev = RE::INPUT_DEVICE::kMouse;
        }
        else if (bEvent->GetDevice() == RE::INPUT_DEVICE::kGamepad) {
            dev = RE::INPUT_DEVICE::kGamepad;
            //Map Gamepad keys to 0-x
            switch (bEvent->GetIDCode()) {
            case RE::BSWin32GamepadDevice::Keys::Key::kUp:
                key = 0;
                break;
            case RE::BSWin32GamepadDevice::Keys::Key::kDown:
                key = 1;
                break;
            case RE::BSWin32GamepadDevice::Keys::Key::kLeft:
                key = 2;
                break;
            case RE::BSWin32GamepadDevice::Keys::Key::kRight:
                key = 3;
                break;
            case RE::BSWin32GamepadDevice::Keys::Key::kStart:
                key = 4;
                break;
            case RE::BSWin32GamepadDevice::Keys::Key::kBack:
                key = 5;
                break;
            case RE::BSWin32GamepadDevice::Keys::Key::kLeftThumb:
                key = 6;
                break;
            case RE::BSWin32GamepadDevice::Keys::Key::kRightThumb:
                key = 7;
                break;
            case RE::BSWin32GamepadDevice::Keys::Key::kLeftShoulder:
                key = 8;
                break;
            case RE::BSWin32GamepadDevice::Keys::Key::kRightShoulder:
                key = 9;
                break;
            case RE::BSWin32GamepadDevice::Keys::Key::kA:
                key = 10;
                break;
            case RE::BSWin32GamepadDevice::Keys::Key::kB:
                key = 11;
                break;
            case RE::BSWin32GamepadDevice::Keys::Key::kX:
                key = 12;
                break;
            case RE::BSWin32GamepadDevice::Keys::Key::kY:
                key = 13;
                break;
            case RE::BSWin32GamepadDevice::Keys::Key::kLeftTrigger:
                key = 14;
                break;
            case RE::BSWin32GamepadDevice::Keys::Key::kRightTrigger:
                key = 15;
                break;
            default:
                //Invalid key
                key = 0;
                dev = RE::INPUT_DEVICE::kNone;
            }
        }

        return std::make_tuple(key, dev);
    }

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

                    auto [key_code, key_device] = get_device_and_input(bEvent);
                    bool is_pressed = bEvent->IsPressed();

                    if (key_device == RE::INPUT_DEVICE::kKeyboard) {
                        //update imgui mods
                        if (key_code == 29 || key_code == 157) {
                            io.AddKeyEvent(ImGuiKey_ModCtrl, is_pressed);
                        }
                        if (key_code == 42 || key_code == 54) {
                            io.AddKeyEvent(ImGuiKey_ModShift, is_pressed);
                            //mod_shift.update(bEvent);
                        }
                        if (key_code == 56 || key_code == 184) {
                            io.AddKeyEvent(ImGuiKey_ModAlt, is_pressed);
                            mod_alt.update(key_code, key_device, is_pressed);
                        }
                    }
                    mod_1.update(key_code, key_device, is_pressed);
                    mod_2.update(key_code, key_device, is_pressed);
                    mod_3.update(key_code, key_device, is_pressed);
                    mod_dual_cast.update(key_code, key_device, is_pressed);
                    mod_show_bar.update(key_code, key_device, is_pressed);

                    //update all keybind states
                    for (size_t i = 0; i < key_spells.size(); ++i) {
                        key_spells[i].update(key_code, key_device, is_pressed);
                    }
                    key_oblivion_cast.update(key_code, key_device, is_pressed);
                    key_oblivion_potion.update(key_code, key_device, is_pressed);

                    if (key_device == RE::INPUT_DEVICE::kMouse) {
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

                    if (!captureEvent && RenderManager::is_dragging_bar() && key_device == RE::INPUT_DEVICE::kKeyboard && key_code == 1) {
                        RenderManager::stop_bar_dragging();
                        captureEvent = true;
                    }

                    //Block control inputs when a special frame is opened (SpellEditor)
                    if (!captureEvent && RenderManager::should_block_game_key_inputs()) {
                        if (key_device == RE::INPUT_DEVICE::kKeyboard || key_device == RE::INPUT_DEVICE::kGamepad) {
                            captureEvent = true;

                            if (key_device == RE::INPUT_DEVICE::kKeyboard && key_code == 1 && bEvent->IsDown()) {
                                //Close Frames when ESC is pressed
                                RenderManager::close_key_blocking_frames();
                            }
                            else {
                                int dx_code = input_to_dx_scancode(key_device, static_cast<uint8_t>(key_code));
                                if (dx_code >= 0 && dx_code < dx_to_imgui.size()) {
                                    ImGuiKey key = dx_to_imgui[dx_code];
                                    if (key != ImGuiKey_None) {
                                        io.AddKeyEvent(key, is_pressed);
                                    }
                                }
                            }
                        }
                    }

                    if (!captureEvent && (key_device == RE::INPUT_DEVICE::kKeyboard || key_device == RE::INPUT_DEVICE::kGamepad || key_device == RE::INPUT_DEVICE::kMouse)) {

                        if (casts::CastingController::is_movement_blocking_cast() && in_ingame_state()) {
                            auto cm = RE::ControlMap::GetSingleton();
                            if (cm) {
                                uint32_t key_forward = cm->GetMappedKey("Forward"sv, key_device);
                                uint32_t key_back= cm->GetMappedKey("Back"sv, key_device);
                                uint32_t key_left = cm->GetMappedKey("Strafe Left"sv, key_device);
                                uint32_t key_right = cm->GetMappedKey("Strafe Right"sv, key_device);

                                if (key_code == key_forward || key_code == key_back || key_code == key_left || key_code == key_right) {
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

                                if (bind.matches(key_code, key_device))
                                {
                                    if (in_binding_menu())
                                    {
                                        if (bEvent->IsDown()) {
                                            handled = true;
                                            RE::TESForm* form = get_current_selected_spell_in_menu();
                                            if (form) {
                                                slot_spell(form, i);
                                            }
                                            if (mod_1.isDown() || mod_2.isDown() || mod_3.isDown()) {
                                                //Do not forward keypress to game if modifier was used, this allows easy double binding with modifiers
                                                captureEvent = true;
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
                            bool cast = key_oblivion_cast.matches(key_code, key_device);
                            bool potion = key_oblivion_potion.matches(key_code, key_device);
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
                            if (key_next.matches(key_code, key_device) && bEvent->IsDown()) {
                                handled = true;
                                Bars::menu_bar_id = Bars::getNextMenuBar(Bars::menu_bar_id);
                                RE::PlaySound(sound_UISkillsForward);
                            }
                            else if (key_prev.matches(key_code, key_device) && bEvent->IsDown()) {
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

    void KeyModifier::update(uint32_t key_code, RE::INPUT_DEVICE key_device, bool is_pressed)
    {
        if (key_device == input_device) {
            if (keycode > 0 && key_code == keycode)
            {
                isDown1 = is_pressed;
            }
            else if (keycode2 > 0 && key_code == keycode2)
            {
                isDown2 = is_pressed;
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

    bool KeyBind::matches(uint32_t key_code, RE::INPUT_DEVICE key_device) const
    {
        return key_device == input_device && key_code == keycode;
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

    void KeyBind::update(uint32_t key_code, RE::INPUT_DEVICE key_device, bool is_pressed)
    {
        if (matches(key_code, key_device)) {
            m_isDown = is_pressed;
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

        if (pc->GetOccupiedFurniture()) {
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

            //Check if player currently is casting, also check staffs
            bool isCasting = pc->IsCasting(nullptr);

            return !(isCasting || as->IsSprinting() || as->IsSwimming() || inJumpState); //|| bowDrawn);
        }
        else return false;
    }

    RE::TESForm* get_current_selected_spell_in_menu()
    {
        RE::UI* ui = RE::UI::GetSingleton();
        if (!ui) return nullptr;
        if (!SpellHotbar::GameData::hasFavMenuSlotBinding()) {
            // code taken from Wheeler
            auto* magMenu = static_cast<RE::MagicMenu*>(ui->GetMenu(RE::MagicMenu::MENU_NAME).get());
            auto* invMenu = static_cast<RE::InventoryMenu*>(ui->GetMenu(RE::InventoryMenu::MENU_NAME).get());
            bool valid_tab = false;
            
            if (invMenu) {
                valid_tab = RenderManager::current_inv_menu_tab_valid_for_hotbar();
            };
            if (!magMenu && !valid_tab) return nullptr;

            if (magMenu) {
                RE::GFxValue selection;
                magMenu->uiMovie->GetVariable(&selection, "_root.Menu_mc.inventoryLists.itemList.selectedEntry.formId");
                if (selection.GetType() == RE::GFxValue::ValueType::kNumber) {
                    RE::FormID formID = static_cast<std::uint32_t>(selection.GetNumber());
                    return RE::TESForm::LookupByID(formID);
                }
            }
            else if (invMenu) {
                //invMenu->uiMovie->GetVariable(&selection, "_root.Menu_mc.inventoryLists.itemList.selectedEntry.formId");
                RE::ItemList* item_list = invMenu->GetRuntimeData().itemList;
                if (item_list != nullptr) {
                    RE::ItemList::Item* item = item_list->GetSelectedItem();
                    if (item != nullptr && item->data.objDesc != nullptr) {
#undef GetObject // undefine stupid windows definition so GetObject() can be called
                        RE::TESBoundObject* obj = item->data.objDesc->GetObject();
#ifdef UNICODE //redefine it
#define GetObject  GetObjectW
#else
#define GetObject  GetObjectA
#endif // !UNICODE
                        if (obj != nullptr) {
                            RE::FormID formID = obj->GetFormID();
                            return RE::TESForm::LookupByID(formID);
                        }
                    }
                }
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
