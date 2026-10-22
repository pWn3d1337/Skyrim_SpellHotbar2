# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

SpellHotbar 2 — a Skyrim SE/AE mod (rework of SpellHotbar v1). An SKSE plugin (C++23, CommonLibSSE-NG) does all casting, input, rendering (Dear ImGui on DX11) and co-save storage; Papyrus scripts are only a thin MCM/init layer. Tested on 1.5.97, 1.6.640 and 1.6.1070. Runtime requirements: SKSE, Address Library, FLICK 1.7.0 or newer (API v7 header vendored in `skse_plugin/third_party/flick`, pinned by commit and hash in `CMakeLists.txt`), SkyUI, a behavior engine run (Pandora or Nemesis, for the cast behavior patch); optional Inventory Interface Information Injector (I4), Custom Skill Framework, ImGui Icons.

The repo does **not** contain the full mod: `SpellHotbar.esp`, compiled `.pex`, meshes/animations, fonts and generated icon atlases live in the dev mod folder `F:\Skyrim Dev\ADT\mods\Spell Hotbar 2` (paths are hardcoded in CMake, the `.ppj`, and the Python scripts).

## Build

### SKSE plugin (`skse_plugin/`)
Requires MSVC (run from a VS x64 Developer prompt), Ninja, and `VCPKG_ROOT` set. Dependencies come from `vcpkg.json` in manifest mode; `commonlibsse-ng` is pinned via the local filesystem registry `CommonLibSSE-NG_fsreg/` (see `vcpkg-configuration.json`): port `12.1.0` is alandtse's fork (`alandtse/CommonLibSSE-NG`, tag v12.1.0, GPL-3.0-or-later), the older `0.1.x` ports are CharmedBaryon's MIT original. The port also fetches the pinned OpenVR headers / import lib and MinHook's hde64 (the upstream build expects a git submodule and FetchContent). To update: add a port folder with new `REF` / `SHA512`, an entry in `versions/` and a new baseline date. A changed port makes vcpkg rebuild the library (about 10 minutes per preset).

```
cd skse_plugin
cmake --preset release_with_debug_info      # also: debug, release
cmake --build build/release_with_debug_info
```

- Post-build copies `SpellHotbar2.dll` (+ `.pdb` for Debug/RelWithDebInfo) into `${OUTPUT_FOLDER}/SKSE/Plugins` — `OUTPUT_FOLDER` is hardcoded at the top of `CMakeLists.txt` (commented alternatives for 1.5.97 / 1.6.1170 dev setups).
- Sources are listed **explicitly** in `CMakeLists.txt`; new `.cpp/.h` files must be added there. (`rendering/texture_buttons_loader.*` exists but is not in the list, so it is not compiled.)
- The release packager expects the DLL at `skse_plugin/build/release_with_debug_info/`.
- Plugin version is `project(... VERSION ...)` in `CMakeLists.txt`.

There are no unit tests or linters; verification is done in-game. Logs go to `Documents/My Games/Skyrim Special Edition/SKSE/SpellHotbar2.log`.

### Papyrus (`papyrus/`)
`papyrus/skyrimse.ppj` is a Pyro project (e.g. via the VS Code Papyrus extension workspace `SkyrimSE.code-workspace`). It compiles `Scripts/Source/*.psc` straight into the dev mod's `Scripts` folder and imports SkyUI SDK, SKSE and Custom Skills Framework sources from local paths.

### Release packaging (`python_scripts/`)
- `create_fomod_installer.py` — current release builder. Generates `fomod/info.xml` + `ModuleConfig.xml` and zips everything into `build/Spell Hotbar 2 - <version>.zip`. Version string, spell packs (`_add_spell_pack`) and perk overhauls (`_add_perk_overhaul`) are set in `__main__`. Adding a new spell pack means adding it here as well as generating its assets.
- `build_release_package.py` — legacy v1 packager.

## Architecture

### SKSE plugin entry and lifecycle (`src/plugin.cpp`)
`SKSEPluginLoad` wires everything together:
- `Bars::init()` — creates hotbars.
- `kDataLoaded` → `GameData::onDataLoad()` loads all CSV/JSON data from `Data/SKSE/Plugins/SpellHotbar/` (`spelldata/`, `transformdata/`, `keynames/`, `localization/translation.txt`, `images/`, `perkdata/dual_cast_perks.csv`). Loaders iterate every CSV in their folder, which is how spell packs plug in without code changes. JSON presets (`presets/`, plus user presets under `Documents/My Games/Skyrim Special Edition/SpellHotbar/presets`) are handled by `storage/user_data_io`.
- `GameLoopHook` (`events/gameloop_hook.h`) — per-frame update driving the casting state machine (`casts/casting_controller`).
- `EventListener` — sinks for action events, spell cast, hit, equip and critical hit events (spell procs, power/shout completion, etc.).
- `RenderManager::install()` — D3D init + DXGI Present thunk hooks; all HUD bars, the Oblivion-mode bar, spell/potion editors, icon edit dialog, bar dragging window and the Advanced Bind Menu are ImGui draws from here.
- `Input::install_hook()` — hooks input dispatch so key presses are handled instantly (and swallowed while in the bind menu).
- Papyrus natives registered in `papyrus_extensions/papyrus_functions.cpp` under class `"SpellHotbar"`; names must match `native` declarations in `papyrus/Scripts/Source/SpellHotbar.psc` (the MCM `SpellHotbarMCM.psc` calls these).
- Co-save serialization (`storage/storage.cpp`) with unique ID `0xB8498471`.

Address-dependent hooks use `REL::RelocationID(SE, AE)` / `REL::VariantID(SE, AE, VR)` so one DLL supports SE and AE.

### Module map (`skse_plugin/src/`)
- `bar/` — `hotbars` (global bar settings + `std::unordered_map<uint32_t, Hotbar>` of all bars), `hotbar` (slots per bar), `oblivion_bar`.
- `casts/` — casting controller (cast timers, GCD/cooldowns, animations, dual cast, concentration, powers/shouts, potions), `cast_art` (casting art in the hands, see below) and `spell_proc` (Battlemage perk procs).
- `game_data/` — global game state/config (`GameData` namespace), CSV loaders (all derive from `csv_loader.h`, tab-separated, parsed with rapidcsv), user custom spell data (in-game editor overrides), localization.
- `input/` — keybinds, modifiers, input modes (Direct Cast / Equip / Oblivion).
- `rendering/` — texture atlas loading (`texture_csv_loader` reads `images/*.csv` UV tables alongside `.dds`), ImGui windows.
- `storage/` — co-save serialization and JSON preset import/export (`user_data_io`, RapidJSON).

### Save format versioning
Records: `HOTB` (bar settings/keybinds), `OBLB` (Oblivion bar), `GDAT`, `SDAT`, `UDAT`. `Storage::save_format` in `storage/storage.h` is the record version. When adding a serialized field: bump `save_format`, append the write at the end of the record in `SaveCallback`, and in `LoadCallback` read it inside `if (version >= N)` with a default in the `else` branch so older saves still load. Keep write/read order identical. `DEBUG_LOG_SERIALIZATION` enables verbose logging.

### Cast behavior (`python_scripts/build_cast_behavior.py`, `casts/casting_controller.cpp`)
Spell casts are animated by an own Havok behavior, not by the shout state. `build_cast_behavior.py` generates everything into the dev mod folder from the vanilla behaviors in the Nemesis cache (converted with hkxcmd, nothing vanilla is in the repo):
- `meshes/actors/character[/_1stperson]/behaviors/SpellHotbarCast.hkx` — charge / release state machine built from the node templates of the vanilla shout behavior, one entry per anim set (`anim_sets`, vanilla cast clips). It declares only its own events and variables.
- `Nemesis_Engine/mod/shbc/` — patch in Nemesis format (Pandora reads it too). `0_master` gets a blender below `RootModifierGenerator`: the vanilla graph as base (bone weight 0.001 on overlay bones, sync master) plus one overlay per weight variable (`overlay_weights`), each an own instance of the cast behavior with a bone weight array. The events are registered in **every** vanilla behavior file; a file that does not know an event gets it as invalid id and its state machines react (locomotion dropped to idle).
- `skse_plugin/src/casts/cast_behavior_anims.h` — generated enum, value of `SHB_iAnim`.

The plugin (`cast_overlay` namespace) sets `SHB_iAnim`, sends `SHB_CastStart/Release/Stop` and drives the weight variables: the arm (or upper body) overlay for the whole cast, a body overlay on top while the player stands still so the whole pose comes from the cast clip. `SHB_bCasting` is the graph's "overlay active" output and doubles as the check that the patch is installed. For animation replacers the plugin also sets `SHB_iCastSpell` (local form id of the spell as in its plugin, ESL aware), `SHB_iCastPlugin` (FNV-1a of the lower case plugin file name, folded to 24 bits so OAR's float compare is exact; the spell edit dialog shows both values) and `SHB_iCastHand` (1 left, 2 right, 3 both) for the whole hotbar spell cast, 0 otherwise; an OAR submod can OR `CompareValues` on these graph variables with its `IsEquipped…` check. The cast behavior only references vanilla clip names, so replacers work without patches. For scripts of other mods (compatibility patches) a hotbar spell cast sends the SKSE mod events `SpellHotbar_CastStart`, `SpellHotbar_CastRelease` and `SpellHotbar_CastStop` (sender: the spell, numArg: the hand, strArg of Stop: `finished` or `interrupted`; `mod_events` in `casting_controller.cpp`). Vanilla cast animation events are deliberately not faked: their listeners check the equipped spell. After changing the script: run it, then run Pandora / Nemesis. `hkxcmd` only reads 32 bit hkx, so the engine output can only be checked by scanning the binary.

### Casting art (`casts/cast_art.cpp`)
A hotbar cast shows the casting art the game uses for the spell: `castingArt` of its costliest magic effect, applied to the player with `ApplyArtObject` on the magic node of the casting hand (`NPC L MagicNode [LMag]` / `NPC R MagicNode [RMag]`, of the skeleton of the current point of view). The art meshes have an own behavior graph (`meshes/magic/magicbehaviors`, in `Skyrim - Animations.bsa`) that listens to the animation events of a regular cast; `cast_art` sets `bLeftHand` / `bRightHand` / `isCastingDual` on the effect and sends `MLh_SpellAimedStart`, `RitualSpellStart`, `DualMagic_Spell…`, `…ConcentrationStart` at the start and `MLh_/MRh_SpellRelease_Event` at the release, once the graph of the effect is loaded. `ApplyArtObject` does not return the effect (the CommonLib declaration is wrong, the value is not a pointer), it is looked up in `ProcessLists`. A spell without casting art shows nothing. The `Casteffect` column of the spell data belongs to the dummy effect system this replaced (art objects and meshes of it are removed).

### Asset pipeline (`python_scripts/`, `SWF_Generator/`)
Spell pack support is data-driven:
1. `spell_lists2/*.csv` and `spell_lists2/mods/<mod>.csv` (tab-separated: Name, FormID, Plugin, Casteffect (no longer used by the plugin), GCD, Cooldown, Casttime, Animation and Animation2 (both no longer used: a hotbar cast plays the clips a regular cast of the spell would, `get_cast_anim_set` in `casting_controller.cpp`), optional Symbol/Shouttext/Ranktext/ScrollID) plus icon PNGs in `vanilla_spell_icons/`, `modded_spell_icons/<mod>/` (matched by normalized spell name) or `*_contributed/`.
2. `stitch_icon_atlas.py` (`stitch_mod`, `i4_mod`, etc., toggled by `if redo_all_img or False:` blocks in `__main__`) writes to the dev mod folder: `images/icons_<mod>.dds/.png/.csv` (atlas + UVs), `spelldata/spells_<mod>.csv`, and I4 files (`InventoryInjector/<esp>.json` + `Interface/SpellHotbar/<mod>_icons.swf`).
3. SWF creation uses `png2swf.exe` (SWFTools) then `SWF_Generator` (Java, JPEXS FFDec lib) jar at `SWF_Generator/out/artifacts/SWF_Generator_jar/SWF_Generator.jar` to rescale and add frame labels.

Python deps: opencv (`cv2`), Pillow, Wand (ImageMagick), numpy, pandas; venvs exist in `python_scripts/.venv` / `venv`.
