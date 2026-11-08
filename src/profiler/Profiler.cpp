#include "profiler/Profiler.h"

#include "core/Java.h"
#include "core/JavaString.h"
#include "core/JavaTime.h"
#include "core/Log.h"

#include <algorithm>

namespace mc {

int Profiler::Result::getColor() const
{
    return (java::hashCode(field_76331_c) & 11184810) + 4473924;
}

void Profiler::clearProfiling()
{
    profilingMap.clear();
    profilingSection.clear();
    sectionList.clear();
}

void Profiler::startSection(const std::string& name)
{
    if (!profilingEnabled)
        return;
    if (!profilingSection.empty())
        profilingSection += ".";
    profilingSection += name;
    sectionList.push_back(profilingSection);
    timestampList.push_back(java::nanoTime());
}

void Profiler::endSection()
{
    if (!profilingEnabled)
        return;
    const int64_t i = java::nanoTime();
    const int64_t j = timestampList.back();
    timestampList.pop_back();
    sectionList.pop_back();
    const int64_t k = i - j;
    if (const int64_t* existing = profilingMap.find(profilingSection))
        profilingMap.put(profilingSection, *existing + k);
    else
        profilingMap.put(profilingSection, k);
    if (k > 100000000LL)
        Log::warn("Something's taking too long! '{}' took aprox {} ms", profilingSection, java::toString(static_cast<double>(k) / 1000000.0));
    profilingSection = !sectionList.empty() ? sectionList.back() : "";
}

std::vector<Profiler::Result> Profiler::getProfilingData(std::string profilerName)
{
    if (!profilingEnabled)
        return {};
    const int64_t* root = profilingMap.find("root");
    int64_t i = root ? *root : 0;
    const int64_t* own = profilingMap.find(profilerName);
    const int64_t j = own ? *own : -1;
    std::vector<Result> list;
    if (!profilerName.empty())
        profilerName += ".";
    const auto isChild = [&](const std::string& s) {
        return s.size() > profilerName.size() && s.starts_with(profilerName) && s.find('.', profilerName.size() + 1) == std::string::npos;
    };
    int64_t k = 0;
    for (const auto& [s, value] : profilingMap) {
        if (isChild(s))
            k += value;
    }
    const float f = static_cast<float>(k);
    if (k < j)
        k = j;
    if (i < k)
        i = k;
    for (const auto& [s1, l] : profilingMap) {
        if (isChild(s1)) {
            const double d0 = static_cast<double>(l) * 100.0 / static_cast<double>(k);
            const double d1 = static_cast<double>(l) * 100.0 / static_cast<double>(i);
            list.push_back({ s1.substr(profilerName.size()), d0, d1 });
        }
    }
    std::vector<std::string> keys;
    for (const auto& entry : profilingMap)
        keys.push_back(entry.first);
    for (const std::string& s3 : keys)
        profilingMap.put(s3, *profilingMap.find(s3) * 999LL / 1000LL);
    if (static_cast<float>(k) > f) {
        list.push_back({ "unspecified", static_cast<double>(static_cast<float>(k) - f) * 100.0 / static_cast<double>(k),
            static_cast<double>(static_cast<float>(k) - f) * 100.0 / static_cast<double>(i) });
    }
    // Java: Collections.sort (stable) by descending share, then descending name.
    std::stable_sort(list.begin(), list.end(), [](const Result& a, const Result& b) {
        if (a.field_76332_a != b.field_76332_a)
            return b.field_76332_a < a.field_76332_a;
        return b.field_76331_c < a.field_76331_c; // section names are ASCII
    });
    list.insert(list.begin(), Result { profilerName, 100.0, static_cast<double>(k) * 100.0 / static_cast<double>(i) });
    return list;
}

} // namespace mc
