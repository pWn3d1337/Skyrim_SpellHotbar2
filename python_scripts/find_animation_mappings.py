import hashlib
from pathlib import Path

extracted_anims_folder = Path(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character")

replacement_anims_folder = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2")

hash_dict: dict[str, str] = dict()

used_anims: set[str] = set()

def get_file_hash(path: Path, algorithm: str = "md5"):
    hfunc = hashlib.new(algorithm)
    with open(path, "rb") as infile:
        while chunk := infile.read(8192):
            hfunc.update(chunk)

    return hfunc.hexdigest()


if __name__ == "__main__":
    for a in extracted_anims_folder.rglob("*.hkx"):
        rel_path = a.relative_to(extracted_anims_folder)
        h = get_file_hash(a)
        hash_dict[h] = rel_path
        #print(f"{rel_path} -> {h}")

    for b in replacement_anims_folder.rglob("*.hkx"):
        rel_path = b.relative_to(replacement_anims_folder)
        if not str(rel_path).startswith("cast_thuum"):
            h = get_file_hash(b)
            if h in hash_dict:
                orig = hash_dict[h]
                used_anims.add(orig)
            else:
                orig = "Unknown"
            print(f"{rel_path} == {orig}")

    print(f"Used Anims: {used_anims}")