import shutil
from enum import Enum
from glob import glob
from pathlib import Path
from typing import Callable

from build_release_package import test_build_release_zip, build_release_zip

dev_mod_root = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2")
dev_mod_root_nordic_ui = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar NordicUI")
dev_mod_root_battlemage = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2 Battlemage")
dev_mod_root_battlemage_csf2 = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2 Battlemage CSF2")
dev_mod_root_battlemage_csf3 = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2 Battlemage CSF3")

project_root = Path(__file__).parent.parent

used_presets = [
    "all_bars.json",
    "oblivion_mode.json",
    "simple.json",
    "controller_simple.json"
]


def copy_files_outfolder(outfile: Path, files: list[tuple[Path, str | Path]], main_folder: str = "data"):
    print(f"Copying to folder: {outfile}...")
    main_path = Path(main_folder)
    for entry in files:
        file_list = glob(str(entry[0]), recursive=True)
        for file_name in file_list:
            if isinstance(entry[1], Path):
                arcname = str(main_path / Path(file_name).relative_to(entry[1]))
            elif isinstance(entry[1], tuple):
                rel_path = Path(entry[1][0])
                arc_folders = Path(entry[1][1])
                arcname = str(main_path / arc_folders / Path(file_name).relative_to(rel_path))
            else:
                fname = Path(file_name).name
                zip_path = Path(entry[1]) / fname
                arcname = str(main_path / zip_path)
            print(f"Copying: {arcname}")
            target_path = outfile / arcname
            if not target_path.parent.exists():
                target_path.parent.mkdir(parents=True)
            if target_path.is_file():
                shutil.copy(file_name, target_path)
            elif target_path.is_dir():
                target_path.mkdir(parents=True)


def _get_info_xml(version: str) -> str:
    return f"""<fomod>
    <Name>Spell Hotbar 2</Name>
    <Author>pWn3d</Author>
    <Version MachineVersion="{version}">{version}</Version>
    <Description>Adds a Spell Hotbar to Skyrim</Description>
    <Website>https://github.com/pWn3d1337/Skyrim_SpellHotbar2</Website>
</fomod>
"""


required_folder = "0000 Required - Main Mod"
battlemage_mod_folder = "2000 Optional - Battle Mage Perks"
battlemage_mod_folder_csf3 = "2001 Optional - Battle Mage Perks CSF3"
battlemage_mod_folder_csf2 = "2002 Optional - Battle Mage Perks CSF2"


def _get_spell_pack_config(name: str, folder: str, pluginfile: str | None = None,
                           credits_name: str | None = None) -> str:
    if pluginfile is None:
        pluginfile = f"{name}.esp"
    if "-" in pluginfile:
        formatted_name = pluginfile[:-4].title()
    else:
        formatted_name = name.replace("_", " ").title()
        formatted_name = formatted_name.replace("c2", "c 2")  # fix storm_calling_magic2

    extra_text = "" if credits_name is None else f"\n\nIcon Credits: {credits_name}"
    return f"""
                        <plugin name="{formatted_name}">
                            <description>Install Icons ans Spell config for {formatted_name}{extra_text}</description>
                            <files>
                                <folder source="{folder}" destination="" priority="0"/>
                            </files>
                            <typeDescriptor>
                                <dependencyType>
                                    <defaultType name="Optional"/>
                                    <patterns>
                                    <pattern>
                                    <dependencies operator="And">
                                    <fileDependency file="{pluginfile}" state="Active"/>
                                    </dependencies>
                                    <type name="Recommended"/>
                                    </pattern>
                                    </patterns>
                                </dependencyType>
                            </typeDescriptor>
                        </plugin>"""


class DualCastPerkConfig(Enum):
    VANILLA = 0,
    ADAMANT = 1,
    SPERG = 2


def _get_dual_cast_perk_flag(flag: DualCastPerkConfig, ref: DualCastPerkConfig) -> str:
    return f"""<flag name="DUAL_CAST_PERKS_{ref.name}">{'On' if flag == ref else 'Off'}</flag>"""


def _get_perk_overhaul_config(formatted_name: str, pluginfile: str, folders: list[str],
                              dual_cast_perks: DualCastPerkConfig) -> str:
    return f"""
                        <plugin name="{formatted_name}">
                            <description>Install compatibility for {formatted_name}</description>
                            <conditionFlags> 
                                {_get_dual_cast_perk_flag(dual_cast_perks, DualCastPerkConfig.VANILLA)} 
                                {_get_dual_cast_perk_flag(dual_cast_perks, DualCastPerkConfig.ADAMANT)} 
                                {_get_dual_cast_perk_flag(dual_cast_perks, DualCastPerkConfig.SPERG)} 
                            </conditionFlags> 
                            <files>
                                {"".join([f'<folder source="{f}" destination="" priority="0"/>' for f in folders])}
                            </files>
                            <typeDescriptor>
                                <type name="Optional"/>
                            </typeDescriptor>
                            <!--typeDescriptor>
                                {f'''<dependencyType>
                                    <defaultType name="Optional"/>
                                    <patterns>
                                    <pattern>
                                    <dependencies operator="And">
                                    <fileDependency file="{pluginfile}" state="Active"/>
                                    </dependencies>
                                    <type name="Recommended"/>
                                    </pattern>
                                    </patterns>
                                </dependencyType>''' if pluginfile is not None else ""}
                            </typeDescriptor-->
                        </plugin>"""


def _get_battle_mage_perk_config(folder: str, folder2: str) -> str:
    return f"""
                        <plugin name="Battle Mage Perk Tree (CSF 3+)">
                            <description>Install Battle Mage Perk Tree, requires Custom Skill Framework (https://www.nexusmods.com/skyrimspecialedition/mods/41780)</description>
                            <files>
                                <folder source="{folder}" destination="" priority="0"/>
                                <folder source="{folder2}" destination="" priority="0"/>
                            </files>
                            <typeDescriptor>
                                <type name="Recommended"/>
                            </typeDescriptor>
                        </plugin>"""

def _get_battle_mage_perk_config_csf2(folder: str, folder2: str) -> str:
    return f"""
                        <plugin name="Battle Mage Perk Tree (CSF 2, 1.5.97 compatible)">
                            <description>Install Battle Mage Perk Tree, requires Custom Skill Framework (https://www.nexusmods.com/skyrimspecialedition/mods/41780)</description>
                            <files>
                                <folder source="{folder}" destination="" priority="0"/>
                                <folder source="{folder2}" destination="" priority="0"/>
                            </files>
                            <typeDescriptor>
                                <type name="Optional"/>
                            </typeDescriptor>
                        </plugin>"""


def _get_profile_config(name: str, json_name: str, desc: str, image: str | None = None) -> str:
    return f"""
                         <plugin name="{name}">
                            <description>{desc}</description>
                            {
    f'<image path = "installer_images\\{image}"/>' if image is not None else ""
    }
                            <files>
                                <file source="9000 ConditionalFiles/auto_profiles/{json_name}.json" destination="SKSE/Plugins/SpellHotbar/presets/auto_profile.json" priority="0"/>
                            </files>
                            <typeDescriptor>
                                <type name="Optional"/>
                            </typeDescriptor>
                        </plugin>"""


def _get_module_config_xml(version: str, spell_packs: list[tuple[str, str, str, str]]) -> str:
    return f"""<config xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="http://qconsulting.ca/fo3/ModConfig5.0.xsd">
    <moduleName>Spell Hotbar 2 - {version} Installer</moduleName>
    <moduleImage path="installer_images\\spell_hotbar_logo.jpg" />
    <requiredInstallFiles>
        <folder source="{required_folder}" destination="" priority="0"/>
    </requiredInstallFiles>
    <installSteps order="Explicit">
        <installStep name="Install Options">
            <optionalFileGroups order="Explicit">
                <group name="Battle Mage Perks" type="SelectAny">
                    <plugins order="Explicit">
                        {_get_battle_mage_perk_config(battlemage_mod_folder, battlemage_mod_folder_csf3)}
                        {_get_battle_mage_perk_config_csf2(battlemage_mod_folder, battlemage_mod_folder_csf2)}
                    </plugins>
                </group>
                <group name="Perk Overhaul" type="SelectExactlyOne">
                    <plugins order="Explicit">
                        {"".join([_get_perk_overhaul_config(p[0], p[1], p[2], p[3]) for p in perk_overhauls])}
                    </plugins>
                </group>
                <group name="Spell Packs" type="SelectAny">
                    <plugins order="Explicit">
                        {"".join([_get_spell_pack_config(s[0], s[1], s[2], s[3]) for s in spell_packs])}
                    </plugins>
                </group>
            </optionalFileGroups>
        </installStep>
        <installStep name="UI Config">
            <optionalFileGroups order="Explicit">
                <group name="Chose UI Textures" type="SelectExactlyOne">
                    <plugins order="Explicit">
                         <plugin name="Default">
                            <description>Keep SpellHotbar's default look matching vanilla skyrim</description>
                            <files>
                            </files>
                            <typeDescriptor>
                                <type name="Recommended"/>
                            </typeDescriptor>
                        </plugin>
                        <plugin name="Nordic UI">
                            <description>Install bronze colored bar overlay textures matching Nordic UI color scheme. Chose Sovngarde font to best match Nordic UI</description>
                            <files>
                                <file source="4000 Interface Files/SKSE/Plugins/SpellHotbar/images/default_icons_nordic.csv" destination="SKSE/Plugins/SpellHotbar/images/default_icons_nordic.csv" priority="0" /> 
                                <file source="4000 Interface Files/SKSE/Plugins/SpellHotbar/images/default_icons_nordic.dds" destination="SKSE/Plugins/SpellHotbar/images/default_icons_nordic.dds" priority="0" /> 
                            </files>
                            <typeDescriptor>
                                <type name="Optional"/>
                            </typeDescriptor>
                        </plugin>
                    </plugins>
                </group>
                <group name="Chose a Text Font" type="SelectExactlyOne">
                    <plugins order="Explicit">
                         <plugin name="Default">
                            <description>Install Skyrims 'Futura Condensed' Font</description>
                            <files>
                                <file source="4000 Interface Files/fonts/text_font.ttf" destination="SKSE/Plugins/SpellHotbar/fonts/text_font.ttf" priority="0" /> 
                            </files>
                            <typeDescriptor>
                                <type name="Recommended"/>
                            </typeDescriptor>
                        </plugin>
                        <plugin name="Sovngarde Font">
                            <description>Install the Sovngarde (https://www.nexusmods.com/skyrimspecialedition/mods/386) Font, use together with Nordic UI, does not support JP/CN, should work with RU and PL</description>
                            <files>
                                <file source="4000 Interface Files/fonts/text_font_sovngarde.ttf" destination="SKSE/Plugins/SpellHotbar/fonts/text_font.ttf" priority="0" /> 
                            </files>
                            <typeDescriptor>
                                <type name="Optional"/>
                            </typeDescriptor>
                        </plugin>
                        <plugin name="PL">
                            <description>Install Skyrims 'Futura Condensed' Font with PL support</description>
                            <files>
                                <file source="4000 Interface Files/fonts/text_font_pl.ttf" destination="SKSE/Plugins/SpellHotbar/fonts/text_font.ttf" priority="0" /> 
                            </files>
                            <typeDescriptor>
                                <type name="Optional"/>
                            </typeDescriptor>
                        </plugin>
                        <plugin name="RU">
                            <description>Install Skyrims 'Futura Condensed' Font with RU support</description>
                            <files>
                                <file source="4000 Interface Files/fonts/text_font_ru.ttf" destination="SKSE/Plugins/SpellHotbar/fonts/text_font-cyrillic.ttf" priority="0" /> 
                            </files>
                            <typeDescriptor>
                                <type name="Optional"/>
                            </typeDescriptor>
                        </plugin>
                        <plugin name="JP">
                            <description>Install Skyrims 'Futura Condensed' Font with JP support</description>
                            <files>
                                <file source="4000 Interface Files/fonts/text_font_jp.ttf" destination="SKSE/Plugins/SpellHotbar/fonts/text_font-japanese.ttf" priority="0" /> 
                            </files>
                            <typeDescriptor>
                                <type name="Optional"/>
                            </typeDescriptor>
                        </plugin>
                        <plugin name="CN">
                            <description>Install a CN font from fonts_cn.swf</description>
                            <files>
                                <file source="4000 Interface Files/fonts/text_font_cn.ttf" destination="SKSE/Plugins/SpellHotbar/fonts/text_font-chinese.ttf" priority="0" /> 
                            </files>
                            <typeDescriptor>
                                <type name="Optional"/>
                            </typeDescriptor>
                        </plugin>
                    </plugins>
                </group>
            </optionalFileGroups>
        </installStep>
        <installStep name="Config Options">
            <optionalFileGroups order="Explicit">
                <group name="Chose a Profile that automatically gets loaded" type="SelectExactlyOne">
                    <plugins order="Explicit">
                         <plugin name="No Auto Profile">
                            <description>All variables will be set to default and no keybinds will be set on initialisation. Will require manual configuration of the mod or manual loading of a profile through MCM.</description>
                            <files>
                            </files>
                            <typeDescriptor>
                                <type name="Recommended"/>
                            </typeDescriptor>
                        </plugin>
                        {_get_profile_config("Simple", "simple",
                                             "Only 1 Bar, no modifiers, Keybinds: 1-10.=", "simple.jpg")}
                        {_get_profile_config("All Bars", "all_bars",
                                             "All weapon bars, ctrl, shift and alt modifiers enabled, Keybinds: 1-10.=, Numpad 4 and Numpad 6 to cycle in menu", "simple.jpg")}
                        {_get_profile_config("Oblivion Mode", "oblivion_mode",
                                             "Activates Oblivion mode, Keybinds: 1-10.= for selection, 'v' and 'b' to cast spell/potion", "oblivion_mode.jpg")}
                        {_get_profile_config("Controller Simple", "controller_simple",
                                             "Starting point for controller config, binds DPad + ABXY with RS/LS as modifiers, non-modifier bar is disabled. Might need config adjustments to not conflict with your setup/MCM menus", "controller_simple.jpg")}
                    </plugins>
                </group>
            </optionalFileGroups>
        </installStep>
    </installSteps>
    <conditionalFileInstalls> 
        <patterns> 
            <pattern> 
                <dependencies operator="And"> 
                    <flagDependency flag="DUAL_CAST_PERKS_VANILLA" value="On"/> 
                </dependencies> 
                <files> 
                    <file source="9000 ConditionalFiles/perkdata_vanilla/dual_cast_perks.csv" destination="SKSE/Plugins/SpellHotbar/perkdata/dual_cast_perks.csv" priority="0" /> 
                </files> 
            </pattern> 
            <pattern> 
                <dependencies operator="And"> 
                    <flagDependency flag="DUAL_CAST_PERKS_ADAMANT" value="On"/> 
                </dependencies> 
                <files> 
                    <file source="9000 ConditionalFiles/perkdata_adamant/dual_cast_perks.csv" destination="SKSE/Plugins/SpellHotbar/perkdata/dual_cast_perks.csv" priority="0" /> 
                </files> 
            </pattern> 
            <pattern> 
                <dependencies operator="And"> 
                    <flagDependency flag="DUAL_CAST_PERKS_SPERG" value="On"/> 
                </dependencies> 
                <files> 
                    <file source="9000 ConditionalFiles/perkdata_sperg/dual_cast_perks.csv" destination="SKSE/Plugins/SpellHotbar/perkdata/dual_cast_perks.csv" priority="0" /> 
                </files> 
            </pattern> 
        </patterns>
    </conditionalFileInstalls>
</config>
"""


spell_packs: list[str, str, str] = list()

perk_overhauls: list[str, str, str, DualCastPerkConfig] = list()


def _add_spell_pack(name: str, plugin: str | None = None, credits_name: str | None = None):
    if plugin is None:
        plugin = f"{name}.esp"
    spell_packs.append((name, plugin, credits_name))


def _add_perk_overhaul(name: str, plugin: str, folders: list[str] | str, dual_casting_perk_config: DualCastPerkConfig):
    if isinstance(folders, str):
        folders = [folders]
    perk_overhauls.append((name, plugin, folders, dual_casting_perk_config))


def _get_spell_pack_folder_name(num: int, modname: str) -> str:
    return f"{1000 + num} Spell Pack - {modname}"


def _get_perk_overhaul_folder_name(num: int, modname: str) -> str:
    return f"{3000 + num} Perk Overhaul - {modname}"


def _get_spell_list(modname: str, num: int, folder_name_getter: Callable[[int, str], str],
                    esp_name: str | None = None) -> list[tuple[Path, tuple[Path, str]]]:
    if esp_name is None:
        esp_name = modname.capitalize()
    esp_name = Path(esp_name).stem  # removes suffix if preset
    folder_name = folder_name_getter(num, modname)
    return [
        (dev_mod_root / f"Interface/SpellHotbar/{modname}_icons.swf", (dev_mod_root, folder_name)),
        (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_{modname}.csv", (dev_mod_root, folder_name)),
        (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_{modname}.dds", (dev_mod_root, folder_name)),
        (dev_mod_root / f"SKSE/Plugins/SpellHotbar/spelldata/spells_{modname}.csv", (dev_mod_root, folder_name)),
        (dev_mod_root / f"SKSE/Plugins/InventoryInjector/{esp_name}.json", (dev_mod_root, folder_name)),
    ]


def get_perk_overhaul_list(name: str, num: int, esp_name: str | None) -> list[tuple[Path, tuple[Path, str]]]:
    return _get_spell_list(name, num, _get_perk_overhaul_folder_name, esp_name)


def get_spell_pack_list(modname: str, num: int, esp_name: str | None = None) -> list[tuple[Path, tuple[Path, str]]]:
    return _get_spell_list(modname, num, _get_spell_pack_folder_name, esp_name)


main_mod_folder = f"./{required_folder}/"

released_files_main_plugin_v2 = [
    (project_root / "skse_plugin/build/release_with_debug_info/SpellHotbar2.dll", main_mod_folder + "SKSE/Plugins"),
    (project_root / "skse_plugin/build/release_with_debug_info/SpellHotbar2.pdb", main_mod_folder + "SKSE/Plugins"),
    (dev_mod_root / "SpellHotbar.esp", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "Scripts/SpellHotbar.pex", (dev_mod_root, main_mod_folder)),
    # if Path, add relative path to root in zip
    (dev_mod_root / "Scripts/SpellHotbarInitQuestScript.pex", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "Scripts/SpellHotbarMCM.pex", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "Scripts/SpellHotbarToggleDualCastingEffect.pex", (dev_mod_root, main_mod_folder)),
    # (dev_mod_root / "Scripts/Source/*.psc", dev_mod_root), script source is no longer in install
    (dev_mod_root / "Interface/SpellHotbar/spell_icons.swf", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "meshes/SpellHotbar/*.nif", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "meshes/**/*.json", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "meshes/**/*.hkx", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/InventoryInjector/SpellHotbar.json", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/animationdata/*.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/effectdata/vanilla_cast_effects.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/fonts/skyrim_symbols_font.ttf", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/default_icons.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/default_icons.dds", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_cooldown.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_cooldown.dds", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla.dds", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_poisons.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_poisons.dds", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_potions.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_potions.dds", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_powers.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_powers.dds", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_spellproc_overlay.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_spellproc_overlay.dds", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/cursor.dds", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/inv_bg.dds", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/extra_icons.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/extra_icons.dds", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/keynames/keynames.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/spelldata/spells_vanilla.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/spelldata/spells_vanilla_poisons.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/spelldata/spells_vanilla_potions.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/spelldata/spells_vanilla_powers.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/transformdata/", (dev_mod_root, main_mod_folder)),
]

released_files_main_plugin_v2 += [
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/presets/{preset}", (dev_mod_root, main_mod_folder)) for preset in
    used_presets
]

battlemage_perk_files = [
    (dev_mod_root_battlemage / "SpellHotbar_BattleMage.esp", (dev_mod_root_battlemage, battlemage_mod_folder)),
    (dev_mod_root_battlemage / "meshes/interface/intbattlemageperkskydome.nif",
     (dev_mod_root_battlemage, battlemage_mod_folder)),
    (dev_mod_root_battlemage / "textures/interface/battlemage_bluemoon.dds",
     (dev_mod_root_battlemage, battlemage_mod_folder)),
    (dev_mod_root_battlemage / "textures/interface/battlemage_constellation.dds",
     (dev_mod_root_battlemage, battlemage_mod_folder)),
    (dev_mod_root_battlemage / "Interface/MetaSkillsMenu/SpellHotbar_Battlemage SpellHotbar.dds",
     (dev_mod_root_battlemage, battlemage_mod_folder)),
    # yes the scripts are located in the main mods folder in the dev setup
    (dev_mod_root / "Scripts/SpellHotbarBattleMageInitQuestScript.pex", (dev_mod_root, battlemage_mod_folder)),
    (dev_mod_root / "Scripts/SpellHotbarOpenBattleMagePerkTree.pex", (dev_mod_root, battlemage_mod_folder)),
    (dev_mod_root_battlemage_csf3 / "SKSE/Plugins/CustomSkills/SpellHotbar_Battlemage.json",
     (dev_mod_root_battlemage_csf3, battlemage_mod_folder_csf3)),
    (dev_mod_root_battlemage_csf2 / "NetScriptFramework/Plugins/CustomSkill.SpellHotbar_Battlemage.config.txt",
     (dev_mod_root_battlemage_csf2, battlemage_mod_folder_csf2))
]

if __name__ == "__main__":
    ONLY_XML_FILES = False
    ONLY_PRINT_FILES = False
    if ONLY_XML_FILES:
        print("Only creating xml files...")

    debug_output_folder = Path(r"F:\Skyrim Dev\WORK\ZIP_OUT")

    version = "0.0.7"
    output_zip_path = project_root / f"build/Spell Hotbar 2 - {version}.zip"

    _add_spell_pack("vulcano")
    _add_spell_pack("arclight")
    _add_spell_pack("desecration")

    _add_spell_pack("thunderchild", "Thunderchild - Epic Shout Package.esp")
    _add_spell_pack("sonic_magic", "Shockwave.esl")
    _add_spell_pack("storm_calling_magic2", "StormCalling.esl")
    _add_spell_pack("astral_magic_2", "Astral.esl")
    _add_spell_pack("elemental_destruction_magic_redux", "Elemental Destruction Magic Redux.esp")

    _add_spell_pack("constellation_magic", "Supernova.esl")
    _add_spell_pack("miracles_of_skyrim", "DS2Miracles.esp")
    _add_spell_pack("dark_hierophant_magic", "Ghostlight.esl")

    _add_spell_pack("andromeda", "Andromeda - Unique Standing Stones of Skyrim.esp")
    _add_spell_pack("sacrosanct", "Sacrosanct - Vampires of Skyrim.esp")

    _add_spell_pack("abyssal_tides_magic", "Aqua.esl")
    _add_spell_pack("abyssal_wind_magic", "Aero.esl")
    _add_spell_pack("winter_wonderland_magic", "Icebloom.esl")
    _add_spell_pack("abyss")
    _add_spell_pack("ancient_blood_magic_2", "AncientBloodII.esl")

    _add_spell_pack("stellaris")

    _add_spell_pack("obscure_magic", "KittySpellPack01.esl")
    _add_spell_pack("holy_templar_magic", "Lightpower.esl")

    _add_spell_pack("apocalypse", "Apocalypse - Magic of Skyrim.esp")
    _add_spell_pack("odin", "Odin - Skyrim Magic Overhaul.esp")
    _add_spell_pack("triumvirate", "Triumvirate - Mage Archetypes.esp")

    _add_spell_pack("mysticism", "MysticismMagic.esp")
    _add_spell_pack("witcher_signs", "W3S.esl")
    _add_spell_pack("elemental_mastery_magic", "KittySpellPack02.esl")

    _add_spell_pack("shadow_spell_package", "ShadowSpellPackage.esp", credits_name="ArchAngelAries")
    _add_spell_pack("star_wars_spell_pack", "starwarsspellpack.esp", credits_name="ArchAngelAries")
    _add_spell_pack("star_wars_spell_pack_esl", "starwarsspellpack_ESLversion.esl", credits_name="ArchAngelAries")
    _add_spell_pack("undead_horse", "Undead Horse.esl", credits_name="ArchAngelAries")

    # perk overhauls
    ordinator_folder = _get_perk_overhaul_folder_name(1, "ordinator")
    sperg_folder = _get_perk_overhaul_folder_name(2, "sperg")
    pos_folder = _get_perk_overhaul_folder_name(3, "path_of_sorcery")

    _add_perk_overhaul("Vanilla/Vokrii", None, [], DualCastPerkConfig.VANILLA)
    _add_perk_overhaul("Ordinator", "Ordinator - Perks of Skyrim.esp", ordinator_folder, DualCastPerkConfig.VANILLA)
    _add_perk_overhaul("SPERG", "SPERG-SSE.esp", sperg_folder, DualCastPerkConfig.SPERG)
    _add_perk_overhaul("Path of Sorcery", "PathOfSorcery.esp", pos_folder, DualCastPerkConfig.VANILLA)
    _add_perk_overhaul("Adamant", "Adamant.esp", [], DualCastPerkConfig.ADAMANT)
    _add_perk_overhaul("Vokriinator", "Vokriinator.esp", ordinator_folder, DualCastPerkConfig.VANILLA)
    _add_perk_overhaul("Vokriinator Black", "Vokriinator Black.esp", [ordinator_folder, sperg_folder, pos_folder],
                       DualCastPerkConfig.VANILLA)

    tmp_folder = output_zip_path.parent / f"tmp_build_{version}"
    if tmp_folder.exists():
        try:
            tmp_folder.unlink()
            tmp_folder.mkdir(parents=True)
        except Exception as e:
            print(f"Could not remove tmp folder: {e}")
    else:
        tmp_folder.mkdir(parents=True)

    fomod_info_xml_path = tmp_folder / "fomod/info.xml"
    if not fomod_info_xml_path.parent.exists():
        fomod_info_xml_path.parent.mkdir(parents=True)

    module_config_path = tmp_folder / "fomod/ModuleConfig.xml"
    if not module_config_path.parent.exists():
        module_config_path.parent.mkdir(parents=True)

    with open(fomod_info_xml_path, "w") as info_xml:
        info_xml.write(_get_info_xml(version))

    spell_packs.sort()
    spell_packs_out = [(s[0], _get_spell_pack_folder_name(i, s[0]), s[1], s[2]) for i, s in enumerate(spell_packs)]

    with open(module_config_path, "w") as module_xml:
        module_xml.write(_get_module_config_xml(version, spell_packs_out))

    if not ONLY_XML_FILES:

        release_files: list[tuple[Path, str | Path]] = list()

        release_files.append((fomod_info_xml_path, "./fomod/"))
        release_files.append((module_config_path, "./fomod/"))
        release_files += released_files_main_plugin_v2

        for i, s in enumerate(spell_packs):
            release_files += get_spell_pack_list(s[0], i, s[1])

        release_files += battlemage_perk_files

        release_files += get_perk_overhaul_list("ordinator", 1, "Ordinator - Perks of Skyrim")
        release_files += get_perk_overhaul_list("sperg", 2, "SPERG-SSE")
        release_files += get_perk_overhaul_list("path_of_sorcery", 3, "PathOfSorcery")

        # ui files
        release_files.append(
            (dev_mod_root / "SKSE/Plugins/SpellHotbar/fonts/text_font.ttf", "4000 Interface Files/fonts"))
        release_files.append(
            (dev_mod_root / "SKSE/Plugins/SpellHotbar/fonts/text_font_sovngarde.ttf", "4000 Interface Files/fonts"))
        release_files.append(
            (dev_mod_root / "SKSE/Plugins/SpellHotbar/fonts/text_font_cn.ttf", "4000 Interface Files/fonts"))
        release_files.append(
            (dev_mod_root / "SKSE/Plugins/SpellHotbar/fonts/text_font_jp.ttf", "4000 Interface Files/fonts"))
        release_files.append(
            (dev_mod_root / "SKSE/Plugins/SpellHotbar/fonts/text_font_ru.ttf", "4000 Interface Files/fonts"))
        ## nordic UI texture
        release_files.append((dev_mod_root_nordic_ui / "SKSE/Plugins/SpellHotbar/images/default_icons_nordic.csv",
                              "4000 Interface Files/SKSE/Plugins/SpellHotbar/images"))
        release_files.append((dev_mod_root_nordic_ui / "SKSE/Plugins/SpellHotbar/images/default_icons_nordic.dds",
                              "4000 Interface Files/SKSE/Plugins/SpellHotbar/images"))

        # conditionalFiles
        release_files.append((dev_mod_root / "SKSE/Plugins/SpellHotbar/perkdata/dual_cast_perks.csv",
                              "9000 ConditionalFiles/perkdata_vanilla"))
        release_files.append((
                             dev_mod_root / "../Spell Hotbar 2 Adamant/SKSE/Plugins/SpellHotbar/perkdata/dual_cast_perks.csv",
                             "9000 ConditionalFiles/perkdata_adamant"))
        release_files.append((
                             dev_mod_root / "../Spell Hotbar 2 SPERG/SKSE/Plugins/SpellHotbar/perkdata/dual_cast_perks.csv",
                             "9000 ConditionalFiles/perkdata_sperg"))

        # installer images
        release_files.append((Path(__file__).parent / "installer_images/spell_hotbar_logo.jpg", "installer_images"))
        release_files.append((Path(__file__).parent / "installer_images/simple.jpg", "installer_images"))
        release_files.append((Path(__file__).parent / "installer_images/oblivion_mode.jpg", "installer_images"))
        release_files.append((Path(__file__).parent / "installer_images/controller_simple.jpg", "installer_images"))

        # auto profiles, this is duplicated because vortex did not handle copying an existing file a second time correctly
        for preset in used_presets:
            release_files.append(
                (dev_mod_root / f"SKSE/Plugins/SpellHotbar/presets/{preset}", f"9000 ConditionalFiles/auto_profiles"))

        if ONLY_PRINT_FILES:
            for p1, p2 in release_files:
                print(f"'{p1}' -> '{p2}'")
        else:
            build_release_zip(output_zip_path, release_files, main_folder="Spell Hotbar 2")
            # copy_files_outfolder(debug_output_folder, release_files, main_folder="Spell Hotbar 2")
