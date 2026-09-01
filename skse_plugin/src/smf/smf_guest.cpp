#include "smf_guest.h"

#include "../input/input.h"
#include "../logger/logger.h"
#include "../mcp/mcp_pages.h"
#include "../rendering/render_manager.h"

#include <array>

namespace SpellHotbar::SmfGuest {
    namespace {
        using WindowInterface = SKSEMenuFramework::Model::WindowInterface;

        SKSEMenuFramework::Model::HudElement* hud_registration{nullptr};
        SKSEMenuFramework::Model::InputEvent* input_registration{nullptr};
        std::array<WindowInterface*, static_cast<std::size_t>(Window::count)> windows{};
        bool ready{false};
        bool attempted{false};

        constexpr std::size_t index(Window window)
        {
            return static_cast<std::size_t>(window);
        }

        void __stdcall render_hud()
        {
            RenderManager::render_hud();
        }

        void __stdcall render_spell_editor()
        {
            RenderManager::render_spell_editor_window();
        }

        void __stdcall render_potion_editor()
        {
            RenderManager::render_potion_editor_window();
        }

        void __stdcall render_bar_dragging()
        {
            RenderManager::render_bar_drag_window();
        }

        void __stdcall render_bind_menu()
        {
            RenderManager::render_bind_menu_window();
        }

        void close_host_menu()
        {
            if (auto* main = SKSEMenuFramework::GetMainWindow()) {
                if (main->IsOpen.load()) {
                    logger::info("Closing the SKSE Menu Framework control panel so a Spell Hotbar 2 window can receive input");
                }
                main->IsOpen.store(false);
            }
        }

        bool has_required_exports(HMODULE module)
        {
            if (!GetProcAddress(module, "RegisterHudElement")) {
                logger::error("SKSE Menu Framework is missing required export 'RegisterHudElement'");
                return false;
            }
            if (!GetProcAddress(module, "AddWindow")) {
                logger::error("SKSE Menu Framework is missing required export 'AddWindow'");
                return false;
            }
            if (!GetProcAddress(module, "RegisterInpoutEvent")) {
                logger::error("SKSE Menu Framework is missing required export 'RegisterInpoutEvent'");
                return false;
            }
            if (!GetProcAddress(module, "AddSectionItem")) {
                logger::error("SKSE Menu Framework is missing required export 'AddSectionItem'");
                return false;
            }
            if (!GetProcAddress(module, "GetMainWindow")) {
                logger::error("SKSE Menu Framework is missing required export 'GetMainWindow'");
                return false;
            }

            // Player-facing minimum is SMF 3.14 (Nexus 120352; this machine's
            // SKSE plugin version is 3.14.0 and the public MAIN file is 3.14.1).
            // GetMenuFrameworkVersion() is stale at 3.7f in the pinned host and
            // is logged only; required exports are the fail-closed gate.
            constexpr float k_required_smf_release = 3.14F;
            const auto get_version = reinterpret_cast<float (*)()>(
                GetProcAddress(module, "GetMenuFrameworkVersion"));
            if (get_version) {
                logger::info(
                    "SKSE Menu Framework GetMenuFrameworkVersion() reported {}; player requirement is {}+ and is not gated on this export",
                    get_version(),
                    k_required_smf_release);
            }

            constexpr std::array required_exports{
                "LoadTexture",
                "DisposeTexture",
                "igGetIO",
                "igGetStyle",
                "igGetMainViewport",
                "igGetWindowDrawList",
                "igGetForegroundDrawList_Nil",
                "igBegin",
                "igEnd",
                "igBeginChild_Str",
                "igEndChild",
                "igImage",
                "ImDrawList_AddImage",
                "ImDrawList_AddText_Vec2",
                "ImDrawList_AddText_FontPtr",
                "ImGuiListClipper_ImGuiListClipper",
                "ImGuiListClipper_destroy",
                "ImGuiListClipper_Begin",
                "ImGuiListClipper_Step",
                "igSeparatorText",
                "igButton",
                "igSmallButton",
                "igInvisibleButton",
                "igSameLine",
                "igNewLine",
                "igDummy",
                "igTextV",
                "igTextUnformatted",
                "igTextColoredV",
                "igPushID_Int",
                "igPushID_Str",
                "igPopID",
                "igCheckbox",
                "igRadioButton_IntPtr",
                "igCombo_Str_arr",
                "igBeginCombo",
                "igEndCombo",
                "igSelectable_Bool",
                "igSliderFloat",
                "igSliderInt",
                "igInputText",
                "igInputTextWithHint",
                "igInputScalar",
                "igBeginDisabled",
                "igEndDisabled",
                "igBeginPopupModal",
                "igEndPopup",
                "igOpenPopup_Str",
                "igCloseCurrentPopup",
                "igSeparator",
                "igBeginTable",
                "igEndTable",
                "igTableSetupColumn",
                "igTableSetupScrollFreeze",
                "igTableHeadersRow",
                "igTableNextRow",
                "igTableNextColumn",
                "igTableGetSortSpecs",
                "igBeginDragDropSource",
                "igEndDragDropSource",
                "igSetDragDropPayload",
                "igBeginDragDropTarget",
                "igEndDragDropTarget",
                "igAcceptDragDropPayload",
                "igGetDragDropPayload",
                "igCollapsingHeader_TreeNodeFlags",
                "igIsItemHovered",
                "igBeginItemTooltip",
                "igEndTooltip",
                "igSetItemDefaultFocus",
                "igSetItemKeyOwner",
                "igPushFont",
                "igPopFont",
                "igPushItemWidth",
                "igPopItemWidth",
                "igPushStyleColor_Vec4",
                "igPopStyleColor",
                "igPushStyleVar_Float",
                "igPushStyleVar_Vec2",
                "igPopStyleVar",
                "igPushTextWrapPos",
                "igPopTextWrapPos",
                "igSetNextWindowSize",
                "igSetNextWindowPos",
                "igSetNextWindowBgAlpha",
                "igGetCursorScreenPos",
                "igSetCursorScreenPos",
                "igGetCursorPos",
                "igSetCursorPosX",
                "igGetContentRegionAvail",
                "igGetWindowPos",
                "igGetWindowSize",
                "igGetWindowWidth",
                "igGetWindowHeight",
                "igGetWindowContentRegionMax",
                "igGetItemRectMax",
                "igGetMousePos",
                "igGetFontSize",
                "igCalcTextSize"};

            for (const auto* export_name : required_exports) {
                if (!GetProcAddress(module, export_name)) {
                    logger::error("SKSE Menu Framework is missing required export '{}'", export_name);
                    return false;
                }
            }
            return true;
        }
    }

    bool install()
    {
        if (attempted) {
            return ready;
        }
        attempted = true;

        const auto module = GetMenuFrameworkModule();
        if (!module) {
            logger::error("SKSE Menu Framework is required; Spell Hotbar 2 UI will remain disabled");
            return false;
        }
        if (!has_required_exports(module)) {
            logger::error("SKSE Menu Framework API is incompatible; Spell Hotbar 2 UI will remain disabled");
            return false;
        }

        hud_registration = SKSEMenuFramework::AddHudElement(render_hud);
        input_registration = SKSEMenuFramework::AddInputEvent(Input::process_event);
        windows[index(Window::spell_editor)] = SKSEMenuFramework::AddWindow(render_spell_editor);
        windows[index(Window::potion_editor)] = SKSEMenuFramework::AddWindow(render_potion_editor);
        windows[index(Window::bar_dragging)] = SKSEMenuFramework::AddWindow(render_bar_dragging);
        windows[index(Window::bind_menu)] = SKSEMenuFramework::AddWindow(render_bind_menu);

        ready = hud_registration != nullptr && input_registration != nullptr;
        for (auto* window : windows) {
            ready = ready && window != nullptr;
            if (window) {
                window->IsOpen.store(false);
                window->BlockUserInput.store(true);
            }
        }

        if (!ready) {
            close_all_windows();
            logger::error("SKSE Menu Framework rejected one or more Spell Hotbar 2 registrations; UI will remain disabled");
            return false;
        }

        Mcp::register_pages();
        logger::info("Spell Hotbar 2 registered one HUD element, one input callback, four blocking windows, and eight MCP pages with SKSE Menu Framework");
        return true;
    }

    bool is_ready()
    {
        return ready;
    }

    bool open_window(Window window)
    {
        if (!ready || window == Window::count) {
            return false;
        }
        close_all_windows();
        close_host_menu();
        windows[index(window)]->IsOpen.store(true);
        return true;
    }

    void close_window(Window window)
    {
        if (window != Window::count) {
            if (auto* interface = windows[index(window)]) {
                interface->IsOpen.store(false);
            }
        }
    }

    void close_all_windows()
    {
        for (auto* window : windows) {
            if (window) {
                window->IsOpen.store(false);
            }
        }
    }

    bool is_window_open(Window window)
    {
        return window != Window::count && windows[index(window)] && windows[index(window)]->IsOpen.load();
    }

    bool is_any_window_open()
    {
        return std::ranges::any_of(windows, [](const auto* window) {
            return window && window->IsOpen.load();
        });
    }
}
