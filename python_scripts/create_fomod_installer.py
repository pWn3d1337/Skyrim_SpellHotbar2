from enum import Enum
from pathlib import Path
from typing import Callable

from build_release_package import test_build_release_zip, build_release_zip

dev_mod_root = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2")
dev_mod_root_nordic_ui = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar NordicUI")
dev_mod_root_battlemage = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2 Battlemage")

project_root = Path(__file__).parent.parent


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


def _get_spell_pack_config(name: str, folder: str, pluginfile: str | None = None) -> str:
    if pluginfile is None:
        pluginfile = f"{name}.esp"
    if "-" in pluginfile:
        formatted_name = pluginfile[:-4].title()
    else:
        formatted_name = name.replace("_", " ").title()
        formatted_name = formatted_name.replace("c2", "c 2")  # fix storm_calling_magic2
    return f"""
                        <plugin name="{formatted_name}">
                            <description>Install Icons ans Spell config for {formatted_name}</description>
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


def _get_perk_overhaul_config(formatted_name: str, pluginfile: str, folders: list[str], dual_cast_perks: DualCastPerkConfig) -> str:
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
                            </typeDescriptor>
                        </plugin>"""


def _get_battle_mage_perk_config(folder: str) -> str:
    return f"""
                        <plugin name="Battle Mage Perk Tree">
                            <description>Install Battle Mage Perk Tree, requires Custom Skill Framework (https://www.nexusmods.com/skyrimspecialedition/mods/41780)</description>
                            <files>
                                <folder source="{folder}" destination="" priority="0"/>
                            </files>
                            <typeDescriptor>
                                <type name="Recommended"/>
                            </typeDescriptor>
                        </plugin>"""


def _get_module_config_xml(version: str, spell_packs: list[tuple[str, str, str]]) -> str:
    return f"""
<config xmlns:xsi="https://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="https://qconsulting.ca/fo3/ModConfig5.0.xsd">
    <moduleName>Spell Hotbar 2 - {version} Installer</moduleName>
    <requiredInstallFiles>
        <folder source="{required_folder}" destination="" priority="0"/>
    </requiredInstallFiles>
    <installSteps order="Explicit">
        <installStep name="Install Options">
            <optionalFileGroups order="Explicit">
                <group name="Battle Mage Perks" type="SelectAny">
                    <plugins order="Explicit">
                        {_get_battle_mage_perk_config(battlemage_mod_folder)}
                    </plugins>
                </group>
                <group name="Spell Packs" type="SelectAny">
                    <plugins order="Explicit">
                        {"".join([_get_spell_pack_config(s[0], s[1], s[2]) for s in spell_packs])}
                    </plugins>
                </group>
                <group name="Perk Overhaul" type="SelectOne">
                        {"".join([_get_perk_overhaul_config(p[0], p[1], p[2], p[3]) for p in perk_overhauls])}
                </group>
            </optionalFileGroups>
        </installStep>
    </installSteps>
</config>
"""


spell_packs: list[str, str] = list()

perk_overhauls: list[str, str, str, DualCastPerkConfig] = list()

def _add_spell_pack(name: str, plugin: str | None = None):
    if plugin is None:
        plugin = f"{name}.esp"
    spell_packs.append((name, plugin))


def _add_perk_overhaul(name: str, plugin: str, folders : list[str] | str, dual_casting_perk_config: DualCastPerkConfig):
    if isinstance(folders, str):
        folders = [folders]
    perk_overhauls.append((name, plugin, folders, dual_casting_perk_config))

def _get_spell_pack_folder_name(num: int, modname: str) -> str:
    return f"{1000 + num} Spell Pack - {modname}"

def _get_perk_overhaul_folder_name(num: int, modname: str) -> str:
    return f"{3000 + num} Perk Overhaul - {modname}"

def _get_spell_list(modname: str, num: int, folder_name_getter: Callable[[int, str], str], esp_name: str | None = None) -> list[tuple[Path, tuple[Path, str]]]:
    if esp_name is None:
        esp_name = modname.capitalize()
    folder_name = folder_name_getter(num, modname)
    return [
        (dev_mod_root / f"Interface/SpellHotbar/{modname}_icons.swf", (dev_mod_root, folder_name)),
        (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_{modname}.csv", (dev_mod_root, folder_name)),
        (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_{modname}.png", (dev_mod_root, folder_name)),
        (dev_mod_root / f"SKSE/Plugins/SpellHotbar/spelldata/spells_{modname}.csv", (dev_mod_root, folder_name)),
        (dev_mod_root / f"SKSE/Plugins/InventoryInjector/{esp_name}.json", (dev_mod_root, folder_name)),
    ]

def get_perk_overhaul_list(name: str, num: int, esp_name: str | None) -> list[tuple[Path, tuple[Path, str]]]:
    return _get_spell_list(name, num, _get_perk_overhaul_folder_name, esp_name)

def get_spell_pack_list(modname: str, num: int, esp_name: str | None = None) -> list[tuple[Path, tuple[Path, str]]]:
    return _get_spell_list(modname, num, _get_spell_pack_folder_name, esp_name)


main_mod_folder = f"./{required_folder}/"

released_files_main_plugin_v2 = [
    (project_root / "skse_plugin/build/release/SpellHotbar2.dll", main_mod_folder + "SKSE/Plugins"),
    (dev_mod_root / "SpellHotbar.esp", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "Scripts/SpellHotbar.pex", (dev_mod_root, main_mod_folder)),
    # if Path, add relative path to root in zip
    (dev_mod_root / "Scripts/SpellHotbarInitQuestScript.pex", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "Scripts/SpellHotbarMCM.pex", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "Scripts/SpellHotbarToggleDualCastingEffect.pex", (dev_mod_root, main_mod_folder)),
    # (dev_mod_root / "Scripts/Source/*.psc", dev_mod_root), script source is no longer in install
    (dev_mod_root / "Interface/SpellHotbar/spell_icons.swf", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "meshes/SpellHotbar/*.nif", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "meshes/**/*.txt", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "meshes/**/*.hkx", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/InventoryInjector/SpellHotbar.json", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/animationdata/*.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/effectdata/vanilla_cast_effects.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/fonts/*.ttf", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/default_icons.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/default_icons.png", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_cooldown.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_cooldown.png", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla.png", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_poisons.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_poisons.png", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_potions.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_potions.png", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_powers.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_powers.png", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_spellproc_overlay.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_spellproc_overlay.png", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/extra_icons.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/extra_icons.png", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/keynames/keynames.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/presets/*.json", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/spelldata/spells_vanilla.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/spelldata/spells_vanilla_poisons.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/spelldata/spells_vanilla_potions.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/spelldata/spells_vanilla_powers.csv", (dev_mod_root, main_mod_folder)),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/transformdata/", (dev_mod_root, main_mod_folder)),
]

battlemage_perk_files = [
    (dev_mod_root_battlemage / "SpellHotbar_BattleMage.esp", (dev_mod_root_battlemage, battlemage_mod_folder)),
    # yes the scripts are located in the main mods folder in the dev setup
    (dev_mod_root / "Scripts/SpellHotbarBattleMageInitQuestScript.pex", (dev_mod_root, battlemage_mod_folder)),
    (dev_mod_root / "Scripts/SpellHotbarOpenBattleMagePerkTree.pex", (dev_mod_root, battlemage_mod_folder)),
    (dev_mod_root_battlemage / "SKSE/Plugins/CustomSkills/SpellHotbar_Battlemage.json",
     (dev_mod_root_battlemage, battlemage_mod_folder))
]

if __name__ == "__main__":
    ONLY_XML_FILES = True
    if ONLY_XML_FILES:
        print("Only creating xml files...")

    version = "0.0.1"
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


    # perk overhauls
    _add_perk_overhaul("Vanilla/Vorkii", None, [], DualCastPerkConfig.VANILLA)
    _add_perk_overhaul("Ordinator", "Ordinator - Perks of Skyrim.esp", "ordinator", DualCastPerkConfig.VANILLA)
    _add_perk_overhaul("SPERG", "SPERG-SSE.esp", "sperg", DualCastPerkConfig.SPERG)
    _add_perk_overhaul("Path Of Sorcery", "SPERG-SSE.esp", "path_of_sorcery", DualCastPerkConfig.VANILLA)
    _add_perk_overhaul("Adamant", "Adamant.esp", [], DualCastPerkConfig.ADAMANT)
    _add_perk_overhaul("Vokriinator", "Vokriinator.esp", "ordinator", DualCastPerkConfig.VANILLA)
    _add_perk_overhaul("Vokriinator Black", "Vokriinator Black.esp", ["ordinator", "sperg", "path_of_sorcery"], DualCastPerkConfig.VANILLA)


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
    spell_packs_out = [(s[0], _get_spell_pack_folder_name(i, s[0]), s[1]) for i, s in enumerate(spell_packs)]

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

        build_release_zip(output_zip_path, release_files, main_folder="Spell Hotbar 2")
