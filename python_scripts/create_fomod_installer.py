from pathlib import Path

from build_release_package import test_build_release_zip

dev_mod_root = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2")
dev_mod_root_nordic_ui = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar NordicUI")

project_root = Path(__file__).parent.parent

def _get_info_xml(version: str) -> str:
    return f"""
<fomod>
    <Name>Spell Hotbar 2</Name>
    <Author>pWn3d</Author>
    <Version MachineVersion="{version}">{version}</Version>
    <Description>Adds a Spell Hotbar to Skyrim</Description>
    <Website>https://github.com/pWn3d1337/Skyrim_SpellHotbar2</Website>
</fomod>
"""

released_files_main_plugin_v2 = [
    (project_root / "skse_plugin/build/release/SpellHotbar2.dll", "SKSE/Plugins"),
    (dev_mod_root / "SpellHotbar.esp", ""),
    (dev_mod_root / "Scripts/*.pex", dev_mod_root),  # if Path, add relative path to root in zip
    #(dev_mod_root / "Scripts/Source/*.psc", dev_mod_root), script source is no longer in install
    (dev_mod_root / "Interface/SpellHotbar/spell_icons.swf", dev_mod_root),
    (dev_mod_root / "meshes/SpellHotbar/*.nif", dev_mod_root),
    (dev_mod_root / "meshes/**/*.txt", dev_mod_root),
    (dev_mod_root / "meshes/**/*.hkx", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/InventoryInjector/SpellHotbar.json", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/animationdata/*.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/effectdata/vanilla_cast_effects.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/fonts/*.ttf", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/default_icons.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/default_icons.png", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_cooldown.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_cooldown.png", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla.png", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_poisons.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_poisons.png", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_potions.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_potions.png", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_powers.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla_powers.png", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_spellproc_overlay.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_spellproc_overlay.png", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/extra_icons.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/extra_icons.png", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/keynames/keynames.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/presets/*.json", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/spelldata/spells_vanilla.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/spelldata/spells_vanilla_poisons.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/spelldata/spells_vanilla_potions.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/spelldata/spells_vanilla_powers.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/transformdata/", dev_mod_root),
]


if __name__ == "__main__":

    version = "0.0.1"
    output_zip_path = project_root / f"build/Spell Hotbar 2 - {version}.zip"

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

    with open(fomod_info_xml_path, "w") as info_xml:
        info_xml.write(_get_info_xml(version))

    release_files: list[tuple[Path, str | Path]] = list()

    release_files.append((fomod_info_xml_path, "./fomod/"))

    main_mod_folder = "./000 Required - Main Mod/"



    test_build_release_zip(output_zip_path, release_files, main_folder="Spell Hotbar 2")

    #print(release_files)
