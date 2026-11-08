#include "block/material/Material.h"

#include <stdexcept>

namespace mc {

// --- MapColor ---

std::array<const MapColor*, 64> MapColor::mapColorArray {};

MapColor::MapColor(int index, int color)
    : colorValue(color)
    , colorIndex(index)
{
    if (index < 0 || index > 63)
        throw std::out_of_range("Map colour ID must be between 0 and 63 (inclusive)");
    mapColorArray[static_cast<size_t>(index)] = this;
}

int MapColor::getMapColor(int brightness) const
{
    int i = 220;
    if (brightness == 3)
        i = 135;
    if (brightness == 2)
        i = 255;
    if (brightness == 1)
        i = 220;
    if (brightness == 0)
        i = 180;
    const int j = ((colorValue >> 16) & 255) * i / 255;
    const int k = ((colorValue >> 8) & 255) * i / 255;
    const int l = (colorValue & 255) * i / 255;
    return -16777216 | (j << 16) | (k << 8) | l;
}

const MapColor MapColor::airColor(0, 0);
const MapColor MapColor::grassColor(1, 8368696);
const MapColor MapColor::sandColor(2, 16247203);
const MapColor MapColor::clothColor(3, 13092807);
const MapColor MapColor::tntColor(4, 16711680);
const MapColor MapColor::iceColor(5, 10526975);
const MapColor MapColor::ironColor(6, 10987431);
const MapColor MapColor::foliageColor(7, 31744);
const MapColor MapColor::snowColor(8, 16777215);
const MapColor MapColor::clayColor(9, 10791096);
const MapColor MapColor::dirtColor(10, 9923917);
const MapColor MapColor::stoneColor(11, 7368816);
const MapColor MapColor::waterColor(12, 4210943);
const MapColor MapColor::woodColor(13, 9402184);
const MapColor MapColor::quartzColor(14, 16776437);
const MapColor MapColor::adobeColor(15, 14188339);
const MapColor MapColor::magentaColor(16, 11685080);
const MapColor MapColor::lightBlueColor(17, 6724056);
const MapColor MapColor::yellowColor(18, 15066419);
const MapColor MapColor::limeColor(19, 8375321);
const MapColor MapColor::pinkColor(20, 15892389);
const MapColor MapColor::grayColor(21, 5000268);
const MapColor MapColor::silverColor(22, 10066329);
const MapColor MapColor::cyanColor(23, 5013401);
const MapColor MapColor::purpleColor(24, 8339378);
const MapColor MapColor::blueColor(25, 3361970);
const MapColor MapColor::brownColor(26, 6704179);
const MapColor MapColor::greenColor(27, 6717235);
const MapColor MapColor::redColor(28, 10040115);
const MapColor MapColor::blackColor(29, 1644825);
const MapColor MapColor::goldColor(30, 16445005);
const MapColor MapColor::diamondColor(31, 6085589);
const MapColor MapColor::lapisColor(32, 4882687);
const MapColor MapColor::emeraldColor(33, 55610);
const MapColor MapColor::obsidianColor(34, 8476209);
const MapColor MapColor::netherrackColor(35, 7340544);

// --- Material ---

Material::Material(const MapColor& color, Kind kind)
    : materialMapColor(color)
{
    switch (kind) {
    case Kind::NORMAL:
        break;
    case Kind::LIQUID: // MaterialLiquid
        setReplaceable();
        setNoPushMobility();
        liquid = true;
        movementBlocking = false;
        solid = false;
        break;
    case Kind::LOGIC: // MaterialLogic
        setAdventureModeExempt();
        solid = false;
        lightBlocking = false;
        movementBlocking = false;
        break;
    case Kind::TRANSPARENT: // MaterialTransparent
        setReplaceable();
        solid = false;
        lightBlocking = false;
        movementBlocking = false;
        break;
    case Kind::PORTAL: // MaterialPortal
        solid = false;
        lightBlocking = false;
        movementBlocking = false;
        break;
    case Kind::WEB: // the anonymous subclass of Material.web
        movementBlocking = false;
        break;
    }
}

Material Material::make(const MapColor& color, Kind kind, void (*configure)(Material&))
{
    Material material(color, kind);
    configure(material);
    return material;
}

const Material Material::air = make(MapColor::airColor, Kind::TRANSPARENT, [](Material&) { });
const Material Material::grass = make(MapColor::grassColor, Kind::NORMAL, [](Material&) { });
const Material Material::ground = make(MapColor::dirtColor, Kind::NORMAL, [](Material&) { });
const Material Material::wood = make(MapColor::woodColor, Kind::NORMAL, [](Material& m) { m.setBurning(); });
const Material Material::rock = make(MapColor::stoneColor, Kind::NORMAL, [](Material& m) { m.setRequiresTool(); });
const Material Material::iron = make(MapColor::ironColor, Kind::NORMAL, [](Material& m) { m.setRequiresTool(); });
const Material Material::anvil = make(MapColor::ironColor, Kind::NORMAL, [](Material& m) { m.setRequiresTool().setImmovableMobility(); });
const Material Material::water = make(MapColor::waterColor, Kind::LIQUID, [](Material& m) { m.setNoPushMobility(); });
const Material Material::lava = make(MapColor::tntColor, Kind::LIQUID, [](Material& m) { m.setNoPushMobility(); });
const Material Material::leaves = make(MapColor::foliageColor, Kind::NORMAL, [](Material& m) { m.setBurning().setTranslucent().setNoPushMobility(); });
const Material Material::plants = make(MapColor::foliageColor, Kind::LOGIC, [](Material& m) { m.setNoPushMobility(); });
const Material Material::vine = make(MapColor::foliageColor, Kind::LOGIC, [](Material& m) { m.setBurning().setNoPushMobility().setReplaceable(); });
const Material Material::sponge = make(MapColor::yellowColor, Kind::NORMAL, [](Material&) { });
const Material Material::cloth = make(MapColor::clothColor, Kind::NORMAL, [](Material& m) { m.setBurning(); });
const Material Material::fire = make(MapColor::airColor, Kind::TRANSPARENT, [](Material& m) { m.setNoPushMobility(); });
const Material Material::sand = make(MapColor::sandColor, Kind::NORMAL, [](Material&) { });
const Material Material::circuits = make(MapColor::airColor, Kind::LOGIC, [](Material& m) { m.setNoPushMobility(); });
const Material Material::carpet = make(MapColor::clothColor, Kind::LOGIC, [](Material& m) { m.setBurning(); });
const Material Material::glass = make(MapColor::airColor, Kind::NORMAL, [](Material& m) { m.setTranslucent().setAdventureModeExempt(); });
const Material Material::redstoneLight = make(MapColor::airColor, Kind::NORMAL, [](Material& m) { m.setAdventureModeExempt(); });
const Material Material::tnt = make(MapColor::tntColor, Kind::NORMAL, [](Material& m) { m.setBurning().setTranslucent(); });
const Material Material::coral = make(MapColor::foliageColor, Kind::NORMAL, [](Material& m) { m.setNoPushMobility(); });
const Material Material::ice = make(MapColor::iceColor, Kind::NORMAL, [](Material& m) { m.setTranslucent().setAdventureModeExempt(); });
const Material Material::packedIce = make(MapColor::iceColor, Kind::NORMAL, [](Material& m) { m.setAdventureModeExempt(); });
const Material Material::snow = make(MapColor::snowColor, Kind::LOGIC, [](Material& m) { m.setReplaceable().setTranslucent().setRequiresTool().setNoPushMobility(); });
const Material Material::craftedSnow = make(MapColor::snowColor, Kind::NORMAL, [](Material& m) { m.setRequiresTool(); });
const Material Material::cactus = make(MapColor::foliageColor, Kind::NORMAL, [](Material& m) { m.setTranslucent().setNoPushMobility(); });
const Material Material::clay = make(MapColor::clayColor, Kind::NORMAL, [](Material&) { });
const Material Material::gourd = make(MapColor::foliageColor, Kind::NORMAL, [](Material& m) { m.setNoPushMobility(); });
const Material Material::dragonEgg = make(MapColor::foliageColor, Kind::NORMAL, [](Material& m) { m.setNoPushMobility(); });
const Material Material::portal = make(MapColor::airColor, Kind::PORTAL, [](Material& m) { m.setImmovableMobility(); });
const Material Material::cake = make(MapColor::airColor, Kind::NORMAL, [](Material& m) { m.setNoPushMobility(); });
const Material Material::web = make(MapColor::clothColor, Kind::WEB, [](Material& m) { m.setRequiresTool().setNoPushMobility(); });
const Material Material::piston = make(MapColor::stoneColor, Kind::NORMAL, [](Material& m) { m.setImmovableMobility(); });
const Material Material::barrier = make(MapColor::airColor, Kind::NORMAL, [](Material& m) { m.setRequiresTool().setImmovableMobility(); });

} // namespace mc
