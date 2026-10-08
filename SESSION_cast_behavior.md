# Session handover: custom cast behavior (Pandora / Nemesis)

Written 2026-10-08, branch `flick`. Purpose: continue this work from another PC.

## Goal

Replace the old cast animation path (force the player into the shout state, swap the shout clips through ~60 OAR submods) with an own Havok behavior that is patched in by Pandora or Nemesis and plays the vanilla cast clips.

## Status

Working in game (tested by the user, Pandora only):

- Fire-and-forget, concentration and dual casts, third and first person
- Standing, running and sneaking, with one-handed, two-handed, dual wield, bow, spell equipped
- Legs animate normally while casting and moving
- Two-handed and dual wield stances cast towards the crosshair

Not tested or not done:

- Ritual clips (only play for two-handed spells with cast time above 1.51 s). In the test setup Fire Storm and Blizzard have a shorter cast time and use the dual set.
- Ritual concentration (anim id 11001) has no ritual set, it maps to the dual concentration set.
- The 15 thuum-style animations (ids 10001-10015) are not ported, they fall back to the plain aimed / dual set.
- The patch has never been run through Nemesis itself.
- The missing-patch message box has not been tested since it was written.
- `create_fomod_installer.py` was changed but not run.

## Nothing is committed

All changes are in the working tree. To move to another PC, commit and push first.

Modified: `skse_plugin/CMakeLists.txt`, `skse_plugin/src/casts/casting_controller.cpp/.h`, `skse_plugin/src/flick/ui/config_tool.cpp`, `skse_plugin/src/game_data/spell_cast_data.cpp/.h`, `python_scripts/create_fomod_installer.py`, `README.md`.

New: `python_scripts/build_cast_behavior.py`, `skse_plugin/src/casts/cast_behavior_anims.h` (generated), `CLAUDE.md` (was untracked before this session and has a new "Cast behavior" section), this file.

The generated behavior files are **not** in the repo. They live in the dev mod folder and are rebuilt by the script:

- `meshes/actors/character/behaviors/SpellHotbarCast.hkx`
- `meshes/actors/character/_1stperson/behaviors/SpellHotbarCast.hkx`
- `Nemesis_Engine/mod/shbc/` (patch, both projects)

## How it works

### Behavior side (`python_scripts/build_cast_behavior.py`)

The script reads the vanilla behaviors from the Nemesis cache, converts them with hkxcmd and writes everything into the dev mod folder.

**`SpellHotbarCast.hkx`** is built from node templates of the vanilla shout behavior. It contains only:

- Root state machine `SHB_CastBehavior`, its `isActive` is bound to the variable `SHB_bCasting`
- State `SHB_Charge`: a selector by `SHB_iAnim`, per anim set a start clip followed by a loop clip
- State `SHB_Release`: a selector by `SHB_iAnim`, per anim set a one-shot release clip (raises `SHB_CastStop` at its end) or a looping hold clip for concentration
- Transitions with a fixed 0.1 s blend
- Its own 4 events and 2 variables, nothing vanilla

The anim sets are the table `anim_sets` in the script: aimed, self, aimed concentration, self concentration, ward (each left and right), staff and staff concentration (left and right), dual aimed / self / aimed concentration / self concentration, ritual aimed / self. All clips are vanilla. The same order is written to `cast_behavior_anims.h`.

**Patch `Nemesis_Engine/mod/shbc/`** (Nemesis format, Pandora reads it):

- `0_master`: new events and variables, and a blender inserted below `RootModifierGenerator`.
  - Child 0 is the vanilla graph. Its bone weights are 0.001 on every bone an overlay can take and 1 elsewhere. It is the sync master.
  - Five overlay children, each a behavior reference to `SpellHotbarCast.hkx` with a bone weight array and a weight bound to a variable:

    | Variable | Bones |
    |---|---|
    | `SHB_fWeightLeft` | left arm |
    | `SHB_fWeightRight` | right arm |
    | `SHB_fWeightBoth` | upper body |
    | `SHB_fWeightBodySingle` | everything except both arms |
    | `SHB_fWeightBodyBoth` | everything below the upper body |

  - With all weights 0 the overlays are inactive and the vanilla graph owns every bone. This is the technique vanilla uses for shield plus spell (`MRh_Shd_Blend`).
- Every other vanilla behavior file: the four events are registered there too.

### Plugin side (`casts/casting_controller.cpp`, namespace `cast_overlay`)

- At cast start: map the spell's anim id, hand, staff and delivery to an anim set (`get_cast_anim_set`), set `SHB_iAnim`, send `SHB_CastStart`.
- The arm overlay (upper body for dual and ritual) is faded in over 0.2 s and stays for the whole cast.
- The matching body overlay is faded in on top while the player stands still, is not sneaking and not jumping (graph variable `Speed` below 1). Then the whole pose comes from the cast clip. It crossfades over 0.25 s when movement starts or stops.
- Release: `SHB_CastRelease`. Cancel: `SHB_CastStop`.
- An animation event sink on the player (added at every cast start) reports `SHB_CastStop` (release clip ended, fade out) and `SHB_CastRelease`.
- A cast is interrupted by attack, block, stagger, mount, swim or bleedout (user decision), or when `SHB_bCasting` is false after a 0.3 s grace time.
- If `SHB_bCasting` does not exist in the graph, the patch is missing: message box once, cast refused.

### Testing tab (`flick/ui/config_tool.cpp`)

Last tab of the "Spell Hotbar 2" tool, switched by `k_testing_tab_enabled` at the top of the file (set to false for releases).

- Set All Skills to 100
- Add 10 Perk Points
- Add Spells (18 vanilla spells, ids from `python_scripts/spell_lists2`)
- Checkbox "Full Body Cast Pose When Standing" for A/B comparison, not saved

## Things that were learned the hard way

Keep these in mind before changing the design.

1. **Events have to be registered in every behavior file.** With the events only in `0_master` and the own hkx, sending `SHB_CastStart` reset the locomotion of the vanilla sub-behaviors to idle (legs stood still while the player slid). A behavior file added by another mod could react the same way.
2. **The own hkx must not declare vanilla variables or events**, and no node may keep a binding by index into the old variable table. A leftover binding made the transition blend time equal to the anim set number in seconds (dual release never visibly arrived). `check_bindings()` in the script now aborts the build on any unexpected binding.
3. **The base child has to be the sync master** of the blender (`indexOfSyncMasterChild` 0), as vanilla does in `MLh_Equip_Moving_Blend`.
4. **Do not wrap `Default_State`.** Transitions return into it with nested state ids (`staggerStop->14.1`, `WeapEquip->14.10`). The blender sits one level above `RootBehaviorGraph` instead.
5. **Overlays that are active together must not share bones.** Each overlay is an own instance of the cast behavior; a body overlay that starts in the middle of a cast is not at the same point of the animation as the arm overlay.
6. **Twist corrections were a dead end.** Turning the torso (spine twist) and holding the head with `hkbTwistModifier` was tried for several rounds to fix the turned two-handed / dual wield stances. It cannot work in general: the upper body comes from the cast clip and the hips from the weapon stance, and the release clips turn the whole body. The full-body pose while standing replaced it.
7. **Absolute bone yaw measured from bone rotation axes was unreliable**, only differences were usable. Joint positions (hip joints, shoulder joints) gave plausible values. Local z runs along the spine, neck and head bones.
8. **Nemesis patch node format**: the hkxcmd XML without the `SERIALIZE_IGNORED` comment lines, one array element per line (vectors without brackets), CRLF. `nemesis_format()` was checked byte for byte against the patches of True Directional Movement and Ultimate Animated Potions NG. A replaced line uses `<!-- MOD_CODE ~shbc~ OPEN -->` new `<!-- ORIGINAL -->` old `<!-- CLOSE -->`.
9. **hkxcmd writes SSE hkx directly** (`-v:AMD64`, byte identical to the Nemesis SSE file for vanilla input) but only reads 32 bit hkx. The output of Pandora can only be checked by scanning the binary for strings and float patterns.
10. A screenshot settled in one step what several rounds of log reading had not. Ask for one early for anything visual.

## Machine specific paths (this PC)

The script and the build have these hardcoded. They have to exist or be adapted on another PC.

| What | Path |
|---|---|
| Dev mod folder (MO2) | `F:\Skyrim Dev\ADT\mods\Spell Hotbar 2` |
| Vanilla behaviors, 32 bit | `F:\Skyrim Dev\ADT\mods\Project New Reign - Nemesis Unlimited Behavior Engine\Nemesis_Engine\cached_behaviors` (first person in `_1stperson`) |
| hkxcmd | `F:\Skyrim Dev\TOOLS\SBT\hkxcmd.exe` |
| Pandora output | `F:\Skyrim Dev\ADT\mods\Pandorra Cache\meshes\...` |
| Python | `python_scripts\.venv\Scripts\python.exe` (standard library only for this script) |
| Plugin log | `Documents\My Games\Skyrim Special Edition\SKSE\SpellHotbar2.log` |
| Screenshots (Print Screen) | `F:\Skyrim Dev\ADT\Game Root\ScreenShotN.png` |
| MSVC environment | `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat` |

## Work loop

1. Behavior change: edit `python_scripts/build_cast_behavior.py`, run it, then run Pandora with "Spell Hotbar 2 - Cast Behavior" enabled.

   ```
   python_scripts\.venv\Scripts\python.exe python_scripts\build_cast_behavior.py
   ```

2. Plugin change: build from a shell with the MSVC environment, then deploy. `OUTPUT_FOLDER` is empty in the CMake cache, the build itself does not copy the DLL.

   ```
   cd skse_plugin
   cmake --build build/release_with_debug_info
   cd ..
   copy_release_plugin.bat
   ```

3. A plugin-only change needs no Pandora run, only a game restart.

## Open items

- **Thuum-style animations**: decide whether to port them. The clips are in the old OAR folder (`meshes/actors/character/OpenAnimationReplacer/SpellHotbar2/cast_thuum_*`). New clips have to be added to the character file's `animationNames` (`defaultmale` / `defaultfemale` node `#0029`, first person `firstperson`) and to `animationsetdatasinglefile`; Ultimate Animated Potions NG shows the patch layout. Until then keep the OAR folder, it is no longer packed by the installer.
- **Ritual and ritual concentration**: test with a slow two-handed spell (set Fire Storm to 3 s in the spell editor). Add a ritual concentration set if wanted.
- **OAR globals** (`SpellHotbar_SpellAnimationType` 0x815, `isCastingConcSpell` 0x834, `Casttimer` 0x838) are still written by the plugin. They were kept on purpose: records in `SpellHotbar.esp` may read them and they are a hook for OAR animation packs. Not verified whether anything reads them.
- **Third party behaviors**: only the vanilla behavior files get the events registered.
- **Nemesis**: run the patch through Nemesis once.
- **Old plan file** `C:\Users\Gernot\.claude\plans\i-want-do-go-synchronous-newell.md` describes the first design (cast state next to the shout state in `mt_behavior`, `1hm_behavior`, `magicbehavior`). It is outdated, this file replaces it.
- The spine of the single-hand cast pose while standing comes from the cast clip, the weapon arm from the stance. Looked good in the tests; a two-handed weapon is then held in one hand.
