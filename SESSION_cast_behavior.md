# Session handover: cast behavior and follow-up work

Last updated 2026-10-24, branch `v2.1`, the 2026-10-24 commits are local (not pushed). Purpose: continue this work from another PC. First version written 2026-10-08 for the cast behavior; this rewrite covers everything since.

## State of the branch

Commits on `v2.1` since the cast behavior (newest first) (the three 2026-10-24 commits for dock glyphs, ritual concentration and swimming are not in this table):

| Commit | Contents |
|---|---|
| `88fd8a3` | Clips of a regular cast, custom animation system removed (not tested in game yet) |
| `4aa8c27` | Unused OAR animation globals 0x815 / 0x834 / 0x838 removed from `SpellHotbar.esp` (provenance: 33 sanctioned changes, record count 24; rebuilt ESP already in the dev mod) |
| `bc6f832` | Graph variables and SKSE mod events for animation replacers / other mods |
| `35c34f2` | Config tool laid out as MCM style option tables |
| `c889467` | FLICK API header v7 (FLICK 1.7.0 now the minimum) |
| `29674d2` | Mounted hotbar casting (spells, powers, shouts, scrolls), mounted aim and torso turn, castbar textures, Vampire Lord melee equip block, testing helpers |
| `ec773b1` | Castbar above the hotbar |
| `f4a7f50` | Casting art of the spell itself on hotbar casts (dummy cast effect system removed) |
| `6efef0b` | License: source MIT, binaries GPL-3.0-or-later |
| `639d13b` | CommonLibSSE-NG switched to alandtse's fork v12.1.0 |
| `34ce406`, `cf60080` | Dual casting movement option (default off) |
| `813b4c5` | Cast spells through an own behavior instead of the shout state |

Untracked files in the working tree (`releases/`, `python_scripts/tts_workspace/`, nolvus spell list and icons, misc scripts) were never part of this work.

## Feature status

Tested and working in game (Pandora):

- Cast behavior: fire-and-forget, concentration, dual, ritual; first and third person; standing, moving, sneaking; all weapon stances.
- Casting art of the spell in the hands (`casts/cast_art.cpp`), with the art behavior's animation events.
- Castbar: `bar_bg.png` background and `bar_cast.png` fill tinted by magic school (gray for shouts), shout castbar with word marks. Textures in `python_scripts/icons/`, packed by the installer.
- Mounted casting: spells, scrolls, powers, shouts on horseback. Dual/ritual casts use both arm overlays, no body overlay. Mounting/dismounting mid-cast interrupts.
- Mounted aim: aimed casts fly at the crosshair and the torso turns towards the camera. A copy of the vanilla mounted bow `BSDirectAtModifier` (`MC_BSDirectAtModifier_Bow` from horsebehavior) is appended to the 0_master root modifier list (third person), enabled by `SHB_bMountedAim`, turn limits bound to `SHB_fMountedAimLimit`. The plugin (`mounted_aim` namespace) sets `bAimActive`, `AimHeadingCurrent = -heading`, `AimPitchCurrent = -pitch` (heading = camera yaw − rider yaw), and after the release fades the limit 90° → 0 over 0.4 s so the torso turns back smoothly.
- Vampire Lord: hand spells cannot be equipped via the hotbar in melee form (graph variable `isLevitating` false).
- Config tool (FLICK sidebar tool "Spell Hotbar 2"): MCM style tables, header per section, label | control, two options per row when wide enough; Windows, Util and Testing one per row; Bars tab is a Bar | Enabled | Inherit table; keybinds rebind by clicking the key name.
- Graph variables and mod events for other mods (pushed, Pandora run done, not checked in game with a real condition/script yet):
  - `SHB_iCastSpell` (local form id, ESL aware), `SHB_iCastPlugin` (FNV-1a of the lower case plugin file name folded to 24 bits), `SHB_iCastHand` (1 left, 2 right, 3 both), set for the whole hotbar spell cast. The spell edit dialog shows a spell's values ("Anim Condition" row). For OAR submods: OR `CompareValues` on these with their `IsEquipped…` check.
  - SKSE mod events `SpellHotbar_CastStart`, `SpellHotbar_CastRelease`, `SpellHotbar_CastStop` (sender: spell, numArg: hand, Stop strArg `finished` / `interrupted`), for compatibility patches. Vanilla cast animation events are deliberately not faked (their listeners read the equipped spell).

Added 2026-10-23/24, tested in game (Pandora):

- Ritual concentration (Lightning Storm): anim set `ritual_aimed_conc` as vanilla `RitualSpellAimConcentrationStart` (`RitualSpell_Charge` / `RitualSpell_Ready`, then `RitualSpell_AimRelease` once, then `DMagAimConLoop`). An `AnimSet` may have both release and hold; the release clip raises `SHB_CastHold` at its end and the plugin fires the spell then (1.5 s fallback), so the beam starts when the hands go forward. Self ritual concentration (no vanilla spell) still uses the dual clips.
- Casting while swimming (option "Cast While Swimming", default on, save format 9, preset key `settings.allow_swim_casting`): arms only as on horseback, surface and underwater; entering or leaving water interrupts. Shouts are blocked in water (the game does not shout while swimming).
- Dock glyphs (`skse_plugin/scripts/dock_icons.py`, from the FLICK PR) copied into the dev mod and packed by the installer from the repo.
- This PC: the CMake cache had the 8.3 short path of ninja (`MICROS~2`), which differs per PC; it now holds the long path.

Done, not tested in game:

- **Custom animation system removed** (`88fd8a3`). Every hotbar spell plays the clips a regular cast would: `get_cast_anim_set(pc, spell, used_hand)` picks by two-handed (ritual), casting type, delivery, ward keyword, hand(s) and staff. No cast-time switch any more (`fast_cast_threshold` gone), no Animation / Animation2 in spell data, editor or presets. The CSV columns are ignored; old co-saves are read and the animation values discarded (no save format change); old preset keys `spell_animation(2)` are ignored. Ritual concentration (Lightning Storm) has its own set (see below).
- Test list: Fireball left/right/dual, Flames, Healing, a ward, Fire Storm with short and long cast time (both ritual now), Lightning Storm, staff, scroll; spell editor and edit dialog open fine.

## Animation replacers (checked)

- The cast behavior references only vanilla clip names, so OAR/DAR replacers swap them automatically and vanilla stays vanilla. Keep it that way: no custom clips.
- **Goetia Animations - Magic Spell Casting** (Nexus 70204): "Main" submod has no conditions, works out of the box. It lacks `mrh_chargeloop`, so right-hand aimed charge mixes Goetia precharge, vanilla loop, Goetia release. That is the replacer's gap (also in normal casting); decision: leave it to the replacer.
- **Casting animations for The Witcher signs**: DAR conditions `IsEquippedLeft(<sign>)`, so it only matches when the sign is equipped. DAR syntax cannot read graph variables; a patch would need an OAR `config.json` using the `SHB_iCast*` variables.
- **Strange Runes** (po3, Nexus 19456): Papyrus script on `BeginCastLeft/Right`, `MLh/MRh_SpellFire_Event`, `CastStop`, reads the equipped spell. Does not work with hotbar casts; decision: only via explicit compatibility patches using the mod events.

## How the cast behavior works

Unchanged from the first version; `CLAUDE.md` ("Cast behavior", "Casting art") has the current summary.

- `python_scripts/build_cast_behavior.py` reads the vanilla behaviors from the Nemesis cache (hkxcmd) and writes into the dev mod: `SpellHotbarCast.hkx` (third and first person) and the Nemesis-format patch `Nemesis_Engine/mod/shbc/`.
- `SpellHotbarCast.hkx`: root state machine `SHB_CastBehavior` (`isActive` → `SHB_bCasting`), `SHB_Charge` (selector by `SHB_iAnim`: start clip then loop), `SHB_Release` (one-shot release raising `SHB_CastStop`, or concentration hold), own events and variables only. Anim sets in `anim_sets`, mirrored in the generated `skse_plugin/src/casts/cast_behavior_anims.h`.
- Patch `0_master`: blender below `RootModifierGenerator`, child 0 the vanilla graph (bone weight 0.001 on overlay bones, sync master), five overlays bound to `SHB_fWeightLeft/Right/Both/BodySingle/BodyBoth`, plus the mounted aim modifier and the variables above (`SHB_iAnim`, `SHB_bCasting`, weights, `SHB_bMountedAim`, `SHB_fMountedAimLimit`, `SHB_iCastSpell/Plugin/Hand`). Events registered in every vanilla behavior file.
- Plugin `cast_overlay`: arm (or upper body) overlay for the whole cast, body overlay on top while standing still; interrupts on attack, block, stagger, mount change, swim, bleedout.

## Things learned the hard way

1. Events have to be registered in every behavior file, otherwise vanilla sub-behaviors drop locomotion to idle.
2. The own hkx must not declare vanilla variables/events or keep bindings into the old variable table (`check_bindings()` aborts on that).
3. The base child of the blender must be the sync master.
4. Do not wrap `Default_State` (nested state ids in transitions).
5. Overlays active together must not share bones.
6. Twist corrections for the turned weapon stances were a dead end; the full-body pose while standing replaced them.
7. Absolute bone yaw from rotation axes is unreliable; joint positions work.
8. Nemesis node format: hkxcmd XML without `SERIALIZE_IGNORED` lines, one array element per line, inline vectors expanded, CRLF.
9. hkxcmd writes SSE hkx but only reads 32 bit; Pandora output can only be checked by scanning the binary for strings.
10. Mounted aim: setting `pc->data.angle` does nothing; the graph variables work. Engine aim heading = rider heading − `AimHeadingCurrent`, aim angle = −`AimPitchCurrent`.
11. A disabled `BSDirectAtModifier` stops acting in the same frame (snap back); fade its limits instead.
12. Ask for a screenshot early for anything visual.
13. FLICK's `Button` measured the hidden `##id` part of labels (uneven widths, off-center text). Fixed upstream by the user's PR (`CalcTextSize(label, nullptr, true)` in `OutlineButton`).

## FLICK

- Header `skse_plugin/third_party/flick/FUCK_API.h` = FUCK_API repo commit `bc52809` (API v7), pinned by commit and SHA-256 in `skse_plugin/CMakeLists.txt`. Minimum FLICK 1.7.0.
- Possibly useful v6/v7 API not used yet: `GetInputCaptureState`, `IsHotkeyEnabled`, `ComboWithKeyBadges`, `IsGameHUDHidden`, `EnumerateHUDWidgets` / `SetWindowAlphaOverride`, sticky lists.
- This PC runs a self-built `FUCK.dll` from the user's fork `F:\Skyrim Dev\FUCK` (button fix, merged upstream) until a FLICK release ships it. Original 1.7.2 DLL/PDB in MO2 mod `FLICK 1.7.2 original backup` (disabled). Fork build: submodules initialized, vcvars64 shell, `cmake --preset vs2022-windows-vcpkg`, `cmake --build build --config Release`, copy `build/Release/FUCK.dll` by hand. On another PC: install the latest FLICK release, or rebuild the fork.

## Dev mod housekeeping done this session

- Old OAR cast animations (56 submods) moved out of the dev mod to MO2 mod `SpellHotbar2 OAR Backup` (keep disabled); the old `animationdata/` CSVs moved there as well.
- Leftover translation keys removed from the dev mod `localization/translation.txt` (`$COLUMN_CASTEFFECT`, `$CAST_EFFECT`, `$NO_EFFECT`, `$COLUMN_ANIM`, `$COLUMN_ANIM2`, `$ANIMATION`, `$ANIMATION2`). The file is CRLF without a trailing newline.
- `SpellHotbar.esp` in the dev mod rebuilt without the three globals (old copy only in this PC's scratchpad).

## Machine specific paths (this PC)

| What | Path |
|---|---|
| Dev mod folder (MO2) | `F:\Skyrim Dev\ADT\mods\Spell Hotbar 2` |
| Vanilla behaviors, 32 bit | `F:\Skyrim Dev\ADT\mods\Project New Reign - Nemesis Unlimited Behavior Engine\Nemesis_Engine\cached_behaviors` |
| hkxcmd | `F:\Skyrim Dev\TOOLS\SBT\hkxcmd.exe` |
| BSA extraction | `F:\Skyrim Dev\TOOLS\BSA Browser-…\bsab.exe` |
| Pandora output / log | `F:\Skyrim Dev\ADT\mods\Pandorra Cache\` (`Engine.log`) |
| FLICK fork | `F:\Skyrim Dev\FUCK` |
| Python | `python_scripts\.venv\Scripts\python.exe` |
| Plugin log | `Documents\My Games\Skyrim Special Edition\SKSE\SpellHotbar2.log` |
| Steam screenshots (F12) | `C:\Program Files (x86)\Steam\userdata\11995243\760\remote\489830\screenshots` |
| MSVC environment | `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat` |

## Work loop

1. Behavior change: edit `python_scripts/build_cast_behavior.py`, run it, then run Pandora.
   ```
   python_scripts\.venv\Scripts\python.exe python_scripts\build_cast_behavior.py
   ```
2. Plugin change: build in a vcvars64 shell, then deploy (the CMake cache has no `OUTPUT_FOLDER`).
   ```
   cd skse_plugin
   cmake --build build/release_with_debug_info
   cd ..
   copy_release_plugin.bat
   ```
3. ESP change: edit `plugin-src/`, update `plugin-src/provenance.json` (and the sanctioned count in `python_scripts/plugin_provenance.py`), run `python_scripts\build_plugins.py`, copy `build\plugins\SpellHotbar.esp` into the dev mod.
4. Plugin-only changes need no Pandora run, only a game restart.

## Open items

- Test `88fd8a3` (clips of a regular cast) and `4aa8c27` (ESP without the globals) in game.
- Test `SHB_iCast*` with a real OAR condition and the mod events with a small Papyrus listener.
- Thuum-style animations were tried and reverted; not planned.
- Nemesis itself was never run with the patch; the missing-patch message box is untested.
- Possibly use FLICK v7 features (input capture, HUD hidden, key badge combos).
- Testing tab (`k_testing_tab_enabled` in `config_tool.cpp`) must be off for releases.
