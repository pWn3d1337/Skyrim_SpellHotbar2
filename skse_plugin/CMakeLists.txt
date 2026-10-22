cmake_minimum_required(VERSION 3.21)

project(SpellHotbar2 VERSION 2.0.10 LANGUAGES CXX)

#output folder to copy dll after building, e.g. "F:/Skyrim Dev/ADT/mods/Spell Hotbar 2".
#Empty by default so a plain configure never deploys anywhere; pass -DOUTPUT_FOLDER=... to enable.
set(OUTPUT_FOLDER "" CACHE PATH "Optional mod folder that receives the built SKSE plugin")


# FLICK's guest header, vendored from github.com/Fuzzlesz/FUCK_API. It is still moving upstream --
# API 7, FLICK 1.7 -- so track a commit, not HEAD, and let FUCK::Connect()'s own
# version check fail closed against an older installed host.
set(FLICK_API_CANONICAL_COMMIT "bc5280921bee6d8585fe18e14302a8bd9b074692")
set(FLICK_API_CANONICAL_SHA256 "f5378cca0b86de99c3d466bdda7d9dc3902d3c3cd911540fa75e89361d01b88e")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/third_party/flick/FUCK_API.h" FLICK_API_HEADER)
# Normalize line endings before hashing, written as CMake escapes rather than literal newlines
# so no editor or checkout can rewrite them.
string(REPLACE "\r\n" "\n" FLICK_API_HEADER "${FLICK_API_HEADER}")
string(REPLACE "\r" "\n" FLICK_API_HEADER "${FLICK_API_HEADER}")
string(SHA256 FLICK_API_SHA256 "${FLICK_API_HEADER}")
if(NOT FLICK_API_SHA256 STREQUAL FLICK_API_CANONICAL_SHA256)
    message(FATAL_ERROR
        "third_party/flick/FUCK_API.h differs from the pinned FLICK header at "
        "${FLICK_API_CANONICAL_COMMIT}. Refresh it from upstream and update the pinned hash.")
endif()


find_package(imgui CONFIG REQUIRED)
find_package(CommonLibSSE CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)

set(source_files src/plugin.cpp
                 src/logger/logger.h
                 src/logger/logger.cpp
                 src/rendering/render_manager.h
                 src/rendering/render_manager.cpp
                 src/rendering/bind_drop.h
                 src/rendering/ui_bridge.h
                 src/mcp/bind_capture.h
                 src/mcp/bind_capture.cpp
                 src/mcp/mcp_preset_name.h
                 src/lifecycle/first_run_policy.h
                 src/lifecycle/lifecycle.h
                 src/lifecycle/lifecycle.cpp
                 src/papyrus_extensions/papyrus_functions.h
                 src/papyrus_extensions/papyrus_functions.cpp
                 src/storage/storage.h
                 src/storage/storage.cpp
                 src/game_data/game_data.h
                 src/game_data/game_data.cpp
                 src/bar/hotbar.h
                 src/bar/hotbar.cpp
                 src/bar/hotbars.h
                 src/bar/hud_geometry.h
                 src/bar/hotbars.cpp
                 src/bar/oblivion_bar.h
                 src/bar/oblivion_bar.cpp
                 "src/input/input.h"
                 "src/input/input.cpp"
                 "src/input/input_hook.h"
                 "src/input/input_hook.cpp"
                 "src/input/keybinds.h"
                 "src/input/keybinds.cpp"
                 "src/input/modes.h"
                 "src/input/modes.cpp"
                 src/rendering/texture_csv_loader.h
                 src/rendering/texture_csv_loader.cpp
                 src/events/eventlistener.h
                 src/events/eventlistener.cpp
                 src/events/animationeventhook.h
                 src/events/animationeventhook.cpp
                 src/events/gameloop_hook.h
                 src/events/gameloop_hook.cpp
                 src/game_data/spell_data_csv_loader.h
                 src/game_data/spell_data_csv_loader.cpp
                 src/game_data/csv_loader.h
                 src/game_data/custom_transform_csv_loader.h
                 src/game_data/custom_transform_csv_loader.cpp
                 src/casts/cast_behavior_anims.h
                 src/casts/cast_art.h
                 src/casts/cast_art.cpp
                 src/casts/casting_controller.h
                 src/casts/casting_controller.cpp
                 src/game_data/keynames_csv_loader.h
                 src/game_data/keynames_csv_loader.cpp
                 "src/input/keycode_helper.h"
                 src/game_data/user_custom_spelldata.h
                 src/game_data/user_custom_spelldata.cpp
                 src/storage/user_data_io.h
                 src/storage/user_data_io.cpp
                 src/game_data/spell_cast_data.h
                 src/game_data/spell_cast_data.cpp
                 src/game_data/user_custom_entry.h
                 src/game_data/user_custom_entry.cpp
                 src/casts/spell_proc.h
                 src/casts/spell_proc.cpp
                 src/game_data/localization.h
                 src/game_data/localization.cpp
)

add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${source_files})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)

target_include_directories(${PROJECT_NAME} PRIVATE "src")

# imgui for its headers only: ImVec2, ImU32 and IM_COL32 in the texture registry and the display
# lists. FUCK.dll does every draw call. The plugin still links the static copy for those symbols;
# its context is never created, which is why no ImGui:: call may appear anywhere in src (below).
target_link_libraries(${PROJECT_NAME} PRIVATE imgui::imgui)

# The FLICK side is compiled apart from the rest of the plugin, without the shared PCH. These
# TUs are the only ones that include FUCK_API.h and the only ones that draw, and they reach the
# plugin's state through headers that carry no drawing at all (rendering/ui_bridge.h,
# flick/flick_watch.h, flick/flick_windows.h).
add_library(sh2_flick OBJECT
    src/flick/flick_pch.h
    src/flick/flick_watch.h
    src/flick/flick_watch.cpp
    src/flick/flick_images.h
    src/flick/flick_images.cpp
    src/flick/flick_windows.h
    src/flick/flick_windows.cpp
    src/flick/flick_hud.cpp
    src/flick/ui/widgets.h
    src/flick/ui/widgets.cpp
    src/flick/ui/bind_menu.h
    src/flick/ui/bind_menu.cpp
    src/flick/ui/spell_editor.h
    src/flick/ui/spell_edit_dialog.h
    src/flick/ui/spell_editor.cpp
    src/flick/ui/spell_edit_dialog.cpp
    src/flick/ui/potion_editor.h
    src/flick/ui/icon_edit_dialog.h
    src/flick/ui/potion_editor.cpp
    src/flick/ui/bar_drag.h
    src/flick/ui/bar_drag.cpp
    src/flick/ui/config_tool.h
    src/flick/ui/config_tool.cpp
)
target_precompile_headers(sh2_flick PRIVATE src/flick/flick_pch.h)
target_compile_features(sh2_flick PRIVATE cxx_std_23)
target_compile_definitions(sh2_flick PRIVATE IMGUI_DEFINE_MATH_OPERATORS)
target_link_libraries(sh2_flick PRIVATE CommonLibSSE::CommonLibSSE imgui::imgui)
target_include_directories(sh2_flick PRIVATE "src")
target_link_libraries(${PROJECT_NAME} PRIVATE sh2_flick)

# Mechanical form of the rule flick_pch.h states: nothing in the plugin may call ImGui::
# directly. Inside a FUCK::IWindow::Draw() SH2's own GImGui is uninitialised, so a stray call
# compiles, links, and crashes the game -- and there is no other ImGui context in the plugin's
# life at all. The real imgui.h is on every include path for its types, so the compiler cannot
# catch this; this does.
file(GLOB_RECURSE SH2_ALL_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
foreach(sh2_source IN LISTS SH2_ALL_SOURCES)
    # Lines that open with a comment marker are prose about the rule, not a breach of it.
    file(STRINGS "${sh2_source}" sh2_imgui_calls REGEX "^[^/]*ImGui::")
    if(sh2_imgui_calls)
        message(FATAL_ERROR "${sh2_source} calls ImGui:: directly; the plugin draws through FUCK:: only:
${sh2_imgui_calls}")
    endif()
endforeach()

#automatically copy to output folder if set
if(OUTPUT_FOLDER)
    set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")

    message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        VERBATIM
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_custom_command(
            TARGET "${PROJECT_NAME}"
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
            VERBATIM
        )
    endif()
endif()
