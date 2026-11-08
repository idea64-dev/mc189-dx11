#pragma once

// Original: net/minecraft/block/material/Material.java (with MaterialLiquid, MaterialLogic, MaterialTransparent,
// MaterialPortal and cobweb's anonymous subclass) and MapColor.java.
// The Java subclasses only override a few flags, so a Material is one class with the overridden answers stored.

#include <array>
#include <string_view>

namespace mc {

class MapColor {
public:
    static std::array<const MapColor*, 64> mapColorArray;

    static const MapColor airColor, grassColor, sandColor, clothColor, tntColor, iceColor, ironColor, foliageColor, snowColor,
        clayColor, dirtColor, stoneColor, waterColor, woodColor, quartzColor, adobeColor, magentaColor, lightBlueColor, yellowColor,
        limeColor, pinkColor, grayColor, silverColor, cyanColor, purpleColor, blueColor, brownColor, greenColor, redColor, blackColor,
        goldColor, diamondColor, lapisColor, emeraldColor, obsidianColor, netherrackColor;

    // Shaded color for the map (brightness 0..3), 0xAARRGGBB.
    int getMapColor(int brightness) const;

    const int colorValue;
    const int colorIndex;

private:
    MapColor(int index, int color);
};

class Material {
public:
    static const Material air, grass, ground, wood, rock, iron, anvil, water, lava, leaves, plants, vine, sponge, cloth, fire, sand,
        circuits, carpet, glass, redstoneLight, tnt, coral, ice, packedIce, snow, craftedSnow, cactus, clay, gourd, dragonEgg,
        portal, cake, web, piston, barrier;

    bool isLiquid() const { return liquid; }
    bool isSolid() const { return solid; }
    bool blocksLight() const { return lightBlocking; }
    bool blocksMovement() const { return movementBlocking; }
    bool getCanBurn() const { return canBurn; }
    bool isReplaceable() const { return replaceable; }
    bool isOpaque() const { return isTranslucent ? false : blocksMovement(); }
    bool isToolNotRequired() const { return requiresNoTool; }
    int getMaterialMobility() const { return mobilityFlag; }
    bool isAdventureModeExempt() const { return adventureModeExempt; }
    const MapColor& getMaterialMapColor() const { return materialMapColor; }

private:
    enum class Kind { NORMAL, LIQUID, LOGIC, TRANSPARENT, PORTAL, WEB };

    Material(const MapColor& color, Kind kind = Kind::NORMAL);

    Material& setTranslucent() { isTranslucent = true; return *this; }
    Material& setRequiresTool() { requiresNoTool = false; return *this; }
    Material& setBurning() { canBurn = true; return *this; }
    Material& setReplaceable() { replaceable = true; return *this; }
    Material& setNoPushMobility() { mobilityFlag = 1; return *this; }
    Material& setImmovableMobility() { mobilityFlag = 2; return *this; }
    Material& setAdventureModeExempt() { adventureModeExempt = true; return *this; }

    // Builds a configured copy; used to initialize the constants.
    static Material make(const MapColor& color, Kind kind, void (*configure)(Material&));

    bool canBurn = false;
    bool replaceable = false;
    bool isTranslucent = false;
    const MapColor& materialMapColor;
    bool requiresNoTool = true;
    int mobilityFlag = 0;
    bool adventureModeExempt = false;
    bool liquid = false;
    bool solid = true;
    bool lightBlocking = true;
    bool movementBlocking = true;
};

} // namespace mc
