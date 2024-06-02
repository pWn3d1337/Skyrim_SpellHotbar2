from pathlib import Path
import cv2


# this removes all png comment from images

def copy_and_clean_image(path_in: Path, path_out: Path):
    if not path_out.parent.exists():
        path_out.parent.mkdir(parents=True)
    img_in = cv2.imread(str(path_in), cv2.IMREAD_UNCHANGED)
    cv2.imwrite(str(path_out), img_in)


def cleanse_images(input_dir: str, output_dir: str, subdirs: bool = True, only_new: bool = False):
    path = Path(input_dir)
    out = Path(output_dir)
    for p in path.glob("*/*.png" if subdirs else "*.png"):
        rel_path = p.relative_to(path)
        out_path = out / rel_path
        if only_new and out_path.exists():
            pass
        else:
            copy_and_clean_image(p, out_path)


if __name__ == "__main__":
    input_dir_root = r"F:\Skyrim Dev\Python_projects\SpellHotbar"
    output_dir_root = r"F:\Skyrim Dev\Skyrim_SpellHotbar2\python_scripts"

    #cleanse_images(f"{input_dir_root}/ai_spell_icons", f"{output_dir_root}/vanilla_spell_icons")
    #cleanse_images(f"{input_dir_root}/ai_spell_icons", f"{output_dir_root}/vanilla_spell_icons", only_new=True)

    #cleanse_images(f"{input_dir_root}/modded_spell_icons", f"{output_dir_root}/modded_spell_icons")
    #cleanse_images(f"{input_dir_root}/icons", f"{output_dir_root}/icons", subdirs=False)
    #cleanse_images(f"{input_dir_root}/icons", f"{output_dir_root}/icons", subdirs=True)

    #cleanse_images(f"{input_dir_root}/ai_spell_icons/shouts_generic", f"{output_dir_root}/vanilla_spell_icons/shouts_generic", subdirs=False)

    #cleanse_images(f"{input_dir_root}/modded_spell_icons/thunderchild", f"{output_dir_root}/modded_spell_icons/thunderchild", subdirs=False)

    #cleanse_images(f"{input_dir_root}/modded_spell_icons/storm_calling_magic2", f"{output_dir_root}/modded_spell_icons/storm_calling_magic2", subdirs=False)
    #cleanse_images(f"{input_dir_root}/modded_spell_icons/sonic_magic", f"{output_dir_root}/modded_spell_icons/sonic_magic", subdirs=False)

    #cleanse_images(f"{input_dir_root}/modded_spell_icons/astral_magic_2", f"{output_dir_root}/modded_spell_icons/astral_magic_2", subdirs=False)

    #cleanse_images(f"{input_dir_root}/modded_spell_icons/apocalypse_alteration", f"{output_dir_root}/modded_spell_icons/apocalypse_alteration", subdirs=False)
    #cleanse_images(f"{input_dir_root}/modded_spell_icons/apocalypse_conjuration", f"{output_dir_root}/modded_spell_icons/apocalypse_conjuration", subdirs=False)
    #cleanse_images(f"{input_dir_root}/modded_spell_icons/apocalypse_destruction", f"{output_dir_root}/modded_spell_icons/apocalypse_destruction", subdirs=False)
    #cleanse_images(f"{input_dir_root}/modded_spell_icons/apocalypse_illusion", f"{output_dir_root}/modded_spell_icons/apocalypse_illusion", subdirs=False)
    #cleanse_images(f"{input_dir_root}/modded_spell_icons/apocalypse_restoration", f"{output_dir_root}/modded_spell_icons/apocalypse_restoration", subdirs=False)

    #cleanse_images(f"{input_dir_root}/modded_spell_icons/elemental_destruction_magic_redux", f"{output_dir_root}/modded_spell_icons/elemental_destruction_magic_redux", subdirs=False)

    #cleanse_images(f"{input_dir_root}/modded_spell_icons/odin_alteration", f"{output_dir_root}/modded_spell_icons/odin_alteration", subdirs=False)
    #cleanse_images(f"{input_dir_root}/modded_spell_icons/odin_conjuration", f"{output_dir_root}/modded_spell_icons/odin_conjuration", subdirs=False)
    #cleanse_images(f"{input_dir_root}/modded_spell_icons/odin_destruction", f"{output_dir_root}/modded_spell_icons/odin_destruction", subdirs=False)
    #cleanse_images(f"{input_dir_root}/modded_spell_icons/odin_illusion", f"{output_dir_root}/modded_spell_icons/odin_illusion", subdirs=False)
    #cleanse_images(f"{input_dir_root}/modded_spell_icons/odin_restoration", f"{output_dir_root}/modded_spell_icons/odin_restoration", subdirs=False)

    #cleanse_images(f"{input_dir_root}/modded_spell_icons/constellation_magic", f"{output_dir_root}/modded_spell_icons/constellation_magic", subdirs=False)

    #cleanse_images(f"{input_dir_root}/modded_spell_icons/miracles_of_skyrim", f"{output_dir_root}/modded_spell_icons/miracles_of_skyrim", subdirs=False)

    #cleanse_images(f"{input_dir_root}/modded_spell_icons/dark_hierophant_magic", f"{output_dir_root}/modded_spell_icons/dark_hierophant_magic", subdirs=False)

    #cleanse_images(f"{input_dir_root}/extra_icons", f"{output_dir_root}/extra_icons", subdirs=False)

    #cleanse_images(f"{input_dir_root}/modded_spell_icons/andromeda", f"{output_dir_root}/modded_spell_icons/andromeda", subdirs=False)

    #cleanse_images(f"{input_dir_root}/modded_spell_icons/ordinator", f"{output_dir_root}/modded_spell_icons/ordinator", subdirs=False)
