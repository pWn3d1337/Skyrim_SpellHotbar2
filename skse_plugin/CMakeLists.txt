cmake_minimum_required(VERSION 3.21)

project(SpellHotbar2 VERSION 2.0.1 LANGUAGES CXX)

#output folder to copy dll after building
set(OUTPUT_FOLDER "F:/Skyrim Dev/ADT/mods/Spell Hotbar 2")
#set(OUTPUT_FOLDER "F:/Skyrim Dev/DEV_1_5_97/mods/Spell Hotbar 2")
#set(OUTPUT_FOLDER "F:/Skyrim Dev/DEV_1_6_1170/mods/Spell Hotbar 2")


find_package(imgui CONFIG REQUIRED)
find_package(CommonLibSSE CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(directxtk CONFIG REQUIRED)

set(source_files src/plugin.cpp
                 src/logger/logger.h
                 src/logger/logger.cpp
                 src/rendering/render_manager.h
                 src/rendering/render_manager.cpp
                 src/rendering/texture_loader.h
                 src/rendering/texture_loader.cpp
                 src/papyrus_extensions/papyrus_functions.h
                 src/papyrus_extensions/papyrus_functions.cpp
                 src/storage/storage.h
                 src/storage/storage.cpp
                 src/game_data/game_data.h
                 src/game_data/game_data.cpp
                 src/bar/hotbar.h
                 src/bar/hotbar.cpp
                 src/bar/hotbars.h
                 src/bar/hotbars.cpp
                 src/bar/oblivion_bar.h
                 src/bar/oblivion_bar.cpp
                 "src/input/input.h"
                 "src/input/input.cpp"
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
                 src/eventS/gameloop_hook.h
                 src/eventS/gameloop_hook.cpp
                 src/game_data/spell_data_csv_loader.h
                 src/game_data/spell_data_csv_loader.cpp
                 src/game_data/spell_casteffect_csv_loader.h
                 src/game_data/spell_casteffect_csv_loader.cpp
                 src/game_data/csv_loader.h
                 src/game_data/custom_transform_csv_loader.h
                 src/game_data/custom_transform_csv_loader.cpp
                 src/game_data/animation_data_csv_loader.h
                 src/game_data/animation_data_csv_loader.cpp
                 src/casts/casting_controller.h
                 src/casts/casting_controller.cpp
                 src/game_data/keynames_csv_loader.h
                 src/game_data/keynames_csv_loader.cpp
                 src/rendering/spell_editor.h
                 src/rendering/spell_editor.cpp
                 "src/input/keycode_helper.h"
                 src/rendering/spell_edit_dialog.h
                 src/rendering/spell_edit_dialog.cpp
                 src/game_data/user_custom_spelldata.h
                 src/game_data/user_custom_spelldata.cpp
                 src/storage/user_data_io.h
                 src/storage/user_data_io.cpp
                 src/game_data/spell_cast_data.h
                 src/game_data/spell_cast_data.cpp
                 src/rendering/potion_editor.h
                 src/rendering/potion_editor.cpp
                 src/game_data/user_custom_entry.h
                 src/game_data/user_custom_entry.cpp
                 src/rendering/icon_edit_dialog.h
                 src/rendering/icon_edit_dialog.cpp
                 src/casts/spell_proc.h
                 src/casts/spell_proc.cpp
                 src/rendering/bar_dragging_config_window.h
                 src/rendering/bar_dragging_config_window.cpp
                 src/rendering/advanced_bind_menu.h
                 src/rendering/advanced_bind_menu.cpp
                 src/rendering/gui_tab_button.h
                 src/rendering/gui_tab_button.cpp
)

add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${source_files})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)

target_include_directories(${PROJECT_NAME} PRIVATE "src")

target_link_libraries(${PROJECT_NAME} PRIVATE imgui::imgui)
target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::DirectXTK)

#automatically copy to output folder if set
if(DEFINED OUTPUT_FOLDER)
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
