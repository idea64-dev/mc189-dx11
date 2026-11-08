"""Compares port screenshots with vanilla ones.

    python tools/compare_screens.py run/vanilla_menu.png run/smoke_1_start.png
        Main menu at 854x480 (GUI scale 2): only the regions that don't depend on the animated panorama -- the
        opaque buttons with their labels, and the pixels of the bottom text and its shadow.

    python tools/compare_screens.py --full run/cmp/vanilla run/cmp/port [names...]
        Whole frames, for static screens (tools/ui_scenario.ps1 output): every NAME.png present in both
        directories, or only the given names. Writes NAME_diff.png next to the port frame, differing pixels in
        red over a dimmed copy.

Exits with 1 if anything differs.
"""
import os
import sys
from PIL import Image

MAIN_MENU_RECTS = {
    "singleplayer": (226, 216, 626, 256),
    "multiplayer": (226, 264, 626, 304),
    "realms": (226, 312, 626, 352),
    "options": (226, 384, 422, 424),
    "quit": (430, 384, 626, 424),
    "language": (178, 384, 218, 424),
}


def compare_main_menu(vanilla_path, port_path):
    vanilla = Image.open(vanilla_path).convert("RGB")
    port = Image.open(port_path).convert("RGB")
    if vanilla.size != port.size:
        print(f"size differs: {vanilla.size} vs {port.size}")
        return 1

    failed = False
    for name, (x0, y0, x1, y1) in MAIN_MENU_RECTS.items():
        bad = sum(1 for y in range(y0, y1) for x in range(x0, x1) if vanilla.getpixel((x, y)) != port.getpixel((x, y)))
        print(f"{name:13s} {bad:6d} differing of {(x1 - x0) * (y1 - y0)}")
        failed |= bad > 0

    text = bad = 0
    width, height = vanilla.size
    for y in range(height - 20, height):
        for x in range(width):
            pixel = vanilla.getpixel((x, y))
            if pixel in ((255, 255, 255), (63, 63, 63)):  # white text and its shadow
                text += 1
                bad += port.getpixel((x, y)) != pixel
    print(f"bottom text   {bad:6d} differing of {text}")
    failed |= bad > 0
    return 1 if failed else 0


def compare_full(vanilla_dir, port_dir, names):
    if not names:
        names = sorted(
            os.path.splitext(f)[0]
            for f in os.listdir(vanilla_dir)
            if f.endswith(".png") and not f.endswith("_diff.png") and os.path.exists(os.path.join(port_dir, f))
        )
    failed = False
    for name in names:
        vanilla = Image.open(os.path.join(vanilla_dir, name + ".png")).convert("RGB")
        port = Image.open(os.path.join(port_dir, name + ".png")).convert("RGB")
        if vanilla.size != port.size:
            print(f"{name:20s} size differs: {vanilla.size} vs {port.size}")
            failed = True
            continue
        vp, pp = vanilla.load(), port.load()
        diff = Image.eval(port, lambda c: c // 3)
        dp = diff.load()
        bad = 0
        width, height = vanilla.size
        for y in range(height):
            for x in range(width):
                if vp[x, y] != pp[x, y]:
                    bad += 1
                    dp[x, y] = (255, 0, 0)
        print(f"{name:20s} {bad:7d} differing of {width * height}")
        if bad:
            diff.save(os.path.join(port_dir, name + "_diff.png"))
            failed = True
    return 1 if failed else 0


def main():
    if len(sys.argv) >= 4 and sys.argv[1] == "--full":
        return compare_full(sys.argv[2], sys.argv[3], sys.argv[4:])
    return compare_main_menu(sys.argv[1], sys.argv[2])


if __name__ == "__main__":
    sys.exit(main())
