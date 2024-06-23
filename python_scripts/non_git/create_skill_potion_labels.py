import cv2
from pathlib import Path
import numpy as np

skill_icons_folder = Path(r"F:\Skyrim Dev\WORK\icon_work\skill_icons")
label_bg = r"F:\Skyrim Dev\WORK\icon_work\skill_bottle_label_round.png"
out_folder = Path(r"F:\Skyrim Dev\WORK\icon_work\skill_labels")

def blend_images(img_base: np.ndarray, img_overlay: np.ndarray, offset:tuple[int, int] = (0,0)):
    #center_overlay_on_image
    if img_base.shape[2] == 3:
        img_base = cv2.cvtColor(img_base, cv2.COLOR_RGB2RGBA)
    overlay = np.zeros_like(img_base)
    off_x = round((img_base.shape[0] - img_overlay.shape[0])*0.5)
    off_y = round((img_base.shape[1] - img_overlay.shape[1])*0.5)

    if off_x > 0 and off_y > 0:
        x0 = off_x + offset[1]
        x1 = overlay.shape[0] - off_x + offset[1]

        y0 = off_y + offset[0]
        y1 = overlay.shape[1] - off_y + offset[0]

        overlay[x0:x1, y0:y1] = img_overlay
    else:
        overlay = img_overlay

    overlay_img = overlay / 255
    img_in = img_base / 255

    if img_in.shape[2] == 3:
        img_in = cv2.cvtColor(img_in, cv2.COLOR_RGB2RGBA)

    result = np.zeros_like(overlay_img)
    overlay_a = np.repeat(overlay_img[:, :, 3, np.newaxis], 3, axis=2)
    result[:, :, :3] = (overlay_img[:, :, :3] * overlay_a + img_in[:, :, :3] * (1 - overlay_a))
    result[:, :, 3] = 1 - ((1 - overlay_img[:, :, 3]) * (1 - img_in[:, :, 3]))
    return result * 255


if __name__ == "__main__":

    bg = cv2.imread(label_bg, cv2.IMREAD_UNCHANGED)

    if True:
        for file in skill_icons_folder.glob("SR*.png"):
            out_name = file.name[len("SR-skill-"):].replace("-", "_")
            print(out_name)
            icon = cv2.imread(str(file), cv2.IMREAD_UNCHANGED)
            out_img = blend_images(bg, icon)

            #cut image to 204x204 inner part
            offset = round((out_img.shape[0]-204)*0.5)
            out_crop = out_img[offset:-offset, offset:-offset]

            cv2.imwrite(f"{out_folder / out_name}", out_crop)

    for file in skill_icons_folder.glob("tg*.png"):
        out_name = file.name[len("tg_"):].replace("-", "_")
        print(out_name)

        icon = cv2.imread(str(file), cv2.IMREAD_UNCHANGED)
        out_img = blend_images(bg, icon)

        # cut image to 204x204 inner part
        offset = round((out_img.shape[0] - 204) * 0.5)
        out_crop = out_img[offset:-offset, offset:-offset]

        cv2.imwrite(f"{out_folder / out_name}", out_crop)