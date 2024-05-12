Scriptname SpellHotbarInitQuestScript extends Quest  

Actor Property playerref Auto

Spell Property SpellHotbar_unbind_spell Auto
Spell Property SpellHotbar_toggle_dual_cast_spell Auto

Function DEBUG_lean_spell(int formId)
	Spell sp = Game.GetFormFromFile(formId,"Skyrim.esm") as Spell
	playerref.AddSpell(sp, false)
EndFunction

Function DEBUG_lean_spell2(int formId, string pluginfile)
	Spell sp = Game.GetFormFromFile(formId,pluginfile) as Spell
	playerref.AddSpell(sp, false)
EndFunction

Function DEBUG_add_perk(int formId, string pluginfile)
	Perk p = Game.GetFormFromFile(formId, pluginfile) as Perk
	playerref.AddPerk(p)
EndFunction

Function DEBUG_Lean_Spells()

	DEBUG_lean_spell(0x12FD0) ; Firebolt
	DEBUG_lean_spell(0x1C789) ; Fireball
	DEBUG_lean_spell(0x7A82B) ; Fire Storm
	DEBUG_lean_spell(0x7E8E4) ; Blizzard
	DEBUG_lean_spell(0x7E8E5) ; Lightning Storm

	DEBUG_lean_spell2(0x043324, "Skyrim.esm")
	DEBUG_lean_spell2(0x05AD5C, "Skyrim.esm")
	DEBUG_lean_spell2(0x043323, "Skyrim.esm")
	DEBUG_lean_spell2(0x05AD5D, "Skyrim.esm")
	DEBUG_lean_spell2(0x017731, "Dragonborn.esm")
	DEBUG_lean_spell2(0x0211EE, "Skyrim.esm")
	DEBUG_lean_spell2(0x051B16, "Skyrim.esm")
	DEBUG_lean_spell2(0x01A4CC, "Skyrim.esm")
	DEBUG_lean_spell2(0x109111, "Skyrim.esm")
	DEBUG_lean_spell2(0x05D175, "Skyrim.esm")
	DEBUG_lean_spell2(0x0177AF, "Dragonborn.esm")
	DEBUG_lean_spell2(0x0211EF, "Skyrim.esm")
	DEBUG_lean_spell2(0x05AD5E, "Skyrim.esm")
	DEBUG_lean_spell2(0x05AD5F, "Skyrim.esm")
	DEBUG_lean_spell2(0x0CDB70, "Skyrim.esm")
	DEBUG_lean_spell2(0x0B62E6, "Skyrim.esm")
	DEBUG_lean_spell2(0x0DA746, "Skyrim.esm")

	;Vampire Lord
	DEBUG_lean_spell2(0x00283b, "Dawnguard.esm")
	DEBUG_lean_spell2(0x01462a, "Dawnguard.esm")

	;Werewolf
	DEBUG_lean_spell(0x92C48)
	DEBUG_lean_spell(0xA1A3E)
	
	;Dual Cast perks
	DEBUG_add_perk(0x153CD, "Skyrim.esm")
	DEBUG_add_perk(0x153CE, "Skyrim.esm")
	DEBUG_add_perk(0x153CF, "Skyrim.esm")
	DEBUG_add_perk(0x153D0, "Skyrim.esm")
	DEBUG_add_perk(0x153D1, "Skyrim.esm")

	;Hierophant absolution - channel with time
	DEBUG_lean_spell2(0x822, "Ghostlight.esl")
EndFunction

Event OnInit()
	If !playerref.HasSpell(SpellHotbar_unbind_spell)
		playerref.AddSpell(SpellHotbar_unbind_spell, false)
	EndIf

	If !playerref.HasSpell(SpellHotbar_toggle_dual_cast_spell)
		playerref.AddSpell(SpellHotbar_toggle_dual_cast_spell, false)
	EndIf

	;TODO disable this for release
	DEBUG_Lean_Spells()
EndEvent