Scriptname SpellHotbarOpenBattleMagePerkTree extends ActiveMagicEffect  

Event OnEffectStart(Actor Target, Actor Caster)
    CustomSkills.OpenCustomSkillMenu("SpellHotbar_Battlemage")
EndEvent
