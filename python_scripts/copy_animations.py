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


def copy_anim_to_multiple_dict(input_anims: dict[str, list[str]], infolder: str, outputfolder: str):
    in_path = Path(infolder)

    for in_anim, out_list in input_anims.items():
        full_in_path = in_path / in_anim
        copy_anim_to_multiple(str(full_in_path), outputfolder, out_list)


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
        #"sneak_turnleft180.hkx",
        #"sneak_turnleft60.hkx",
        #"sneak_turnright180.hkx",
        #"sneak_turnright60.hkx",
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

    anims_1p_turn_move = [
        "1hm_1stp_run.hkx",
        "1hm_1stp_turnleft.hkx",
        "1hm_1stp_turnright.hkx",
        "1hm_1stp_walk.hkx",
        "1hmlefthand_1stp_run.hkx",
        "1hmlefthand_1stp_walk.hkx",
        "2hm_1stp_run.hkx",
        "2hm_1stp_walk.hkx",
        "bow_1stp_run.hkx",
        "bow_1stp_walk.hkx",
        "dlc01/crossbow_1stp_run.hkx",
        "dlc01/crossbow_1stp_walk.hkx",
        "dw_1stp_turnleft.hkx",
        "dw_1stp_turnright.hkx",
        "h2h_1stp_run.hkx",
        "h2h_1stp_walk.hkx",
        "magic_lhand_1stp_run.hkx",
        "magic_lhand_1stp_walk.hkx",
        "magic_rhand_1stp_run.hkx",
        "magic_rhand_1stp_walk.hkx",
        "sneak1hm_1stp_run.hkx",
        "sneak1hm_1stp_walk.hkx",
        "sneakbow_1stp_run.hkx",
        "sneakbow_1stp_walk.hkx",
        "staffmagic_1stp_run.hkx",
        "staffmagic_1stp_walk.hkx",
        "staffmagicright_1stp_run.hkx",
        "staffmagicright_1stp_walk.hkx",
        "tor_1stp_run.hkx",
        "tor_1stp_walk.hkx"
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

    # copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\animations\mrh_wardloop.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_right_conc_ward\animations",
    #    anims_no_shout)

    # copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\_1stperson\animations\mrh_wardloop.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_right_conc_ward\_1stperson\animations",
    #    anims_1p_no_shout)

    # copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\animations\dmagselfconloop.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_dual_conc_self\animations",
    #    anims_no_shout)

    # copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\_1stperson\animations\dmagselfconloop.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_dual_conc_self\_1stperson\animations",
    #    anims_1p_no_shout)

    _1st_person_walkrun = [
        "mt_runbackward.hkx",
        "mt_runbackwardleft.hkx",
        "mt_runbackwardright.hkx",
        "mt_runforward.hkx",
        # "mt_runforwardcamerabob.hkx",
        "mt_runforwardleft.hkx",
        "mt_runforwardright.hkx",
        "mt_runleft.hkx",
        "mt_runright.hkx",
        # "mt_sprintforward.hkx",
        # "mt_sprintforwardsword.hkx",
        "mt_walkbackward.hkx",
        "mt_walkbackwardleft.hkx",
        "mt_walkbackwardright.hkx",
        "mt_walkforward.hkx",
        # "mt_walkforwardcamerabob.hkx",
        "mt_walkforwardleft.hkx",
        "mt_walkforwardright.hkx",
        # "mt_walkingcamera.hkx",
        "mt_walkleft.hkx",
        "mt_walkright.hkx"
    ]

    walkanims = [
        #"1hm_1stp_walk.hkx",
        "1hm_walkbackward.hkx",
        "1hm_walkbackwardleft.hkx",
        "1hm_walkbackwardright.hkx",
        "1hm_walkforward.hkx",
        "1hm_walkforwardleft.hkx",
        "1hm_walkforwardright.hkx",
        "1hm_walkleft.hkx",
        "1hm_walkright.hkx",
        "2hm_walkbackward.hkx",
        "2hm_walkbackwardleft.hkx",
        "2hm_walkbackwardright.hkx",
        "2hm_walkforward.hkx",
        "2hm_walkforwardleft.hkx",
        "2hm_walkforwardright.hkx",
        "2hm_walkleft.hkx",
        "2hm_walkright.hkx",
        "2hw_walkarmblend.hkx",
        #"bow_1stp_walk.hkx",
        "bow_walkbackward.hkx",
        "bow_walkbackwardleft.hkx",
        "bow_walkbackwardright.hkx",
        "bow_walkforward.hkx",
        "bow_walkforwardleft.hkx",
        "bow_walkforwardright.hkx",
        "bow_walkleft.hkx",
        "bow_walkright.hkx",
        "bowdrawn_walkbackward.hkx",
        "bowdrawn_walkbackwardleft.hkx",
        "bowdrawn_walkbackwardright.hkx",
        "bowdrawn_walkforward.hkx",
        "bowdrawn_walkforwardleft.hkx",
        "bowdrawn_walkforwardright.hkx",
        "bowdrawn_walkleft.hkx",
        "bowdrawn_walkright.hkx",
        "dlc01/crossbow_walkbackward.hkx",
        "dlc01/crossbow_walkbackwardleft.hkx",
        "dlc01/crossbow_walkbackwardright.hkx",
        "dlc01/crossbow_walkforward.hkx",
        "dlc01/crossbow_walkforwardleft.hkx",
        "dlc01/crossbow_walkforwardright.hkx",
        "dlc01/crossbow_walkleft.hkx",
        "dlc01/crossbow_walkright.hkx",
        "female/mt_walkbackward.hkx",
        "female/mt_walkbackwardleft.hkx",
        "female/mt_walkbackwardright.hkx",
        "female/mt_walkforward.hkx",
        "female/mt_walkforwardleft.hkx",
        "female/mt_walkforwardright.hkx",
        "female/mt_walkleft.hkx",
        "female/mt_walkright.hkx",
        "mag_walkbackward.hkx",
        "mag_walkbackwardleft.hkx",
        "mag_walkbackwardright.hkx",
        "mag_walkforward.hkx",
        "mag_walkforwardleft.hkx",
        "mag_walkforwardright.hkx",
        "mag_walkleft.hkx",
        "mag_walkright.hkx",
        "magcast_walkbackward.hkx",
        "magcast_walkbckwrdleft.hkx",
        "magcast_walkbckwrdrht.hkx",
        "magcast_walkforward.hkx",
        "magcast_walkforwrdleft.hkx",
        "magcast_walkfrwrdright.hkx",
        "magcast_walkleft.hkx",
        "magcast_walkright.hkx",
        #"magic_lhand_1stp_walk.hkx",
        #"magic_rhand_1stp_walk.hkx",
        "male/mt_walkbackward.hkx",
        "male/mt_walkbackwardleft.hkx",
        "male/mt_walkbackwardright.hkx",
        "male/mt_walkforward.hkx",
        "male/mt_walkforwardleft.hkx",
        "male/mt_walkforwardright.hkx",
        "male/mt_walkleft.hkx",
        "male/mt_walkright.hkx",
        #"sneakwalk_bckward.hkx",
        #"sneakwalk_bckwrdleft.hkx",
        #"sneakwalk_bckwrdright.hkx",
        #"sneakwalk_forward.hkx",
        #"sneakwalk_frwrdright.hkx",
        #"sneakwalk_fwrwrdleft.hkx",
        #"sneakwalk_left.hkx",
        #"sneakwalk_right.hkx",
        #"staffmagic_1stp_walk.hkx",
        #"staffmagic_walkarm.hkx",
        #"staffmagicright_1stp_walk.hkx",
        #"staffmagicright_walkarm.hkx",
        #"tor_1stp_walk.hkx"
    ]

    runanims = [
        #"1hm_1stp_run.hkx",
        "1hm_runbackward.hkx",
        "1hm_runbackwardleft.hkx",
        "1hm_runbackwardright.hkx",
        "1hm_runforward.hkx",
        "1hm_runforwardleft.hkx",
        "1hm_runforwardright.hkx",
        "1hm_runleft.hkx",
        "1hm_runright.hkx",
        "1hm_runstrafeleft.hkx",
        "1hm_runstraferight.hkx",
        "2hm_runbackward.hkx",
        "2hm_runbackwardleft.hkx",
        "2hm_runbackwardright.hkx",
        "2hm_runforward.hkx",
        "2hm_runforwardleft.hkx",
        "2hm_runforwardright.hkx",
        "2hm_runleft.hkx",
        "2hm_runright.hkx",
        "2hm_runstrafeleft.hkx",
        "2hm_runstraferight.hkx",
        #"bow_1stp_run.hkx",
        "bow_runbackward.hkx",
        "bow_runbackwardleft.hkx",
        "bow_runbackwardright.hkx",
        "bow_runforward.hkx",
        "bow_runforwardleft.hkx",
        "bow_runforwardright.hkx",
        "bow_runleft.hkx",
        "bow_runright.hkx",
        "bow_runstrafeleft.hkx",
        "bow_runstraferight.hkx",
        "dlc01/crossbow_runbackward.hkx",
        "dlc01/crossbow_runbackwardleft.hkx",
        "dlc01/crossbow_runbackwardright.hkx",
        "dlc01/crossbow_runforward.hkx",
        "dlc01/crossbow_runforwardleft.hkx",
        "dlc01/crossbow_runforwardright.hkx",
        "dlc01/crossbow_runleft.hkx",
        "dlc01/crossbow_runright.hkx",
        "dlc01/crossbow_runstrafeleft.hkx",
        "dlc01/crossbow_runstraferight.hkx",
        "female/mt_runbackward.hkx",
        "female/mt_runbackwardleft.hkx",
        "female/mt_runbackwardright.hkx",
        "female/mt_runforward.hkx",
        "female/mt_runforwardleft.hkx",
        "female/mt_runforwardright.hkx",
        "female/mt_runleft.hkx",
        "female/mt_runright.hkx",
        "mag_runbackward.hkx",
        "mag_runbackwardleft.hkx",
        "mag_runbackwardright.hkx",
        "mag_runforward.hkx",
        "mag_runforwardleft.hkx",
        "mag_runforwardright.hkx",
        "mag_runleft.hkx",
        "mag_runright.hkx",
        "mag_runstrafeleft.hkx",
        "mag_runstraferight.hkx",
        "magcast_runbackward.hkx",
        "magcast_runbackwrdleft.hkx",
        "magcast_runbckwrdright.hkx",
        "magcast_runforward.hkx",
        "magcast_runforwardleft.hkx",
        "magcast_runfrwrdright.hkx",
        "magcast_runleft.hkx",
        "magcast_runright.hkx",
        "magcast_runstrafeleft.hkx",
        "magcast_runstraferight.hkx",
        #"magic_lhand_1stp_run.hkx",
        #"magic_rhand_1stp_run.hkx",
        "male/mt_runbackward.hkx",
        "male/mt_runbackwardleft.hkx",
        "male/mt_runbackwardright.hkx",
        "male/mt_runforward.hkx",
        "male/mt_runforwardleft.hkx",
        "male/mt_runforwardright.hkx",
        "male/mt_runleft.hkx",
        "male/mt_runright.hkx",
        #"sneakrun_backward.hkx",
        #"sneakrun_bckwrdleft.hkx",
        #"sneakrun_bckwrdright.hkx",
        #"sneakrun_forward.hkx",
        #"sneakrun_forwardroll.hkx",
        #"sneakrun_frwrdleft.hkx",
        #"sneakrun_frwrdright.hkx",
        #"sneakrun_left.hkx",
        #"sneakrun_right.hkx",
        #"staffmagic_1stp_run.hkx",
        #"staffmagic_runarm.hkx",
        #"staffmagicright_1stp_run.hkx",
        #"staffmagicright_runarm.hkx",
        #"tor_1stp_run.hkx"
    ]



    # copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_left_conc\_1stperson\animations\mt_shout_exhale.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_left_conc\_1stperson\animations",
    #    _1st_person_walkrun)

    #copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_left_conc\_1stperson\animations\mt_shout_exhale.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_left_conc\_1stperson\animations",
    #    anims_1p_turn_move)

    #copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_right_conc\_1stperson\animations\mt_shout_exhale.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_right_conc\_1stperson\animations",
    #    anims_1p_turn_move + _1st_person_walkrun)

    #Not good!
    #copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_right_conc\animations\mt_shout_exhale.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_right_conc\animations",
    #    walkanims + runanims + anims_turn)

    #Not good!
    #copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_left_conc\animations\mt_shout_exhale.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_left_conc\animations",
    #    walkanims + runanims + anims_turn)

    idle_anims = [
        "bow_idleheld.hkx",
        "dw1hm1hmidle.hkx",
        "h2h_idle.hkx",
        "mlh_idle.hkx",
        "staff_idle.hkx",
        "staffright_idle.hkx",
        "1hm_idle.hkx",
        "2hm_idle.hkx",
        "2hw_idle.hkx",
        "male/mt_idle.hkx",
        "female/mt_idle.hkx",
        "DLC01/crossbow_idleheld.hkx",
    ]

    sneak_idle_anims = [
        "sneak1hm_idle.hkx",
        "sneakh2h_idle.hkx",
        "sneakmtidle.hkx"
    ]

    sneak_walk = {
        "sneakwalk_forward.hkx": [
            "female/mt_walkforward.hkx",
            "male/mt_walkforward.hkx",
            "female/mt_runforward.hkx",
            "male/mt_runforward.hkx"
        ],
        "sneakwalk_bckward.hkx": [
            "female/mt_walkbackward.hkx",
            "male/mt_walkbackward.hkx",
            "female/mt_runbackward.hkx",
            "male/mt_runbackward.hkx"
        ],
        "sneakwalk_left.hkx": [
            "female/mt_walkleft.hkx",
            "male/mt_walkleft.hkx",
            "female/mt_runleft.hkx",
            "male/mt_runleft.hkx"
        ],
        "sneakwalk_right.hkx": [
            "female/mt_walkright.hkx",
            "male/mt_walkright.hkx",
            "female/mt_runright.hkx",
            "male/mt_runright.hkx"
        ],
        "sneakwalk_bckwrdleft.hkx": [
            "female/mt_walkbackwardleft.hkx",
            "male/mt_walkbackwardleft.hkx",
            "female/mt_runbackwardleft.hkx",
            "male/mt_runbackwardleft.hkx"
        ],
        "sneakwalk_bckwrdright.hkx": [
            "female/mt_walkbackwardright.hkx",
            "male/mt_walkbackwardright.hkx",
            "female/mt_runbackwardright.hkx",
            "male/mt_runbackwardright.hkx"
        ],
        "sneakwalk_frwrdright.hkx": [
            "female/mt_walkforwardright.hkx",
            "male/mt_walkforwardright.hkx",
            "female/mt_runforwardright.hkx",
            "male/mt_runforwardright.hkx"
        ],
        "sneakwalk_fwrwrdleft.hkx": [
            "female/mt_walkforwardleft.hkx",
            "male/mt_walkforwardleft.hkx",
            "female/mt_runforwardleft.hkx",
            "male/mt_runforwardleft.hkx"
        ],
        "sneakmtidle.hkx":
            idle_anims

    }

    #copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_ritual_aimed_conc\animations\mt_shout_exhale.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_ritual_aimed_conc\animations",
    #    idle_anims)

    #copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_ritual_aimed_conc\animations\mt_shout_exhale.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_ritual_aimed_conc\animations",
    #    sneak_idle_anims)

    #copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_left_conc_self\_1stperson\animations\mt_shout_exhale.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_left_conc_self\_1stperson\animations",
    #    anims_1p_turn_move + _1st_person_walkrun)

    #copy_anim_to_multiple(
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_right_conc_self\_1stperson\animations\mt_shout_exhale.hkx",
    #    r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_1h_right_conc_self\_1stperson\animations",
    #    anims_1p_turn_move + _1st_person_walkrun)

    #sneak run anims
    #copy_anim_to_multiple_dict(sneak_walk, r"F:\Skyrim Dev\TOOLS\EXTRACT\Animations_BSA\meshes\actors\character\animations", r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\meshes\actors\character\OpenAnimationReplacer\SpellHotbar2\cast_running_sneak\animations")
