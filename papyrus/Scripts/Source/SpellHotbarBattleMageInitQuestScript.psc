Scriptname SpellHotbarBattleMageInitQuestScript extends Quest  

Actor Property playerref Auto
Spell Property SpellHotbar_BattleMage_OpenSkillMenuPower Auto

Event OnInit()
	If !playerref.HasSpell(SpellHotbar_BattleMage_OpenSkillMenuPower)
		playerref.AddSpell(SpellHotbar_BattleMage_OpenSkillMenuPower, false)
	EndIf
EndEvent