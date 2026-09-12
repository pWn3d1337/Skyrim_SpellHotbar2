"""Write a PNG sibling for every DDS atlas Spell Hotbar 2 loads, for the FLICK-hosted dock.

FLICK's image loader is libpng-based and cannot read DDS (its DLL carries libpng strings and no
DDS or WIC loader; Menu Studio ships PNG thumbnails for the same reason). SH2's own loader
prefers `.dds` when both exist, so a PNG beside each atlas changes nothing on the SMF side and
gives the FLICK side a file it can open. UVs are identical: same pixels, same layout.

Usage:
    python dds_to_png.py <source images dir> [<destination images dir>]

Destination defaults to the source. Existing PNGs newer than their DDS are left alone.
Requires Pillow (reads DXT1/3/5 DDS).
"""
import sys
from pathlib import Path

from PIL import Image


def convert(src_dir: Path, dst_dir: Path) -> int:
    written = 0
    for dds in sorted(src_dir.rglob("*.dds")):
        rel = dds.relative_to(src_dir)
        png = (dst_dir / rel).with_suffix(".png")
        if png.exists() and png.stat().st_mtime >= dds.stat().st_mtime:
            continue
        png.parent.mkdir(parents=True, exist_ok=True)
        with Image.open(dds) as im:
            im.convert("RGBA").save(png, optimize=True)
        print(f"{rel} -> {png.relative_to(dst_dir)} ({png.stat().st_size // 1024} KB)")
        written += 1
    return written


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    src = Path(sys.argv[1])
    dst = Path(sys.argv[2]) if len(sys.argv) > 2 else src
    n = convert(src, dst)
    print(f"{n} PNG(s) written")
