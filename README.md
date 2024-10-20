A total rework to SpellHotbar (https://github.com/pWn3d1337/Skyrim_SpellHotbar)

# Beta State
The mod is currently in a beta state and just being released to the public for testing

# Changes to v1

## SKSE based
All spell casting is now triggered by SKSE plugin and not by papyrus. Casting is now strongly tied to animations. 

## Input improvements
Modifiers are no longer hardcoded to CTRL, SHIFT and ALT and up to 3 modifiers can now be freely configured. All controller buttons should also be supported.
Key inputs are instantly registered without papyrus delay.

## First Person and Concentration Spell support
First person and concentration spells are now handled

## Dual Casting
Spells now can be bound to either Hand or both hands (Indicated by R, L and D in the bind menu). To toggle the hand, just press the Keybind in the binding menu again.
No R,L or D text indicated 'Auto' mode, there a spell will chose the hand depending on equipped items and uses Dual Casting if enabled. Dual Casting can be toggled by a power that is automatically added to the player
or by holding Keybind modifier that can be defined in the MCM. A spell can only be set to dual cast (D) in the menu if the spell supports dual casting and the player has the corresponding perk. (During the fomod installer select your correct perk overhaul so the dual cast perks are configured correctly)

## Support for Potions, Food Items and Scrolls
Potions, Food items and scrolls are now also supported. Scrolls behave very similar as spells. Potions will be used as equipping them in inventory. (Mods like animated potions are compatible). Optionall a gcd for potions can be configured to avoid insane potion spam when not using and animated potion mod. Self brewed potions (Dynamic Forms) are also supported with generic icons (changing depending on potions value) and changing their color depending on the strongest effect.

## Bar positioning and Layout
Positioning is now scaled to your resolution (using display height) and the Bar can be set to an anchor point (bottom, left, top, left-top, ...). This allows presets to also be transfered between different aspect ratios. (A left side vertical bar will work similar on 16:9 and 21:9 for example. 
Also the bar can now be changed in Layout by changing row and column count, this allows fully vertical bars or boxes. It is also possible to create a circle.

## Input Modes
It is now possible to chose the input mode.
* Direct cast - Like in V1, spells are casted when pressing the skill, powers are equipped and used. (compared to v1 the old power is re-equipped afterwards).
* Equip Mode - Will equip a spell or power instead of casting it. By setting hand it can be chosen which hand it will be equipped.
* Oblivion Mode - Tries to recreate Oblivion style casting. Adds a 2nd bar to show the currently equipped Spell and Potion. (Power can also be displayed). There is an extra shortcut for casting the equipped spell like in 'Direct Cast' mode. The regular keybinds will change the current selected spell/power/potion

## Battlemage Perktree [Optional]
Requires [Custom Skill Framework v3](https://www.nexusmods.com/skyrimspecialedition/mods/41780) to access the perktree, alternatively there is an MCM option under 'Perks' to disable Perk requirements.
Uses regular perk points. Allows to get "Spell Procs" which turns the next casted spell into near-instant cast and 50% less mana cost. Spell Procs are visible as animated golden border around spells. And the spell learn sound is played on trigger.
At start only novice and apprentice spell can consume procs and there is a 10s (configurable in mcm) cooldown in gaining a new proc after consuming one.

### Available Perks:
Note: chances and timing is configurable in MCM
#### Triggers (starting perks, no perk requirement):
* Cast On Concentration: Casting a concentration spell for 6 seconds can trigger Novice or Apprentice spell procs. Requires a Magic Skill of at least 25.
* Cast On Block: Timed Blocking can trigger Novice or Apprentice spell procs. Requires 'Block' 25.
* Cast On Crit: Critical hits can trigger Novice or Apprentice spell procs. Requires 'One-Handed' or 'Two-Handed' or 'Archery' 25.
* Cast On Backstab: Sneak Attacks can trigger Novice or Apprentice spell procs. Requires 'Sneak' 25.
* Cast On Power Attack: Power Attack hits can trigger Novice or Apprentice spell procs. Requires 'One-Handed' or 'Two-Handed' 25.
#### Advanced Perks
* Expert Battlemage (Requires at least one trigger): Spell Procs can trigger Adept and Expert spells. Reduces Cooldown by 20%. Requires 'Alteration' 50.
* Master Battlemage (Requires Expert Battlemage): Spell Procs can trigger Master spells. Reduces Cooldown by 40%. Requires 'Alteration' 75.   (Note 40% reduction in total, not additive)



