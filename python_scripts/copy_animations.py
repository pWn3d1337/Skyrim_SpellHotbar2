import shutil
from pathlib import Path

import pandas as pd

animation_lists = Path(__file__).parent / "animation_list" / "vanilla_move_anims.csv"


def copy_anims(anim_src_folder: Path, base_dar_output_folder: Path):
    df = pd.read_csv(animation_lists, sep='\t')

    for index, row in df.iterrows():
        src_anim = Path(anim_src_folder) / row.iloc[0]
        for i in range(1, row.size):
            if not pd.isna(row.iloc[i]):
                target_anim = Path(base_dar_output_folder) / row.iloc[i]

                if not target_anim.parent.exists():
                    print(f"Creating Directory: {target_anim.parent}")
                    target_anim.parent.mkdir(parents=True)
                print(f"Copying {src_anim} -> {target_anim}")
                shutil.copy(src_anim, target_anim)


def copy_anim_to_multiple(input_anim: str, output_folder: str, filenames: list[str]):
    path_in = Path(input_anim)
    path_folder_out = Path(output_folder)

    if path_in.exists():
        for filename in filenames:
            out_path = path_folder_out / filename

            if not out_path.parent.exists():
                print(f"Creating Folder {out_path.parent}")
                out_path.parent.mkdir(parents=True)

            print(f"Copying {input_anim} -> {out_path}")
            shutil.copy(input_anim, out_path)
    else:
        print(f"Path not existing: {input_anim}")


if __name__ == "__main__":
    # out_path = r"F:\Skyrim Dev\ADT\mods\Spell Hotbar\meshes\actors\character\animations\DynamicAnimationReplacer\_CustomConditions\119999"

    # anim_src_folder = r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\animations"

    # copy_anims(anim_src_folder, out_path)

    # copy channel anims
    anims = [
        "bow_idleheld.hkx",
        "dw1hm1hmidle.hkx",
        "h2h_idle.hkx",
        "mlh_idle.hkx",
        "mt_shout_exhale.hkx",
        # "mt_shout_exhale_long.hkx",
        "staff_idle.hkx",
        "staffright_idle.hkx",
        "1hm_idle.hkx",
        "1hm_shout_exhale.hkx",
        "mt_shout_inhale.hkx",
        "2hm_idle.hkx",
        "2hw_idle.hkx",
        "male/mt_idle.hkx",
        "female/mt_idle.hkx",
        "DLC01/crossbow_idleheld.hkx",
    ]
    anims_no_shout = [
        "bow_idleheld.hkx",
        "dw1hm1hmidle.hkx",
        "h2h_idle.hkx",
        "mlh_idle.hkx",
        # "mt_shout_exhale.hkx",
        # "mt_shout_exhale_long.hkx",
        "staff_idle.hkx",
        "staffright_idle.hkx",
        "1hm_idle.hkx",
        # "1hm_shout_exhale.hkx",
        # "mt_shout_inhale.hkx",
        "2hm_idle.hkx",
        "2hw_idle.hkx",
        "male/mt_idle.hkx",
        "female/mt_idle.hkx",
        "DLC01/crossbow_idleheld.hkx",
    ]
    anims_turn = [
        "1hm_turnleft180.hkx",
        "1hm_turnleft60.hkx",
        "1hm_turnright180.hkx",
        "1hm_turnright60.hkx",
        "2hm_turnleft180.hkx",
        "2hm_turnleft60.hkx",
        "2hm_turnright180.hkx",
        "2hm_turnright60.hkx",
        "bow_turnleft180.hkx",
        "bow_turnleft60.hkx",
        "bow_turnright180.hkx",
        "bow_turnright60.hkx",
        "dlc01/crossbow_turnleft180.hkx",
        "dlc01/crossbow_turnleft60.hkx",
        "dlc01/crossbow_turnright180.hkx",
        "dlc01/crossbow_turnright60.hkx",
        "mag_turnleft180.hkx",
        "mag_turnleft60.hkx",
        "mag_turnright180.hkx",
        "mag_turnright60.hkx",
        "magcast_turnleft180.hkx",
        "magcast_turnleft60.hkx",
        "magcast_turnright180.hkx",
        "magcast_turnright60.hkx",
        "mt_turnleft180.hkx",
        "mt_turnleft360.hkx",
        "mt_turnleft60.hkx",
        "mt_turnright180.hkx",
        "mt_turnright360.hkx",
        "mt_turnright60.hkx",
        "shd_turnleft180.hkx",
        "shd_turnleft60.hkx",
        "shd_turnright180.hkx",
        "shd_turnright60.hkx",
        "staffmagiccast_turnleft180.hkx",
        "staffmagiccast_turnleft60.hkx",
        "staffmagiccast_turnright180.hkx",
        "staffmagiccast_turnright60.hkx"
    ]

    anims_1p = [
        "bow_idleheld.hkx",
        "dw1hm1hmidle.hkx",
        "h2h_idle.hkx",
        "mlh_idle.hkx",
        "mt_shout_exhale.hkx",
        # "mt_shout_exhale_long.hkx",
        "staff_idle.hkx",
        "staffright_idle.hkx",
        "1hm_idle.hkx",
        "1hm_shout_exhale.hkx",
        "mt_shout_inhale.hkx",
        "2hm_idle.hkx",
        "2hw_idle.hkx",
        "mt_idle.hkx",
        "DLC01/crossbow_idleheld.hkx",
    ]

    anims_1p_no_shout = [
        "bow_idleheld.hkx",
        "dw1hm1hmidle.hkx",
        "h2h_idle.hkx",
        "mlh_idle.hkx",
        # "mt_shout_exhale.hkx",
        # "mt_shout_exhale_long.hkx",
        "staff_idle.hkx",
        "staffright_idle.hkx",
        "1hm_idle.hkx",
        # "1hm_shout_exhale.hkx",
        # "mt_shout_inhale.hkx",
        "2hm_idle.hkx",
        "2hw_idle.hkx",
        "mt_idle.hkx",
        "DLC01/crossbow_idleheld.hkx",
    ]

    anims_1p_turn = [
        "1hm_1stp_turnleft.hkx",
        "1hm_1stp_turnright.hkx",
        "dw_1stp_turnleft.hkx",
        "dw_1stp_turnright.hkx"
    ]

    # copy_anim_to_multiple(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar\meshes\actors\character\animations\DynamicAnimationReplacer\_CustomConditions\111802\mt_shout_exhale.hkx",
    #                       r"F:\Skyrim Dev\ADT\mods\Spell Hotbar\meshes\actors\character\animations\DynamicAnimationReplacer\_CustomConditions\111803",
    #                       anims)

    # anim_in = r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\_1stperson\animations\mrh_aimedconcentration.hkx"
    #
    # copy_anim_to_multiple(anim_in,
    #                     r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_left_conc_idle\_1stperson\animations",
    #                      anims)
    #

    # copy_anim_to_multiple(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\animations\mrh_aimedconcentration.hkx",
    #                      r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_right_conc_idle\animations",
    #                      anims)

    # copy_anim_to_multiple(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\_1stperson\animations\mrh_aimedconcentration.hkx",
    #                      r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_right_conc_idle\_1stperson\animations",
    #                      anims_1p)

    # copy_anim_to_multiple(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\_1stperson\animations\mlhmrh_aimedconcentrationloop.hkx",
    #                      r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_left_conc_idle\_1stperson\animations",
    #                      anims_1p)

    # copy_anim_to_multiple(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\_1stperson\animations\stfmagic_aimconloop.hkx",
    #                      r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_left_conc_staff\_1stperson\animations",
    #                      anims_1p)

    # copy_anim_to_multiple(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\animations\stfmagic_aimconloop.hkx",
    #                      r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_left_conc_staff\animations",
    #                      anims)

    # copy_anim_to_multiple(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\animations\mlh_selfconcentration.hkx",
    #                      r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_left_conc_self\animations",
    #                      anims)
    #
    # copy_anim_to_multiple(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\_1stperson\animations\mlh_selfconcentration.hkx",
    #                      r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_left_conc_self\_1stperson\animations",
    #                      anims_1p)
    #

    # copy_anim_to_multiple(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\animations\mrh_selfconcentration.hkx",
    #                      r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_right_conc_self\animations",
    #                      anims)
    #
    # copy_anim_to_multiple(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\_1stperson\animations\mrh_selfconcentration.hkx",
    #                      r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_right_conc_self\_1stperson\animations",
    #                      anims_1p)

    # copy_anim_to_multiple(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\animations\dmagaimconloop.hkx",
    #                      r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_ritual_aimed_conc_start\animations",
    #                      #anims_no_shout)
    #                      anims_turn)

    # copy_anim_to_multiple(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\animations\dmagaimconloop.hkx",
    #                      r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_ritual_aimed_conc\animations",
    #                      anims_turn)

    # copy_anim_to_multiple(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\_1stperson\animations\dmagaimconloop.hkx",
    #                      r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_ritual_aimed_conc_start\_1stperson\animations",
    #                      anims_1p_no_shout)

    # copy_anim_to_multiple(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\_1stperson\animations\dmagaimconloop.hkx",
    #                      r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_ritual_aimed_conc\_1stperson\animations",
    #                      anims_1p_no_shout)

    # copy_anim_to_multiple(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\_1stperson\animations\dmagaimconloop.hkx",
    #                      r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_ritual_aimed_conc\_1stperson\animations",
    #                      anims_1p_turn)

    # copy_anim_to_multiple(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\animations\dmagaimconloop.hkx",
    #                      r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_dual_conc\animations",
    #                      anims_turn) # + nims_no_shout

    # copy_anim_to_multiple(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\_1stperson\animations\dmagaimconloop.hkx",
    #                      r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_dual_conc\_1stperson\animations",
    #                      anims_1p_turn) # + anims_1p_no_shout

    # copy_anim_to_multiple(r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\animations\mlh_wardloop.hkx",
    #                      r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_left_conc_ward\animations",
    #                      anims_no_shout)

    # copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\_1stperson\animations\mlh_wardloop.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_left_conc_ward\_1stperson\animations",
    #    anims_1p_no_shout)

    #copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\animations\mrh_wardloop.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_right_conc_ward\animations",
    #    anims_no_shout)

    #copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\_1stperson\animations\mrh_wardloop.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_right_conc_ward\_1stperson\animations",
    #    anims_1p_no_shout)

    #copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\animations\dmagselfconloop.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_dual_conc_self\animations",
    #    anims_no_shout)

    #copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\_1stperson\animations\dmagselfconloop.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_dual_conc_self\_1stperson\animations",
    #    anims_1p_no_shout)
