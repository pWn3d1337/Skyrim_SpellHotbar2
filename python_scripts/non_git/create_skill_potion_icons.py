import cv2
from pathlib import Path
import numpy as np

skill_data = [
    ("of Alteration", "Alteration.png"),
    ("of the Defender", "Block.png"),
    ("Conjurers", "Conjuration.png"),
    ("of Destruction", "Destruction.png"),
    ("Enchanters", "Enchanting.png"),
    ("of the Knight", "Heavy_Armor.png"),
    ("of Illusion", "Illusion.png"),
    ("Skirmishers", "Light_Armor.png"),
    ("of Lockpicking", "Lockpicking.png"),
    ("of True Shot", "Archery.png"),
    ("of the Warrior", "One_Handed.png"),
    ("of Glibness", "Speech.png"),
    ("of Pickpocketing", "Pickpocket.png"),
    ("of the Healer", "Restoration.png"),
    ("Blacksmiths", "Smithing.png"),
    ("of Light Feet", "Sneak.png"),
    ("of the Berserker", "Two_Handed.png"),
    ("of Haggling", "Barter.png")
]

tg_potion_data = [
    ("of Plunder", "plunder.png"),
    ("of Conflict", "conflict.png"),
    ("of Larceny", "larceny.png"),
    ("of Keenshot", "keenshot.png"),
    ("of Escape", "escape.png"),
]

#Missing: haggling

hue_shift = 10
sat_shift = 50
potions = [
    ("Potion", (70,70), (-2,14), hue_shift, sat_shift),
    ("Draught", (70,70), (2,35), hue_shift, sat_shift),
    ("Philter", (78,78), (0,20), hue_shift, sat_shift),
    ("Elixir", (84,84), (4,30), hue_shift, sat_shift)
]

potions_tg = potions + \
    [
        ("Grand Elixir", (80, 80), (2, 15), hue_shift, sat_shift),
        ("Prime Elixir", (110, 110), (0, 10), hue_shift, sat_shift)
    ]

def hsv_shift(img_in: np.ndarray, shift_h: int, shift_s: int ) -> np.ndarray:
    alpha = img_in[:,:,3]
    img_opaque = img_in[:,:,:3]
    img_hsv = cv2.cvtColor(img_opaque, cv2.COLOR_RGB2HSV)
    h = img_hsv[:,:,0]
    s = img_hsv[:,:,1]

    h = cv2.add(h, shift_h)
    img_hsv[:,:,0] = h

    s = cv2.add(s, shift_s)
    img_hsv[:,:,1] = s

    img_rgb = cv2.cvtColor(img_hsv, cv2.COLOR_HSV2RGB)

    img_out = np.dstack((img_rgb, alpha))
    return img_out

def _blend_internal(img_base, overlay):
    overlay_img = overlay / 255
    img_in = img_base / 255

    result = np.zeros_like(overlay_img)
    overlay_a = np.repeat(overlay_img[:, :, 3, np.newaxis], 3, axis=2)
    result[:, :, :3] = (overlay_img[:, :, :3] * overlay_a + img_in[:, :, :3] * (1 - overlay_a))
    result[:, :, 3] = 1 - ((1 - overlay_img[:, :, 3]) * (1 - img_in[:, :, 3]))
    return result * 255
def blend_images(img_base: np.ndarray, img_overlay: np.ndarray, offset:tuple[int, int] = (0,0)):
    #center_overlay_on_image
    if img_base.shape[2] == 3:
        img_base = cv2.cvtColor(img_base, cv2.COLOR_RGB2RGBA)
    overlay = np.zeros_like(img_base)
    overlay_overlay = np.zeros_like(img_base)

    off_x = round((img_base.shape[0] - img_overlay.shape[0])*0.5)
    off_y = round((img_base.shape[1] - img_overlay.shape[1])*0.5)

    if off_x > 0 and off_y > 0:
        x0 = off_x + offset[1]
        x1 = overlay.shape[0] - off_x + offset[1]

        y0 = off_y + offset[0]
        y1 = overlay.shape[1] - off_y + offset[0]

        overlay_overlay[x0:x1, y0:y1] = img_base[x0:x1, y0:y1]
        overlay_overlay[:,:,3] = 0 #round(255 * 0.25)
        overlay_overlay[x0:x1, y0:y1, 3] = img_overlay[:,:,3] * 0.25

        overlay[x0:x1, y0:y1] = img_overlay
    else:
        overlay = img_overlay

    img_out = _blend_internal(img_base, overlay)
    img_out = _blend_internal(img_out, overlay_overlay)
    return img_out

potion_icons = Path(r"F:\Skyrim Dev\WORK\icon_work\potion_icons")
skill_labels = Path(r"F:\Skyrim Dev\WORK\icon_work\skill_labels")
out_dir = Path(r"F:\Skyrim Dev\WORK\icon_work\skill_pots_out")

def resize_image(img_in: np.ndarray, dst: tuple[int, int]) -> np.ndarray:
    img_out = cv2.resize(img_in, dsize=dst, interpolation=cv2.INTER_AREA)
    return img_out

if __name__ == "__main__":
    if True:
        for skill_dat in skill_data:
            name = skill_dat[0]

            pname=""
            for pot_dat in potions:
                pot_name = pot_dat[0]

                if name.startswith("of"):
                    pname = f"{pot_name} {name}"
                else:
                    pname = f"{name} {pot_name}"

                print(f"{pname}")
                pot = cv2.imread(f"{potion_icons / pot_name}.png", cv2.IMREAD_UNCHANGED)
                label = cv2.imread(f"{skill_labels / skill_dat[1]}", cv2.IMREAD_UNCHANGED)

                label_scaled = resize_image(label, pot_dat[1])
                label_scaled = hsv_shift(label_scaled, pot_dat[3], pot_dat[4])

                out_img = blend_images(pot, label_scaled, pot_dat[2])

                out_filename = f"{pname.lower().replace(" ", "_")}.png"
                cv2.imwrite(str(out_dir / out_filename), out_img)

    for tg_pot in tg_potion_data:
        name = tg_pot[0]

        pname = ""
        for pot_dat in potions_tg:
            pot_name = pot_dat[0]

            if name.startswith("of"):
                pname = f"{pot_name} {name}"
            else:
                pname = f"{name} {pot_name}"

            print(f"{pname}")
            pot = cv2.imread(f"{potion_icons / pot_name}.png", cv2.IMREAD_UNCHANGED)
            label = cv2.imread(f"{skill_labels / tg_pot[1]}", cv2.IMREAD_UNCHANGED)

            label_scaled = resize_image(label, pot_dat[1])
            label_scaled = hsv_shift(label_scaled, pot_dat[3], pot_dat[4])

            out_img = blend_images(pot, label_scaled, pot_dat[2])

            out_filename = f"{pname.lower().replace(" ", "_")}.png"
            cv2.imwrite(str(out_dir / out_filename), out_img)