#pragma once

// Original: net/minecraft/world/gen/structure/MapGenMineshaft.java and StructureMineshaftStart.java

#include "world/gen/FlatGeneratorInfo.h"
#include "world/gen/structure/MapGenStructure.h"

namespace mc {

class MapGenMineshaft : public MapGenStructure {
public:
    MapGenMineshaft() = default;
    explicit MapGenMineshaft(const FlatGeneratorInfo::Options& options);

    std::string getStructureName() const override { return "Mineshaft"; }

protected:
    bool canSpawnStructureAtCoords(int chunkX, int chunkZ) override;
    std::shared_ptr<StructureStart> getStructureStart(int chunkX, int chunkZ) override;

private:
    double field_82673_e = 0.004;
};

class StructureMineshaftStart : public StructureStart {
public:
    StructureMineshaftStart() = default;
    StructureMineshaftStart(World* worldIn, Random& rand, int chunkX, int chunkZ);

    std::string_view getStructureName() const override { return "Mineshaft"; }
};

} // namespace mc
