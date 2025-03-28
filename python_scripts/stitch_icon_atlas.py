import os
import subprocess

import cv2
from PIL import Image, ImageDraw, ImageFont, ImageColor
from pathlib import Path
from wand import image as wand_image  # to avoid confusion

import numpy as np
import pandas as pd
import math
import json

mod_folder_path = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2")
mod_root_path = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar 2\SKSE\Plugins\SpellHotbar")

# only needed when creating icon swf for I4
# download from http://www.swftools.org/download.html
png2swf_executable_path = r"F:\Skyrim Dev\TOOLS\SWFTools\png2swf.exe"

# PARAMS FOR RANK TEXT
rank_text_font = str(Path(__file__).parent / "icons/sovngarde_font.ttf")
# rank_text_font = str(Path(__file__).parent / "icons/9_$ConsoleFont_Futura Condensed.ttf")

# text anchor pos is center for vertical (Y), and right for horizontal (X)
rank_text_pos_x = 0.95  # 0.75 = middle right quarter
rank_text_pox_y = 0.75  # 0.75 = middle bottom quarter
rank_text_scale = 0.6  # font size compared to image size
# rank_text_color_fill = (32, 165, 218, 255)  # Colors are BGRA not RGBA!!!! that's some opencv thing (actually it's PIL this time)
rank_text_color_fill = (
    255, 255, 255, 255)  # Colors are BGRA not RGBA!!!! that's some opencv thing (actually it's PIL this time)
rank_text_color_border = (0, 0, 0, 255)

swf_jar_path = str(Path(__file__).parent / "../SWF_Generator/out/artifacts/SWF_Generator_jar/SWF_Generator.jar")
overlay_icons = {}

image_size = 128
swf_icon_size = 32

# default icon names used
default_icons = [
    "UNKNOWN",
    "BAR_EMPTY",
    "BAR_OVERLAY",
    "BAR_HIGHLIGHT",
    "UNBIND_SLOT",
    "LESSER_POWER",
    "GREATER_POWER",
    "DESTRUCTION_FIRE_NOVICE",
    "DESTRUCTION_FIRE_APPRENTICE",
    "DESTRUCTION_FIRE_ADEPT",
    "DESTRUCTION_FIRE_EXPERT",
    "DESTRUCTION_FIRE_MASTER",
    "DESTRUCTION_FROST_NOVICE",
    "DESTRUCTION_FROST_APPRENTICE",
    "DESTRUCTION_FROST_ADEPT",
    "DESTRUCTION_FROST_EXPERT",
    "DESTRUCTION_FROST_MASTER",
    "DESTRUCTION_SHOCK_NOVICE",
    "DESTRUCTION_SHOCK_APPRENTICE",
    "DESTRUCTION_SHOCK_ADEPT",
    "DESTRUCTION_SHOCK_EXPERT",
    "DESTRUCTION_SHOCK_MASTER",
    "DESTRUCTION_GENERIC_NOVICE",
    "DESTRUCTION_GENERIC_APPRENTICE",
    "DESTRUCTION_GENERIC_ADEPT",
    "DESTRUCTION_GENERIC_EXPERT",
    "DESTRUCTION_GENERIC_MASTER",
    "ALTERATION_NOVICE",
    "ALTERATION_APPRENTICE",
    "ALTERATION_ADEPT",
    "ALTERATION_EXPERT",
    "ALTERATION_MASTER",
    "RESTORATION_FRIENDLY_NOVICE",
    "RESTORATION_FRIENDLY_APPRENTICE",
    "RESTORATION_FRIENDLY_ADEPT",
    "RESTORATION_FRIENDLY_EXPERT",
    "RESTORATION_FRIENDLY_MASTER",
    "RESTORATION_HOSTILE_NOVICE",
    "RESTORATION_HOSTILE_APPRENTICE",
    "RESTORATION_HOSTILE_ADEPT",
    "RESTORATION_HOSTILE_EXPERT",
    "RESTORATION_HOSTILE_MASTER",
    "ILLUSION_FRIENDLY_NOVICE",
    "ILLUSION_FRIENDLY_APPRENTICE",
    "ILLUSION_FRIENDLY_ADEPT",
    "ILLUSION_FRIENDLY_EXPERT",
    "ILLUSION_FRIENDLY_MASTER",
    "ILLUSION_HOSTILE_NOVICE",
    "ILLUSION_HOSTILE_APPRENTICE",
    "ILLUSION_HOSTILE_ADEPT",
    "ILLUSION_HOSTILE_EXPERT",
    "ILLUSION_HOSTILE_MASTER",
    "CONJURATION_BOUND_WEAPON_NOVICE",
    "CONJURATION_BOUND_WEAPON_APPRENTICE",
    "CONJURATION_BOUND_WEAPON_ADEPT",
    "CONJURATION_BOUND_WEAPON_EXPERT",
    "CONJURATION_BOUND_WEAPON_MASTER",
    "CONJURATION_SUMMON_NOVICE",
    "CONJURATION_SUMMON_APPRENTICE",
    "CONJURATION_SUMMON_ADEPT",
    "CONJURATION_SUMMON_EXPERT",
    "CONJURATION_SUMMON_MASTER",
    "SHOUT_GENERIC",
    "SINGLE_CAST",
    "DUAL_CAST",
    "SCROLL_OVERLAY",
    # "NO_OVERLAY", No need to stitch
    "GENERIC_POTION",
    "GENERIC_POTION_SMALL",
    "GENERIC_POTION_LARGE",
    "GENERIC_POISON",
    "GENERIC_POISON_SMALL",
    "GENERIC_POISON_LARGE",
    "GENERIC_FOOD",
    "GENERIC_FOOD_SOUP",
    "GENERIC_FOOD_DRINK",
    "TAB_ALL",
    "TAB_SPELLS",
    "TAB_SCROLLS",
    "TAB_POTIONS",
    "TAB_POISONS",
    "TAB_FOOD"
]


def next_pow2(x: int) -> int:
    """
    Return next greater equal power of 2
    :param x:
    :return:
    """
    return 1 << (x - 1).bit_length()


def write_png_and_dds(cvImage: np.ndarray, output_base: str) -> None:
    cv2.imwrite(f"{output_base}.png", cvImage)
    im_rgba = cv2.cvtColor(cvImage, cv2.COLOR_BGRA2RGBA)
    with wand_image.Image.from_array(im_rgba) as img:
        img.compression = "dxt5"
        output_path = Path(f"{output_base}.dds")
        if output_path.exists():
            os.remove(output_path)
        img.save(filename=str(output_path))


def _get_image_name(spell_name: str) -> str:
    return spell_name.lower().replace(" ", "_").replace("'", "").replace(":", "").replace("-", "_").replace("’", "")


def _get_overlay(spell_name: str) -> str | None:
    if len(splitted := spell_name.split("_")) > 1:
        if (k := splitted[0].lower()) in overlay_icons:
            return k
    return None


def _find_image(base_name: str, folders: list[str]) -> str | None:
    for f in folders:
        folder = Path(f)
        images = folder.glob(f"{base_name}*.png")
        for img in images:
            return str(img)
    return None


def add_dragonlang_text_to_image(img: np.ndarray, img_size: int, dragonstr: str, dragon_font,
                                 stroke_width: int = 3) -> np.ndarray:
    pil_img = Image.fromarray(img)
    draw = ImageDraw.Draw(pil_img)
    text = dragonstr.upper().replace(' ', '\n').replace('_', '\n')
    draw.text(xy=(img_size * 0.5, img_size * 0.5), text=text, font=dragon_font,
              anchor="mm",
              align="center", stroke_fill=ImageColor.getrgb("black"), stroke_width=stroke_width, spacing=0)
    img_out = np.array(pil_img)
    return img_out


def add_rank_text_to_image(img: np.ndarray, img_size: int, ranktext: str, rank_font,
                           stroke_width: int = 1) -> np.ndarray:
    pil_img = Image.fromarray(img)
    draw = ImageDraw.Draw(pil_img)
    draw.text(xy=(img_size * rank_text_pos_x, img_size * rank_text_pox_y), text=ranktext, font=rank_font,
              anchor="rm", align="center", fill=rank_text_color_fill, stroke_fill=rank_text_color_border,
              stroke_width=stroke_width, spacing=0)
    img_out = np.array(pil_img)
    return img_out


def resize_image_with_pil(img: np.ndarray, img_size: int) -> np.ndarray:
    """PIL has higher quality downsampling with LANCZOS"""
    pil_img = Image.fromarray(img)
    pil_img = pil_img.resize((img_size, img_size), Image.Resampling.LANCZOS)
    img_out = np.array(pil_img)
    return img_out


def _stitch_images_internal(df: pd.DataFrame, alpha_mask_path: str | None) -> tuple[pd.DataFrame, np.ndarray]:
    dragon_font_path = Path(__file__).parent / 'icons/6_$DragonFont_Dragon_script.ttf'
    dragon_font = ImageFont.truetype(str(dragon_font_path), round(image_size * 0.5))

    rank_font = ImageFont.truetype(str(rank_text_font), round(image_size * rank_text_scale))

    def _get_unique_key(row: pd.Series) -> str | None:
        if not pd.isna(row['Path']):
            ret = row['Path']
            shouttext = ""
            ranktext = ""
            overlaystr = ""
            if "Shouttext" in row.index:
                shouttext = "\t" + row["Shouttext"] if not pd.isna(row["Shouttext"]) else ""
            if "Ranktext" in row.index:
                ranktext = "\t" + row["Ranktext"] if not pd.isna(row["Ranktext"]) else ""
            if "overlay" in row.index:
                overlaystr = "\t" + row["overlay"] if not pd.isna(row["overlay"]) else ""

            return f"{ret}{shouttext}{ranktext}{overlaystr}"

        return None

    df['unique_key'] = df.apply(_get_unique_key, axis=1)

    df_pathes = df[~pd.isna(df['Path'])]

    num_icons = len(df_pathes['Path'])
    num_images = len(pd.unique(df_pathes['unique_key']))

    row_len = math.ceil(math.sqrt(num_images))

    atlas_size = next_pow2(row_len * image_size)

    # adjust row len to required atlas size
    # row_len = math.floor(atlas_size/image_size)

    if alpha_mask_path is None:
        alpha_mask = np.ones((image_size, image_size), dtype="uint8") * 255
    else:
        alpha_mask = cv2.imread(alpha_mask_path, cv2.IMREAD_GRAYSCALE)
        # alpha_mask = cv2.resize(alpha_mask, (image_size, image_size), interpolation=cv2.INTER_AREA)
        alpha_mask = resize_image_with_pil(alpha_mask, image_size)

    cvImage = np.zeros((atlas_size, atlas_size, 4), dtype="uint8")

    already_used_textures = dict()

    # drop all invalid
    df = df_pathes

    x = 0
    y = 0
    path_index = df.columns.get_loc('Path')
    u0_index = df.columns.get_loc('u0')
    v0_index = df.columns.get_loc('v0')
    u1_index = df.columns.get_loc('u1')
    v1_index = df.columns.get_loc('v1')
    use_mask_index = df.columns.get_loc('use_mask')
    overlay_index = df.columns.get_loc('overlay') if 'overlay' in df.columns else -1
    dragontext_index = df.columns.get_loc('Shouttext') if 'Shouttext' in df.columns else -1
    ranktext_index = df.columns.get_loc('Ranktext') if 'Ranktext' in df.columns else -1
    unique_key_index = df.columns.get_loc('unique_key')

    overlay_images = {}
    if overlay_index >= 0:
        for k, v in overlay_icons.items():
            im = cv2.imread(v, cv2.IMREAD_UNCHANGED)
            if im is None:
                raise IOError(f"Could not load {v}")
            # im = cv2.resize(im, (image_size, image_size), interpolation=cv2.INTER_AREA)
            im = resize_image_with_pil(im, image_size)
            overlay_images[k] = im

    for i in range(num_icons):
        path_to_load = df.iloc[i, path_index]
        if not pd.isna(path_to_load) and Path(path_to_load).exists():
            unique_key = df.iloc[i, unique_key_index]
            if str(unique_key) not in already_used_textures:
                img = cv2.imread(path_to_load, cv2.IMREAD_UNCHANGED)
                # img = cv2.resize(img, (image_size, image_size), interpolation=cv2.INTER_AREA)
                img = resize_image_with_pil(img, image_size)

                if img.ndim < 3:
                    img = cv2.cvtColor(img, cv2.COLOR_GRAY2RGBA)

                # set alpha
                if img.shape[2] == 3:
                    img = cv2.cvtColor(img, cv2.COLOR_RGB2RGBA)

                # if overlay
                if overlay_index >= 0:
                    if not pd.isna(overlay := df.iloc[i, overlay_index]):
                        overlay_img = overlay_images[overlay] / 255
                        img_in = img / 255
                        result = np.zeros_like(overlay_img)
                        overlay_a = np.repeat(overlay_img[:, :, 3, np.newaxis], 3, axis=2)
                        result[:, :, :3] = (overlay_img[:, :, :3] * overlay_a + img_in[:, :, :3] * (1 - overlay_a))
                        result[:, :, 3] = 1 - ((1 - overlay_img[:, :, 3]) * (1 - img_in[:, :, 3]))
                        img = result * 255

                # if dragon font text
                if dragontext_index >= 0:
                    dragonstr: str = df.iloc[i, dragontext_index]
                    if not pd.isna(dragonstr):
                        img = add_dragonlang_text_to_image(img, image_size, dragonstr, dragon_font)

                if ranktext_index >= 0:
                    ranktext: str = df.iloc[i, ranktext_index]
                    if not pd.isna(ranktext):
                        img = add_rank_text_to_image(img, image_size, ranktext, rank_font)

                if df.iloc[i, use_mask_index]:
                    # multiply alpha mask
                    alpha_factor = alpha_mask.astype(float) / 255.0
                    img[:, :, 3] = np.multiply(img[:, :, 3], alpha_factor)

                cvImage[y: y + image_size, x: x + image_size] = img

                df.iloc[i, u0_index] = x / atlas_size
                df.iloc[i, u1_index] = (x + image_size) / atlas_size
                df.iloc[i, v0_index] = y / atlas_size
                df.iloc[i, v1_index] = (y + image_size) / atlas_size
                x = x + image_size
                if (x + image_size) > atlas_size:
                    x = 0
                    y = y + image_size
                already_used_textures[str(unique_key)] = (
                    df.iloc[i, u0_index], df.iloc[i, u1_index], df.iloc[i, v0_index], df.iloc[i, v1_index])

            else:
                print(f"{unique_key} already stitched, using existing coordinates")
                coordinates = already_used_textures[unique_key]
                df.iloc[i, u0_index] = coordinates[0]
                df.iloc[i, u1_index] = coordinates[1]
                df.iloc[i, v0_index] = coordinates[2]
                df.iloc[i, v1_index] = coordinates[3]

    return df, cvImage


def stitch_default_icons(names: list[str], icon_root: list[str], output_base: Path | str, alpha_mask_path: str | None,
                         alphamask_empty=False, alphamask_overlay=False, alphamask_highlight=False,
                         add_school_icon: bool = True):
    ser = pd.Series(names)
    df = pd.DataFrame(ser)
    df.columns = ["IconName"]

    df['Filename'] = df['IconName'].apply(_get_image_name)
    df['Path'] = df['Filename'].apply(_find_image, args=(icon_root,))
    df['u0'] = np.nan
    df['v0'] = np.nan
    df['u1'] = np.nan
    df['v1'] = np.nan
    df['use_mask'] = True
    df['overlay'] = df['IconName'].apply(_get_overlay)

    def is_shout(x) -> str | None:
        return "DS" if "shout" in str(x) else None

    df["Shouttext"] = df["Filename"].apply(is_shout)

    if not alphamask_empty:
        df.loc[(df['IconName'] == 'BAR_EMPTY', 'use_mask')] = False

    if not alphamask_overlay:
        df.loc[(df['IconName'] == 'BAR_OVERLAY', 'use_mask')] = False

    if not alphamask_highlight:
        df.loc[(df['IconName'] == 'BAR_HIGHLIGHT', 'use_mask')] = False

    df_no_image_found = df[pd.isna(df['Path'])]
    print("No Icon files found for:")
    print(df_no_image_found['IconName'])

    df, cvImage = _stitch_images_internal(df, alpha_mask_path)

    write_png_and_dds(cvImage, output_base)
    df_out = df[["IconName", "u0", "v0", "u1", "v1"]]
    df_out.to_csv(f"{output_base}.csv", index=False, sep='\t')


def stitch_folder(spell_list: list[str], icon_root: list[str], output_base: Path | str, alpha_mask_path: str | None,
                  output_data: Path | str | None = None):
    df = None
    for spells in spell_list:
        df_temp = pd.read_csv(spells, sep="\t")
        if "Symbol" not in df_temp.columns:
            df_temp["Symbol"] = np.nan
        if df is None:
            df = df_temp
        else:
            df = pd.concat([df, df_temp])

    # df.columns = ["Name", "FormID", "PluginFile"]
    df['Filename'] = df['Name'].apply(_get_image_name)
    df['Path'] = df['Filename'].apply(_find_image, args=(icon_root,))
    df['u0'] = np.nan
    df['v0'] = np.nan
    df['u1'] = np.nan
    df['v1'] = np.nan
    df['use_mask'] = True

    df_no_image_found = df[pd.isna(df['Path'])]
    print("No Icon files found for:")
    print(df_no_image_found[['Name', 'Plugin']])

    df, cvImage = _stitch_images_internal(df, alpha_mask_path)

    write_png_and_dds(cvImage, output_base)

    df_forms = df[~pd.isna(df["FormID"])]
    if "ScrollID" in df.columns:
        df_scrolls = df[~pd.isna(df["ScrollID"])]
        ind_spell_scrolls = ~pd.isna(df_scrolls["FormID"])  # scrolls that are part of spell, add name suffix
        df_scrolls.loc[ind_spell_scrolls, "Name"] = df_scrolls.loc[ind_spell_scrolls, "Name"] + " Scroll"
        df_scrolls = df_scrolls.drop(columns="FormID", inplace=False)
        df_scrolls = df_scrolls.rename(columns={"ScrollID": "FormID"})
        df_forms = pd.concat([df_forms, df_scrolls])

    df_out = df_forms[["FormID", "Plugin", "u0", "v0", "u1", "v1", "Name"]]
    df_out.to_csv(f"{output_base}.csv", index=False, sep='\t')

    if output_data is not None:
        data_columns = ["Name", "FormID", "Plugin", "Casteffect", "GCD", "Cooldown", "Casttime", "Animation",
                        "Animation2", "Symbol"]
        df_out_spell_data = df_forms[data_columns]
        df_out_spell_data.to_csv(f"{output_data}.csv", index=False, sep='\t')


def create_cooldown_progress_overlay(output_base: Path | str, alpha_mask_path: str | None, radius: float = 1.0,
                                     alpha: float = 0.5):
    atlas_size = next_pow2(8 * image_size)

    alpha_i = round(255 * alpha)

    if alpha_mask_path is None:
        alpha_mask = np.ones((image_size, image_size), dtype="uint8") * 255
    else:
        alpha_mask = cv2.imread(alpha_mask_path, cv2.IMREAD_GRAYSCALE)
        # alpha_mask = cv2.resize(alpha_mask, (image_size, image_size), interpolation=cv2.INTER_LANCZOS4)
        alpha_mask = resize_image_with_pil(alpha_mask, image_size)

    alpha_factor = alpha_mask.astype(float) / 255.0

    cvImage = np.zeros((atlas_size, atlas_size, 4), dtype="uint8")

    ser = pd.Series([f"{i}" for i in range(64)])
    ser.name = "Cooldown"
    df = pd.DataFrame(ser)
    df['u0'] = np.nan
    df['v0'] = np.nan
    df['u1'] = np.nan
    df['v1'] = np.nan

    u0_index = df.columns.get_loc('u0')
    v0_index = df.columns.get_loc('v0')
    u1_index = df.columns.get_loc('u1')
    v1_index = df.columns.get_loc('v1')
    x = 0
    y = 0
    for i in range(64):
        img = np.zeros((image_size, image_size, 4), dtype="uint8")
        prog = 1.0 / 64.0 * i
        p = int(image_size * prog)
        img[p:, :, :] = (0, 0, 0, alpha_i)

        flags = cv2.INTER_CUBIC + cv2.WARP_FILL_OUTLIERS + cv2.WARP_POLAR_LINEAR + cv2.WARP_INVERSE_MAP

        r = image_size * radius

        pol = cv2.warpPolar(img, (image_size, image_size), (image_size / 2, image_size / 2), r, flags)
        pol = cv2.rotate(pol, cv2.ROTATE_90_COUNTERCLOCKWISE)

        pol[:, :, 3] = np.multiply(pol[:, :, 3], alpha_factor)

        cvImage[y: y + image_size, x: x + image_size] = pol

        df.iloc[i, u0_index] = x / atlas_size
        df.iloc[i, u1_index] = (x + image_size) / atlas_size
        df.iloc[i, v0_index] = y / atlas_size
        df.iloc[i, v1_index] = (y + image_size) / atlas_size
        x = x + image_size
        if (x + image_size) > atlas_size:
            x = 0
            y = y + image_size

    write_png_and_dds(cvImage, output_base)
    df.to_csv(f"{output_base}.csv", index=False, sep='\t')


def create_i4_icons(spell_list: list[str], icon_root: list[str], output_path: Path | str, alpha_mask_path: str | None,
                    tmp_icons_dir: Path | str, swf_out_path: Path | str, swf_prefix: str = "spell",
                    write_icons_and_swf: bool = True, add_unbind_slot: bool = False):
    schema_link = "https://raw.githubusercontent.com/Exit-9B/InventoryInjector/main/docs/InventoryInjector.schema.json"

    dragon_font_path = Path(__file__).parent / 'icons/6_$DragonFont_Dragon_script.ttf'
    dragon_font = ImageFont.truetype(str(dragon_font_path), round(swf_icon_size * 0.5))

    rank_font = ImageFont.truetype(str(rank_text_font), round(swf_icon_size * rank_text_scale))

    df = None
    for spells in spell_list:
        df_temp = pd.read_csv(spells, sep="\t")
        if df is None:
            df = df_temp
        else:
            df = pd.concat([df, df_temp])

    if "Shouttext" not in df.columns:
        df["Shouttext"] = np.nan

    if "Ranktext" not in df.columns:
        df["Ranktext"] = np.nan

    if add_unbind_slot:
        df_temp = pd.DataFrame([["Unbind Slot", "0x000810", "SpellHotbar.esp", "", -1, -1, -1, -1, -1, np.nan, np.nan]])
        df_temp.columns = ["Name", "FormID", "Plugin", "Casteffect", "GCD", "Cooldown", "Casttime", "Animation",
                           "Animation2", "Shouttext", "Ranktext"]
        df = pd.concat([df, df_temp])

    df['Filename'] = df['Name'].apply(_get_image_name)
    df['Path'] = df['Filename'].apply(_find_image, args=(icon_root,))

    df_no_image_found = df[pd.isna(df['Path'])]
    print("No Icon files found for:")
    print(df_no_image_found[['Name', 'Plugin']])

    df = df[~pd.isna(df['Path'])]

    df['FormID_String'] = df['Plugin'] + '|' + df['FormID'].str[2:].str.lstrip('0').str.upper()

    entry_list = []

    for index, row in df.iterrows():
        file_suffix = ""
        if not pd.isna(row["Shouttext"]):
            file_suffix = file_suffix + f"_{row['Shouttext']}"

        if not pd.isna(row["Ranktext"]):
            file_suffix = file_suffix + f"_{row['Ranktext']}"

        if file_suffix in ["_", "__"]:
            file_suffix = ""

        entry = {
            "match": {
                # "formType": "Spell",
                "formId": f"{row['FormID_String']}"
            },
            "assign": {
                "iconSource": f"SpellHotbar/{swf_prefix}_icons.swf",
                "iconLabel": f"{row['Filename']}{file_suffix}",
                "iconColor": "#ffffff"
            }
        }
        entry_list.append(entry)

    output_json = {
        "$schema": schema_link,
        "rules": entry_list
    }

    print(f"writing json: {output_path}")
    with open(output_path, 'w') as f:
        json.dump(output_json, f, indent=4)

    if write_icons_and_swf:
        if "Shouttext" not in df.columns:
            df["Shouttext"] = np.nan
        if "Ranktext" not in df.columns:
            df["Ranktext"] = np.nan

        df_icons = df[['Filename', 'Path', 'Shouttext', 'Ranktext']]

        def _get_unique_key(row: pd.Series) -> str:
            shouttext = f"_{row['Shouttext']}" if not pd.isna(row['Shouttext']) else ''
            ranktext = f"_{row['Ranktext']}" if not pd.isna(row['Ranktext']) else ''
            return f"{row['Filename']}{shouttext}{ranktext}"

        df_icons = df_icons[~pd.isna(df_icons['Path'])]
        df_icons['unique_key'] = df_icons.apply(_get_unique_key, axis=1)
        df_icons = df_icons.drop_duplicates()
        df_icons = df_icons.sort_values(by='Filename')

        out_pathes = []
        for index, row in df_icons.iterrows():
            img = cv2.imread(row['Path'], cv2.IMREAD_UNCHANGED)
            # img = cv2.resize(img, (swf_icon_size, swf_icon_size), interpolation=cv2.INTER_LANCZOS4)
            img = resize_image_with_pil(img, swf_icon_size)

            if not pd.isna(row["Shouttext"]):
                img = add_dragonlang_text_to_image(img, swf_icon_size, row["Shouttext"], dragon_font, stroke_width=1)

            if not pd.isna(row["Ranktext"]):
                img = add_rank_text_to_image(img, swf_icon_size, row["Ranktext"], rank_font, stroke_width=1)

            out_img_path = f"{tmp_icons_dir}/{row['unique_key']}.png"
            cv2.imwrite(out_img_path, img)  # no dds needed here
            out_pathes.append(out_img_path)

        proc = subprocess.run([png2swf_executable_path, "-o", f"{swf_out_path}", "-X", "128", "-Y", "128"] + out_pathes)

        labels = df_icons['unique_key'].to_list()
        proc2 = subprocess.run(
            ["java", "-jar", swf_jar_path,
             f"{swf_out_path}", f"{swf_out_path}"] + labels)
    else:
        print("Skipping write of images and SWF")


def stitch_mod(modname: str):
    stitch_folder([rf"{project_root}\spell_lists2\mods\{modname}.csv"],
                  [rf"{project_root}\modded_spell_icons\{modname}"],
                  mod_root_path / f"images/icons_{modname}", alpha_mask,
                  output_data=mod_root_path / f"spelldata/spells_{modname}")


def i4_mod(modname: str, tmp_icons_dir: str, spell_list: list[str] = None, icon_root: list[str] = None,
           esp_name: str = None):
    swf_out_path = mod_root_path / f"../../../Interface/SpellHotbar/{modname}_icons.swf"
    if spell_list is None:
        spell_list = [rf"{project_root}\spell_lists2\mods\{modname}.csv"]
    if icon_root is None:
        icon_root = [rf"{project_root}\modded_spell_icons\{modname}"]
    if esp_name is None:
        esp_name = f"{modname.capitalize()}"
    create_i4_icons(spell_list=spell_list,
                    icon_root=icon_root,
                    output_path=mod_root_path / f"../InventoryInjector/{esp_name}.json",
                    alpha_mask_path=None,
                    tmp_icons_dir=tmp_icons_dir,
                    swf_out_path=swf_out_path,
                    write_icons_and_swf=True,
                    swf_prefix=modname)


def i4_extra_icons(icon_dir: str | Path, tmp_icons_dir: str | Path, swf_out_path: str | Path):
    dir_path = Path(icon_dir)
    names = [img.stem for img in dir_path.glob("*.png")]

    ser = pd.Series(names)
    df = pd.DataFrame(ser)
    df.columns = ["IconName"]

    out_pathes = []
    for index, row in df.iterrows():
        img_path = dir_path / f"{row['IconName']}.png"
        img = cv2.imread(str(img_path), cv2.IMREAD_UNCHANGED)
        img = resize_image_with_pil(img, swf_icon_size)

        out_img_path = f"{tmp_icons_dir}/{row['IconName']}.png"
        cv2.imwrite(out_img_path, img)  # no dds needed here
        out_pathes.append(out_img_path)

    proc = subprocess.run([png2swf_executable_path, "-o", f"{swf_out_path}", "-X", "128", "-Y", "128"] + out_pathes)

    labels = df['IconName'].to_list()
    proc2 = subprocess.run(
        ["java", "-jar", swf_jar_path,
         f"{swf_out_path}", f"{swf_out_path}"] + labels)


def stitch_extra_icons(dir: str, output_base: Path | str, alpha_mask_path: str | None):
    dir_path = Path(dir)
    names = [img.stem for img in dir_path.glob("*.png")]

    ser = pd.Series(names)
    df = pd.DataFrame(ser)
    df.columns = ["IconName"]

    df['Filename'] = df['IconName'].apply(_get_image_name)
    df['Path'] = df['Filename'].apply(_find_image, args=([dir],))
    df['u0'] = np.nan
    df['v0'] = np.nan
    df['u1'] = np.nan
    df['v1'] = np.nan
    df['use_mask'] = True
    df['overlay'] = df['IconName'].apply(_get_overlay)

    def is_shout(x) -> str | None:
        return "DS" if "shout" in str(x) else None

    df["Shouttext"] = df["Filename"].apply(is_shout)

    df_no_image_found = df[pd.isna(df['Path'])]
    print("No Icon files found for:")
    print(df_no_image_found['IconName'])
    df, cvImage = _stitch_images_internal(df, alpha_mask_path)

    write_png_and_dds(cvImage, output_base)
    df_out = df[["IconName", "u0", "v0", "u1", "v1"]]
    df_out.to_csv(f"{output_base}.csv", index=False, sep='\t')


def create_spellproc_overlay(input_folder: Path | str, output_base: Path | str, alpha_mask_path: str | None,
                             expansion_pixels: int = 0):
    folder_path = Path(input_folder)

    if alpha_mask_path is None:
        alpha_mask = np.ones((image_size, image_size), dtype="uint8") * 255
    else:
        alpha_mask = cv2.imread(alpha_mask_path, cv2.IMREAD_GRAYSCALE)
        # alpha_mask = cv2.resize(alpha_mask, (image_size, image_size), interpolation=cv2.INTER_LANCZOS4)
        alpha_mask = resize_image_with_pil(alpha_mask, image_size)

    images = list()
    for img in folder_path.glob("*.png"):
        images.append(img)

    row_len = math.ceil(math.sqrt(len(images)))
    atlas_size = next_pow2(row_len * image_size)
    cvImage = np.zeros((atlas_size, atlas_size, 4), dtype="uint8")

    ser = pd.Series([f"{i}" for i in range(len(images))])
    ser.name = "SpellProc"
    df = pd.DataFrame(ser)
    df['u0'] = np.nan
    df['v0'] = np.nan
    df['u1'] = np.nan
    df['v1'] = np.nan

    u0_index = df.columns.get_loc('u0')
    v0_index = df.columns.get_loc('v0')
    u1_index = df.columns.get_loc('u1')
    v1_index = df.columns.get_loc('v1')
    x = 0
    y = 0

    for i, img in enumerate(images):
        bgr = cv2.imread(str(img), cv2.IMREAD_COLOR)
        hsv = cv2.cvtColor(bgr, cv2.COLOR_BGR2HSV)

        img_a = np.zeros((bgr.shape[0], bgr.shape[1], 4), dtype="uint8")
        img_a[:, :, 0:3] = bgr
        img_a[:, :, 3] = hsv[:, :, 2]

        img_exp = np.zeros((bgr.shape[0] + expansion_pixels * 2, bgr.shape[1] + expansion_pixels * 2, 4), dtype="uint8")
        img_exp[expansion_pixels:-expansion_pixels, expansion_pixels:-expansion_pixels, :] = img_a

        # im = cv2.resize(img_exp, (image_size, image_size), interpolation=cv2.INTER_AREA)
        im = resize_image_with_pil(img_exp, image_size)

        # multiply alpha mask
        alpha_factor = alpha_mask.astype(float) / 255.0
        im[:, :, 3] = np.multiply(im[:, :, 3], alpha_factor)

        cvImage[y: y + image_size, x: x + image_size] = im

        df.iloc[i, u0_index] = x / atlas_size
        df.iloc[i, u1_index] = (x + image_size) / atlas_size
        df.iloc[i, v0_index] = y / atlas_size
        df.iloc[i, v1_index] = (y + image_size) / atlas_size
        x = x + image_size
        if (x + image_size) > atlas_size:
            x = 0
            y = y + image_size

    write_png_and_dds(cvImage, output_base)
    df.to_csv(f"{output_base}.csv", index=False, sep='\t')


def stitch_folder_contributed(mod_name: str, alpha_mask, icon_folder_name: str | None = None):
    if icon_folder_name is None:
        icon_folder_name = mod_name
    stitch_folder([rf"{project_root}\spell_lists2\mods_contributed\{mod_name}.csv"],
                  [rf"{project_root}\modded_spell_icons_contributed\{icon_folder_name}"],
                  mod_root_path / f"images/icons_{mod_name}", alpha_mask,
                  output_data=mod_root_path / f"spelldata/spells_{mod_name}")


def i4_mod_contributed(mod_name: str, esp_name: str | None, icon_folder_name: str | None = None):
    if icon_folder_name is None:
        icon_folder_name = mod_name
    i4_mod(mod_name, tmp_icons_dir, esp_name=esp_name,
           spell_list=[rf"{project_root}\spell_lists2\mods_contributed\{mod_name}.csv"],
           icon_root=[rf"{project_root}\modded_spell_icons_contributed\{icon_folder_name}"])


if __name__ == "__main__":
    project_root = Path(__file__).parent

    redo_all_img: bool = False

    overlay_icons = overlay_icons | {k: f"{project_root}\\icons\\school_{k}.png" for k in
                                     ["alteration", "conjuration", "destruction",
                                      "illusion", "restoration"]}
    spell_lists = [
        rf"{project_root}\spell_lists2\alteration.csv",
        rf"{project_root}\spell_lists2\destruction.csv",
        rf"{project_root}\spell_lists2\restoration.csv",
        rf"{project_root}\spell_lists2\illusion.csv",
        rf"{project_root}\spell_lists2\conjuration.csv"
    ]

    spell_lists_powers = [
        rf"{project_root}\spell_lists2\powers.csv",
        rf"{project_root}\spell_lists2\shouts.csv"
    ]

    spell_lists_i4_only = [
        rf"{project_root}\spell_lists2\spellhotbar_i4_only.csv",
    ]

    spell_lists_potions = [
        rf"{project_root}\spell_lists2\potions.csv",
    ]

    spell_lists_poisons = [
        rf"{project_root}\spell_lists2\poisons.csv"
    ]

    icon_root_folders = [
        rf"{project_root}\vanilla_spell_icons\alteration",
        rf"{project_root}\vanilla_spell_icons\destruction",
        rf"{project_root}\vanilla_spell_icons\restoration",
        rf"{project_root}\vanilla_spell_icons\illusion",
        rf"{project_root}\vanilla_spell_icons\conjuration"
    ]

    icon_root_folders_powers = [
        rf"{project_root}\vanilla_spell_icons\powers",
        rf"{project_root}\vanilla_spell_icons\shouts"
    ]

    icon_root_folders_potions = [
        rf"{project_root}\vanilla_spell_icons\potions",
    ]

    icon_root_folders_poisons = [
        rf"{project_root}\vanilla_spell_icons\poisons"
    ]

    default_icons_folders = [
        rf"{project_root}\icons",
        rf"{project_root}\vanilla_spell_icons\alteration_generic",
        rf"{project_root}\vanilla_spell_icons\restoration_generic",
        rf"{project_root}\vanilla_spell_icons\destruction_generic",
        rf"{project_root}\vanilla_spell_icons\illusion_generic",
        rf"{project_root}\vanilla_spell_icons\conjuration_generic",
        rf"{project_root}\vanilla_spell_icons\shouts_generic",
        rf"{project_root}\vanilla_spell_icons\potions_generic",
        rf"{project_root}\vanilla_spell_icons\foods_generic",
    ]

    # alpha_mask = none
    alpha_mask = rf"{project_root}\icons\alpha_mask.png"

    if redo_all_img or False:
        stitch_folder(spell_lists, icon_root_folders, mod_root_path / "images/icons_vanilla", alpha_mask,
                      output_data=mod_root_path / "spelldata/spells_vanilla")

    if redo_all_img or False:
        stitch_folder(spell_lists_powers, icon_root_folders_powers, mod_root_path / "images/icons_vanilla_powers",
                      alpha_mask,
                      output_data=mod_root_path / "spelldata/spells_vanilla_powers")

    if redo_all_img or False:
        stitch_folder(spell_lists_potions, icon_root_folders_potions, mod_root_path / "images/icons_vanilla_potions",
                      alpha_mask,
                      output_data=mod_root_path / "spelldata/spells_vanilla_potions")

    if redo_all_img or False:
        stitch_folder(spell_lists_poisons, icon_root_folders_poisons, mod_root_path / "images/icons_vanilla_poisons",
                      alpha_mask,
                      output_data=mod_root_path / "spelldata/spells_vanilla_poisons")

    if redo_all_img or False:
        stitch_default_icons(default_icons, default_icons_folders, mod_root_path / "images/default_icons", alpha_mask,
                             alphamask_empty=False, alphamask_overlay=False, alphamask_highlight=False,
                             add_school_icon=False)

    if redo_all_img or False:
        create_cooldown_progress_overlay(mod_root_path / "images/icons_cooldown", alpha_mask, radius=1.0, alpha=0.8)

    tmp_icons_dir = r"F:\Skyrim Dev\WORK\TMP\icons"
    if redo_all_img or False:
        swf_path = str(mod_folder_path / "Interface/SpellHotbar/spell_icons.swf")
        create_i4_icons(spell_lists + spell_lists_powers + spell_lists_i4_only,
                        icon_root_folders + default_icons_folders + icon_root_folders_powers,
                        mod_root_path / "../InventoryInjector/SpellHotbar.json",
                        alpha_mask,
                        tmp_icons_dir=tmp_icons_dir, swf_out_path=swf_path, write_icons_and_swf=True,
                        add_unbind_slot=True)

    if redo_all_img or False:
        # mods: vulcano
        stitch_mod("vulcano")
        i4_mod("vulcano", tmp_icons_dir)
        stitch_mod("arclight")
        i4_mod("arclight", tmp_icons_dir)
        stitch_mod("desecration")
        i4_mod("desecration", tmp_icons_dir)

    if redo_all_img or False:
        t_archetypes = ["druid", "cleric", "shadow_mage", "warlock", "shaman"]

        triumvirate_spell_lists = [rf"{project_root}\spell_lists2\mods\triumvirate_{a}.csv" for a in t_archetypes]
        triumvirate_icon_folders = [rf"{project_root}\modded_spell_icons\triumvirate_{a}" for a in t_archetypes]
        stitch_folder(triumvirate_spell_lists, triumvirate_icon_folders, mod_root_path / f"images/icons_triumvirate",
                      alpha_mask,
                      output_data=mod_root_path / "spelldata/spells_triumvirate")

        i4_mod("triumvirate", tmp_icons_dir,
               triumvirate_spell_lists,
               triumvirate_icon_folders,
               esp_name="Triumvirate - Mage Archetypes")

    # create nordic_ui files
    if redo_all_img or False:
        mod_nordic_ui_root_path = Path(r"F:\Skyrim Dev\ADT\mods\Spell Hotbar NordicUI\SKSE\Plugins\SpellHotbar")
        stitch_default_icons(default_icons, [rf"{project_root}\icons\nordic_ui"],
                             mod_nordic_ui_root_path / "images/default_icons_nordic", alpha_mask,
                             alphamask_empty=False, alphamask_overlay=False, alphamask_highlight=False,
                             add_school_icon=False)

    if redo_all_img or False:
        stitch_mod("thunderchild")
        i4_mod("thunderchild", tmp_icons_dir, esp_name="Thunderchild - Epic Shout Package")

        stitch_mod("sonic_magic")
        i4_mod("sonic_magic", tmp_icons_dir, esp_name="Shockwave")

        stitch_mod("storm_calling_magic2")
        i4_mod("storm_calling_magic2", tmp_icons_dir, esp_name="StormCalling")

        stitch_mod("astral_magic_2")
        i4_mod("astral_magic_2", tmp_icons_dir, esp_name="Astral")

        stitch_mod("elemental_destruction_magic_redux")
        i4_mod("elemental_destruction_magic_redux", tmp_icons_dir, esp_name="Elemental Destruction Magic Redux")

    a_schools = ["alteration", "conjuration", "destruction", "illusion", "restoration"]
    apoc_spell_lists = [rf"{project_root}\spell_lists2\mods\apocalypse_{s}.csv" for s in a_schools]
    apoc_icon_folders = [rf"{project_root}\modded_spell_icons\apocalypse_{s}" for s in a_schools]
    if redo_all_img or False:
        stitch_folder(apoc_spell_lists, apoc_icon_folders, mod_root_path / f"images/icons_apocalypse", alpha_mask,
                      output_data=mod_root_path / "spelldata/spells_apocalypse")

        i4_mod("apocalypse", tmp_icons_dir, apoc_spell_lists, apoc_icon_folders,
               esp_name="Apocalypse - Magic of Skyrim")

    odin_spell_lists = [rf"{project_root}\spell_lists2\mods\odin_{s}.csv" for s in a_schools]
    odin_icon_folders = [rf"{project_root}\modded_spell_icons\odin_{s}" for s in a_schools]
    if redo_all_img or False:
        stitch_folder(odin_spell_lists, odin_icon_folders, mod_root_path / f"images/icons_odin", alpha_mask,
                      output_data=mod_root_path / "spelldata/spells_odin")

        i4_mod("odin", tmp_icons_dir, odin_spell_lists, odin_icon_folders,
               esp_name="Odin - Skyrim Magic Overhaul")

    if redo_all_img or False:
        stitch_mod("constellation_magic")
        i4_mod("constellation_magic", tmp_icons_dir, esp_name="Supernova")

        stitch_mod("miracles_of_skyrim")
        i4_mod("miracles_of_skyrim", tmp_icons_dir, esp_name="DS2Miracles")

        stitch_mod("dark_hierophant_magic")
        i4_mod("dark_hierophant_magic", tmp_icons_dir, esp_name="Ghostlight")

        stitch_extra_icons(rf"{project_root}\extra_icons", mod_root_path / "images/extra_icons", alpha_mask)

        stitch_mod("andromeda")
        i4_mod("andromeda", tmp_icons_dir, esp_name="Andromeda - Unique Standing Stones of Skyrim")

        stitch_mod("ordinator")
        i4_mod("ordinator", tmp_icons_dir, esp_name="Ordinator - Perks of Skyrim")

        stitch_mod("sacrosanct")
        i4_mod("sacrosanct", tmp_icons_dir, esp_name="Sacrosanct - Vampires of Skyrim")

        stitch_mod("abyssal_tides_magic")
        i4_mod("abyssal_tides_magic", tmp_icons_dir, esp_name="Aqua")

        stitch_mod("abyssal_wind_magic")
        i4_mod("abyssal_wind_magic", tmp_icons_dir, esp_name="Aero")

        stitch_mod("winter_wonderland_magic")
        i4_mod("winter_wonderland_magic", tmp_icons_dir, esp_name="Icebloom")

        stitch_mod("abyss")
        i4_mod("abyss", tmp_icons_dir, esp_name="Abyss")

        stitch_mod("ancient_blood_magic_2")
        i4_mod("ancient_blood_magic_2", tmp_icons_dir, esp_name="AncientBloodII")

        create_spellproc_overlay(rf"{project_root}\icons\spellproc_anim",
                                 mod_root_path / "images/icons_spellproc_overlay", alpha_mask, 10)

        stitch_mod("stellaris")
        i4_mod("stellaris", tmp_icons_dir)

        stitch_mod("obscure_magic")
        i4_mod("obscure_magic", tmp_icons_dir, esp_name="KittySpellPack01")

        stitch_mod("holy_templar_magic")
        i4_mod("holy_templar_magic", tmp_icons_dir, esp_name="Lightpower")

        stitch_mod("witcher_signs")
        i4_mod("witcher_signs", tmp_icons_dir, esp_name="W3S")

    m_schools = ["alteration", "conjuration", "destruction", "illusion", "restoration"]
    mysticism_spell_lists = [rf"{project_root}\spell_lists2\mods\mysticism_{s}.csv" for s in m_schools]
    mysticism_icon_folders = [rf"{project_root}\modded_spell_icons\mysticism_{s}" for s in m_schools]

    if redo_all_img or False:
        stitch_folder(mysticism_spell_lists, mysticism_icon_folders, mod_root_path / f"images/icons_mysticism",
                      alpha_mask, output_data=mod_root_path / "spelldata/spells_mysticism")

        i4_mod("mysticism", tmp_icons_dir, mysticism_spell_lists, mysticism_icon_folders, esp_name="MysticismMagic")

        stitch_mod("sperg")
        i4_mod("sperg", tmp_icons_dir, esp_name="SPERG-SSE")

        stitch_mod("path_of_sorcery")
        i4_mod("path_of_sorcery", tmp_icons_dir, esp_name="PathOfSorcery")

    if redo_all_img or False:
        stitch_mod("elemental_mastery_magic")
        i4_mod("elemental_mastery_magic", tmp_icons_dir, esp_name="KittySpellPack02")


    """contributed mods:"""
    if redo_all_img or False:
        #credits ArchAngelAries
        stitch_folder_contributed("shadow_spell_package", alpha_mask)
        i4_mod_contributed("shadow_spell_package", esp_name="ShadowSpellPackage")

    if redo_all_img or False:
        # credits ArchAngelAries
        stitch_folder_contributed("star_wars_spell_pack", alpha_mask)
        i4_mod_contributed("star_wars_spell_pack", esp_name="starwarsspellpack")

        stitch_folder_contributed("star_wars_spell_pack_esl", alpha_mask, icon_folder_name="star_wars_spell_pack")
        i4_mod_contributed("star_wars_spell_pack_esl", esp_name="starwarsspellpack_ESLversion", icon_folder_name="star_wars_spell_pack")

    if redo_all_img or False:
        #credits ArchAngelAries
        stitch_folder_contributed("undead_horse", alpha_mask)
        i4_mod_contributed("undead_horse", esp_name="Undead Horse")

    # icons done by user request
    # stitch_extra_icons(r"F:\Skyrim Dev\Skyrim_SpellHotbar2\python_scripts\user_icons\squeetsquib\Icons", mod_root_path / "images/squeetsquib_icons", alpha_mask)
    # i4_extra_icons(r"F:\Skyrim Dev\Skyrim_SpellHotbar2\python_scripts\user_icons\squeetsquib\Icons", tmp_icons_dir,
    #               mod_folder_path / "Interface/SpellHotbar/squeetsquib_icons.swf")
