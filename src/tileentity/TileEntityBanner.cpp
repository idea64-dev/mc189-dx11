#include "tileentity/TileEntityBanner.h"

#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"
#include "network/play/ServerPlayPackets.h"

#include <array>

namespace mc {

namespace {

constexpr std::array<TileEntityBanner::PatternInfo, 39> kPatterns { {
    { "base", "b" },
    { "square_bottom_left", "bl" },
    { "square_bottom_right", "br" },
    { "square_top_left", "tl" },
    { "square_top_right", "tr" },
    { "stripe_bottom", "bs" },
    { "stripe_top", "ts" },
    { "stripe_left", "ls" },
    { "stripe_right", "rs" },
    { "stripe_center", "cs" },
    { "stripe_middle", "ms" },
    { "stripe_downright", "drs" },
    { "stripe_downleft", "dls" },
    { "small_stripes", "ss" },
    { "cross", "cr" },
    { "straight_cross", "sc" },
    { "triangle_bottom", "bt" },
    { "triangle_top", "tt" },
    { "triangles_bottom", "bts" },
    { "triangles_top", "tts" },
    { "diagonal_left", "ld" },
    { "diagonal_up_right", "rd" },
    { "diagonal_up_left", "lud" },
    { "diagonal_right", "rud" },
    { "circle", "mc" },
    { "rhombus", "mr" },
    { "half_vertical", "vh" },
    { "half_horizontal", "hh" },
    { "half_vertical_right", "vhr" },
    { "half_horizontal_bottom", "hhb" },
    { "border", "bo" },
    { "curly_border", "cbo" },
    { "creeper", "cre" },
    { "gradient", "gra" },
    { "gradient_up", "gru" },
    { "bricks", "bri" },
    { "skull", "sku" },
    { "flower", "flo" },
    { "mojang", "moj" },
} };

} // namespace

const TileEntityBanner::PatternInfo& TileEntityBanner::info(EnumBannerPattern pattern)
{
    return kPatterns[static_cast<size_t>(pattern)];
}

std::optional<TileEntityBanner::EnumBannerPattern> TileEntityBanner::getPatternByID(std::string_view id)
{
    for (size_t i = 0; i < kPatterns.size(); ++i) {
        if (kPatterns[i].patternID == id)
            return static_cast<EnumBannerPattern>(i);
    }
    return std::nullopt;
}

void TileEntityBanner::setItemValues(const ItemStack& stack)
{
    patterns = nullptr;
    if (stack.hasTagCompound() && stack.getTagCompound()->hasKey("BlockEntityTag", 10)) {
        const auto nbttagcompound = stack.getTagCompound()->getCompoundTag("BlockEntityTag");
        if (nbttagcompound->hasKey("Patterns"))
            patterns = std::static_pointer_cast<NBTTagList>(nbttagcompound->getTagList("Patterns", 10)->copy());
        if (nbttagcompound->hasKey("Base", 99))
            baseColor = nbttagcompound->getInteger("Base");
        else
            baseColor = stack.getMetadata() & 15;
    } else {
        baseColor = stack.getMetadata() & 15;
    }
    patternList.reset();
    colorList.reset();
    patternResourceLocation = "";
    field_175119_g = true;
}

void TileEntityBanner::writeToNBT(NBTTagCompound& compound) const
{
    TileEntity::writeToNBT(compound);
    setBaseColorAndPatterns(compound, baseColor, patterns);
}

void TileEntityBanner::setBaseColorAndPatterns(NBTTagCompound& compound, int baseColorIn, const std::shared_ptr<NBTTagList>& patternsIn)
{
    compound.setInteger("Base", baseColorIn);
    if (patternsIn)
        compound.setTag("Patterns", patternsIn);
}

void TileEntityBanner::readFromNBT(const NBTTagCompound& compound)
{
    TileEntity::readFromNBT(compound);
    baseColor = compound.getInteger("Base");
    patterns = compound.getTagList("Patterns", 10);
    patternList.reset();
    colorList.reset();
    patternResourceLocation.reset();
    field_175119_g = true;
}

int TileEntityBanner::getBaseColor(ItemStack& stack)
{
    const auto nbttagcompound = stack.getSubCompound("BlockEntityTag", false);
    return nbttagcompound && nbttagcompound->hasKey("Base") ? nbttagcompound->getInteger("Base") : stack.getMetadata();
}

int TileEntityBanner::getPatterns(ItemStack& stack)
{
    const auto nbttagcompound = stack.getSubCompound("BlockEntityTag", false);
    return nbttagcompound && nbttagcompound->hasKey("Patterns") ? nbttagcompound->getTagList("Patterns", 10)->tagCount() : 0;
}

const std::vector<TileEntityBanner::EnumBannerPattern>& TileEntityBanner::getPatternList()
{
    static const std::vector<EnumBannerPattern> kNone;
    initializeBannerData();
    return patternList ? *patternList : kNone;
}

const std::vector<EnumDyeColor>& TileEntityBanner::getColorList()
{
    static const std::vector<EnumDyeColor> kNone;
    initializeBannerData();
    return colorList ? *colorList : kNone;
}

const std::string& TileEntityBanner::getPatternResourceLocation()
{
    initializeBannerData();
    return *patternResourceLocation;
}

void TileEntityBanner::initializeBannerData()
{
    if (!patternList || !colorList || !patternResourceLocation) {
        if (!field_175119_g) {
            patternResourceLocation = "";
        } else {
            patternList.emplace();
            colorList.emplace();
            patternList->push_back(EnumBannerPattern::BASE);
            colorList->push_back(dyeColorByDyeDamage(baseColor));
            patternResourceLocation = "b" + std::to_string(baseColor);
            if (patterns) {
                for (int i = 0; i < patterns->tagCount(); ++i) {
                    const auto nbttagcompound = patterns->getCompoundTagAt(i);
                    const std::optional<EnumBannerPattern> pattern = getPatternByID(nbttagcompound->getString("Pattern"));
                    if (pattern) {
                        patternList->push_back(*pattern);
                        const int j = nbttagcompound->getInteger("Color");
                        colorList->push_back(dyeColorByDyeDamage(j));
                        *patternResourceLocation += std::string(info(*pattern).patternID) + std::to_string(j);
                    }
                }
            }
        }
    }
}

void TileEntityBanner::removeBannerData(ItemStack& stack)
{
    const auto nbttagcompound = stack.getSubCompound("BlockEntityTag", false);
    if (nbttagcompound && nbttagcompound->hasKey("Patterns", 9)) {
        const auto nbttaglist = nbttagcompound->getTagList("Patterns", 10);
        if (nbttaglist->tagCount() > 0) {
            nbttaglist->removeTag(nbttaglist->tagCount() - 1);
            if (nbttaglist->hasNoTags()) {
                stack.getTagCompound()->removeTag("BlockEntityTag");
                if (stack.getTagCompound()->hasNoTags())
                    stack.setTagCompound(nullptr);
            }
        }
    }
}

std::shared_ptr<Packet> TileEntityBanner::getDescriptionPacket()
{
    auto nbttagcompound = std::make_shared<NBTTagCompound>();
    writeToNBT(*nbttagcompound);
    return std::make_shared<S35PacketUpdateTileEntity>(pos, 6, nbttagcompound);
}

} // namespace mc
