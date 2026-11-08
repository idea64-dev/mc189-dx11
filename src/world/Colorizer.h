#pragma once

// Original: net/minecraft/world/ColorizerGrass.java and ColorizerFoliage.java
// The 256x256 colormaps are filled from textures/colormap/*.png by the reload listeners (stage 7); until then
// they are all zero, as in Java before resources load.

#include <array>
#include <vector>

namespace mc {

class ColorizerGrass {
public:
    static void setGrassBiomeColorizer(std::vector<int> buffer) { grassBuffer() = std::move(buffer); }
    static int getGrassColor(double temperature, double humidity)
    {
        humidity = humidity * temperature;
        const int i = static_cast<int>((1.0 - temperature) * 255.0);
        const int j = static_cast<int>((1.0 - humidity) * 255.0);
        const int k = j << 8 | i;
        auto& buffer = grassBuffer();
        return k > static_cast<int>(buffer.size()) ? -65281 : buffer[static_cast<size_t>(k)];
    }

private:
    static std::vector<int>& grassBuffer()
    {
        static std::vector<int> buffer(65536, 0);
        return buffer;
    }
};

class ColorizerFoliage {
public:
    static void setFoliageBiomeColorizer(std::vector<int> buffer) { foliageBuffer() = std::move(buffer); }
    static int getFoliageColor(double temperature, double humidity)
    {
        humidity = humidity * temperature;
        const int i = static_cast<int>((1.0 - temperature) * 255.0);
        const int j = static_cast<int>((1.0 - humidity) * 255.0);
        return foliageBuffer()[static_cast<size_t>(j << 8 | i)];
    }
    static int getFoliageColorPine() { return 6396257; }
    static int getFoliageColorBirch() { return 8431445; }
    static int getFoliageColorBasic() { return 4764952; }

private:
    static std::vector<int>& foliageBuffer()
    {
        static std::vector<int> buffer(65536, 0);
        return buffer;
    }
};

} // namespace mc
