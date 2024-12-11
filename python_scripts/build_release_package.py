from pathlib import Path
from zipfile import ZipFile, ZIP_STORED, ZIP_LZMA
from glob import glob

# this scripts builds a releasable zip file


dev_mod_root = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2")
dev_mod_root_nordic_ui = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar NordicUI")

project_root = Path(__file__).parent.parent

released_files_main_plugin = [
    (project_root / "skse_plugin/build/release/SpellHotbar2.dll", "SKSE/Plugins"),
    (dev_mod_root / "SpellHotbar.esp", ""),
    (dev_mod_root / "Scripts/*.pex", dev_mod_root),  # if Path, add relative path to root in zip
    (dev_mod_root / "Scripts/Source/*.psc", dev_mod_root),
    (dev_mod_root / "Interface/SpellHotbar/spell_icons.swf", dev_mod_root),
    (dev_mod_root / "meshes/SpellHotbar/*.nif", dev_mod_root),
    (dev_mod_root / "meshes/**/*.txt", dev_mod_root),
    (dev_mod_root / "meshes/**/*.hkx", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/InventoryInjector/SpellHotbar.json", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/effectdata/vanilla_cast_effects.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/fonts/*.ttf", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/default_icons.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/default_icons.png", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_cooldown.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_cooldown.png", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/images/icons_vanilla.png", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/presets/*.json", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/spelldata/spells_vanilla.csv", dev_mod_root),
    (dev_mod_root / "SKSE/Plugins/SpellHotbar/transformdata", dev_mod_root),
]

released_files_nordic_ui_plugin = [
    (dev_mod_root_nordic_ui / "SKSE/Plugins/SpellHotbar/fonts/9_$ConsoleFont_Futura Condensed.ttf",
     dev_mod_root_nordic_ui),
    (dev_mod_root_nordic_ui / "SKSE/Plugins/SpellHotbar/images/*.csv", dev_mod_root_nordic_ui),
    (dev_mod_root_nordic_ui / "SKSE/Plugins/SpellHotbar/images/*.png", dev_mod_root_nordic_ui),
]


def get_spell_pack_list(modname: str, esp_name: str | None = None):
    if esp_name is None:
        esp_name = modname.capitalize()
    return [
        (dev_mod_root / f"Interface/SpellHotbar/{modname}_icons.swf", dev_mod_root),
        (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_{modname}.csv", dev_mod_root),
        (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_{modname}.png", dev_mod_root),
        (dev_mod_root / f"SKSE/Plugins/SpellHotbar/spelldata/spells_{modname}.csv", dev_mod_root),
        (dev_mod_root / f"SKSE/Plugins/InventoryInjector/{esp_name}.json", dev_mod_root),
    ]


released_files_triumvirate_spellpack = [
    (dev_mod_root / f"Interface/SpellHotbar/triumvirate_icons.swf", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/InventoryInjector/Triumvirate - Mage Archetypes.json", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_druid.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_druid.png", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/spelldata/spells_triumvirate_druid.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_cleric.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_cleric.png", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/spelldata/spells_triumvirate_cleric.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_shadow_mage.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_shadow_mage.png", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/spelldata/spells_triumvirate_shadow_mage.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_warlock.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_warlock.png", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/spelldata/spells_triumvirate_warlock.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_shaman.csv", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/images/icons_triumvirate_shaman.png", dev_mod_root),
    (dev_mod_root / f"SKSE/Plugins/SpellHotbar/spelldata/spells_triumvirate_shaman.csv", dev_mod_root),
]


def build_release_zip(outfile: Path, files: list[tuple[Path, str | Path]], main_folder: str = "data"):
    print(f"Building Release zip: {outfile}...")
    main_path = Path(main_folder)
    with ZipFile(outfile, mode="w", compression=ZIP_LZMA) as zfile:
        for entry in files:
            file_list = glob(str(entry[0]), recursive=True)
            if file_list:
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
                    print(f"Adding: {arcname}")
                    zfile.write(str(file_name), arcname=arcname)
            else:
                raise FileNotFoundError(f"No files found matching: {entry[0]}")
    print("Done\n")


def test_build_release_zip(outfile: Path, files: list[tuple[Path, str | Path | tuple[str | Path, str | Path]]],
                           main_folder: str = "Spell Hotbar"):
    main_path = Path(main_folder)
    for entry in files:
        file_list = glob(str(entry[0]), recursive=True)
        for file_name in file_list:
            if isinstance(entry[1], Path):
                arcname = str(main_path / Path(file_name).relative_to(entry[1]))
            elif isinstance(entry[1], tuple):
                rel_path = entry[1][0]
                arc_folders = Path(entry[1][1])
                arcname = str(main_path / arc_folders / Path(file_name).relative_to(rel_path))
            else:
                fname = Path(file_name).name
                zip_path = Path(entry[1]) / fname
                arcname = str(main_path / zip_path)
            print(arcname)


if __name__ == "__main__":
    version = "0.2.0"

    release_zip_path = project_root / f"build/Spell Hotbar {version}.zip"
    # build_release_zip(release_zip_path, released_files_main_plugin)

    # build_release_zip(project_root / f"build/Spell Hotbar Nordic UI 1.0.zip", released_files_nordic_ui_plugin)

    # for modname in ["vulcano", "arclight", "desecration"]:
    #    build_release_zip(project_root / f"build/Spell Hotbar - {modname.capitalize()} 1.0.zip", get_spell_pack_list(modname))

    # build_release_zip(project_root / f"build/Spell Hotbar - Triumvirate 1.0.zip", released_files_triumvirate_spellpack)

    # build_release_zip(project_root / f"build/Spell Hotbar - Thunderchild 1.0.zip",
    #                  get_spell_pack_list("thunderchild", esp_name="Thunderchild - Epic Shout Package"))

    # build_release_zip(project_root / "build/Spell Hotbar - Sonic Magic 1.0.zip", get_spell_pack_list("sonic_magic", esp_name="Shockwave"))

    # build_release_zip(project_root / "build/Spell Hotbar - Storm Calling Magic 2 1.0.zip", get_spell_pack_list("storm_calling_magic2", esp_name="StormCalling"))

    # build_release_zip(project_root / "build/Spell Hotbar - Astral Magic 2 1.0.zip",
    #                  get_spell_pack_list("astral_magic_2", esp_name="Astral"))

    # build_release_zip(project_root / "build/Spell Hotbar - Apocalypse 1.0.zip",
    #                  get_spell_pack_list("apocalypse", esp_name="Apocalypse - Magic of Skyrim"))

    # build_release_zip(project_root / "build/Spell Hotbar - Odin 1.0.zip",
    #                  get_spell_pack_list("odin", esp_name="Odin - Skyrim Magic Overhaul"))

    # build_release_zip(project_root / "build/Spell Hotbar - Dark Hierophant Magic 1.0.zip",
    #                  get_spell_pack_list("dark_hierophant_magic", esp_name="Ghostlight"))

    test_build_release_zip(project_root / "build/Spell Hotbar - Dark Hierophant Magic 1.0.zip",
                           get_spell_pack_list("dark_hierophant_magic", esp_name="Ghostlight"))
