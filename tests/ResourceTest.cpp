// Integration tests against the installed Minecraft 1.8.9 (skipped when none is found).

#include "client/main/GameInstallation.h"
#include "client/renderer/texture/TextureUtil.h"
#include "client/resources/DefaultResourcePack.h"
#include "client/resources/ResourceIndex.h"
#include "core/Sha1.h"
#include "core/ZipArchive.h"

#include <doctest/doctest.h>

#include <memory>
#include <optional>

using namespace mc;

namespace {

std::optional<GameInstallation> installation()
{
    static const std::optional<GameInstallation> found = findGameInstallation(".", {}, {}, "1.8.9", "1.8");
    return found;
}

std::unique_ptr<DefaultResourcePack> defaultPack()
{
    const auto game = installation();
    return std::make_unique<DefaultResourcePack>(std::make_shared<const ZipArchive>(game->clientJar),
        ResourceIndex(game->assetsDir, "1.8").getResourceMap());
}

} // namespace

TEST_CASE("installed client jar is the vanilla one")
{
    if (!installation()) {
        MESSAGE("Minecraft 1.8.9 not installed, skipping");
        return;
    }
    CHECK(sha1File(installation()->clientJar) == std::string(kClientJarSha1));
}

TEST_CASE("DefaultResourcePack reads from the jar and the asset index")
{
    if (!installation()) {
        MESSAGE("Minecraft 1.8.9 not installed, skipping");
        return;
    }
    const auto pack = defaultPack();

    // From the jar: the main menu logo.
    const ResourceLocation logo("textures/gui/title/minecraft.png");
    REQUIRE(pack->resourceExists(logo));
    const BufferedImage image = TextureUtil::readBufferedImage(pack->getInputStream(logo));
    CHECK(image.width == 256);
    CHECK(image.height == 256);

    // From the asset index only: window icons and sounds.
    const auto icon = pack->getInputStreamAssets(ResourceLocation("icons/icon_32x32.png"));
    REQUIRE(icon);
    const BufferedImage iconImage = TextureUtil::readBufferedImage(*icon);
    CHECK(iconImage.width == 32);
    CHECK(iconImage.height == 32);
    CHECK(pack->resourceExists(ResourceLocation("sounds/random/click.ogg")));

    // Splash texts are in the jar.
    const std::vector<uint8_t> splashes = pack->getInputStream(ResourceLocation("texts/splashes.txt"));
    CHECK(splashes.size() > 1000);

    CHECK_THROWS_AS(pack->getInputStream(ResourceLocation("textures/does/not/exist.png")), ResourceNotFoundException);
    CHECK_FALSE(pack->resourceExists(ResourceLocation("textures/does/not/exist.png")));
}

TEST_CASE("palette PNGs decode like ImageIO")
{
    if (!installation()) {
        MESSAGE("Minecraft 1.8.9 not installed, skipping");
        return;
    }
    const auto pack = defaultPack();
    // 1-bit palette with transparency: ascii.png is RGBA but unicode pages are palette PNGs.
    const BufferedImage page = TextureUtil::readBufferedImage(pack->getInputStream(ResourceLocation("textures/font/unicode_page_04.png")));
    CHECK(page.width == 256);
    bool sawTransparent = false;
    bool sawOpaqueWhite = false;
    for (const uint32_t pixel : page.pixels) {
        sawTransparent |= (pixel >> 24) == 0;
        sawOpaqueWhite |= pixel == 0xFFFFFFFFu;
    }
    CHECK(sawTransparent);
    CHECK(sawOpaqueWhite);
}
