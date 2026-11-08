// Original: net/minecraft/world/World.java -- the client-side sky, cloud, fog and star colours.

#include "world/World.h"

#include "entity/Entity.h"
#include "util/MathHelper.h"
#include "world/WorldProvider.h"
#include "world/biome/BiomeGenBase.h"

namespace mc {

namespace {

constexpr float kPi = 3.1415927f; // (float)Math.PI

} // namespace

Vec3 World::getSkyColor(const Entity* entityIn, float partialTicks)
{
    const float f = getCelestialAngle(partialTicks);
    float f1 = MathHelper::cos(f * kPi * 2.0f) * 2.0f + 0.5f;
    f1 = MathHelper::clamp_float(f1, 0.0f, 1.0f);
    const int i = MathHelper::floor_double(entityIn->posX);
    const int j = MathHelper::floor_double(entityIn->posY);
    const int k = MathHelper::floor_double(entityIn->posZ);
    const BlockPos blockpos(i, j, k);
    BiomeGenBase* biomegenbase = getBiomeGenForCoords(blockpos);
    const float f2 = biomegenbase->getFloatTemperature(blockpos);
    const int l = biomegenbase->getSkyColorByTemp(f2);
    float f3 = static_cast<float>(l >> 16 & 255) / 255.0f;
    float f4 = static_cast<float>(l >> 8 & 255) / 255.0f;
    float f5 = static_cast<float>(l & 255) / 255.0f;
    f3 = f3 * f1;
    f4 = f4 * f1;
    f5 = f5 * f1;
    const float f6 = getRainStrength(partialTicks);

    if (f6 > 0.0f) {
        const float f7 = (f3 * 0.3f + f4 * 0.59f + f5 * 0.11f) * 0.6f;
        const float f8 = 1.0f - f6 * 0.75f;
        f3 = f3 * f8 + f7 * (1.0f - f8);
        f4 = f4 * f8 + f7 * (1.0f - f8);
        f5 = f5 * f8 + f7 * (1.0f - f8);
    }

    const float f10 = getThunderStrength(partialTicks);
    if (f10 > 0.0f) {
        const float f11 = (f3 * 0.3f + f4 * 0.59f + f5 * 0.11f) * 0.2f;
        const float f9 = 1.0f - f10 * 0.75f;
        f3 = f3 * f9 + f11 * (1.0f - f9);
        f4 = f4 * f9 + f11 * (1.0f - f9);
        f5 = f5 * f9 + f11 * (1.0f - f9);
    }

    if (lastLightningBolt > 0) {
        float f12 = static_cast<float>(lastLightningBolt) - partialTicks;
        if (f12 > 1.0f)
            f12 = 1.0f;
        f12 = f12 * 0.45f;
        f3 = f3 * (1.0f - f12) + 0.8f * f12;
        f4 = f4 * (1.0f - f12) + 0.8f * f12;
        f5 = f5 * (1.0f - f12) + 1.0f * f12;
    }

    return Vec3(static_cast<double>(f3), static_cast<double>(f4), static_cast<double>(f5));
}

float World::getSunBrightness(float partialTicks)
{
    const float f = getCelestialAngle(partialTicks);
    float f1 = 1.0f - (MathHelper::cos(f * kPi * 2.0f) * 2.0f + 0.2f);
    f1 = MathHelper::clamp_float(f1, 0.0f, 1.0f);
    f1 = 1.0f - f1;
    f1 = static_cast<float>(static_cast<double>(f1) * (1.0 - static_cast<double>(getRainStrength(partialTicks) * 5.0f) / 16.0));
    f1 = static_cast<float>(static_cast<double>(f1) * (1.0 - static_cast<double>(getThunderStrength(partialTicks) * 5.0f) / 16.0));
    return f1 * 0.8f + 0.2f;
}

Vec3 World::getCloudColour(float partialTicks)
{
    const float f = getCelestialAngle(partialTicks);
    float f1 = MathHelper::cos(f * kPi * 2.0f) * 2.0f + 0.5f;
    f1 = MathHelper::clamp_float(f1, 0.0f, 1.0f);
    float f2 = static_cast<float>(cloudColour >> 16 & 255LL) / 255.0f;
    float f3 = static_cast<float>(cloudColour >> 8 & 255LL) / 255.0f;
    float f4 = static_cast<float>(cloudColour & 255LL) / 255.0f;
    const float f5 = getRainStrength(partialTicks);

    if (f5 > 0.0f) {
        const float f6 = (f2 * 0.3f + f3 * 0.59f + f4 * 0.11f) * 0.6f;
        const float f7 = 1.0f - f5 * 0.95f;
        f2 = f2 * f7 + f6 * (1.0f - f7);
        f3 = f3 * f7 + f6 * (1.0f - f7);
        f4 = f4 * f7 + f6 * (1.0f - f7);
    }

    f2 = f2 * (f1 * 0.9f + 0.1f);
    f3 = f3 * (f1 * 0.9f + 0.1f);
    f4 = f4 * (f1 * 0.85f + 0.15f);
    const float f9 = getThunderStrength(partialTicks);

    if (f9 > 0.0f) {
        const float f10 = (f2 * 0.3f + f3 * 0.59f + f4 * 0.11f) * 0.2f;
        const float f8 = 1.0f - f9 * 0.95f;
        f2 = f2 * f8 + f10 * (1.0f - f8);
        f3 = f3 * f8 + f10 * (1.0f - f8);
        f4 = f4 * f8 + f10 * (1.0f - f8);
    }

    return Vec3(static_cast<double>(f2), static_cast<double>(f3), static_cast<double>(f4));
}

Vec3 World::getFogColor(float partialTicks)
{
    const float f = getCelestialAngle(partialTicks);
    return provider->getFogColor(f, partialTicks);
}

float World::getStarBrightness(float partialTicks)
{
    const float f = getCelestialAngle(partialTicks);
    float f1 = 1.0f - (MathHelper::cos(f * kPi * 2.0f) * 2.0f + 0.25f);
    f1 = MathHelper::clamp_float(f1, 0.0f, 1.0f);
    return f1 * f1 * 0.5f;
}

} // namespace mc
