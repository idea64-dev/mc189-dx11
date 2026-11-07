"""Compares a port screenshot with a vanilla one inside the regions that do not depend on animation.

    python tools/compare_screens.py run/vanilla_menu.png run/smoke_1_start.png

Regions are for the main menu at 854x480 (GUI scale 2): the opaque buttons with their labels, and the
pixels of the bottom text and its shadow. Exits with 1 if any of them differ.
"""
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


def main():
    vanilla = Image.open(sys.argv[1]).convert("RGB")
    port = Image.open(sys.argv[2]).convert("RGB")
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


if __name__ == "__main__":
    sys.exit(main())
