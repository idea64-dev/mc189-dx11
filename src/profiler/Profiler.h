#pragma once

// Original: net/minecraft/profiler/Profiler.java -- nested timing sections, only recorded while profilingEnabled
// (the F3 pie chart and the debug profile command turn it on).

#include "core/JavaHashMap.h"

#include <cstdint>
#include <string>
#include <vector>

namespace mc {

class Profiler {
public:
    struct Result {
        std::string field_76331_c; // section name
        double field_76332_a; // percent of the parent
        double field_76330_b; // percent of root
        int getColor() const;
    };

    void clearProfiling();
    void startSection(const std::string& name);
    void endSection();
    // Java returns null while profiling is off; empty here.
    std::vector<Result> getProfilingData(std::string profilerName);
    void endStartSection(const std::string& name)
    {
        endSection();
        startSection(name);
    }
    std::string getNameOfLastSection() const { return sectionList.empty() ? "[UNKNOWN]" : sectionList.back(); }

    bool profilingEnabled = false;

private:
    std::vector<std::string> sectionList;
    std::vector<int64_t> timestampList;
    std::string profilingSection;
    JavaHashMap<std::string, int64_t> profilingMap;
};

} // namespace mc
