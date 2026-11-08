#include "client/gui/GuiCustomizeWorldScreen.h"

#include "client/Minecraft.h"
#include "client/gui/FontRenderer.h"
#include "client/gui/GuiCreateWorld.h"
#include "client/gui/GuiPageButtonList.h"
#include "client/gui/GuiSlider.h"
#include "client/gui/GuiTextField.h"
#include "client/renderer/DxStateManager.h"
#include "client/renderer/Tessellator.h"
#include "client/renderer/texture/TextureManager.h"
#include "client/renderer/vertex/DefaultVertexFormats.h"
#include "client/resources/Locale.h"
#include "core/Java.h"
#include "core/JavaLocale.h"
#include "core/JavaString.h"
#include "platform/Keyboard.h"
#include "util/MathHelper.h"
#include "world/biome/BiomeNames.h"

#include <climits>
#include <cmath>

namespace mc {

namespace {

using Entry = GuiPageButtonList::GuiListEntry;

// Java: the predicate of the page-4 edit boxes -- empty, or a finite non-negative float.
bool isValidNumber(const std::string& text)
{
    const auto f = java::tryParseFloat(text);
    return text.empty() || (f && std::isfinite(*f) && *f >= 0.0f);
}

} // namespace

GuiCustomizeWorldScreen::GuiCustomizeWorldScreen(std::shared_ptr<GuiCreateWorld> parent, const std::string& settingsJson)
    : field_175343_i(std::move(parent))
{
    loadSettingsJson(settingsJson);
}

GuiCustomizeWorldScreen::~GuiCustomizeWorldScreen() = default;

void GuiCustomizeWorldScreen::initGui()
{
    int i = 0;
    int j = 0;
    if (field_175349_r) {
        i = field_175349_r->getPage();
        j = field_175349_r->getAmountScrolled();
    }
    field_175341_a = I18n::format("options.customizeTitle");
    buttonList.clear();
    field_175345_v = &addButton<GuiButton>(302, 20, 5, 80, 20, I18n::format("createWorld.customize.custom.prev"));
    field_175344_w = &addButton<GuiButton>(303, width - 100, 5, 80, 20, I18n::format("createWorld.customize.custom.next"));
    field_175346_u = &addButton<GuiButton>(304, width / 2 - 187, height - 27, 90, 20, I18n::format("createWorld.customize.custom.defaults"));
    field_175347_t = &addButton<GuiButton>(301, width / 2 - 92, height - 27, 90, 20, I18n::format("createWorld.customize.custom.randomize"));
    field_175350_z = &addButton<GuiButton>(305, width / 2 + 3, height - 27, 90, 20, I18n::format("createWorld.customize.custom.presets"));
    field_175348_s = &addButton<GuiButton>(300, width / 2 + 98, height - 27, 90, 20, I18n::format("gui.done"));
    field_175346_u->enabled = field_175338_A;
    field_175352_x = &addButton<GuiButton>(306, width / 2 - 55, 160, 50, 20, I18n::format("gui.yes"));
    field_175352_x->visible = false;
    field_175351_y = &addButton<GuiButton>(307, width / 2 + 5, 160, 50, 20, I18n::format("gui.no"));
    field_175351_y->visible = false;
    if (field_175339_B != 0) {
        field_175352_x->visible = true;
        field_175351_y->visible = true;
    }
    func_175325_f();
    if (i != 0) {
        field_175349_r->setPage(i);
        field_175349_r->scrollBy(j);
        func_175328_i();
    }
}

void GuiCustomizeWorldScreen::handleMouseInput()
{
    GuiScreen::handleMouseInput();
    field_175349_r->handleMouseInput();
}

void GuiCustomizeWorldScreen::func_175325_f()
{
    const ChunkProviderSettings::Factory& f = field_175336_F;
    const GuiSlider::FormatHelper format = [this](int id, const std::string& name, float value) { return getText(id, name, value); };
    const auto slider = [&](int id, const char* key, bool visible, float min, float max, float value) {
        return Entry::slider(id, I18n::format(key), visible, format, min, max, value);
    };
    const auto button = [&](int id, const char* key, bool value) { return Entry::button(id, I18n::format(key), true, value); };
    const auto label = [&](int id, const std::string& text) { return Entry::label(id, text, false); };
    const auto box = [&](int id, const std::string& text) { return Entry::editBox(id, text, false, isValidNumber); };
    const auto fl = [](int v) { return static_cast<float>(v); };

    GuiPageButtonList::Page page0 { slider(160, "createWorld.customize.custom.seaLevel", true, 1.0f, 255.0f, fl(f.seaLevel)),
        button(148, "createWorld.customize.custom.useCaves", f.useCaves), button(150, "createWorld.customize.custom.useStrongholds", f.useStrongholds),
        button(151, "createWorld.customize.custom.useVillages", f.useVillages), button(152, "createWorld.customize.custom.useMineShafts", f.useMineShafts),
        button(153, "createWorld.customize.custom.useTemples", f.useTemples), button(210, "createWorld.customize.custom.useMonuments", f.useMonuments),
        button(154, "createWorld.customize.custom.useRavines", f.useRavines), button(149, "createWorld.customize.custom.useDungeons", f.useDungeons),
        slider(157, "createWorld.customize.custom.dungeonChance", true, 1.0f, 100.0f, fl(f.dungeonChance)),
        button(155, "createWorld.customize.custom.useWaterLakes", f.useWaterLakes),
        slider(158, "createWorld.customize.custom.waterLakeChance", true, 1.0f, 100.0f, fl(f.waterLakeChance)),
        button(156, "createWorld.customize.custom.useLavaLakes", f.useLavaLakes),
        slider(159, "createWorld.customize.custom.lavaLakeChance", true, 10.0f, 100.0f, fl(f.lavaLakeChance)),
        button(161, "createWorld.customize.custom.useLavaOceans", f.useLavaOceans),
        slider(162, "createWorld.customize.custom.fixedBiome", true, -1.0f, 37.0f, fl(f.fixedBiome)),
        slider(163, "createWorld.customize.custom.biomeSize", true, 1.0f, 8.0f, fl(f.biomeSize)),
        slider(164, "createWorld.customize.custom.riverSize", true, 1.0f, 5.0f, fl(f.riverSize)) };

    GuiPageButtonList::Page page1;
    struct Ore {
        int labelId;
        const char* labelKey;
        int ids[4];
        bool lapis;
        int values[4];
    };
    const Ore ores[] {
        { 416, "tile.dirt.name", { 165, 166, 167, 168 }, false, { f.dirtSize, f.dirtCount, f.dirtMinHeight, f.dirtMaxHeight } },
        { 417, "tile.gravel.name", { 169, 170, 171, 172 }, false, { f.gravelSize, f.gravelCount, f.gravelMinHeight, f.gravelMaxHeight } },
        { 418, "tile.stone.granite.name", { 173, 174, 175, 176 }, false, { f.graniteSize, f.graniteCount, f.graniteMinHeight, f.graniteMaxHeight } },
        { 419, "tile.stone.diorite.name", { 177, 178, 179, 180 }, false, { f.dioriteSize, f.dioriteCount, f.dioriteMinHeight, f.dioriteMaxHeight } },
        { 420, "tile.stone.andesite.name", { 181, 182, 183, 184 }, false, { f.andesiteSize, f.andesiteCount, f.andesiteMinHeight, f.andesiteMaxHeight } },
        { 421, "tile.oreCoal.name", { 185, 186, 187, 189 }, false, { f.coalSize, f.coalCount, f.coalMinHeight, f.coalMaxHeight } },
        { 422, "tile.oreIron.name", { 190, 191, 192, 193 }, false, { f.ironSize, f.ironCount, f.ironMinHeight, f.ironMaxHeight } },
        { 423, "tile.oreGold.name", { 194, 195, 196, 197 }, false, { f.goldSize, f.goldCount, f.goldMinHeight, f.goldMaxHeight } },
        { 424, "tile.oreRedstone.name", { 198, 199, 200, 201 }, false, { f.redstoneSize, f.redstoneCount, f.redstoneMinHeight, f.redstoneMaxHeight } },
        { 425, "tile.oreDiamond.name", { 202, 203, 204, 205 }, false, { f.diamondSize, f.diamondCount, f.diamondMinHeight, f.diamondMaxHeight } },
        { 426, "tile.oreLapis.name", { 206, 207, 208, 209 }, true, { f.lapisSize, f.lapisCount, f.lapisCenterHeight, f.lapisSpread } },
    };
    for (const Ore& ore : ores) {
        page1.push_back(label(ore.labelId, I18n::format(ore.labelKey)));
        page1.push_back(std::nullopt);
        page1.push_back(slider(ore.ids[0], "createWorld.customize.custom.size", false, 1.0f, 50.0f, fl(ore.values[0])));
        page1.push_back(slider(ore.ids[1], "createWorld.customize.custom.count", false, 0.0f, 40.0f, fl(ore.values[1])));
        page1.push_back(slider(ore.ids[2], ore.lapis ? "createWorld.customize.custom.center" : "createWorld.customize.custom.minHeight", false, 0.0f, 255.0f,
            fl(ore.values[2])));
        page1.push_back(slider(ore.ids[3], ore.lapis ? "createWorld.customize.custom.spread" : "createWorld.customize.custom.maxHeight", false, 0.0f, 255.0f,
            fl(ore.values[3])));
    }

    struct Noise {
        int sliderId;
        int labelId;
        int boxId;
        const char* key;
        float min;
        float max;
        float value;
        bool wide; // "%5.3f" rather than "%2.3f"
    };
    const Noise noises[] {
        { 100, 400, 132, "createWorld.customize.custom.mainNoiseScaleX", 1.0f, 5000.0f, f.mainNoiseScaleX, true },
        { 101, 401, 133, "createWorld.customize.custom.mainNoiseScaleY", 1.0f, 5000.0f, f.mainNoiseScaleY, true },
        { 102, 402, 134, "createWorld.customize.custom.mainNoiseScaleZ", 1.0f, 5000.0f, f.mainNoiseScaleZ, true },
        { 103, 403, 135, "createWorld.customize.custom.depthNoiseScaleX", 1.0f, 2000.0f, f.depthNoiseScaleX, true },
        { 104, 404, 136, "createWorld.customize.custom.depthNoiseScaleZ", 1.0f, 2000.0f, f.depthNoiseScaleZ, true },
        { 105, 405, 137, "createWorld.customize.custom.depthNoiseScaleExponent", 0.01f, 20.0f, f.depthNoiseScaleExponent, false },
        { 106, 406, 138, "createWorld.customize.custom.baseSize", 1.0f, 25.0f, f.baseSize, false },
        { 107, 407, 139, "createWorld.customize.custom.coordinateScale", 1.0f, 6000.0f, f.coordinateScale, true },
        { 108, 408, 140, "createWorld.customize.custom.heightScale", 1.0f, 6000.0f, f.heightScale, true },
        { 109, 409, 141, "createWorld.customize.custom.stretchY", 0.01f, 50.0f, f.stretchY, false },
        { 110, 410, 142, "createWorld.customize.custom.upperLimitScale", 1.0f, 5000.0f, f.upperLimitScale, true },
        { 111, 411, 143, "createWorld.customize.custom.lowerLimitScale", 1.0f, 5000.0f, f.lowerLimitScale, true },
        { 112, 412, 144, "createWorld.customize.custom.biomeDepthWeight", 1.0f, 20.0f, f.biomeDepthWeight, false },
        { 113, 413, 145, "createWorld.customize.custom.biomeDepthOffset", 0.0f, 20.0f, f.biomeDepthOffset, false },
        { 114, 414, 146, "createWorld.customize.custom.biomeScaleWeight", 1.0f, 20.0f, f.biomeScaleWeight, false },
        { 115, 415, 147, "createWorld.customize.custom.biomeScaleOffset", 0.0f, 20.0f, f.biomeScaleOffset, false },
    };
    GuiPageButtonList::Page page2;
    GuiPageButtonList::Page page3;
    for (const Noise& noise : noises) {
        page2.push_back(slider(noise.sliderId, noise.key, false, noise.min, noise.max, noise.value));
        page3.push_back(label(noise.labelId, I18n::format(noise.key) + ":"));
        page3.push_back(box(noise.boxId, java::formatFixed(static_cast<double>(noise.value), noise.wide ? 5 : 2, 3)));
    }

    field_175349_r = std::make_unique<GuiPageButtonList>(*mc, width, height, 32, height - 32, 25, *this,
        std::vector<GuiPageButtonList::Page> { std::move(page0), std::move(page1), std::move(page2), std::move(page3) });
    for (int i = 0; i < 4; ++i)
        field_175342_h[static_cast<size_t>(i)] = I18n::format("createWorld.customize.custom.page" + std::to_string(i));
    func_175328_i();
}

std::string GuiCustomizeWorldScreen::getSettingsJson() const
{
    std::string json = field_175336_F.toString();
    std::erase(json, '\n');
    return json;
}

void GuiCustomizeWorldScreen::loadSettingsJson(const std::string& json)
{
    field_175336_F = json.empty() ? ChunkProviderSettings::Factory() : ChunkProviderSettings::Factory::jsonToFactory(json);
}

void GuiCustomizeWorldScreen::onText(int id, const std::string& text)
{
    float f = 0.0f;
    try {
        f = java::parseFloat(text);
    } catch (const std::invalid_argument&) {
    }
    float f1 = 0.0f;
    ChunkProviderSettings::Factory& s = field_175336_F;
    switch (id) {
    case 132: f1 = s.mainNoiseScaleX = MathHelper::clamp_float(f, 1.0f, 5000.0f); break;
    case 133: f1 = s.mainNoiseScaleY = MathHelper::clamp_float(f, 1.0f, 5000.0f); break;
    case 134: f1 = s.mainNoiseScaleZ = MathHelper::clamp_float(f, 1.0f, 5000.0f); break;
    case 135: f1 = s.depthNoiseScaleX = MathHelper::clamp_float(f, 1.0f, 2000.0f); break;
    case 136: f1 = s.depthNoiseScaleZ = MathHelper::clamp_float(f, 1.0f, 2000.0f); break;
    case 137: f1 = s.depthNoiseScaleExponent = MathHelper::clamp_float(f, 0.01f, 20.0f); break;
    case 138: f1 = s.baseSize = MathHelper::clamp_float(f, 1.0f, 25.0f); break;
    case 139: f1 = s.coordinateScale = MathHelper::clamp_float(f, 1.0f, 6000.0f); break;
    case 140: f1 = s.heightScale = MathHelper::clamp_float(f, 1.0f, 6000.0f); break;
    case 141: f1 = s.stretchY = MathHelper::clamp_float(f, 0.01f, 50.0f); break;
    case 142: f1 = s.upperLimitScale = MathHelper::clamp_float(f, 1.0f, 5000.0f); break;
    case 143: f1 = s.lowerLimitScale = MathHelper::clamp_float(f, 1.0f, 5000.0f); break;
    case 144: f1 = s.biomeDepthWeight = MathHelper::clamp_float(f, 1.0f, 20.0f); break;
    case 145: f1 = s.biomeDepthOffset = MathHelper::clamp_float(f, 0.0f, 20.0f); break;
    case 146: f1 = s.biomeScaleWeight = MathHelper::clamp_float(f, 1.0f, 20.0f); break;
    case 147: f1 = s.biomeScaleOffset = MathHelper::clamp_float(f, 0.0f, 20.0f); break;
    default: break;
    }
    if (f1 != f && f != 0.0f)
        static_cast<GuiTextField*>(field_175349_r->getComponent(id))->setText(func_175330_b(id, f1));
    static_cast<GuiSlider*>(field_175349_r->getComponent(id - 132 + 100))->setSliderValue(f1, false);
    if (!(field_175336_F == field_175334_E))
        func_181031_a(true);
}

void GuiCustomizeWorldScreen::func_181031_a(bool changed)
{
    field_175338_A = changed;
    field_175346_u->enabled = changed;
}

std::string GuiCustomizeWorldScreen::getText(int id, const std::string& name, float value) const
{
    return name + ": " + func_175330_b(id, value);
}

std::string GuiCustomizeWorldScreen::func_175330_b(int id, float value) const
{
    switch (id) {
    case 100: case 101: case 102: case 103: case 104: case 107: case 108: case 110: case 111:
    case 132: case 133: case 134: case 135: case 136: case 139: case 140: case 142: case 143:
        return java::formatFixed(static_cast<double>(value), 5, 3);
    case 105: case 106: case 109: case 112: case 113: case 114: case 115:
    case 137: case 138: case 141: case 144: case 145: case 146: case 147:
        return java::formatFixed(static_cast<double>(value), 2, 3);
    case 162: {
        if (value < 0.0f)
            return I18n::format("gui.all");
        const int biome = static_cast<int>(value) >= kHellBiomeId ? static_cast<int>(value) + 2 : static_cast<int>(value);
        const char* name = biomeNameById(biome);
        return name ? name : "?";
    }
    default:
        return java::formatInt(java::f2i(value));
    }
}

void GuiCustomizeWorldScreen::onToggle(int id, bool value)
{
    ChunkProviderSettings::Factory& s = field_175336_F;
    switch (id) {
    case 148: s.useCaves = value; break;
    case 149: s.useDungeons = value; break;
    case 150: s.useStrongholds = value; break;
    case 151: s.useVillages = value; break;
    case 152: s.useMineShafts = value; break;
    case 153: s.useTemples = value; break;
    case 154: s.useRavines = value; break;
    case 155: s.useWaterLakes = value; break;
    case 156: s.useLavaLakes = value; break;
    case 161: s.useLavaOceans = value; break;
    case 210: s.useMonuments = value; break;
    default: break;
    }
    if (!(field_175336_F == field_175334_E))
        func_181031_a(true);
}

void GuiCustomizeWorldScreen::onTick(int id, float value)
{
    ChunkProviderSettings::Factory& s = field_175336_F;
    const int v = java::f2i(value);
    switch (id) {
    case 100: s.mainNoiseScaleX = value; break;
    case 101: s.mainNoiseScaleY = value; break;
    case 102: s.mainNoiseScaleZ = value; break;
    case 103: s.depthNoiseScaleX = value; break;
    case 104: s.depthNoiseScaleZ = value; break;
    case 105: s.depthNoiseScaleExponent = value; break;
    case 106: s.baseSize = value; break;
    case 107: s.coordinateScale = value; break;
    case 108: s.heightScale = value; break;
    case 109: s.stretchY = value; break;
    case 110: s.upperLimitScale = value; break;
    case 111: s.lowerLimitScale = value; break;
    case 112: s.biomeDepthWeight = value; break;
    case 113: s.biomeDepthOffset = value; break;
    case 114: s.biomeScaleWeight = value; break;
    case 115: s.biomeScaleOffset = value; break;
    case 157: s.dungeonChance = v; break;
    case 158: s.waterLakeChance = v; break;
    case 159: s.lavaLakeChance = v; break;
    case 160: s.seaLevel = v; break;
    case 162: s.fixedBiome = v; break;
    case 163: s.biomeSize = v; break;
    case 164: s.riverSize = v; break;
    case 165: s.dirtSize = v; break;
    case 166: s.dirtCount = v; break;
    case 167: s.dirtMinHeight = v; break;
    case 168: s.dirtMaxHeight = v; break;
    case 169: s.gravelSize = v; break;
    case 170: s.gravelCount = v; break;
    case 171: s.gravelMinHeight = v; break;
    case 172: s.gravelMaxHeight = v; break;
    case 173: s.graniteSize = v; break;
    case 174: s.graniteCount = v; break;
    case 175: s.graniteMinHeight = v; break;
    case 176: s.graniteMaxHeight = v; break;
    case 177: s.dioriteSize = v; break;
    case 178: s.dioriteCount = v; break;
    case 179: s.dioriteMinHeight = v; break;
    case 180: s.dioriteMaxHeight = v; break;
    case 181: s.andesiteSize = v; break;
    case 182: s.andesiteCount = v; break;
    case 183: s.andesiteMinHeight = v; break;
    case 184: s.andesiteMaxHeight = v; break;
    case 185: s.coalSize = v; break;
    case 186: s.coalCount = v; break;
    case 187: s.coalMinHeight = v; break;
    case 189: s.coalMaxHeight = v; break;
    case 190: s.ironSize = v; break;
    case 191: s.ironCount = v; break;
    case 192: s.ironMinHeight = v; break;
    case 193: s.ironMaxHeight = v; break;
    case 194: s.goldSize = v; break;
    case 195: s.goldCount = v; break;
    case 196: s.goldMinHeight = v; break;
    case 197: s.goldMaxHeight = v; break;
    case 198: s.redstoneSize = v; break;
    case 199: s.redstoneCount = v; break;
    case 200: s.redstoneMinHeight = v; break;
    case 201: s.redstoneMaxHeight = v; break;
    case 202: s.diamondSize = v; break;
    case 203: s.diamondCount = v; break;
    case 204: s.diamondMinHeight = v; break;
    case 205: s.diamondMaxHeight = v; break;
    case 206: s.lapisSize = v; break;
    case 207: s.lapisCount = v; break;
    case 208: s.lapisCenterHeight = v; break;
    case 209: s.lapisSpread = v; break;
    default: break;
    }
    if (id >= 100 && id < 116) {
        if (auto* field = static_cast<GuiTextField*>(field_175349_r->getComponent(id - 100 + 132)))
            field->setText(func_175330_b(id, value));
    }
    if (!(field_175336_F == field_175334_E))
        func_181031_a(true);
}

void GuiCustomizeWorldScreen::actionPerformed(GuiButton& button)
{
    if (!button.enabled)
        return;
    switch (button.id) {
    case 300:
        field_175343_i->chunkProviderSettingsJson = field_175336_F.toString();
        mc->displayGuiScreen(field_175343_i);
        break;
    case 301:
        for (int i = 0; i < field_175349_r->getSize(); ++i) {
            GuiPageButtonList::GuiEntry& entry = field_175349_r->getListEntry(i);
            for (Gui* gui : { entry.func_178022_a(), entry.func_178021_b() }) {
                if (auto* slider = dynamic_cast<GuiSlider*>(gui)) {
                    // Java evaluates left to right: the first nextFloat scales, the second offsets.
                    const float scale = 0.75f + random.nextFloat() * 0.5f;
                    const float offset = random.nextFloat() * 0.1f - 0.05f;
                    const float f = slider->getSliderPosition() * scale + offset;
                    slider->setSliderPosition(MathHelper::clamp_float(f, 0.0f, 1.0f));
                } else if (auto* listButton = dynamic_cast<GuiListButton*>(gui)) {
                    listButton->setValue(random.nextBoolean());
                }
            }
        }
        return;
    case 302:
        field_175349_r->previousPage();
        func_175328_i();
        break;
    case 303:
        field_175349_r->nextPage();
        func_175328_i();
        break;
    case 304:
        if (field_175338_A)
            func_175322_b(304);
        break;
    case 305:
        mc->displayGuiScreen(std::make_shared<GuiScreenCustomizePresets>(std::static_pointer_cast<GuiCustomizeWorldScreen>(shared_from_this())));
        break;
    case 306:
        func_175331_h();
        break;
    case 307:
        field_175339_B = 0;
        func_175331_h();
        break;
    default:
        break;
    }
}

void GuiCustomizeWorldScreen::func_175326_g()
{
    field_175336_F.func_177863_a();
    func_175325_f();
    func_181031_a(false);
}

void GuiCustomizeWorldScreen::func_175322_b(int id)
{
    field_175339_B = id;
    func_175329_a(true);
}

void GuiCustomizeWorldScreen::func_175331_h()
{
    switch (field_175339_B) {
    case 300:
        // Java: actionPerformed((GuiListButton) list.func_178061_c(300)) -- there is no control 300, so this would
        // throw; field_175339_B is only ever 304 or 0.
        break;
    case 304:
        func_175326_g();
        break;
    default:
        break;
    }
    field_175339_B = 0;
    field_175340_C = true;
    func_175329_a(false);
}

void GuiCustomizeWorldScreen::func_175329_a(bool confirming)
{
    field_175352_x->visible = confirming;
    field_175351_y->visible = confirming;
    field_175347_t->enabled = !confirming;
    field_175348_s->enabled = !confirming;
    field_175345_v->enabled = !confirming;
    field_175344_w->enabled = !confirming;
    field_175346_u->enabled = field_175338_A && !confirming;
    field_175350_z->enabled = !confirming;
    field_175349_r->setButtonsEnabled(!confirming);
}

void GuiCustomizeWorldScreen::func_175328_i()
{
    field_175345_v->enabled = field_175349_r->getPage() != 0;
    field_175344_w->enabled = field_175349_r->getPage() != field_175349_r->getPageCount() - 1;
    field_175333_f = I18n::format("book.pageIndicator", field_175349_r->getPage() + 1, field_175349_r->getPageCount());
    field_175335_g = field_175342_h[static_cast<size_t>(field_175349_r->getPage())];
    field_175347_t->enabled = field_175349_r->getPage() != field_175349_r->getPageCount() - 1;
}

void GuiCustomizeWorldScreen::keyTyped(char16_t typedChar, int keyCode)
{
    GuiScreen::keyTyped(typedChar, keyCode);
    if (field_175339_B != 0)
        return;
    switch (keyCode) {
    case 200: func_175327_a(1.0f); break;
    case 208: func_175327_a(-1.0f); break;
    default: field_175349_r->onKeyPressed(typedChar, keyCode); break;
    }
}

void GuiCustomizeWorldScreen::func_175327_a(float step)
{
    auto* field = dynamic_cast<GuiTextField*>(field_175349_r->getFocusedControl());
    if (!field)
        return;
    float f = step;
    if (isShiftKeyDown()) {
        f = step * 0.1f;
        if (isCtrlKeyDown())
            f *= 0.1f;
    } else if (isCtrlKeyDown()) {
        f = step * 10.0f;
        if (isAltKeyDown())
            f *= 10.0f;
    }
    if (const auto f1 = java::tryParseFloat(field->getText())) {
        const float value = *f1 + f;
        const int i = field->getId();
        const std::string s = func_175330_b(field->getId(), value);
        field->setText(s);
        onText(i, s);
    }
}

void GuiCustomizeWorldScreen::mouseClicked(int mouseX, int mouseY, int mouseButton)
{
    GuiScreen::mouseClicked(mouseX, mouseY, mouseButton);
    if (field_175339_B == 0 && !field_175340_C)
        field_175349_r->mouseClicked(mouseX, mouseY, mouseButton);
}

void GuiCustomizeWorldScreen::mouseReleased(int mouseX, int mouseY, int state)
{
    GuiScreen::mouseReleased(mouseX, mouseY, state);
    if (field_175340_C)
        field_175340_C = false;
    else if (field_175339_B == 0)
        field_175349_r->mouseReleased(mouseX, mouseY, state);
}

void GuiCustomizeWorldScreen::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawDefaultBackground();
    field_175349_r->drawScreen(mouseX, mouseY, partialTicks);
    drawCenteredString(*fontRendererObj, field_175341_a, width / 2, 2, 16777215);
    drawCenteredString(*fontRendererObj, field_175333_f, width / 2, 12, 16777215);
    drawCenteredString(*fontRendererObj, field_175335_g, width / 2, 22, 16777215);
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
    if (field_175339_B == 0)
        return;
    drawRect(0, 0, width, height, INT_MIN);
    drawHorizontalLine(width / 2 - 91, width / 2 + 90, 99, -2039584);
    drawHorizontalLine(width / 2 - 91, width / 2 + 90, 185, -6250336);
    drawVerticalLine(width / 2 - 91, 99, 185, -2039584);
    drawVerticalLine(width / 2 + 90, 99, 185, -6250336);
    DxStateManager::disableLighting();
    DxStateManager::disableFog();
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    mc->getTextureManager().bindTexture(optionsBackground);
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX_COLOR);
    worldrenderer.pos(static_cast<double>(width / 2 - 90), 185.0, 0.0).tex(0.0, 2.65625).color(64, 64, 64, 64).endVertex();
    worldrenderer.pos(static_cast<double>(width / 2 + 90), 185.0, 0.0).tex(5.625, 2.65625).color(64, 64, 64, 64).endVertex();
    worldrenderer.pos(static_cast<double>(width / 2 + 90), 100.0, 0.0).tex(5.625, 0.0).color(64, 64, 64, 64).endVertex();
    worldrenderer.pos(static_cast<double>(width / 2 - 90), 100.0, 0.0).tex(0.0, 0.0).color(64, 64, 64, 64).endVertex();
    tessellator.draw();
    drawCenteredString(*fontRendererObj, I18n::format("createWorld.customize.custom.confirmTitle"), width / 2, 105, 16777215);
    drawCenteredString(*fontRendererObj, I18n::format("createWorld.customize.custom.confirm1"), width / 2, 125, 16777215);
    drawCenteredString(*fontRendererObj, I18n::format("createWorld.customize.custom.confirm2"), width / 2, 135, 16777215);
    field_175352_x->drawButton(*mc, mouseX, mouseY);
    field_175351_y->drawButton(*mc, mouseX, mouseY);
}

// --- GuiScreenCustomizePresets ---

namespace {

struct PresetInfo {
    std::string field_178955_a; // name
    ResourceLocation field_178953_b; // icon
    ChunkProviderSettings::Factory field_178954_c;
};

// Java: the class initializer, which runs (and translates the names) the first time the screen is opened.
const std::vector<PresetInfo>& presets()
{
    static const std::vector<PresetInfo> list = [] {
        std::vector<PresetInfo> result;
        const auto add = [&](const char* key, const char* icon, const char* json) {
            result.push_back({ I18n::format(key), ResourceLocation(icon), ChunkProviderSettings::Factory::jsonToFactory(json) });
        };
        add("createWorld.customize.custom.preset.waterWorld", "textures/gui/presets/water.png",
            R"({ "coordinateScale":684.412, "heightScale":684.412, "upperLimitScale":512.0, "lowerLimitScale":512.0, "depthNoiseScaleX":200.0, "depthNoiseScaleZ":200.0, "depthNoiseScaleExponent":0.5, "mainNoiseScaleX":5000.0, "mainNoiseScaleY":1000.0, "mainNoiseScaleZ":5000.0, "baseSize":8.5, "stretchY":8.0, "biomeDepthWeight":2.0, "biomeDepthOffset":0.5, "biomeScaleWeight":2.0, "biomeScaleOffset":0.375, "useCaves":true, "useDungeons":true, "dungeonChance":8, "useStrongholds":true, "useVillages":true, "useMineShafts":true, "useTemples":true, "useRavines":true, "useWaterLakes":true, "waterLakeChance":4, "useLavaLakes":true, "lavaLakeChance":80, "useLavaOceans":false, "seaLevel":255 })");
        add("createWorld.customize.custom.preset.isleLand", "textures/gui/presets/isles.png",
            R"({"coordinateScale":3000.0, "heightScale":6000.0, "upperLimitScale":250.0, "lowerLimitScale":512.0, "depthNoiseScaleX":200.0, "depthNoiseScaleZ":200.0, "depthNoiseScaleExponent":0.5, "mainNoiseScaleX":80.0, "mainNoiseScaleY":160.0, "mainNoiseScaleZ":80.0, "baseSize":8.5, "stretchY":10.0, "biomeDepthWeight":1.0, "biomeDepthOffset":0.0, "biomeScaleWeight":1.0, "biomeScaleOffset":0.0, "useCaves":true, "useDungeons":true, "dungeonChance":8, "useStrongholds":true, "useVillages":true, "useMineShafts":true, "useTemples":true, "useRavines":true, "useWaterLakes":true, "waterLakeChance":4, "useLavaLakes":true, "lavaLakeChance":80, "useLavaOceans":false, "seaLevel":63 })");
        add("createWorld.customize.custom.preset.caveDelight", "textures/gui/presets/delight.png",
            R"({"coordinateScale":684.412, "heightScale":684.412, "upperLimitScale":512.0, "lowerLimitScale":512.0, "depthNoiseScaleX":200.0, "depthNoiseScaleZ":200.0, "depthNoiseScaleExponent":0.5, "mainNoiseScaleX":5000.0, "mainNoiseScaleY":1000.0, "mainNoiseScaleZ":5000.0, "baseSize":8.5, "stretchY":5.0, "biomeDepthWeight":2.0, "biomeDepthOffset":1.0, "biomeScaleWeight":4.0, "biomeScaleOffset":1.0, "useCaves":true, "useDungeons":true, "dungeonChance":8, "useStrongholds":true, "useVillages":true, "useMineShafts":true, "useTemples":true, "useRavines":true, "useWaterLakes":true, "waterLakeChance":4, "useLavaLakes":true, "lavaLakeChance":80, "useLavaOceans":false, "seaLevel":63 })");
        add("createWorld.customize.custom.preset.mountains", "textures/gui/presets/madness.png",
            R"({"coordinateScale":738.41864, "heightScale":157.69133, "upperLimitScale":801.4267, "lowerLimitScale":1254.1643, "depthNoiseScaleX":374.93652, "depthNoiseScaleZ":288.65228, "depthNoiseScaleExponent":1.2092624, "mainNoiseScaleX":1355.9908, "mainNoiseScaleY":745.5343, "mainNoiseScaleZ":1183.464, "baseSize":1.8758626, "stretchY":1.7137525, "biomeDepthWeight":1.7553768, "biomeDepthOffset":3.4701107, "biomeScaleWeight":1.0, "biomeScaleOffset":2.535211, "useCaves":true, "useDungeons":true, "dungeonChance":8, "useStrongholds":true, "useVillages":true, "useMineShafts":true, "useTemples":true, "useRavines":true, "useWaterLakes":true, "waterLakeChance":4, "useLavaLakes":true, "lavaLakeChance":80, "useLavaOceans":false, "seaLevel":63 })");
        add("createWorld.customize.custom.preset.drought", "textures/gui/presets/drought.png",
            R"({"coordinateScale":684.412, "heightScale":684.412, "upperLimitScale":512.0, "lowerLimitScale":512.0, "depthNoiseScaleX":200.0, "depthNoiseScaleZ":200.0, "depthNoiseScaleExponent":0.5, "mainNoiseScaleX":1000.0, "mainNoiseScaleY":3000.0, "mainNoiseScaleZ":1000.0, "baseSize":8.5, "stretchY":10.0, "biomeDepthWeight":1.0, "biomeDepthOffset":0.0, "biomeScaleWeight":1.0, "biomeScaleOffset":0.0, "useCaves":true, "useDungeons":true, "dungeonChance":8, "useStrongholds":true, "useVillages":true, "useMineShafts":true, "useTemples":true, "useRavines":true, "useWaterLakes":true, "waterLakeChance":4, "useLavaLakes":true, "lavaLakeChance":80, "useLavaOceans":false, "seaLevel":20 })");
        add("createWorld.customize.custom.preset.caveChaos", "textures/gui/presets/chaos.png",
            R"({"coordinateScale":684.412, "heightScale":684.412, "upperLimitScale":2.0, "lowerLimitScale":64.0, "depthNoiseScaleX":200.0, "depthNoiseScaleZ":200.0, "depthNoiseScaleExponent":0.5, "mainNoiseScaleX":80.0, "mainNoiseScaleY":160.0, "mainNoiseScaleZ":80.0, "baseSize":8.5, "stretchY":12.0, "biomeDepthWeight":1.0, "biomeDepthOffset":0.0, "biomeScaleWeight":1.0, "biomeScaleOffset":0.0, "useCaves":true, "useDungeons":true, "dungeonChance":8, "useStrongholds":true, "useVillages":true, "useMineShafts":true, "useTemples":true, "useRavines":true, "useWaterLakes":true, "waterLakeChance":4, "useLavaLakes":true, "lavaLakeChance":80, "useLavaOceans":false, "seaLevel":6 })");
        add("createWorld.customize.custom.preset.goodLuck", "textures/gui/presets/luck.png",
            R"({"coordinateScale":684.412, "heightScale":684.412, "upperLimitScale":512.0, "lowerLimitScale":512.0, "depthNoiseScaleX":200.0, "depthNoiseScaleZ":200.0, "depthNoiseScaleExponent":0.5, "mainNoiseScaleX":80.0, "mainNoiseScaleY":160.0, "mainNoiseScaleZ":80.0, "baseSize":8.5, "stretchY":12.0, "biomeDepthWeight":1.0, "biomeDepthOffset":0.0, "biomeScaleWeight":1.0, "biomeScaleOffset":0.0, "useCaves":true, "useDungeons":true, "dungeonChance":8, "useStrongholds":true, "useVillages":true, "useMineShafts":true, "useTemples":true, "useRavines":true, "useWaterLakes":true, "waterLakeChance":4, "useLavaLakes":true, "lavaLakeChance":80, "useLavaOceans":true, "seaLevel":40 })");
        return result;
    }();
    return list;
}

} // namespace

GuiScreenCustomizePresets::GuiScreenCustomizePresets(std::shared_ptr<GuiCustomizeWorldScreen> parent)
    : field_175314_r(std::move(parent))
{
    presets(); // the Java class initializer
}

GuiScreenCustomizePresets::~GuiScreenCustomizePresets() = default;

void GuiScreenCustomizePresets::initGui()
{
    buttonList.clear();
    Keyboard::enableRepeatEvents(true);
    field_175315_a = I18n::format("createWorld.customize.custom.presets.title");
    field_175313_s = I18n::format("createWorld.customize.presets.share");
    field_175312_t = I18n::format("createWorld.customize.presets.list");
    field_175317_i = std::make_unique<GuiTextField>(2, *fontRendererObj, 50, 40, width - 100, 20);
    field_175311_g = std::make_shared<ListPreset>(*this, *mc);
    field_175317_i->setMaxStringLength(2000);
    field_175317_i->setText(field_175314_r->getSettingsJson());
    field_175316_h = &addButton<GuiButton>(0, width / 2 - 102, height - 27, 100, 20, I18n::format("createWorld.customize.presets.select"));
    addButton<GuiButton>(1, width / 2 + 3, height - 27, 100, 20, I18n::format("gui.cancel"));
    func_175304_a();
}

void GuiScreenCustomizePresets::handleMouseInput()
{
    GuiScreen::handleMouseInput();
    const std::shared_ptr<ListPreset> list = field_175311_g;
    list->handleMouseInput();
}

void GuiScreenCustomizePresets::onGuiClosed()
{
    Keyboard::enableRepeatEvents(false);
}

void GuiScreenCustomizePresets::mouseClicked(int mouseX, int mouseY, int mouseButton)
{
    field_175317_i->mouseClicked(mouseX, mouseY, mouseButton);
    GuiScreen::mouseClicked(mouseX, mouseY, mouseButton);
}

void GuiScreenCustomizePresets::keyTyped(char16_t typedChar, int keyCode)
{
    if (!field_175317_i->textboxKeyTyped(typedChar, keyCode))
        GuiScreen::keyTyped(typedChar, keyCode);
}

void GuiScreenCustomizePresets::actionPerformed(GuiButton& button)
{
    switch (button.id) {
    case 0:
        field_175314_r->loadSettingsJson(field_175317_i->getText());
        mc->displayGuiScreen(field_175314_r);
        break;
    case 1:
        mc->displayGuiScreen(field_175314_r);
        break;
    default:
        break;
    }
}

void GuiScreenCustomizePresets::drawScreen(int mouseX, int mouseY, float partialTicks)
{
    drawDefaultBackground();
    field_175311_g->drawScreen(mouseX, mouseY, partialTicks);
    drawCenteredString(*fontRendererObj, field_175315_a, width / 2, 8, 16777215);
    drawString(*fontRendererObj, field_175313_s, 50, 30, 10526880);
    drawString(*fontRendererObj, field_175312_t, 50, 70, 10526880);
    field_175317_i->drawTextBox();
    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}

void GuiScreenCustomizePresets::updateScreen()
{
    field_175317_i->updateCursorCounter();
    GuiScreen::updateScreen();
}

void GuiScreenCustomizePresets::func_175304_a()
{
    field_175316_h->enabled = func_175305_g();
}

bool GuiScreenCustomizePresets::func_175305_g() const
{
    return (field_175311_g->field_178053_u > -1 && field_175311_g->field_178053_u < static_cast<int>(presets().size()))
        || field_175317_i->getText16().size() > 1;
}

GuiScreenCustomizePresets::ListPreset::ListPreset(GuiScreenCustomizePresets& screenIn, Minecraft& mcIn)
    : GuiSlot(mcIn, screenIn.width, screenIn.height, 80, screenIn.height - 32, 38)
    , screen(screenIn)
{
}

int GuiScreenCustomizePresets::ListPreset::getSize() const
{
    return static_cast<int>(presets().size());
}

void GuiScreenCustomizePresets::ListPreset::elementClicked(int slotIndex, bool, int, int)
{
    field_178053_u = slotIndex;
    screen.func_175304_a();
    screen.field_175317_i->setText(presets()[static_cast<size_t>(screen.field_175311_g->field_178053_u)].field_178954_c.toString());
}

void GuiScreenCustomizePresets::ListPreset::func_178051_a(int x, int y, const ResourceLocation& texture)
{
    const int i = x + 5;
    screen.drawHorizontalLine(i - 1, i + 32, y - 1, -2039584);
    screen.drawHorizontalLine(i - 1, i + 32, y + 32, -6250336);
    screen.drawVerticalLine(i - 1, y - 1, y + 32, -2039584);
    screen.drawVerticalLine(i + 32, y - 1, y + 32, -6250336);
    DxStateManager::color(1.0f, 1.0f, 1.0f, 1.0f);
    mc.getTextureManager().bindTexture(texture);
    Tessellator& tessellator = Tessellator::getInstance();
    WorldRenderer& worldrenderer = tessellator.getWorldRenderer();
    worldrenderer.begin(7, DefaultVertexFormats::POSITION_TEX);
    worldrenderer.pos(static_cast<double>(i + 0), static_cast<double>(y + 32), 0.0).tex(0.0, 1.0).endVertex();
    worldrenderer.pos(static_cast<double>(i + 32), static_cast<double>(y + 32), 0.0).tex(1.0, 1.0).endVertex();
    worldrenderer.pos(static_cast<double>(i + 32), static_cast<double>(y + 0), 0.0).tex(1.0, 0.0).endVertex();
    worldrenderer.pos(static_cast<double>(i + 0), static_cast<double>(y + 0), 0.0).tex(0.0, 0.0).endVertex();
    tessellator.draw();
}

void GuiScreenCustomizePresets::ListPreset::drawSlot(int entryID, int insideLeft, int yPos, int, int, int)
{
    const PresetInfo& info = presets()[static_cast<size_t>(entryID)];
    func_178051_a(insideLeft, yPos, info.field_178953_b);
    screen.fontRendererObj->drawString(info.field_178955_a, insideLeft + 32 + 10, yPos + 14, 16777215);
}

} // namespace mc
