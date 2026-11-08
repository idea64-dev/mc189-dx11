#include "potion/PotionHelper.h"

#include "core/Java.h"

#include <array>
#include <unordered_map>

namespace mc {

namespace {

constexpr const char* kPotionPrefixes[32] { "potion.prefix.mundane", "potion.prefix.uninteresting", "potion.prefix.bland", "potion.prefix.clear",
    "potion.prefix.milky", "potion.prefix.diffuse", "potion.prefix.artless", "potion.prefix.thin", "potion.prefix.awkward", "potion.prefix.flat",
    "potion.prefix.bulky", "potion.prefix.bungling", "potion.prefix.buttered", "potion.prefix.smooth", "potion.prefix.suave",
    "potion.prefix.debonair", "potion.prefix.thick", "potion.prefix.elegant", "potion.prefix.fancy", "potion.prefix.charming",
    "potion.prefix.dashing", "potion.prefix.refined", "potion.prefix.cordial", "potion.prefix.sparkling", "potion.prefix.potent",
    "potion.prefix.foul", "potion.prefix.odorless", "potion.prefix.rank", "potion.prefix.harsh", "potion.prefix.acrid", "potion.prefix.gross",
    "potion.prefix.stinky" };

// Java keeps these in HashMaps keyed by potion id; only lookups are made, so arrays by id do.
struct Requirements {
    std::array<const char*, 32> requirements {};
    std::array<const char*, 32> amplifiers {};

    Requirements()
    {
        requirements[static_cast<size_t>(Potion::regeneration->getId())] = "0 & !1 & !2 & !3 & 0+6";
        requirements[static_cast<size_t>(Potion::moveSpeed->getId())] = "!0 & 1 & !2 & !3 & 1+6";
        requirements[static_cast<size_t>(Potion::fireResistance->getId())] = "0 & 1 & !2 & !3 & 0+6";
        requirements[static_cast<size_t>(Potion::heal->getId())] = "0 & !1 & 2 & !3";
        requirements[static_cast<size_t>(Potion::poison->getId())] = "!0 & !1 & 2 & !3 & 2+6";
        requirements[static_cast<size_t>(Potion::weakness->getId())] = "!0 & !1 & !2 & 3 & 3+6";
        requirements[static_cast<size_t>(Potion::harm->getId())] = "!0 & !1 & 2 & 3";
        requirements[static_cast<size_t>(Potion::moveSlowdown->getId())] = "!0 & 1 & !2 & 3 & 3+6";
        requirements[static_cast<size_t>(Potion::damageBoost->getId())] = "0 & !1 & !2 & 3 & 3+6";
        requirements[static_cast<size_t>(Potion::nightVision->getId())] = "!0 & 1 & 2 & !3 & 2+6";
        requirements[static_cast<size_t>(Potion::invisibility->getId())] = "!0 & 1 & 2 & 3 & 2+6";
        requirements[static_cast<size_t>(Potion::waterBreathing->getId())] = "0 & !1 & 2 & 3 & 2+6";
        requirements[static_cast<size_t>(Potion::jump->getId())] = "0 & 1 & !2 & 3 & 3+6";
        for (const Potion* potion : { Potion::moveSpeed, Potion::digSpeed, Potion::damageBoost, Potion::regeneration, Potion::harm, Potion::heal,
                 Potion::resistance, Potion::poison, Potion::jump })
            amplifiers[static_cast<size_t>(potion->getId())] = "5";
    }
};

const Requirements& requirements()
{
    static const Requirements value;
    return value;
}

int isFlagSet(int value, int bit)
{
    return PotionHelper::checkFlag(value, bit) ? 1 : 0;
}

int isFlagUnset(int value, int bit)
{
    return PotionHelper::checkFlag(value, bit) ? 0 : 1;
}

int countSetFlags(int value)
{
    int i;
    for (i = 0; value > 0; ++i)
        value &= value - 1;
    return i;
}

int getPotionEffect(bool negate, bool multiply, bool invert, int comparison, int bit, int factor, int dataValue)
{
    int i = 0;
    if (negate) {
        i = isFlagUnset(dataValue, bit);
    } else if (comparison != -1) {
        if (comparison == 0 && countSetFlags(dataValue) == bit)
            i = 1;
        else if (comparison == 1 && countSetFlags(dataValue) > bit)
            i = 1;
        else if (comparison == 2 && countSetFlags(dataValue) < bit)
            i = 1;
    } else {
        i = isFlagSet(dataValue, bit);
    }
    if (multiply)
        i *= factor;
    if (invert)
        i *= -1;
    return i;
}

int brewBitOperations(int value, int bit, bool clear, bool toggle, bool require)
{
    if (require) {
        if (!PotionHelper::checkFlag(value, bit))
            return 0;
    } else if (clear) {
        value &= ~(1 << bit);
    } else if (toggle) {
        if ((value & 1 << bit) == 0)
            value |= 1 << bit;
        else
            value &= ~(1 << bit);
    } else {
        value |= 1 << bit;
    }
    return value;
}

// String.indexOf(ch, from) on ASCII text.
int indexOf(std::string_view text, char c, int from)
{
    const size_t pos = text.find(c, static_cast<size_t>(from < 0 ? 0 : from));
    return pos == std::string_view::npos ? -1 : static_cast<int>(pos);
}

} // namespace

int PotionHelper::calcPotionLiquidColor(const std::vector<PotionEffect>& effects)
{
    const int i = 3694022;
    if (effects.empty())
        return i;
    float f = 0.0f;
    float f1 = 0.0f;
    float f2 = 0.0f;
    float f3 = 0.0f;
    for (const PotionEffect& potioneffect : effects) {
        if (potioneffect.getIsShowParticles()) {
            const int j = Potion::potionTypes[static_cast<size_t>(potioneffect.getPotionID())]->getLiquidColor();
            for (int k = 0; k <= potioneffect.getAmplifier(); ++k) {
                f += static_cast<float>(j >> 16 & 255) / 255.0f;
                f1 += static_cast<float>(j >> 8 & 255) / 255.0f;
                f2 += static_cast<float>(j >> 0 & 255) / 255.0f;
                ++f3;
            }
        }
    }
    if (f3 == 0.0f)
        return 0;
    f = f / f3 * 255.0f;
    f1 = f1 / f3 * 255.0f;
    f2 = f2 / f3 * 255.0f;
    return java::f2i(f) << 16 | java::f2i(f1) << 8 | java::f2i(f2);
}

bool PotionHelper::getAreAmbient(const std::vector<PotionEffect>& effects)
{
    for (const PotionEffect& potioneffect : effects) {
        if (!potioneffect.getIsAmbient())
            return false;
    }
    return true;
}

int PotionHelper::getLiquidColor(int dataValue, bool bypassCache)
{
    if (bypassCache)
        return calcPotionLiquidColor(getPotionEffects(dataValue, true));
    static std::unordered_map<int, int> cache;
    if (const auto it = cache.find(dataValue); it != cache.end())
        return it->second;
    const int i = calcPotionLiquidColor(getPotionEffects(dataValue, false));
    cache[dataValue] = i;
    return i;
}

const char* PotionHelper::getPotionPrefix(int dataValue)
{
    return kPotionPrefixes[getPotionPrefixIndex(dataValue)];
}

int PotionHelper::parsePotionEffects(std::string_view text, int start, int end, int dataValue)
{
    if (start >= static_cast<int>(text.size()) || end < 0 || start >= end)
        return 0;
    const int i = indexOf(text, '|', start);
    if (i >= 0 && i < end) {
        const int l1 = parsePotionEffects(text, start, i - 1, dataValue);
        if (l1 > 0)
            return l1;
        const int j2 = parsePotionEffects(text, i + 1, end, dataValue);
        return j2 > 0 ? j2 : 0;
    }
    const int j = indexOf(text, '&', start);
    if (j >= 0 && j < end) {
        const int i2 = parsePotionEffects(text, start, j - 1, dataValue);
        if (i2 <= 0)
            return 0;
        const int k2 = parsePotionEffects(text, j + 1, end, dataValue);
        return k2 <= 0 ? 0 : (i2 > k2 ? i2 : k2);
    }
    bool flag = false;
    bool flag1 = false;
    bool flag2 = false;
    bool flag3 = false;
    bool flag4 = false;
    int k = -1;
    int l = 0;
    int i1 = 0;
    int j1 = 0;
    const auto flush = [&] {
        j1 += getPotionEffect(flag3, flag1, flag4, k, l, i1, dataValue);
        flag3 = false;
        flag4 = false;
        flag = false;
        flag1 = false;
        flag2 = false;
        i1 = 0;
        l = 0;
        k = -1;
    };
    for (int k1 = start; k1 < end; ++k1) {
        const char c0 = text[static_cast<size_t>(k1)];
        if (c0 >= '0' && c0 <= '9') {
            if (flag) {
                i1 = c0 - '0';
                flag1 = true;
            } else {
                l = l * 10;
                l = l + (c0 - '0');
                flag2 = true;
            }
        } else if (c0 == '*') {
            flag = true;
        } else if (c0 == '!') {
            if (flag2)
                flush();
            flag3 = true;
        } else if (c0 == '-') {
            if (flag2)
                flush();
            flag4 = true;
        } else if (c0 != '=' && c0 != '<' && c0 != '>') {
            if (c0 == '+' && flag2)
                flush();
        } else {
            if (flag2)
                flush();
            if (c0 == '=')
                k = 0;
            else if (c0 == '<')
                k = 2;
            else if (c0 == '>')
                k = 1;
        }
    }
    if (flag2)
        j1 += getPotionEffect(flag3, flag1, flag4, k, l, i1, dataValue);
    return j1;
}

std::vector<PotionEffect> PotionHelper::getPotionEffects(int dataValue, bool includeUsable)
{
    std::vector<PotionEffect> list;
    for (const Potion* potion : Potion::potionTypes) {
        if (potion && (!potion->isUsable() || includeUsable)) {
            const char* s = requirements().requirements[static_cast<size_t>(potion->getId())];
            if (s) {
                const std::string_view sv(s);
                int i = parsePotionEffects(sv, 0, static_cast<int>(sv.size()), dataValue);
                if (i > 0) {
                    int j = 0;
                    if (const char* s1 = requirements().amplifiers[static_cast<size_t>(potion->getId())]) {
                        const std::string_view sv1(s1);
                        j = parsePotionEffects(sv1, 0, static_cast<int>(sv1.size()), dataValue);
                        if (j < 0)
                            j = 0;
                    }
                    if (potion->isInstant()) {
                        i = 1;
                    } else {
                        i = 1200 * (i * 3 + (i - 1) * 2);
                        i = i >> j;
                        i = static_cast<int>(java::round(static_cast<double>(i) * potion->getEffectiveness()));
                        if ((dataValue & 16384) != 0)
                            i = static_cast<int>(java::round(static_cast<double>(i) * 0.75 + 0.5));
                    }
                    PotionEffect potioneffect(potion->getId(), i, j);
                    if ((dataValue & 16384) != 0)
                        potioneffect.setSplashPotion(true);
                    list.push_back(potioneffect);
                }
            }
        }
    }
    return list;
}

int PotionHelper::applyIngredient(int dataValue, std::string_view ingredient)
{
    const int j = static_cast<int>(ingredient.size());
    bool flag = false;
    bool flag1 = false;
    bool flag2 = false;
    bool flag3 = false;
    int k = 0;
    const auto flush = [&] {
        dataValue = brewBitOperations(dataValue, k, flag2, flag1, flag3);
        flag3 = false;
        flag1 = false;
        flag2 = false;
        flag = false;
        k = 0;
    };
    for (int l = 0; l < j; ++l) {
        const char c0 = ingredient[static_cast<size_t>(l)];
        if (c0 >= '0' && c0 <= '9') {
            k = k * 10;
            k = k + (c0 - '0');
            flag = true;
        } else if (c0 == '!') {
            if (flag)
                flush();
            flag1 = true;
        } else if (c0 == '-') {
            if (flag)
                flush();
            flag2 = true;
        } else if (c0 == '+') {
            if (flag)
                flush();
        } else if (c0 == '&') {
            if (flag)
                flush();
            flag3 = true;
        }
    }
    if (flag)
        dataValue = brewBitOperations(dataValue, k, flag2, flag1, flag3);
    return dataValue & 32767;
}

int PotionHelper::getPotionPrefixIndexFlags(int value, int a, int b, int c, int d, int e)
{
    return (checkFlag(value, a) ? 16 : 0) | (checkFlag(value, b) ? 8 : 0) | (checkFlag(value, c) ? 4 : 0) | (checkFlag(value, d) ? 2 : 0)
        | (checkFlag(value, e) ? 1 : 0);
}

} // namespace mc
