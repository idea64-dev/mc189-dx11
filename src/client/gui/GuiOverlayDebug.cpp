#include "client/gui/GuiOverlayDebug.h"

#include "block/Block.h"
#include "block/properties/IProperty.h"
#include "block/state/BlockState.h"
#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/ScaledResolution.h"
#include "client/multiplayer/WorldClient.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/RenderGlobal.h"
#include "client/settings/GameSettings.h"
#include "core/JavaLocale.h"
#include "entity/player/EntityPlayerMP.h"
#include "entity/player/EntityPlayerSP.h"
#include "platform/Display.h"
#include "render/gl/GL.h"
#include "render/gl/GLConstants.h"
#include "server/integrated/IntegratedServer.h"
#include "server/management/ServerConfigurationManager.h"
#include "util/ChatComponent.h"
#include "util/MathHelper.h"
#include "world/DifficultyInstance.h"
#include "world/WorldType.h"
#include "world/biome/BiomeGenBase.h"
#include "world/biome/WorldChunkManager.h"
#include "world/chunk/Chunk.h"
#include "world/chunk/IChunkProvider.h"

#include <Windows.h>
#include <psapi.h>

#include <thread>

namespace mc {

namespace {

std::string fmtInt(int64_t value)
{
    return java::formatInt(value);
}

// String.format("%0<width>d", value) for non-negative values.
std::string zeroPadded(int64_t value, int width)
{
    std::string s = java::formatInt(value);
    while (static_cast<int>(s.size()) < width)
        s.insert(s.begin(), '0');
    return s;
}

// String.format("% <width>d", value) for non-negative values: a leading space, padded to the width.
std::string spaceFlagged(int64_t value, int width)
{
    std::string s = " " + java::formatInt(value);
    while (static_cast<int>(s.size()) < width)
        s.insert(s.begin(), ' ');
    return s;
}

int64_t bytesToMb(int64_t bytes)
{
    return bytes / 1024 / 1024;
}

// Java: OpenGlHelper.getCpu() -- "<logical processors>x <processor name>".
std::string getCpu()
{
    static const std::string cpu = [] {
        wchar_t name[256] {};
        DWORD size = sizeof(name);
        std::string processorName = "<unknown>";
        if (RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"ProcessorNameString", RRF_RT_REG_SZ, nullptr, name, &size)
            == ERROR_SUCCESS) {
            const int len = WideCharToMultiByte(CP_UTF8, 0, name, -1, nullptr, 0, nullptr, nullptr);
            processorName.assign(static_cast<size_t>(len > 0 ? len - 1 : 0), '\0');
            WideCharToMultiByte(CP_UTF8, 0, name, -1, processorName.data(), len, nullptr, nullptr);
        }
        return std::to_string(std::thread::hardware_concurrency()) + "x " + processorName;
    }();
    return cpu;
}

std::string facingName(EnumFacing facing)
{
    return std::string(getName(facing));
}

} // namespace

GuiOverlayDebug::GuiOverlayDebug(Minecraft* mcIn)
    : mc(mcIn)
    , fontRenderer(mcIn->fontRendererObj.get())
{
}

void GuiOverlayDebug::renderDebugInfo(const ScaledResolution& scaledResolutionIn)
{
    mc->mcProfiler.startSection("debug");
    DxStateManager::pushMatrix();
    renderDebugInfoLeft();
    renderDebugInfoRight(scaledResolutionIn);
    DxStateManager::popMatrix();
    // Java: the lagometer when showLagometer is on (FrameTimer, not ported yet).
    mc->mcProfiler.endSection();
}

bool GuiOverlayDebug::isReducedDebug() const
{
    return mc->thePlayer->hasReducedDebug() || mc->gameSettings->reducedDebugInfo;
}

void GuiOverlayDebug::renderDebugInfoLeft()
{
    const std::vector<std::string> list = call();

    for (size_t i = 0; i < list.size(); ++i) {
        const std::string& s = list[i];

        if (!s.empty()) {
            const int j = fontRenderer->FONT_HEIGHT;
            const int k = fontRenderer->getStringWidth(s);
            const int i1 = 2 + j * static_cast<int>(i);
            drawRect(1, i1 - 1, 2 + k + 1, i1 + j - 1, -1873784752);
            fontRenderer->drawString(s, 2, i1, 14737632);
        }
    }
}

void GuiOverlayDebug::renderDebugInfoRight(const ScaledResolution& scaledRes)
{
    const std::vector<std::string> list = getDebugInfoRight();

    for (size_t i = 0; i < list.size(); ++i) {
        const std::string& s = list[i];

        if (!s.empty()) {
            const int j = fontRenderer->FONT_HEIGHT;
            const int k = fontRenderer->getStringWidth(s);
            const int l = scaledRes.getScaledWidth() - 2 - k;
            const int i1 = 2 + j * static_cast<int>(i);
            drawRect(l - 1, i1 - 1, l + k + 1, i1 + j - 1, -1873784752);
            fontRenderer->drawString(s, l, i1, 14737632);
        }
    }
}

std::vector<std::string> GuiOverlayDebug::call()
{
    Entity* view = mc->getRenderViewEntity();
    const BlockPos blockpos(view->posX, view->getEntityBoundingBox().minY, view->posZ);
    // Java: "P: " + effectRenderer.getStatistics() -- the particle count; no particles yet.
    const std::string particles = "P: 0. T: All: " + std::to_string(mc->theWorld->getLoadedEntityList().size());
    const std::string providerName = mc->theWorld->getChunkProvider()->makeString();
    const std::string header = "Minecraft 1.8.9 (" + mc->getVersion() + "/vanilla)";

    if (isReducedDebug()) {
        return { header, mc->debug, mc->renderGlobal->getDebugInfoRenders(), mc->renderGlobal->getDebugInfoEntities(), particles, providerName, "",
            "Chunk-relative: " + fmtInt(blockpos.getX() & 15) + " " + fmtInt(blockpos.getY() & 15) + " " + fmtInt(blockpos.getZ() & 15) };
    }

    const EnumFacing enumfacing = view->getHorizontalFacing();
    std::string s = "Invalid";

    switch (enumfacing) {
    case EnumFacing::NORTH: s = "Towards negative Z"; break;
    case EnumFacing::SOUTH: s = "Towards positive Z"; break;
    case EnumFacing::WEST: s = "Towards negative X"; break;
    case EnumFacing::EAST: s = "Towards positive X"; break;
    default: break;
    }

    std::vector<std::string> list { header, mc->debug, mc->renderGlobal->getDebugInfoRenders(), mc->renderGlobal->getDebugInfoEntities(), particles,
        providerName, "",
        "XYZ: " + java::formatFixed(view->posX, 0, 3) + " / " + java::formatFixed(view->getEntityBoundingBox().minY, 0, 5) + " / "
            + java::formatFixed(view->posZ, 0, 3),
        "Block: " + fmtInt(blockpos.getX()) + " " + fmtInt(blockpos.getY()) + " " + fmtInt(blockpos.getZ()),
        "Chunk: " + fmtInt(blockpos.getX() & 15) + " " + fmtInt(blockpos.getY() & 15) + " " + fmtInt(blockpos.getZ() & 15) + " in " + fmtInt(blockpos.getX() >> 4)
            + " " + fmtInt(blockpos.getY() >> 4) + " " + fmtInt(blockpos.getZ() >> 4),
        "Facing: " + facingName(enumfacing) + " (" + s + ") (" + java::formatFixed(MathHelper::wrapAngleTo180_float(view->rotationYaw), 0, 1) + " / "
            + java::formatFixed(MathHelper::wrapAngleTo180_float(view->rotationPitch), 0, 1) + ")" };

    if (mc->theWorld != nullptr && mc->theWorld->isBlockLoaded(blockpos)) {
        Chunk* chunk = mc->theWorld->getChunkFromBlockCoords(blockpos);
        list.push_back("Biome: " + chunk->getBiome(blockpos, mc->theWorld->getWorldChunkManager())->biomeName);
        list.push_back("Light: " + std::to_string(chunk->getLightSubtracted(blockpos, 0)) + " (" + std::to_string(chunk->getLightFor(EnumSkyBlock::SKY, blockpos))
            + " sky, " + std::to_string(chunk->getLightFor(EnumSkyBlock::BLOCK, blockpos)) + " block)");
        DifficultyInstance difficultyinstance = getDifficultyForLocation(*mc->theWorld, blockpos);

        if (mc->isIntegratedServerRunning() && mc->getIntegratedServer() != nullptr) {
            // Java reads the server's player and world from the client thread as well.
            EntityPlayerMP* entityplayermp = mc->getIntegratedServer()->getConfigurationManager()->getPlayerByUUID(mc->thePlayer->getUniqueID());

            if (entityplayermp != nullptr)
                difficultyinstance = getDifficultyForLocation(*entityplayermp->worldObj, BlockPos(entityplayermp->posX, entityplayermp->posY, entityplayermp->posZ));
        }

        list.push_back("Local Difficulty: " + java::formatFixed(difficultyinstance.getAdditionalDifficulty(), 0, 2) + " (Day "
            + fmtInt(mc->theWorld->getWorldTime() / 24000) + ")");
    }

    if (mc->objectMouseOver && mc->objectMouseOver->typeOfHit == MovingObjectPosition::MovingObjectType::BLOCK && mc->objectMouseOver->getBlockPos()) {
        const BlockPos& blockpos1 = *mc->objectMouseOver->getBlockPos();
        list.push_back("Looking at: " + fmtInt(blockpos1.getX()) + " " + fmtInt(blockpos1.getY()) + " " + fmtInt(blockpos1.getZ()));
    }

    return list;
}

std::vector<std::string> GuiOverlayDebug::getDebugInfoRight()
{
    PROCESS_MEMORY_COUNTERS_EX counters {};
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters));
    MEMORYSTATUSEX memory { sizeof(memory) };
    GlobalMemoryStatusEx(&memory);
    // Java: maxMemory, totalMemory and the used part of it.
    const int64_t i = static_cast<int64_t>(memory.ullTotalPhys);
    const int64_t j = static_cast<int64_t>(counters.PrivateUsage);
    const int64_t l = static_cast<int64_t>(counters.WorkingSetSize);

    std::vector<std::string> list { "C++: MSVC 64bit", "Mem: " + spaceFlagged(l * 100 / i, 2) + "% " + zeroPadded(bytesToMb(l), 3) + "/" + zeroPadded(bytesToMb(i), 3) + "MB",
        "Allocated: " + spaceFlagged(j * 100 / i, 2) + "% " + zeroPadded(bytesToMb(j), 3) + "MB", "", "CPU: " + getCpu(), "",
        "Display: " + fmtInt(Display::getWidth()) + "x" + fmtInt(Display::getHeight()) + " (" + GL::glGetString(GL::GL_VENDOR) + ")", GL::glGetString(GL::GL_RENDERER),
        GL::glGetString(GL::GL_VERSION) };

    if (isReducedDebug())
        return list;

    if (mc->objectMouseOver && mc->objectMouseOver->typeOfHit == MovingObjectPosition::MovingObjectType::BLOCK && mc->objectMouseOver->getBlockPos()) {
        const BlockPos& blockpos = *mc->objectMouseOver->getBlockPos();
        const IBlockState* iblockstate = mc->theWorld->getBlockState(blockpos);

        if (mc->theWorld->getWorldType() != &WorldType::DEBUG_WORLD)
            iblockstate = iblockstate->getBlock()->getActualState(iblockstate, mc->theWorld, blockpos);

        list.emplace_back("");
        const auto name = Block::blockRegistry.getNameForObject(iblockstate->getBlock());
        list.push_back(name ? name->toString() : "null");

        for (const IProperty* property : iblockstate->getPropertyNames()) {
            std::string s = property->getValueName(iblockstate->getRawValue(*property));

            // Java: Boolean.TRUE / FALSE values are coloured.
            if (s == "true")
                s = EnumChatFormatting::toString('a') + s;
            else if (s == "false")
                s = EnumChatFormatting::toString('c') + s;

            list.push_back(property->getName() + ": " + s);
        }
    }

    return list;
}

} // namespace mc
