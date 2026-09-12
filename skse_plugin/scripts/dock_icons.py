"""Draw the in-menu dock's control glyphs: gear, lock, unlock, arrow_left, arrow_right, close.

White on transparent, 64 px, supersampled 4x. FLICK's font has no icon glyphs and SH2 ships no
control art, so these are generated rather than sourced. Output goes to
data/SKSE/Plugins/SpellHotbar/images/dock/ (repo) and should be copied to the mod alongside the
rest of that folder.

Usage: python dock_icons.py [<output dir>]
"""
import math
import sys
from pathlib import Path

from PIL import Image, ImageDraw

SIZE = 64
SS = 4
W = SIZE * SS
WHITE = (255, 255, 255, 255)


def canvas():
    im = Image.new("RGBA", (W, W), (0, 0, 0, 0))
    return im, ImageDraw.Draw(im)


def finish(im: Image.Image, path: Path):
    im = im.resize((SIZE, SIZE), Image.LANCZOS)
    path.parent.mkdir(parents=True, exist_ok=True)
    im.save(path)
    print(path.name)


def gear(path: Path):
    im, d = canvas()
    c = W / 2
    teeth = 8
    r_out, r_in, r_hole = W * 0.46, W * 0.34, W * 0.13
    pts = []
    for i in range(teeth * 2):
        a0 = (i / (teeth * 2)) * 2 * math.pi
        a1 = ((i + 1) / (teeth * 2)) * 2 * math.pi
        r = r_out if i % 2 == 0 else r_in
        # square-ish teeth: two points per segment
        pts.append((c + r * math.cos(a0), c + r * math.sin(a0)))
        pts.append((c + r * math.cos(a1), c + r * math.sin(a1)))
    d.polygon(pts, fill=WHITE)
    d.ellipse((c - r_hole, c - r_hole, c + r_hole, c + r_hole), fill=(0, 0, 0, 0))
    finish(im, path)


def lock(path: Path, open_: bool):
    im, d = canvas()
    body = (W * 0.18, W * 0.46, W * 0.82, W * 0.94)
    d.rounded_rectangle(body, radius=W * 0.06, fill=WHITE)
    # shackle: an arc; open lock lifts the right leg
    t = W * 0.09
    if not open_:
        d.arc((W * 0.26, W * 0.08, W * 0.74, W * 0.62), 180, 360, fill=WHITE, width=int(t))
        d.rectangle((W * 0.26, W * 0.34, W * 0.26 + t, W * 0.50), fill=WHITE)
        d.rectangle((W * 0.74 - t, W * 0.34, W * 0.74, W * 0.50), fill=WHITE)
    else:
        d.arc((W * 0.26, W * 0.02, W * 0.74, W * 0.56), 180, 360, fill=WHITE, width=int(t))
        d.rectangle((W * 0.26, W * 0.28, W * 0.26 + t, W * 0.50), fill=WHITE)
        d.rectangle((W * 0.74 - t, W * 0.28, W * 0.74, W * 0.36), fill=WHITE)
    # keyhole
    d.ellipse((W * 0.44, W * 0.60, W * 0.56, W * 0.72), fill=(0, 0, 0, 0))
    d.rectangle((W * 0.47, W * 0.66, W * 0.53, W * 0.82), fill=(0, 0, 0, 0))
    finish(im, path)


def arrow(path: Path, left: bool):
    im, d = canvas()
    t = W * 0.11
    if left:
        pts = [(W * 0.66, W * 0.12), (W * 0.30, W * 0.50), (W * 0.66, W * 0.88)]
    else:
        pts = [(W * 0.34, W * 0.12), (W * 0.70, W * 0.50), (W * 0.34, W * 0.88)]
    d.line(pts, fill=WHITE, width=int(t), joint="curve")
    finish(im, path)


def close(path: Path):
    # The tabbed window's close control (ticket 08): a plain X, stroke matched to the arrows.
    im, d = canvas()
    t = W * 0.11
    d.line([(W * 0.22, W * 0.22), (W * 0.78, W * 0.78)], fill=WHITE, width=int(t))
    d.line([(W * 0.78, W * 0.22), (W * 0.22, W * 0.78)], fill=WHITE, width=int(t))
    finish(im, path)


if __name__ == "__main__":
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[2] / "data/SKSE/Plugins/SpellHotbar/images/dock"
    gear(out / "gear.png")
    lock(out / "lock.png", open_=False)
    lock(out / "unlock.png", open_=True)
    arrow(out / "arrow_left.png", left=True)
    arrow(out / "arrow_right.png", left=False)
    close(out / "close.png")
