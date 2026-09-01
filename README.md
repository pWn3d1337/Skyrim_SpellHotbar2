![#spellhotbar2_banner](docs/images/spellhotbar2_banner.png)  
A total rework to SpellHotbar (https://github.com/pWn3d1337/Skyrim_SpellHotbar)

# Beta State
The mod is currently still in a beta state, but there are no more missing major features before a release. If you want to report Bugs create an Issue here, please also provide information about your skyrim binary version and include the log file in case of crashes or loading issues. ("C:/Users/USERNAME/Documents/My Games/Skyrim Special Edition/SKSE/SpellHotbar2.log"). Crash logs can also be helpful in case of a crash.

## Contribute
### Presets
If you want to help out you can create custom presets. I would like to add more presets to the installer.

### Video and Image material
If you have created cool Screenshots or video clips using the mods I could also use some promotion Material for the Mod Page in the future.

### Test the Mod and Report Issues
:-)

## Requirements
[SKSE](https://skse.silverlock.org/)  
[Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)  
[SKSE Menu Framework 3.14+](https://www.nexusmods.com/skyrimspecialedition/mods/120352) ([runtime source](https://github.com/QTR-Modding/SKSE-Menu-Framework-3), [consumer API](https://github.com/QTR-Modding/SKSE-Menu-Framework-3-API)) — required UI host; install separately, this mod does not redistribute it  
[OAR](https://www.nexusmods.com/skyrimspecialedition/mods/92109)  
[Inventory Interface Information Injector](https://www.nexusmods.com/skyrimspecialedition/mods/85702) - if you want icons in the swf menu  
[Custom Skill Framework v3](https://www.nexusmods.com/skyrimspecialedition/mods/41780) - when using custom perk tree, a v2 compatible config is also provided for 1.5.97 users    
[Sprint Sneak Movement Speed Fix](https://www.nexusmods.com/skyrimspecialedition/mods/86631) - fixes sneak hotbar casting related movement speed  
[ImGui Icons](https://www.nexusmods.com/skyrimspecialedition/mods/114790) - OPTIONAL, allows keybind icons to be used instead of text, different styles also work.

SkyUI is not required for Spell Hotbar 2. Configuration lives in SKSE Menu Framework's Mod Control Panel (eight pages: Keybinds, Spell Bind Menu, Settings, Bars, Perks, Presets, Spells, Util).

Saves stay compatible with upstream 0.0.14: SKSE co-save format `5` and JSON presets format `2`. The HUD uses SKSE Menu Framework's default ImGui face. Optional FOMOD font files still install into `Data/SKSE/Plugins/Fonts` so the host can discover them; this guest does not call `PushFont`.

Opposed to v1 DAR is no longer supported as OAR is a superior alternative in any way and has an easier file structure.  
All papyrus lib requirements have been dropped.

## Supported Skyrim Version
The mod is built with CommonLib-NG and was tested on 1.5.97, 1.6.640 and 1.6.1070. Other versions will not be officially supported.

## Building
From `skse_plugin`, configure with CMake preset `release` (`VCPKG_ROOT` must point at a CommonLibSSE-NG triplet matching `CMakePresets.json`). The Release DLL lands in `skse_plugin/build/release/SpellHotbar2.dll`. Rebuild both ESPs with `python python_scripts/build_plugins.py` (Spriggit 0.40.1). Static SMF guest checks: `python python_scripts/verify_smf_guest_boundary.py`, `python python_scripts/verify_smf_ui_smoke.py`, and `python python_scripts/verify_smf_migration.py`. The FOMOD/release zip scripts read packed assets from `SPELLHOTBAR_ASSET_ROOT` (defaults to the upstream maintainer's `F:\Skyrim Dev\ADT\mods` layout) and never copy `SKSEMenuFramework.dll`.

The consumer API header and LGPL-2.1 license are vendored under `skse_plugin/third_party/skse-menu-framework/`; see `UPSTREAM.md` there for pinned commits and hashes.

# Updating from v1:
* Keep a backup of your save.  
* Do a clean save (save game without mod). Make sure to remove all Files from v1. Use [Resaver](https://www.nexusmods.com/skyrimspecialedition/mods/5031/) to remove unattached script fragments.  
* Make sure your Game still works.  
* Install V2 (Also check changed requirements, OAR is now needed)

# Changes to v1

## SKSE based
All spell casting is now triggered by SKSE plugin and not by papyrus. Casting is now strongly tied to animations. 

## FOMOD Installer
Spell Packs and Perk Overhauls can now be selected inside a FOMOD installer. Spell Packs auto check depending on your installed plugins.
You can also chose an "Auto Profile" that will be applied on new game or first install.
![#installer](docs/images/installer.jpg)  

## Auto Profiles
If a file called "..Data\SKSE\Plugins\SpellHotbar\presets\auto_profile.json" is present in your install it will automatically load on new game or first init. This allows mod packs to redistribute the Mod Control Panel configuration. An auto profile can be chosen during the installer.

## Input improvements
Modifiers are no longer hardcoded to CTRL, SHIFT and ALT and up to 3 modifiers can now be freely configured. All controller buttons should also be supported.
Key inputs are instantly registered without papyrus delay.

## First Person and Concentration Spell support
First person and concentration spells are now handled

## Dual Casting
Spells now can be bound to either Hand or both hands (Indicated by R, L and D in the bind menu). To toggle the hand, just press the Keybind in the binding menu again.
No R,L or D text indicated 'Auto' mode, there a spell will chose the hand depending on equipped items and uses Dual Casting if enabled. Dual Casting can be toggled by a power that is automatically added to the player
or by holding Keybind modifier that can be defined in the Mod Control Panel. A spell can only be set to dual cast (D) in the menu if the spell supports dual casting and the player has the corresponding perk. (During the fomod installer select your correct perk overhaul so the dual cast perks are configured correctly). For balancing and animation reasons Dual casting blocks movement. (Technically by capturing your movement inputs, so no scripts are going to break from it)

### First Person, Concentration and Dual Casting in Youtube Preview:  
[![spellhotbar2_yt_preview](https://img.youtube.com/vi/5aj7-3XC50I/0.jpg)](https://www.youtube.com/watch?v=5aj7-3XC50I)

## Support for Potions, Food Items and Scrolls
Potions, Food items and scrolls are now also supported. Scrolls behave very similar as spells. Potions will be used as equipping them in inventory. (Mods like animated potions are compatible). Optionall a gcd for potions can be configured to avoid insane potion spam when not using and animated potion mod. Self brewed potions (Dynamic Forms) are also supported with generic icons (changing depending on potions value) and changing their color depending on the strongest effect.

![#potions](docs/images/potions.jpg)  

## Bar positioning and Layout
Positioning is now scaled to your resolution (using display height) and the Bar can be set to an anchor point (bottom, left, top, left-top, ...). This allows presets to also be transfered between different aspect ratios. (A left side vertical bar will work similar on 16:9 and 21:9 for example. 
Also the bar can now be changed in Layout by changing row and column count, this allows fully vertical bars or boxes. It is also possible to create a circle.

![#bar_layout1](docs/images/bar_layout1.jpg)  
![#bar_layout2](docs/images/bar_layout2.jpg)  

## Input Modes
It is now possible to chose the input mode.
* Direct cast - Like in V1, spells are casted when pressing the skill, powers are equipped and used. (compared to v1 the old power is re-equipped afterwards).
* Equip Mode - Will equip a spell or power instead of casting it. By setting hand it can be chosen which hand it will be equipped.
* Oblivion Mode - Tries to recreate Oblivion style casting. Adds a 2nd bar to show the currently equipped Spell and Potion. (Power can also be displayed). There is an extra shortcut for casting the equipped spell like in 'Direct Cast' mode. The regular keybinds will change the current selected spell/power/potion

![#Oblivion_mode](docs/images/oblivion_mode.jpg)  

## Battlemage Perktree [Optional]
Requires [Custom Skill Framework v3](https://www.nexusmods.com/skyrimspecialedition/mods/41780) to access the perktree. The Mod Control Panel's Perks page can open the tree or disable perk requirements.
Uses regular perk points. Allows to get "Spell Procs" which turns the next casted spell into near-instant cast and 50% less mana cost. Spell Procs are visible as animated golden border around spells. And the spell learn sound is played on trigger.
At start only novice and apprentice spells can consume procs, and there is a 10s cooldown (configurable in the Mod Control Panel) before gaining another proc after one is consumed.

![#Spell_Procs](docs/images/spell_proc.jpg)

### Available Perks:
Note: chances and timing are configurable in the Mod Control Panel.
#### Triggers (starting perks, no perk requirement):
* Cast On Concentration: Casting a concentration spell for 6 seconds can trigger Novice or Apprentice spell procs. Requires a Magic Skill of at least 25.
* Cast On Block: Timed Blocking can trigger Novice or Apprentice spell procs. Requires 'Block' 25.
* Cast On Crit: Critical hits can trigger Novice or Apprentice spell procs. Requires 'One-Handed' or 'Two-Handed' or 'Archery' 25.
* Cast On Backstab: Sneak Attacks can trigger Novice or Apprentice spell procs. Requires 'Sneak' 25.
* Cast On Power Attack: Power Attack hits can trigger Novice or Apprentice spell procs. Requires 'One-Handed' or 'Two-Handed' 25.
#### Advanced Perks
* Expert Battlemage (Requires at least one trigger): Spell Procs can trigger Adept and Expert spells. Reduces Cooldown by 20%. Requires 'Alteration' 50.
* Master Battlemage (Requires Expert Battlemage): Spell Procs can trigger Master spells. Reduces Cooldown by 40%. Requires 'Alteration' 75.   (Note 40% reduction in total, not additive)
 
![#BattleMage_Perks1](docs/images/battlemage_1.jpg) ![#BattleMage_Perks2](docs/images/battlemage_2.jpg)  

## Ingame Editor
The Mod Control Panel can open editors for Spells and Potions. They support custom icons and values such as spell animation; edits remain save-specific and are stored in the SKSE co-save.

![#Ingame_Editor](docs/images/spell_editor1.jpg) ![#Ingame_Editor2](docs/images/spell_editor2.jpg)  

## Advanced Bind Menu
Allows the user to have a dedicated menu to slot skills with drag and drop; inputs are NOT forwarded to the game while this menu is open. This avoids key conflicts. Open it from the Mod Control Panel's Spell Bind Menu page (the panel closes so the bind window can receive clicks), or configure the "Open Binding Menu" key on the Keybinds page and press it while the Magic Menu or a supported inventory tab is open (potions, food, scrolls).  
![#Binding Menu](docs/images/binding_menu.jpg)

## Currently supported Spell/Perk mods with icons
[Abyss](https://www.nexusmods.com/skyrimspecialedition/mods/83329)  
[Abyssal Tides Magic](https://www.nexusmods.com/skyrimspecialedition/mods/97892)  
[Abyssal Wind Magic](https://www.nexusmods.com/skyrimspecialedition/mods/96352)  
[Ancient Blood Magic II](https://www.nexusmods.com/skyrimspecialedition/mods/115106)  
[Andromeda - Unique Standing Stones of Skyrim](https://www.nexusmods.com/skyrimspecialedition/mods/14910)  
[Apocalypse - Magic Of Skyrim](https://www.nexusmods.com/skyrimspecialedition/mods/1090)  
[Arclight](https://www.nexusmods.com/skyrimspecialedition/mods/90405)  
[Astral Magic 2](https://www.nexusmods.com/skyrimspecialedition/mods/69938)  
[Constellation Magic](https://www.nexusmods.com/skyrimspecialedition/mods/92104)  
[Dark Hierophant Magic](https://www.nexusmods.com/skyrimspecialedition/mods/108499)  
[Desecration](https://www.nexusmods.com/skyrimspecialedition/mods/90832)  
[Elemental Destruction Magic Redux](https://www.nexusmods.com/skyrimspecialedition/mods/37211)
[Elemental Mastery Magic](https://www.nexusmods.com/skyrimspecialedition/mods/139953)  
[Holy Templar Magic](https://www.nexusmods.com/skyrimspecialedition/mods/113360)  
[Miracles of Skyrim](https://www.nexusmods.com/skyrimspecialedition/mods/71708)  
[Mysticism - A Magic Overhaul](https://www.nexusmods.com/skyrimspecialedition/mods/27839)  
[Obscure Magic](https://www.nexusmods.com/skyrimspecialedition/mods/103805)  
[Odin - Skyrim Magic Overhaul](https://www.nexusmods.com/skyrimspecialedition/mods/46000)  
[Ordinator - Perks of Skyrim](https://www.nexusmods.com/skyrimspecialedition/mods/1137)  
[Path of Sorcery - Magic Perk Overhaul](https://www.nexusmods.com/skyrimspecialedition/mods/6660)  
[Sacrosanct - Vampires of Skyrim](https://www.nexusmods.com/skyrimspecialedition/mods/3928)  
[Sonic Magic](https://www.nexusmods.com/skyrimspecialedition/mods/76360)  
[SPERG - Skyrim Perk Enhancements and Rebalanced Gameplay SE](https://www.nexusmods.com/skyrimspecialedition/mods/14180)  
[Stellaris](https://www.nexusmods.com/skyrimspecialedition/mods/86833)  
[Storm Calling Magic 2](https://www.nexusmods.com/skyrimspecialedition/mods/89643)  
[Thunderchild - Epic Shouts and Immersion](https://www.nexusmods.com/skyrimspecialedition/mods/1460)  
[Triumvirate - Mage Archetypes](https://www.nexusmods.com/skyrimspecialedition/mods/39170)  
[Vulcano](https://www.nexusmods.com/skyrimspecialedition/mods/88689)  
[Winter Wonderland Magic](https://www.nexusmods.com/skyrimspecialedition/mods/93871)  
[The Witcher Signs](https://www.nexusmods.com/skyrimspecialedition/mods/90645)  

### Icons Provided by Community:
#### credits ArchAngelAries
[Skull Commander Armor - Undead Horse](https://www.nexusmods.com/skyrimspecialedition/mods/116221)  
[Shadow Spell Package](https://www.nexusmods.com/skyrimspecialedition/mods/13299)  
[Star Wars Spell Pack](https://www.nexusmods.com/skyrimspecialedition/mods/82637)  

# Credits
[Mrowpurr](https://github.com/mrowrpurr) for Tutorials & Templates  

## SKSE
[CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG)  
[SKSE Menu Framework](https://github.com/QTR-Modding/SKSE-Menu-Framework-3) ([consumer API](https://github.com/QTR-Modding/SKSE-Menu-Framework-3-API), [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/120352))  
[RapidCSV](https://github.com/d99kris/rapidcsv)  
[RapidJSON](https://rapidjson.org/)   
[spdlog](https://github.com/gabime/spdlog)  
[stb_image](https://github.com/nothings/stb)  
[SKSE](https://skse.silverlock.org/)  
[Address Library](https://www.nexusmods.com/skyrimspecialedition/mods/32444)  

### Important References
[Wheeler](https://github.com/D7ry/wheeler)  
[LamasTinyHUD](https://github.com/mlthelama/LamasTinyHUD)  
[Immersive Equipment Displays](https://github.com/SlavicPotato/ied-dev)  

## Papyrus / esp
[Thu'um - Fully Animated Shouts](https://www.nexusmods.com/skyrimspecialedition/mods/50559), big thanks for open permission shout anims  

## Tools
[SWF Tools](http://www.swftools.org/) - png2swf.exe
