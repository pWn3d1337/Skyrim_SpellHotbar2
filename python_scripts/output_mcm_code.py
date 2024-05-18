"""
This script creates the MCM papyrus source code for enable and inherit bars
"""


def id_from_chars(key: str):
    return ((ord(key[0]) * 256 + ord(key[1])) * 256 + ord(key[2])) * 256 + ord(key[3])


def output_button_def(name: str, state_name: str, id: int):
    return \
        f'\tAddHeaderOption("{name}")\n' \
        f'\tAddEmptyOption()\n' \
        f'\tAddToggleOptionST("{state_name}_Enabled", "Enabled", SpellHotbar.getBarEnabled({id}))\n' \
        f'\tAddMenuOptionST("{state_name}_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode({id})])\n'


def output_states(state_name: str, id: int):
    return \
        f'State {state_name}_Enabled\n' \
        f'	Event OnSelectST()\n' \
        f'		SetToggleOptionValueST(SpellHotbar.toggleBarEnabled({id}))\n' \
        f'	EndEvent\n' \
        f'	Event OnHighlightST()\n' \
        f'		SetInfoText("Disabling specific Hotbars will hide them in UI and avoid them during inheritence");\n' \
        f'	EndEvent\n' \
        f'EndState\n' \
        f'State {state_name}_Inherit\n' \
        f'	Event OnMenuOpenST()\n' \
        f'		SetMenuDialogOptions(inherit_options)\n' \
        f'		SetMenuDialogStartIndex(SpellHotbar.getInheritMode({id}))\n' \
        f'		SetMenuDialogDefaultIndex(0)\n' \
        f'	EndEvent\n' \
        f'	Event OnMenuAcceptST(int index)\n' \
        f'		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode({id}, index)])\n' \
        f'	EndEvent\n' \
        f'	Event OnHighlightST()\n' \
        f'		SetInfoText("Default: inherit from non-modifier bar first; Same Modifier: Inherit from parent bar with same modifier instead of non-modifier bar")\n' \
        f'	EndEvent\n' \
        f'	\n' \
        f'	Event OnDefaultST()\n' \
        f'		SetMenuOptionValueST(inherit_options[SpellHotbar.setInheritMode({id}, 0)])\n' \
        f'	EndEvent\n' \
        f'EndState\n'


def output_button_def_pair(bar1: tuple, bar2: tuple):
    def str_or_empty(text: str | None):
        return text if text is not None else "\tAddEmptyOption()\n"

    return str_or_empty(f'\tAddHeaderOption("{bar1[0]}")\n') + str_or_empty(f'\tAddHeaderOption("{bar2[0]}")\n') + \
        str_or_empty(f'\tAddToggleOptionST("{bar1[1]}_Enabled", "Enabled", SpellHotbar.getBarEnabled({bar1[2]}))\n') + \
        str_or_empty(f'\tAddToggleOptionST("{bar2[1]}_Enabled", "Enabled", SpellHotbar.getBarEnabled({bar2[2]}))\n') + \
        str_or_empty(
            f'\tAddMenuOptionST("{bar1[1]}_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode({bar1[2]})])\n') + \
        str_or_empty(
            f'\tAddMenuOptionST("{bar2[1]}_Inherit", "Inherit Mode", inherit_options[SpellHotbar.getInheritMode({bar2[2]})])\n')


def add_bar_with_sneak(l: list[tuple], bar_name: str, key: str):
    id = id_from_chars(key)
    bar_text = f"{bar_name} Bar"
    state_name = f"{bar_name.upper().replace(' ', '_').replace('-', '_')}"

    if state_name[0].isdigit():
        state_name = f"S_{state_name}"

    bar_text_sneak = f"{bar_name} Sneak Bar"
    state_name_sneak = f"{state_name}_SNEAK"
    id_sneak = id + 1

    l.append((bar_text, state_name, id))
    l.append((bar_text_sneak, state_name_sneak, id_sneak))


def output_json_save_code(id: int):
    return \
        f'JMap.setInt(data, "bar.{id}.enabled", SpellHotbar.getBarEnabled({id}) as int)\n' \
        f'JMap.setInt(data, "bar.{id}.inherit", SpellHotbar.getInheritMode({id}))\n'


def output_json_load_code(id: int):
    return \
        f'\tbar_enabled = JMap.getInt(data, "bar.{id}.enabled") > 0\n' \
        f'\tif (bar_enabled != SpellHotbar.getBarEnabled({id}))\n' \
        f'\t	SpellHotbar.toggleBarEnabled({id})\n' \
        f'\tEndIf\n' \
        f'\tSpellHotbar.setInheritMode({id}, JMap.getInt(data, "bar.{id}.inherit"))\n'


def is_not_none_bar(x: tuple):
    return len(x) == 3 and x[0] is not None and x[1] is not None and x[2] is not None


if __name__ == "__main__":
    """
    Set this bools to enable which code will be printed
    """
    print_buttons = False
    print_states = False
    print_json_save = False
    print_json_load = True

    """
    Bars definition
    """
    bars_list = [
        ("Sneak Bar", "MAIN_BAR_SNEAK", 1296124239),
        (None, None, None),  # main bar can't be disabled, so sneak has no partner (empty in mcm)
    ]
    add_bar_with_sneak(bars_list, "Melee", "MELE")
    add_bar_with_sneak(bars_list, "1H-Shield", "1HSD")
    add_bar_with_sneak(bars_list, "1H-Spell", "1HSP")
    add_bar_with_sneak(bars_list, "Dual Wield", "1HDW")
    add_bar_with_sneak(bars_list, "Two-Handed", "2HND")
    add_bar_with_sneak(bars_list, "Ranged", "RNGD")
    add_bar_with_sneak(bars_list, "Magic", "MAGC")

    if print_buttons:
        # iterate in pairs of 2
        for a, b in zip(bars_list[0::2], bars_list[1::2]):
            print(output_button_def_pair(a, b))

    if print_states:
        for x in bars_list:
            if is_not_none_bar(x):
                print(output_states(x[1], x[2]))

    if print_json_save:
        # output json save code
        for x in bars_list:
            if is_not_none_bar(x):
                print(output_json_save_code(x[2]))

    if print_json_load:
        for x in bars_list:
            if is_not_none_bar(x):
                print(output_json_load_code(x[2]))
