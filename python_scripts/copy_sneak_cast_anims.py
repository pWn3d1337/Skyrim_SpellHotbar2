import shutil
from pathlib import Path

dir = r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2"


def copy_over(p_in: Path, p_out: Path):
    if p_in.exists() and not p_out.exists():
        shutil.copy(p_in, p_out)
        print(f"Copying {p_in} -> {p_out}")

def check_subfolder(dir: Path):
    if dir.exists():

        shout_inhale = dir / "1hm_shout_inhale.hkx"
        shout_exhale = dir / "mt_shout_exhale.hkx"

        shout_inhale_sneak = dir / "sneak1hm_shout_inhale.hkx"
        shout_exhale_sneak = dir / "sneak1hm_shout_exhale.hkx"

        copy_over(shout_inhale, shout_inhale_sneak)
        copy_over(shout_exhale, shout_exhale_sneak)


if __name__ == "__main__":
    for folder in Path(dir).glob("*"):
        if folder.is_dir() and not folder.name.lower().endswith("off"):
            check_subfolder(folder / "animations")
            check_subfolder(folder / "_1stperson/animations")