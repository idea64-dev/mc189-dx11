#include "profiler/PlayerUsageSnooper.h"

#include <Windows.h>
#include <psapi.h>
#include <rpc.h>

#include <thread>

namespace mc {

namespace {

// java.util.UUID.randomUUID().toString()
std::string randomUuid()
{
    UUID uuid {};
    UuidCreate(&uuid);
    RPC_CSTR text = nullptr;
    std::string result;
    if (UuidToStringA(&uuid, &text) == RPC_S_OK) {
        result = reinterpret_cast<const char*>(text);
        RpcStringFreeA(&text);
    }
    return result;
}

OSVERSIONINFOW windowsVersion()
{
    OSVERSIONINFOW info { sizeof(info) };
    using RtlGetVersionFn = LONG(WINAPI*)(OSVERSIONINFOW*);
    if (HMODULE ntdll = GetModuleHandleW(L"ntdll.dll")) {
        if (auto rtlGetVersion = reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion")))
            rtlGetVersion(&info);
    }
    return info;
}

} // namespace

PlayerUsageSnooper::PlayerUsageSnooper(std::string sideIn, IPlayerUsage& playerStatCollector, int64_t startTime)
    : uniqueID(randomUuid())
    , side(std::move(sideIn))
    , playerStatsCollector(playerStatCollector)
    , minecraftStartTimeMilis(startTime)
{
}

void PlayerUsageSnooper::startSnooper()
{
    if (!isRunning) {
        isRunning = true;
        addOSData();
    }
}

void PlayerUsageSnooper::addOSData()
{
    // Java also reports the -X JVM arguments and java.version; there is no JVM here.
    addClientStat("snooper_token", uniqueID);
    addStatToSnooper("snooper_token", uniqueID);
    const OSVERSIONINFOW version = windowsVersion();
    // os.name as Java 8 reports it on Windows 10/11
    addStatToSnooper("os_name", version.dwMajorVersion >= 10 && version.dwBuildNumber >= 22000 ? "Windows 11" : "Windows 10");
    addStatToSnooper("os_version", std::to_string(version.dwMajorVersion) + "." + std::to_string(version.dwMinorVersion));
    addStatToSnooper("os_architecture", "amd64");
    addClientStat("version", "1.8.9");
    playerStatsCollector.addServerTypeToSnooper(*this);
}

void PlayerUsageSnooper::addMemoryStatsToSnooper()
{
    // Java's Runtime heap figures; the closest native equivalents are the process's committed memory and the
    // machine's physical memory.
    PROCESS_MEMORY_COUNTERS_EX counters {};
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters));
    MEMORYSTATUSEX memory { sizeof(memory) };
    GlobalMemoryStatusEx(&memory);
    addStatToSnooper("memory_total", std::to_string(counters.PrivateUsage));
    addStatToSnooper("memory_max", std::to_string(memory.ullTotalPhys));
    addStatToSnooper("memory_free", std::to_string(memory.ullAvailPhys));
    addStatToSnooper("cpu_cores", std::to_string(std::thread::hardware_concurrency()));
    playerStatsCollector.addServerStatsToSnooper(*this);
}

void PlayerUsageSnooper::addClientStat(const std::string& statName, std::string statValue)
{
    clientStats.insert_or_assign(statName, std::move(statValue));
}

void PlayerUsageSnooper::addStatToSnooper(const std::string& statName, std::string statValue)
{
    snooperStats.insert_or_assign(statName, std::move(statValue));
}

std::map<std::string, std::string> PlayerUsageSnooper::getCurrentStats()
{
    addMemoryStatsToSnooper();
    std::map<std::string, std::string> map = snooperStats;
    for (const auto& [key, value] : clientStats)
        map.insert_or_assign(key, value);
    return map;
}

} // namespace mc
