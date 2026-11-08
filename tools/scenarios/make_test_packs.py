"""Writes small self-made resource packs into the game directories of tools/ui_scenario.ps1 (run/cmp/<target>/game)
for tools/scenarios/resourcepacks.txt:

    TestPack.zip   pack_format 1, JSON description with colours and styles, own pack.png, replaces the options
                   background (options_background.png) with a checkerboard
    FuturePack/    folder pack, pack_format 2 (shown as incompatible), plain description, no pack.png

    python tools/scenarios/make_test_packs.py
"""
import io
import json
import os
import zipfile
from PIL import Image

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")


def png(image):
    data = io.BytesIO()
    image.save(data, "PNG")
    return data.getvalue()


def icon():
    image = Image.new("RGBA", (64, 64))
    for y in range(64):
        for x in range(64):
            image.putpixel((x, y), (x * 4, y * 4, 160, 255))
    return png(image)


def background():
    image = Image.new("RGB", (16, 16))
    for y in range(16):
        for x in range(16):
            image.putpixel((x, y), (120, 40, 40) if (x // 4 + y // 4) % 2 else (40, 40, 120))
    return png(image)


def write(game_dir):
    packs = os.path.join(game_dir, "resourcepacks")
    os.makedirs(packs, exist_ok=True)

    meta = {"pack": {"pack_format": 1, "description": {"text": "Test ", "color": "gold", "extra": [
        {"text": "pack", "bold": True}, {"translate": "gui.done", "color": "aqua"}]}}}
    with zipfile.ZipFile(os.path.join(packs, "TestPack.zip"), "w") as z:
        z.writestr("pack.mcmeta", json.dumps(meta))
        z.writestr("pack.png", icon())
        z.writestr("assets/minecraft/textures/gui/options_background.png", background())

    folder = os.path.join(packs, "FuturePack")
    os.makedirs(folder, exist_ok=True)
    with open(os.path.join(folder, "pack.mcmeta"), "w", encoding="utf-8") as f:
        json.dump({"pack": {"pack_format": 2, "description": "A pack from the future with a long description that wraps"}}, f)


for target in ("port", "vanilla"):
    write(os.path.join(ROOT, "run", "cmp", target, "game"))
print("test packs written")
