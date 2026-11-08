#pragma once

// Original: net/minecraft/tileentity/TileEntityBanner.java -- the base colour and pattern layers of a banner.

#include "item/EnumDyeColor.h"
#include "tileentity/TileEntity.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mc {

class NBTTagCompound;
class NBTTagList;

class TileEntityBanner : public TileEntity {
public:
    enum class EnumBannerPattern {
        BASE, SQUARE_BOTTOM_LEFT, SQUARE_BOTTOM_RIGHT, SQUARE_TOP_LEFT, SQUARE_TOP_RIGHT, STRIPE_BOTTOM, STRIPE_TOP, STRIPE_LEFT,
        STRIPE_RIGHT, STRIPE_CENTER, STRIPE_MIDDLE, STRIPE_DOWNRIGHT, STRIPE_DOWNLEFT, STRIPE_SMALL, CROSS, STRAIGHT_CROSS,
        TRIANGLE_BOTTOM, TRIANGLE_TOP, TRIANGLES_BOTTOM, TRIANGLES_TOP, DIAGONAL_LEFT, DIAGONAL_RIGHT, DIAGONAL_LEFT_MIRROR,
        DIAGONAL_RIGHT_MIRROR, CIRCLE_MIDDLE, RHOMBUS_MIDDLE, HALF_VERTICAL, HALF_HORIZONTAL, HALF_VERTICAL_MIRROR,
        HALF_HORIZONTAL_MIRROR, BORDER, CURLY_BORDER, CREEPER, GRADIENT, GRADIENT_UP, BRICKS, SKULL, FLOWER, MOJANG
    };
    struct PatternInfo {
        std::string_view patternName;
        std::string_view patternID;
    };
    // Java: the patterns' names and ids (the crafting layers and stacks come with the banner recipes).
    static const PatternInfo& info(EnumBannerPattern pattern);
    static std::optional<EnumBannerPattern> getPatternByID(std::string_view id);

    void setItemValues(const ItemStack& stack);
    void writeToNBT(NBTTagCompound& compound) const override;
    static void setBaseColorAndPatterns(NBTTagCompound& compound, int baseColorIn, const std::shared_ptr<NBTTagList>& patternsIn);
    void readFromNBT(const NBTTagCompound& compound) override;
    int getBaseColor() const { return baseColor; }
    std::shared_ptr<Packet> getDescriptionPacket() override;
    static int getBaseColor(ItemStack& stack);
    static int getPatterns(ItemStack& stack);
    const std::vector<EnumBannerPattern>& getPatternList();
    const std::shared_ptr<NBTTagList>& getPatterns() const { return patterns; }
    const std::vector<EnumDyeColor>& getColorList();
    const std::string& getPatternResourceLocation();
    static void removeBannerData(ItemStack& stack);
    std::string_view getMappingName() const override { return "Banner"; }

private:
    void initializeBannerData();

    int baseColor = 0;
    std::shared_ptr<NBTTagList> patterns;
    bool field_175119_g = false;
    // Java: null until initializeBannerData fills them.
    std::optional<std::vector<EnumBannerPattern>> patternList;
    std::optional<std::vector<EnumDyeColor>> colorList;
    std::optional<std::string> patternResourceLocation;
};

} // namespace mc
