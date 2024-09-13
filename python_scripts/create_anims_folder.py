from pathlib import Path
import json
from copy_animations import copy_anim_to_multiple

path_to_mo2_mods = Path(__file__).parent / r"..\..\ADT\mods"
out_folder = path_to_mo2_mods / r"Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2"

anim_path = Path(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\animations")
anim_path_1p = Path(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\_1stperson\animations")


def _create_anim_json(folder_name: str, name: str, anim_id: int, casting_source: int, is_conc_casting: tuple[str, int],
                      anim_id_offset: int = 0):
    jstring = f'''{{
    "name": "{name}",
    "description": "{name} casting animation",
    "priority": {99000000 + anim_id * 10 + anim_id_offset},
    "interruptible": true,
    "conditions": [
        {{
            "condition": "CompareValues",
            "requiredVersion": "1.0.0.0",
            "Value A": {{
                "form": {{
                    "pluginName": "SpellHotbar.esp",
                    "formID": "815"
                }}
            }},
            "Comparison": "==",
            "Value B": {{
                "value": {anim_id:.1f}
            }}
        }},
        {{
            "condition": "CompareValues",
            "requiredVersion": "1.0.0.0",
            "Value A": {{
                "form": {{
                    "pluginName": "SpellHotbar.esp",
                    "formID": "835"
                }}
            }},
            "Comparison": "==",
            "Value B": {{
                "value": {casting_source:.1f}
            }}
        }},
        {{
            "condition": "CompareValues",
            "requiredVersion": "1.0.0.0",
            "Value A": {{
                "form": {{
                    "pluginName": "SpellHotbar.esp",
                    "formID": "834"
                }}
            }},
            "Comparison": "{is_conc_casting[0]}",
            "Value B": {{
                "value": {is_conc_casting[1]:.1f}
            }}
        }}
    ]
}}'''
    if not (out_folder / folder_name).exists():
        (out_folder / folder_name).mkdir()

    with open(out_folder / folder_name / "config.json", "w") as outfile:
        outfile.write(jstring)


def create_anim_folder(folder_name: str, anim_id: int, casting_source: int, is_conc_casting: tuple[str, int], anim_id_offset: int,
                       inhale_anim: str, exhale_anim: str | None = None, inhale_anim_1p: str | None = None,
                       exhale_anim_1p: str | None = None, sneak_inhale_anim: str | None = None,
                       sneak_exhale_anim: str | None = None):
    name = folder_name.replace("_", " ")
    _create_anim_json(folder_name, name, anim_id, casting_source, is_conc_casting, anim_id_offset)

    if inhale_anim_1p is None:
        inhale_anim_1p = inhale_anim

    inhale_anim = anim_path / inhale_anim
    inhale_anim_1p = anim_path_1p / inhale_anim_1p

    if exhale_anim is None:
        exhale_anim = inhale_anim
    else:
        exhale_anim = anim_path / exhale_anim
        exhale_anim = anim_path / exhale_anim

    if sneak_inhale_anim is None:
        sneak_inhale_anim = inhale_anim
    else:
        sneak_inhale_anim = anim_path / sneak_inhale_anim

    if sneak_exhale_anim is None:
        sneak_exhale_anim = exhale_anim
    else:
        sneak_exhale_anim = anim_path / sneak_exhale_anim

    if exhale_anim_1p is None:
        exhale_anim_1p = inhale_anim_1p
    else:
        exhale_anim_1p = anim_path_1p / exhale_anim_1p

    outf = str(out_folder / folder_name / "animations")
    copy_anim_to_multiple(str(inhale_anim), outf, ["1hm_shout_inhale.hkx", "mt_shout_inhale.hkx"])
    copy_anim_to_multiple(str(exhale_anim), outf, ["mt_shout_exhale.hkx"])
    copy_anim_to_multiple(str(sneak_inhale_anim), outf, ["sneak1hm_shout_inhale.hkx"])
    copy_anim_to_multiple(str(sneak_exhale_anim), outf, ["sneak1hm_shout_exhale.hkx"])

    outf_1p = str(out_folder / folder_name / "_1stperson/animations")
    copy_anim_to_multiple(str(inhale_anim_1p), outf_1p, ["1hm_shout_inhale.hkx", "mt_shout_inhale.hkx"])
    copy_anim_to_multiple(str(exhale_anim_1p), outf_1p, ["mt_shout_exhale.hkx"])


if __name__ == "__main__":
    # print(f"{out_folder.exists()}: {out_folder}")
    # _create_anim_json("test", "testt", 101, 0, 0, 0)

    # anim_path = Path(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\animations")
    # anim_path_1p = Path(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\animations")

    #create_anim_folder("cast_1h_left_conc_start", 1001, casting_source=0, is_conc_casting=("==", 0), anim_id_offset=0,
    #                   #inhale_anim="mlh_preaimedcon.hkx")
    #                   inhale_anim="mlh_chargeloop.khx")

    #create_anim_folder("cast_1h_left_conc", 1001, casting_source=0, is_conc_casting=(">", 0), anim_id_offset=1,
    #                   inhale_anim="mlhmrh_aimedconcentrationloop.hkx", inhale_anim_1p="mlh_aimedconcentration")
