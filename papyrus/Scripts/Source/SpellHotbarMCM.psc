Scriptname SpellHotbarMCM extends SKI_ConfigBase
  
;import PO3_SKSEFunctions

SpellHotbarInitQuestScript Property Main Auto

int[] oid_spellkeybinds

string[] inherit_options
string[] known_presets
string[] known_bar_presets
string[] bar_show_options
string[] bar_show_options_transformed
string[] text_show_options
string[] anchor_points
string[] input_modes

string presets_root = "Data/SKSE/Plugins/SpellHotbar/presets/"
string bars_root = "Data/SKSE/Plugins/SpellHotbar/bars/"
string user_presets_path = "SpellHotbar/presets/"
string user_bars_path = "SpellHotbar/bars/"
int presets_json_format = 2

; SCRIPT VERSION
int function GetVersion()
	return 1
endFunction

Event OnConfigInit()
    ModName = "Spell Hotbar"
    Pages = new String[6]
    Pages[0] = "Keybinds"
    Pages[1] = "Settings"
    Pages[2] = "Bars"
	Pages[3] = "Presets"
	Pages[4] = "Spells"
	Pages[5] = "Util"

    inherit_options = new String[3]
    inherit_options[0] = "Default"
    inherit_options[1] = "Same Modifier"
    inherit_options[2] = "No Inheritance"

	bar_show_options = new String[6]
	bar_show_options[0] = "Always"
	bar_show_options[1] = "Never"
	bar_show_options[2] = "Combat"
	bar_show_options[3] = "Drawn Weapon"
	bar_show_options[4] = "Combat or Drawn"
	bar_show_options[5] = "Combat And Drawn"

	bar_show_options_transformed = new String[3]
	bar_show_options_transformed[0] = "Always"
	bar_show_options_transformed[1] = "Never"
	bar_show_options_transformed[2] = "Combat"

	text_show_options = new String[3]
	text_show_options[0] = "Never"
	text_show_options[1] = "Fade"
	text_show_options[2] = "Always"

	anchor_points = new String[9]
	anchor_points[0] = "Bottom"
	anchor_points[1] = "Left"
	anchor_points[2] = "Top"
	anchor_points[3] = "Right"
	anchor_points[4] = "Bottom Left"
	anchor_points[5] = "Top Left"
	anchor_points[6] = "Bottom Right"
	anchor_points[7] = "Top Right"
	anchor_points[8] = "Center"

	input_modes = new String[3]
	input_modes[0] = "Cast Directly"
	input_modes[1] = "Equip"
	input_modes[2] = "Oblivion-Style"
EndEvent

; reinit config on update
Event OnVersionUpdate(int a_version)
	OnConfigInit() ; just rerun init after a version change
EndEvent

Event OnPageReset(string page)
    if page == "Keybinds"
        oid_spellkeybinds = new int[21]
        SetCursorFillMode(LEFT_TO_RIGHT)
        AddHeaderOption("Skill Bindings")
        AddHeaderOption("")

        int i = 0
        While (i < 6)
            int key_val = SpellHotbar.getKeyBind(i)
			int key_val2 = SpellHotbar.getKeyBind(i+6)
            oid_spellkeybinds[i]=AddKeyMapOption("Hotbar Skill "+(i+1), key_val) 
            oid_spellkeybinds[i+6]=AddKeyMapOption("Hotbar Skill "+(i+7), key_val2)
            i += 1
        EndWhile
        
        AddHeaderOption("Menu Bindings")
        AddHeaderOption("")

		; the 13 and 12 are intentionally swapped, internally next is saved before, but in the gui it will be swapped
		oid_spellkeybinds[13] = AddKeyMapOption("Previous Bar", SpellHotbar.getKeyBind(13))
		oid_spellkeybinds[12] = AddKeyMapOption("Next Bar", SpellHotbar.getKeyBind(12))

		AddHeaderOption("Modifier Bindings")
        AddHeaderOption("")
		oid_spellkeybinds[14] = AddKeyMapOption("Bar Modifier 1", SpellHotbar.getKeyBind(14))
		oid_spellkeybinds[15] = AddKeyMapOption("Bar Modifier 2", SpellHotbar.getKeyBind(15))
		oid_spellkeybinds[16] = AddKeyMapOption("Bar Modifier 3", SpellHotbar.getKeyBind(16))
		oid_spellkeybinds[17] = AddKeyMapOption("Dual Casting Modifier", SpellHotbar.getKeyBind(17))
		oid_spellkeybinds[18] = AddKeyMapOption("Show Bar Modifier", SpellHotbar.getKeyBind(18))
		AddEmptyOption()

		AddHeaderOption("Oblivion Mode Bindings")
		AddHeaderOption("")
		oid_spellkeybinds[19] = AddKeyMapOption("Cast Spell", SpellHotbar.getKeyBind(19))
		oid_spellkeybinds[20] = AddKeyMapOption("Use Potion", SpellHotbar.getKeyBind(20))

    ElseIf page == "Settings"
        AddHeaderOption("Bar Configuration")
		AddHeaderOption("")

		AddMenuOptionST("InputMode", "SpellHotbar Mode", input_modes[SpellHotbar.getInputMode()])
		AddEmptyOption();

        AddToggleOptionST("DisableNonModBar", "Disable Non-Modifier Bar", SpellHotbar.isNonModBarDisabled())

        AddSliderOptionST("SlotsPerBar", "Slots per Bar", SpellHotbar.getNumberOfSlots() as float)

		AddMenuOptionST("BarShowOptions", "Show HUD Bar", bar_show_options[SpellHotbar.getHudBarShowMode()])
		AddMenuOptionST("BarTextShowOptions", "Show Bar Text", text_show_options[SpellHotbar.getTextShowMode()])
		
		AddMenuOptionST("BarShowOptionsVL", "Show HUD Bar (Vampire Lord)", bar_show_options_transformed[SpellHotbar.getHudBarShowModeVampireLord()])
		AddMenuOptionST("BarShowOptionsWW", "Show HUD Bar (Werewolf)", bar_show_options_transformed[SpellHotbar.getHudBarShowModeWerewolf()])
	
		AddToggleOptionST("UseDefaultBarWhenSheathed", "Use Default bar when Sheated", SpellHotbar.isDefaultBarWhenSheathed())
        AddToggleOptionST("DisableMenuRendering", "Disable Menu Rendering", SpellHotbar.isDisableMenuRendering())

        AddHeaderOption("Bar Positioning")
		AddHeaderOption("")

        AddSliderOptionST("SlotScale", "Slot Scale", SpellHotbar.getSlotScale(), "{2}")
        AddSliderOptionST("BarOffsetX", "Offset X", SpellHotbar.getOffsetX(false))
		AddSliderOptionST("SlotSpacing", "Slot Spacing", SpellHotbar.getSlotSpacing())
        AddSliderOptionST("BarOffsetY", "Offset Y", SpellHotbar.getOffsetY(false))

		AddMenuOptionST("BarAnchorPoint", "Anchor Point", anchor_points[SpellHotbar.getBarAnchorPoint()])
		AddEmptyOption()

		AddHeaderOption("Gameplay")
		AddHeaderOption("")
		AddSliderOptionST("PotionGCD", "Potion GCD", SpellHotbar.getPotionGCD(), "{2}")
		AddEmptyOption()

		AddHeaderOption("Oblivion Mode Bar")
		AddHeaderOption("")

		AddSliderOptionST("OblivionSlotScale", "Slot Scale", SpellHotbar.getOblivionSlotScale(), "{2}")
        AddSliderOptionST("OblivionBarOffsetX", "Offset X", SpellHotbar.getOblivionOffsetX(false))
		AddSliderOptionST("OblivionSlotSpacing", "Slot Spacing", SpellHotbar.getOblivionSlotSpacing())
        AddSliderOptionST("OblivionBarOffsetY", "Offset Y", SpellHotbar.getOblivionOffsetY(false))

		AddMenuOptionST("OblivionBarAnchorPoint", "Anchor Point", anchor_points[SpellHotbar.getOblivionBarAnchorPoint()])
		AddToggleOptionST("OblivionPowerSlot", "Show Power", SpellHotbar.isShowOblivionBarPower())


    ElseIf (page == "Bars")

        ;bar_names[0] = "Sneak"
        ;bar_ids[0] = 1296124239

		AddHeaderOption("Sneak Bar")
		AddEmptyOption()
		AddToggleOptionST("MAIN_BAR_SNEAK_Enabled", "Enabled", SpellHotbar.getBarEnabled(1296124239))
		AddEmptyOption()
		AddMenuOptionST("MAIN_BAR_SNEAK_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode(1296124239)])
		AddEmptyOption()

		AddHeaderOption("Melee Bar")
		AddHeaderOption("Melee Sneak Bar")
		AddToggleOptionST("MELEE_Enabled", "Enabled", SpellHotbar.getBarEnabled(1296387141))
		AddToggleOptionST("MELEE_SNEAK_Enabled", "Enabled", SpellHotbar.getBarEnabled(1296387142))
		AddMenuOptionST("MELEE_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode(1296387141)])
		AddMenuOptionST("MELEE_SNEAK_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode(1296387142)])

		AddHeaderOption("1H-Shield Bar")
		AddHeaderOption("1H-Shield Sneak Bar")
		AddToggleOptionST("S_1H_SHIELD_Enabled", "Enabled", SpellHotbar.getBarEnabled(826823492))
		AddToggleOptionST("S_1H_SHIELD_SNEAK_Enabled", "Enabled", SpellHotbar.getBarEnabled(826823493))
		AddMenuOptionST("S_1H_SHIELD_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode(826823492)])
		AddMenuOptionST("S_1H_SHIELD_SNEAK_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode(826823493)])

		AddHeaderOption("1H-Spell Bar")
		AddHeaderOption("1H-Spell Sneak Bar")
		AddToggleOptionST("S_1H_SPELL_Enabled", "Enabled", SpellHotbar.getBarEnabled(826823504))
		AddToggleOptionST("S_1H_SPELL_SNEAK_Enabled", "Enabled", SpellHotbar.getBarEnabled(826823505))
		AddMenuOptionST("S_1H_SPELL_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode(826823504)])
		AddMenuOptionST("S_1H_SPELL_SNEAK_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode(826823505)])

		AddHeaderOption("Dual Wield Bar")
		AddHeaderOption("Dual Wield Sneak Bar")
		AddToggleOptionST("DUAL_WIELD_Enabled", "Enabled", SpellHotbar.getBarEnabled(826819671))
		AddToggleOptionST("DUAL_WIELD_SNEAK_Enabled", "Enabled", SpellHotbar.getBarEnabled(826819672))
		AddMenuOptionST("DUAL_WIELD_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode(826819671)])
		AddMenuOptionST("DUAL_WIELD_SNEAK_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode(826819672)])

		AddHeaderOption("Two-Handed Bar")
		AddHeaderOption("Two-Handed Sneak Bar")
		AddToggleOptionST("TWO_HANDED_Enabled", "Enabled", SpellHotbar.getBarEnabled(843599428))
		AddToggleOptionST("TWO_HANDED_SNEAK_Enabled", "Enabled", SpellHotbar.getBarEnabled(843599429))
		AddMenuOptionST("TWO_HANDED_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode(843599428)])
		AddMenuOptionST("TWO_HANDED_SNEAK_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode(843599429)])

		AddHeaderOption("Ranged Bar")
		AddHeaderOption("Ranged Sneak Bar")
		AddToggleOptionST("RANGED_Enabled", "Enabled", SpellHotbar.getBarEnabled(1380861764))
		AddToggleOptionST("RANGED_SNEAK_Enabled", "Enabled", SpellHotbar.getBarEnabled(1380861765))
		AddMenuOptionST("RANGED_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode(1380861764)])
		AddMenuOptionST("RANGED_SNEAK_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode(1380861765)])

		AddHeaderOption("Magic Bar")
		AddHeaderOption("Magic Sneak Bar")
		AddToggleOptionST("MAGIC_Enabled", "Enabled", SpellHotbar.getBarEnabled(1296123715))
		AddToggleOptionST("MAGIC_SNEAK_Enabled", "Enabled", SpellHotbar.getBarEnabled(1296123716))
		AddMenuOptionST("MAGIC_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode(1296123715)])
		AddMenuOptionST("MAGIC_SNEAK_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode(1296123716)])

	ElseIf (page == "Presets")
		checkPresets()
		checkBarPresets()

		AddHeaderOption("Save")
		AddHeaderOption("Load")

		AddInputOptionST("SavePresetState", "Save Config as...", "")
		AddMenuOptionST("LoadPresetState", "Load Config", "")
		AddInputOptionST("SaveBarsState", "Save Bars as...", "")
		AddMenuOptionST("LoadBarsState", "Load Bars", "")

	ElseIf (page == "Spells")
		AddToggleOptionST("OpenSpellEditor", "Open Spell Editor ...", false);
		AddEmptyOption()

	ElseIf (page == "Util")
		AddToggleOptionST("ReloadResourcesState", "Reload Resources ...", false);
		AddEmptyOption()
		AddToggleOptionST("ReloadSpellDataState", "Reload Spell Data ...", false);
		AddEmptyOption()
		AddToggleOptionST("ClearBarsState", "Clear all Bars", false);
		AddEmptyOption()
		AddToggleOptionST("ShowDragBar", "Drag Main Bar", false)
		AddToggleOptionST("ShowOblivionModeDragBar", "Drag Oblivion Mode Bar", false)
	EndIf
EndEvent

State InputMode
	Event OnMenuOpenST()
		SetMenuDialogOptions(input_modes)
		SetMenuDialogStartIndex(SpellHotbar.getInputMode())
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(input_modes[SpellHotbar.setInputMode(index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Set mode of SpellHotbar")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(input_modes[SpellHotbar.setInputMode(0)])
	EndEvent
EndState

State ShowDragBar
    Event OnSelectST()
        SetToggleOptionValueST(false)
		SpellHotbar.showDragBar(0)
    EndEvent
    Event OnHighlightST()
        SetInfoText("Show a dragable Bar for positioning")
    EndEvent
EndState

State ShowOblivionModeDragBar
    Event OnSelectST()
        SetToggleOptionValueST(false)
		SpellHotbar.showDragBar(1)
    EndEvent
    Event OnHighlightST()
        SetInfoText("Show a dragable Bar for oblivion mode bar positioning")
    EndEvent
EndState

State OpenSpellEditor
    Event OnSelectST()
        SetToggleOptionValueST(false)
		SpellHotbar.openSpellEditor()
    EndEvent
    Event OnHighlightST()
        SetInfoText("Open Editor to change Spell Settings and Icons")
    EndEvent
EndState

State ReloadResourcesState
    Event OnSelectST()
        SetToggleOptionValueST(false)
		if ShowMessage("Reload all resources from disk?", true, "$Yes", "$No")
			SpellHotbar.reloadResources()
		EndIf
    EndEvent
    Event OnHighlightST()
        SetInfoText("Reload Resouces (textures) from disk, see 'SpellHotbar.log' for errors")
    EndEvent
EndState

State ReloadSpellDataState
    Event OnSelectST()
        SetToggleOptionValueST(false)
		if ShowMessage("Reload spell data from disk?", true, "$Yes", "$No")
			SpellHotbar.reloadData()
		EndIf
    EndEvent
    Event OnHighlightST()
        SetInfoText("Reload spell data from disk, see 'SpellHotbar.log' for errors")
    EndEvent
EndState

State ClearBarsState
    Event OnSelectST()
        SetToggleOptionValueST(false)
		if ShowMessage("This will clear all slotted skills from all bars, continue?", true, "$Yes", "$No")
			SpellHotbar.clearBars()
		EndIf
    EndEvent
    Event OnHighlightST()
        SetInfoText("Clear all slotted skills form all bars.")
    EndEvent
EndState

Function checkPresets()
	known_presets = SpellHotbar.getConfigPresets()
EndFunction

Function checkBarPresets()
	known_bar_presets = SpellHotbar.getBarsPresets()
EndFunction

State SavePresetState
	Event OnInputOpenST()
		SetInputOptionValueST("")
	EndEvent
	Event OnInputAcceptST(string name)
		if name != ""
			if ShowMessage("Save current settings as '" + name +"'?", true, "$Yes", "$No")
				if saveSettingsAsPreset(name)
					SetInputOptionValueST(name)
				EndIf
			Endif
		EndIf
	EndEvent
	Event OnDefaultST()
        SetInputOptionValueST("")
    EndEvent
    Event OnHighlightST()
        SetInfoText("Save current settings as .json preset")
    EndEvent
EndState

State LoadPresetState
	Event OnMenuOpenST()
		checkPresets()
		SetMenuDialogOptions(known_presets)
		SetMenuDialogStartIndex(0)
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		if index > 0
			string preset = known_presets[index]
			if ShowMessage("Load settings from preset '" + preset +"'?", true, "$Yes", "$No")
				if (loadSettingsFromPreset(preset, True, True))
					SetMenuOptionValueST(preset)
				EndIf
			EndIf
		EndIf
	EndEvent
	Event OnHighlightST()
		SetInfoText("Load settings from a Preset")
	EndEvent
EndState

State SaveBarsState
	Event OnInputOpenST()
		SetInputOptionValueST("")
	EndEvent
	Event OnInputAcceptST(string name)
		if name != ""
			if ShowMessage("Save current bars as '" + name +".json'?", true, "$Yes", "$No")
				string out_file = JContainers.userDirectory() + user_bars_path + name + ".json"
				bool exists = SpellHotbar.fileExists(out_file)
				if !exists || ShowMessage("'" + name +".json' exists, overwrite?", true, "$Yes", "$No")
					if SpellHotbar.saveBarsToFile(out_file)
						SetInputOptionValueST(name)
					else
						ShowMessage("Error saving bars, see SpellHotbar.log for details", false)
					EndIf
				EndIf
			Endif
		EndIf
	EndEvent
	Event OnDefaultST()
        SetInputOptionValueST("")
    EndEvent
    Event OnHighlightST()
        SetInfoText(".json is added automatically, only enter filenname")
    EndEvent
EndState

State LoadBarsState
	Event OnMenuOpenST()
		checkBarPresets()
		SetMenuDialogOptions(known_bar_presets)
		SetMenuDialogStartIndex(0)
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		if index > 0
			string preset = known_bar_presets[index]
			string file_path_user = JContainers.userDirectory() + user_bars_path + preset
			string file_path_mod = bars_root + preset
			if ShowMessage("Load bars from '" + preset +"'?", true, "$Yes", "$No")
				if (SpellHotbar.loadBarsFromFile(file_path_mod, file_path_user))
					SetMenuOptionValueST(preset)
				Else
					ShowMessage("Error loading bars, see SpellHotbar.log for details", false)
				EndIf
			EndIf
		EndIf
	EndEvent
	Event OnHighlightST()
		SetInfoText("Load bars from file")
	EndEvent
EndState

bool Function loadSettingsFromPreset(string preset_name, bool show_errors, bool include_user_dir)
	bool success = SpellHotbar.loadConfig(preset_name, include_user_dir)
	If (!success && show_errors )
		ShowMessage("There has been a problem while loading the file, no settings changed", false)
	EndIf

	return success
EndFunction

bool Function saveSettingsAsPreset(string preset_name)
	bool success = SpellHotbar.saveConfig(preset_name)
	If (!success)
		ShowMessage("There has been a problem while saving the file", false)
	EndIf
	return success
EndFunction

State OblivionSlotScale
    Event OnSliderOpenST()
        SetSliderDialogStartValue(SpellHotbar.getOblivionSlotScale())
        SetSliderDialogDefaultValue(1.0)
        SetSliderDialogRange(0.01, 5.0)
        SetSliderDialogInterval(0.01)
    EndEvent
    Event OnSliderAcceptST(float a_value)
        SetSliderOptionValueST(SpellHotbar.setOblivionSlotScale(a_value), "{2}");
    EndEvent
    Event OnHighlightST()
        SetInfoText("Change the size of the Oblivion Bar slots");
    EndEvent
EndState

State OblivionSlotSpacing
    Event OnSliderOpenST()
        SetSliderDialogStartValue(SpellHotbar.getOblivionSlotSpacing())
        SetSliderDialogDefaultValue(8.0)
        SetSliderDialogRange(0.0, 50.0)
        SetSliderDialogInterval(1.0)
    EndEvent
    Event OnSliderAcceptST(float a_value)
        SetSliderOptionValueST(SpellHotbar.setOblivionSlotSpacing(a_value));
    EndEvent
    Event OnHighlightST()
        SetInfoText("Change the spacing of the Oblivion Bar slots");
    EndEvent
EndState

State OblivionBarOffsetX
    Event OnSliderOpenST()
        SetSliderDialogStartValue(SpellHotbar.getOblivionOffsetX(false))
        SetSliderDialogDefaultValue(0.0)
        SetSliderDialogRange(-2000.0, 2000.0)
        SetSliderDialogInterval(1.0)
    EndEvent
    Event OnSliderAcceptST(float a_value)
        SetSliderOptionValueST(SpellHotbar.setOblivionOffsetX(a_value, false));
    EndEvent
    Event OnHighlightST()
        SetInfoText("Move the oblivion bar in X direction");
    EndEvent
EndState

State OblivionBarOffsetY
    Event OnSliderOpenST()
        SetSliderDialogStartValue(SpellHotbar.getOblivionOffsetY(false))
        SetSliderDialogDefaultValue(0.0)
        SetSliderDialogRange(-2000.0, 2000.0)
        SetSliderDialogInterval(1.0)
    EndEvent
    Event OnSliderAcceptST(float a_value)
        SetSliderOptionValueST(SpellHotbar.setOblivionOffsetY(a_value, false));
    EndEvent
    Event OnHighlightST()
        SetInfoText("Move the oblivion bar in Y direction");
    EndEvent
EndState

State OblivionBarAnchorPoint
	Event OnMenuOpenST()
		SetMenuDialogOptions(anchor_points)
		SetMenuDialogStartIndex(SpellHotbar.getOblivionBarAnchorPoint())
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(anchor_points[SpellHotbar.setOblivionBarAnchorPoint(index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Set anchor point for oblivion bar position offset.")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(anchor_points[SpellHotbar.setOblivionBarAnchorPoint(0)])
	EndEvent
EndState

State OblivionPowerSlot
    Event OnSelectST()
        SetToggleOptionValueST(SpellHotbar.toggleShowOblivionBarPower())
    EndEvent
    Event OnHighlightST()
        SetInfoText("Show the current equipped power on oblivion bar");
    EndEvent
EndState


State SlotsPerBar
    Event OnSliderOpenST()
        SetSliderDialogStartValue(SpellHotbar.getNumberOfSlots() as float)
        SetSliderDialogDefaultValue(12.0)
        SetSliderDialogRange(1.0, 12.0)
        SetSliderDialogInterval(1.0)
    EndEvent
    Event OnSliderAcceptST(float a_value)
        int slots = SpellHotbar.setNumberOfSlots(a_value as int);
        SetSliderOptionValueST(slots as float);
    EndEvent
    Event OnHighlightST()
        SetInfoText("Limit the number of Slots per Bar");
    EndEvent
EndState

State BarShowOptions
	Event OnMenuOpenST()
		SetMenuDialogOptions(bar_show_options)
		SetMenuDialogStartIndex(SpellHotbar.getHudBarShowMode())
		SetMenuDialogDefaultIndex(4)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(bar_show_options[SpellHotbar.setHudBarShowMode(index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Chose on which conditions the HUD bar is shown.")
	EndEvent
	Event OnDefaultST()
		SetMenuOptionValueST(bar_show_options[SpellHotbar.setHudBarShowMode(4)])
	EndEvent
EndState

State BarTextShowOptions
	Event OnMenuOpenST()
		SetMenuDialogOptions(text_show_options)
		SetMenuDialogStartIndex(SpellHotbar.getTextShowMode())
		SetMenuDialogDefaultIndex(1)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(text_show_options[SpellHotbar.setTextShowMode(index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Chose when the HUD bar text is shown.")
	EndEvent
	Event OnDefaultST()
		SetMenuOptionValueST(text_show_options[SpellHotbar.setTextShowMode(1)])
	EndEvent
EndState

State BarShowOptionsVL
	Event OnMenuOpenST()
		SetMenuDialogOptions(bar_show_options_transformed)
		SetMenuDialogStartIndex(SpellHotbar.getHudBarShowModeVampireLord())
		SetMenuDialogDefaultIndex(2)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(bar_show_options_transformed[SpellHotbar.setHudBarShowModeVampireLord(index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Chose on which conditions the HUD bar is shown when transformed as Vampire Lord")
	EndEvent
	Event OnDefaultST()
		SetMenuOptionValueST(bar_show_options_transformed[SpellHotbar.setHudBarShowModeVampireLord(2)])
	EndEvent
EndState

State BarShowOptionsWW
	Event OnMenuOpenST()
		SetMenuDialogOptions(bar_show_options_transformed)
		SetMenuDialogStartIndex(SpellHotbar.getHudBarShowModeWerewolf())
		SetMenuDialogDefaultIndex(1)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(bar_show_options_transformed[SpellHotbar.setHudBarShowModeWerewolf(index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Chose on which conditions the HUD bar is shown when transformed as Werewolf")
	EndEvent
	Event OnDefaultST()
		SetMenuOptionValueST(bar_show_options_transformed[SpellHotbar.setHudBarShowModeWerewolf(1)])
	EndEvent
EndState

State PotionGCD
    Event OnSliderOpenST()
        SetSliderDialogStartValue(SpellHotbar.getPotionGCD())
        SetSliderDialogDefaultValue(1.0)
        SetSliderDialogRange(0.1, 10.0)
        SetSliderDialogInterval(0.05)
    EndEvent
    Event OnSliderAcceptST(float a_value)
        SetSliderOptionValueST(SpellHotbar.setPotionGCD(a_value), "{2}");
    EndEvent
    Event OnHighlightST()
        SetInfoText("Change the Hotbar Global Cooldown when drinking potions");
    EndEvent
	Event OnDefaultST()
		SetSliderOptionValueST(1.0, "{2}")
	EndEvent
EndState

State SlotScale
    Event OnSliderOpenST()
        SetSliderDialogStartValue(SpellHotbar.getSlotScale())
        SetSliderDialogDefaultValue(1.0)
        SetSliderDialogRange(0.01, 5.0)
        SetSliderDialogInterval(0.01)
    EndEvent
    Event OnSliderAcceptST(float a_value)
        SetSliderOptionValueST(SpellHotbar.setSlotScale(a_value), "{2}");
    EndEvent
    Event OnHighlightST()
        SetInfoText("Change the size of the skill slots");
    EndEvent
EndState

State SlotSpacing
    Event OnSliderOpenST()
        SetSliderDialogStartValue(SpellHotbar.getSlotSpacing())
        SetSliderDialogDefaultValue(8.0)
        SetSliderDialogRange(0.0, 50.0)
        SetSliderDialogInterval(1.0)
    EndEvent
    Event OnSliderAcceptST(float a_value)
        SetSliderOptionValueST(SpellHotbar.setSlotSpacing(a_value));
    EndEvent
    Event OnHighlightST()
        SetInfoText("Change the spacing of the skill slots");
    EndEvent
EndState

State BarOffsetX
    Event OnSliderOpenST()
        SetSliderDialogStartValue(SpellHotbar.getOffsetX(false))
        SetSliderDialogDefaultValue(0.0)
        SetSliderDialogRange(-2000.0, 2000.0)
        SetSliderDialogInterval(1.0)
    EndEvent
    Event OnSliderAcceptST(float a_value)
        SetSliderOptionValueST(SpellHotbar.setOffsetX(a_value, false));
    EndEvent
    Event OnHighlightST()
        SetInfoText("Move the bar in X direction");
    EndEvent
EndState

State BarOffsetY
    Event OnSliderOpenST()
        SetSliderDialogStartValue(SpellHotbar.getOffsetY(false))
        SetSliderDialogDefaultValue(0.0)
        SetSliderDialogRange(-2000.0, 2000.0)
        SetSliderDialogInterval(1.0)
    EndEvent
    Event OnSliderAcceptST(float a_value)
        SetSliderOptionValueST(SpellHotbar.setOffsetY(a_value, false));
    EndEvent
    Event OnHighlightST()
        SetInfoText("Move the bar in Y direction");
    EndEvent
EndState

State DisableNonModBar
    Event OnSelectST()
        SetToggleOptionValueST(SpellHotbar.toggleDisableNonModBar())
    EndEvent
    Event OnHighlightST()
        SetInfoText("Disable Hotbars without modifier");
    EndEvent
EndState

State UseDefaultBarWhenSheathed
    Event OnSelectST()
        SetToggleOptionValueST(SpellHotbar.toggleDefaultBarWhenSheathed())
    EndEvent
    Event OnHighlightST()
        SetInfoText("Use Default bar when no weapon/magic is drawn");
    EndEvent
EndState

State DisableMenuRendering
    Event OnSelectST()
        SetToggleOptionValueST(SpellHotbar.toggleDisableMenuRendering())
    EndEvent
    Event OnHighlightST()
        SetInfoText("Disable the rendering of menus for slotting spells, hotkeys and slotting still works");
    EndEvent
EndState

Event OnOptionKeyMapChange(int option, int keyCode, string conflictControl, string conflictName)
    if keyCode > 1

		bool continue = True
		int i=0
		While (i < oid_spellkeybinds.Length) && continue
			if (option == oid_spellkeybinds[i])
				int actualBind = SpellHotbar.setKeyBind(i, keyCode)
				SetKeyMapOptionValue(oid_spellkeybinds[i], actualBind)
				continue = False ; we found the bind, stop loop
			EndIf
			i += 1
		EndWhile
		; update all spell keys, as multiple can change when conflicts happen (old one unbinded)
		i=0
		While (i < oid_spellkeybinds.Length)
			int actualBind = SpellHotbar.getKeyBind(i)
			SetKeyMapOptionValue(oid_spellkeybinds[i], actualBind)
			i += 1
		EndWhile

    EndIf
EndEvent

Event OnOptionDefault(int option)
    int i = 0
    bool continue = True
    While (i < oid_spellkeybinds.Length) && continue
        if (option == oid_spellkeybinds[i])
			SpellHotbar.setKeyBind(i, -1)
            SetKeyMapOptionValue(oid_spellkeybinds[i], -1)
            continue = False ; we found the bind, stop loop
        EndIf
        i += 1
    EndWhile
EndEvent

Event OnOptionHighlight(int option)
    int i = 0
    bool continue = True
    While (i < oid_spellkeybinds.Length) && continue
        if (option == oid_spellkeybinds[i])
            SetInfoText("Reset to Default to unbind")
            continue = False ; we found the bind, stop loop
        EndIf
        i += 1
    EndWhile
EndEvent

State BarAnchorPoint
	Event OnMenuOpenST()
		SetMenuDialogOptions(anchor_points)
		SetMenuDialogStartIndex(SpellHotbar.getBarAnchorPoint())
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(anchor_points[SpellHotbar.setBarAnchorPoint(index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Set anchor point for bar position offset.")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(anchor_points[SpellHotbar.setBarAnchorPoint(0)])
	EndEvent
EndState

; Bar toggle states
; Auto generated from pythons script
State MAIN_BAR_SNEAK_Enabled
	Event OnSelectST()
		SetToggleOptionValueST(SpellHotbar.toggleBarEnabled(1296124239))
	EndEvent
	Event OnHighlightST()
		SetInfoText("Disabling specific Hotbars will hide them in UI and avoid them during inheritence");
	EndEvent
EndState
State MAIN_BAR_SNEAK_Inherit
	Event OnMenuOpenST()
		SetMenuDialogOptions(inherit_options)
		SetMenuDialogStartIndex(SpellHotbar.getInheritMode(1296124239))
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(1296124239, index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Default: inherit from non-modifier bar first; Same Modifier: Inherit from parent bar with same modifier instead of non-modifier bar")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(1296124239, 0)])
	EndEvent
EndState

State MELEE_Enabled
	Event OnSelectST()
		SetToggleOptionValueST(SpellHotbar.toggleBarEnabled(1296387141))
	EndEvent
	Event OnHighlightST()
		SetInfoText("Disabling specific Hotbars will hide them in UI and avoid them during inheritence");
	EndEvent
EndState
State MELEE_Inherit
	Event OnMenuOpenST()
		SetMenuDialogOptions(inherit_options)
		SetMenuDialogStartIndex(SpellHotbar.getInheritMode(1296387141))
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(1296387141, index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Default: inherit from non-modifier bar first; Same Modifier: Inherit from parent bar with same modifier instead of non-modifier bar")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(1296387141, 0)])
	EndEvent
EndState

State MELEE_SNEAK_Enabled
	Event OnSelectST()
		SetToggleOptionValueST(SpellHotbar.toggleBarEnabled(1296387142))
	EndEvent
	Event OnHighlightST()
		SetInfoText("Disabling specific Hotbars will hide them in UI and avoid them during inheritence");
	EndEvent
EndState
State MELEE_SNEAK_Inherit
	Event OnMenuOpenST()
		SetMenuDialogOptions(inherit_options)
		SetMenuDialogStartIndex(SpellHotbar.getInheritMode(1296387142))
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(1296387142, index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Default: inherit from non-modifier bar first; Same Modifier: Inherit from parent bar with same modifier instead of non-modifier bar")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(1296387142, 0)])
	EndEvent
EndState

State S_1H_SHIELD_Enabled
	Event OnSelectST()
		SetToggleOptionValueST(SpellHotbar.toggleBarEnabled(826823492))
	EndEvent
	Event OnHighlightST()
		SetInfoText("Disabling specific Hotbars will hide them in UI and avoid them during inheritence");
	EndEvent
EndState
State S_1H_SHIELD_Inherit
	Event OnMenuOpenST()
		SetMenuDialogOptions(inherit_options)
		SetMenuDialogStartIndex(SpellHotbar.getInheritMode(826823492))
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(826823492, index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Default: inherit from non-modifier bar first; Same Modifier: Inherit from parent bar with same modifier instead of non-modifier bar")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(826823492, 0)])
	EndEvent
EndState

State S_1H_SHIELD_SNEAK_Enabled
	Event OnSelectST()
		SetToggleOptionValueST(SpellHotbar.toggleBarEnabled(826823493))
	EndEvent
	Event OnHighlightST()
		SetInfoText("Disabling specific Hotbars will hide them in UI and avoid them during inheritence");
	EndEvent
EndState
State S_1H_SHIELD_SNEAK_Inherit
	Event OnMenuOpenST()
		SetMenuDialogOptions(inherit_options)
		SetMenuDialogStartIndex(SpellHotbar.getInheritMode(826823493))
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(826823493, index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Default: inherit from non-modifier bar first; Same Modifier: Inherit from parent bar with same modifier instead of non-modifier bar")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(826823493, 0)])
	EndEvent
EndState

State S_1H_SPELL_Enabled
	Event OnSelectST()
		SetToggleOptionValueST(SpellHotbar.toggleBarEnabled(826823504))
	EndEvent
	Event OnHighlightST()
		SetInfoText("Disabling specific Hotbars will hide them in UI and avoid them during inheritence");
	EndEvent
EndState
State S_1H_SPELL_Inherit
	Event OnMenuOpenST()
		SetMenuDialogOptions(inherit_options)
		SetMenuDialogStartIndex(SpellHotbar.getInheritMode(826823504))
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(826823504, index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Default: inherit from non-modifier bar first; Same Modifier: Inherit from parent bar with same modifier instead of non-modifier bar")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(826823504, 0)])
	EndEvent
EndState

State S_1H_SPELL_SNEAK_Enabled
	Event OnSelectST()
		SetToggleOptionValueST(SpellHotbar.toggleBarEnabled(826823505))
	EndEvent
	Event OnHighlightST()
		SetInfoText("Disabling specific Hotbars will hide them in UI and avoid them during inheritence");
	EndEvent
EndState
State S_1H_SPELL_SNEAK_Inherit
	Event OnMenuOpenST()
		SetMenuDialogOptions(inherit_options)
		SetMenuDialogStartIndex(SpellHotbar.getInheritMode(826823505))
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(826823505, index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Default: inherit from non-modifier bar first; Same Modifier: Inherit from parent bar with same modifier instead of non-modifier bar")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(826823505, 0)])
	EndEvent
EndState

State DUAL_WIELD_Enabled
	Event OnSelectST()
		SetToggleOptionValueST(SpellHotbar.toggleBarEnabled(826819671))
	EndEvent
	Event OnHighlightST()
		SetInfoText("Disabling specific Hotbars will hide them in UI and avoid them during inheritence");
	EndEvent
EndState
State DUAL_WIELD_Inherit
	Event OnMenuOpenST()
		SetMenuDialogOptions(inherit_options)
		SetMenuDialogStartIndex(SpellHotbar.getInheritMode(826819671))
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(826819671, index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Default: inherit from non-modifier bar first; Same Modifier: Inherit from parent bar with same modifier instead of non-modifier bar")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(826819671, 0)])
	EndEvent
EndState

State DUAL_WIELD_SNEAK_Enabled
	Event OnSelectST()
		SetToggleOptionValueST(SpellHotbar.toggleBarEnabled(826819672))
	EndEvent
	Event OnHighlightST()
		SetInfoText("Disabling specific Hotbars will hide them in UI and avoid them during inheritence");
	EndEvent
EndState
State DUAL_WIELD_SNEAK_Inherit
	Event OnMenuOpenST()
		SetMenuDialogOptions(inherit_options)
		SetMenuDialogStartIndex(SpellHotbar.getInheritMode(826819672))
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(826819672, index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Default: inherit from non-modifier bar first; Same Modifier: Inherit from parent bar with same modifier instead of non-modifier bar")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(826819672, 0)])
	EndEvent
EndState

State TWO_HANDED_Enabled
	Event OnSelectST()
		SetToggleOptionValueST(SpellHotbar.toggleBarEnabled(843599428))
	EndEvent
	Event OnHighlightST()
		SetInfoText("Disabling specific Hotbars will hide them in UI and avoid them during inheritence");
	EndEvent
EndState
State TWO_HANDED_Inherit
	Event OnMenuOpenST()
		SetMenuDialogOptions(inherit_options)
		SetMenuDialogStartIndex(SpellHotbar.getInheritMode(843599428))
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(843599428, index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Default: inherit from non-modifier bar first; Same Modifier: Inherit from parent bar with same modifier instead of non-modifier bar")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(843599428, 0)])
	EndEvent
EndState

State TWO_HANDED_SNEAK_Enabled
	Event OnSelectST()
		SetToggleOptionValueST(SpellHotbar.toggleBarEnabled(843599429))
	EndEvent
	Event OnHighlightST()
		SetInfoText("Disabling specific Hotbars will hide them in UI and avoid them during inheritence");
	EndEvent
EndState
State TWO_HANDED_SNEAK_Inherit
	Event OnMenuOpenST()
		SetMenuDialogOptions(inherit_options)
		SetMenuDialogStartIndex(SpellHotbar.getInheritMode(843599429))
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(843599429, index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Default: inherit from non-modifier bar first; Same Modifier: Inherit from parent bar with same modifier instead of non-modifier bar")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(843599429, 0)])
	EndEvent
EndState

State RANGED_Enabled
	Event OnSelectST()
		SetToggleOptionValueST(SpellHotbar.toggleBarEnabled(1380861764))
	EndEvent
	Event OnHighlightST()
		SetInfoText("Disabling specific Hotbars will hide them in UI and avoid them during inheritence");
	EndEvent
EndState
State RANGED_Inherit
	Event OnMenuOpenST()
		SetMenuDialogOptions(inherit_options)
		SetMenuDialogStartIndex(SpellHotbar.getInheritMode(1380861764))
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(1380861764, index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Default: inherit from non-modifier bar first; Same Modifier: Inherit from parent bar with same modifier instead of non-modifier bar")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(1380861764, 0)])
	EndEvent
EndState

State RANGED_SNEAK_Enabled
	Event OnSelectST()
		SetToggleOptionValueST(SpellHotbar.toggleBarEnabled(1380861765))
	EndEvent
	Event OnHighlightST()
		SetInfoText("Disabling specific Hotbars will hide them in UI and avoid them during inheritence");
	EndEvent
EndState
State RANGED_SNEAK_Inherit
	Event OnMenuOpenST()
		SetMenuDialogOptions(inherit_options)
		SetMenuDialogStartIndex(SpellHotbar.getInheritMode(1380861765))
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(1380861765, index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Default: inherit from non-modifier bar first; Same Modifier: Inherit from parent bar with same modifier instead of non-modifier bar")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(1380861765, 0)])
	EndEvent
EndState

State MAGIC_Enabled
	Event OnSelectST()
		SetToggleOptionValueST(SpellHotbar.toggleBarEnabled(1296123715))
	EndEvent
	Event OnHighlightST()
		SetInfoText("Disabling specific Hotbars will hide them in UI and avoid them during inheritence");
	EndEvent
EndState
State MAGIC_Inherit
	Event OnMenuOpenST()
		SetMenuDialogOptions(inherit_options)
		SetMenuDialogStartIndex(SpellHotbar.getInheritMode(1296123715))
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(1296123715, index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Default: inherit from non-modifier bar first; Same Modifier: Inherit from parent bar with same modifier instead of non-modifier bar")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(1296123715, 0)])
	EndEvent
EndState

State MAGIC_SNEAK_Enabled
	Event OnSelectST()
		SetToggleOptionValueST(SpellHotbar.toggleBarEnabled(1296123716))
	EndEvent
	Event OnHighlightST()
		SetInfoText("Disabling specific Hotbars will hide them in UI and avoid them during inheritence");
	EndEvent
EndState
State MAGIC_SNEAK_Inherit
	Event OnMenuOpenST()
		SetMenuDialogOptions(inherit_options)
		SetMenuDialogStartIndex(SpellHotbar.getInheritMode(1296123716))
		SetMenuDialogDefaultIndex(0)
	EndEvent
	Event OnMenuAcceptST(int index)
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(1296123716, index)])
	EndEvent
	Event OnHighlightST()
		SetInfoText("Default: inherit from non-modifier bar first; Same Modifier: Inherit from parent bar with same modifier instead of non-modifier bar")
	EndEvent
	
	Event OnDefaultST()
		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode(1296123716, 0)])
	EndEvent
EndState
