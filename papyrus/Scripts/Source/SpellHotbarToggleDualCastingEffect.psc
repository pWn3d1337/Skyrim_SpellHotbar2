Scriptname SpellHotbarToggleDualCastingEffect extends ActiveMagicEffect  

GlobalVariable Property SpellHotbar_UseDualCasting Auto

Sound Property SoundToggleOn Auto
Sound Property SoundToggleOff Auto

Event OnEffectStart(Actor Target, Actor Caster)

    if SpellHotbar_UseDualCasting.GetValueInt() == 0
        SpellHotbar_UseDualCasting.SetValueInt(1)
        SoundToggleOn.Play(Caster)
    Else
        SpellHotbar_UseDualCasting.SetValueInt(0)
        SoundToggleOff.Play(Caster)
    EndIf
EndEvent
