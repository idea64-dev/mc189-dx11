#include "client/gui/GuiPageButtonList.h"

#include "client/Minecraft.h"
#include "client/gui/GuiLabel.h"
#include "client/gui/GuiResponder.h"
#include "client/gui/GuiScreen.h"
#include "client/gui/GuiTextField.h"

#include <algorithm>

namespace mc {

namespace {

// String.split(";"): trailing empty strings are dropped, so a string of only separators gives nothing, while
// the empty string gives one empty string.
std::vector<std::string> splitSemicolons(const std::string& s)
{
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        const size_t end = s.find(';', start);
        parts.push_back(s.substr(start, end == std::string::npos ? std::string::npos : end - start));
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
    if (s.empty())
        return parts;
    while (!parts.empty() && parts.back().empty())
        parts.pop_back();
    return parts;
}

} // namespace

GuiPageButtonList::GuiPageButtonList(Minecraft& mcIn, int widthIn, int heightIn, int topIn, int bottomIn, int slotHeightIn, GuiResponder& responder,
    std::vector<Page> pages)
    : GuiListExtended(mcIn, widthIn, heightIn, topIn, bottomIn, slotHeightIn)
    , field_178078_x(std::move(pages))
    , field_178076_z(responder)
{
    centerListVertically = false;
    func_178069_s();
    func_178055_t();
}

GuiPageButtonList::~GuiPageButtonList() = default;

void GuiPageButtonList::func_178069_s()
{
    for (const Page& page : field_178078_x) {
        for (size_t i = 0; i < page.size(); i += 2) {
            const std::optional<GuiListEntry>& entry = page[i];
            const std::optional<GuiListEntry> none;
            const std::optional<GuiListEntry>& entry1 = i < page.size() - 1 ? page[i + 1] : none;
            Gui* gui = createControl(entry, 0, !entry1.has_value());
            Gui* gui1 = createControl(entry1, 160, !entry.has_value());
            field_178074_u.push_back(std::make_unique<GuiEntry>(mc, gui, gui1));
            if (entry && gui) {
                field_178073_v[entry->id] = gui;
                if (auto* field = dynamic_cast<GuiTextField*>(gui))
                    field_178072_w.push_back(field);
            }
            if (entry1 && gui1) {
                field_178073_v[entry1->id] = gui1;
                if (auto* field = dynamic_cast<GuiTextField*>(gui1))
                    field_178072_w.push_back(field);
            }
        }
    }
}

void GuiPageButtonList::func_178055_t()
{
    field_178074_u.clear();
    const Page& page = field_178078_x[static_cast<size_t>(field_178077_y)];
    for (size_t i = 0; i < page.size(); i += 2) {
        const std::optional<GuiListEntry>& entry = page[i];
        const bool hasRight = i < page.size() - 1 && page[i + 1].has_value();
        // Java looks the left entry up unconditionally (a null left entry would throw).
        Gui* gui = getComponent(entry.value().id);
        Gui* gui1 = hasRight ? getComponent(page[i + 1]->id) : nullptr;
        field_178074_u.push_back(std::make_unique<GuiEntry>(mc, gui, gui1));
    }
}

void GuiPageButtonList::setPage(int page)
{
    if (page == field_178077_y)
        return;
    const int i = field_178077_y;
    field_178077_y = page;
    func_178055_t();
    func_178060_e(i, page);
    amountScrolled = 0.0f;
}

void GuiPageButtonList::previousPage()
{
    if (field_178077_y > 0)
        setPage(field_178077_y - 1);
}

void GuiPageButtonList::nextPage()
{
    if (field_178077_y < static_cast<int>(field_178078_x.size()) - 1)
        setPage(field_178077_y + 1);
}

Gui* GuiPageButtonList::getComponent(int id) const
{
    const auto it = field_178073_v.find(id);
    return it == field_178073_v.end() ? nullptr : it->second;
}

void GuiPageButtonList::func_178060_e(int oldPage, int newPage)
{
    for (const auto& entry : field_178078_x[static_cast<size_t>(oldPage)]) {
        if (entry)
            setVisible(getComponent(entry->id), false);
    }
    for (const auto& entry : field_178078_x[static_cast<size_t>(newPage)]) {
        if (entry)
            setVisible(getComponent(entry->id), true);
    }
}

void GuiPageButtonList::setVisible(Gui* gui, bool visible)
{
    if (auto* button = dynamic_cast<GuiButton*>(gui))
        button->visible = visible;
    else if (auto* field = dynamic_cast<GuiTextField*>(gui))
        field->setVisible(visible);
    else if (auto* label = dynamic_cast<GuiLabel*>(gui))
        label->visible = visible;
}

Gui* GuiPageButtonList::createControl(const std::optional<GuiListEntry>& entry, int xOffset, bool alone)
{
    if (!entry)
        return nullptr;
    const int x = width / 2 - 155 + xOffset;
    std::unique_ptr<Gui> control;
    switch (entry->kind) {
    case GuiListEntry::Kind::SLIDER: {
        auto slider = std::make_unique<GuiSlider>(field_178076_z, entry->id, x, 0, entry->text, entry->min, entry->max, entry->value, entry->format);
        slider->visible = entry->visible;
        control = std::move(slider);
        break;
    }
    case GuiListEntry::Kind::BUTTON: {
        auto button = std::make_unique<GuiListButton>(field_178076_z, entry->id, x, 0, entry->text, entry->buttonValue);
        button->visible = entry->visible;
        control = std::move(button);
        break;
    }
    case GuiListEntry::Kind::EDIT_BOX: {
        auto field = std::make_unique<GuiTextField>(entry->id, *mc.fontRendererObj, x, 0, 150, 20);
        field->setText(entry->text);
        field->setGuiResponder(&field_178076_z);
        field->setVisible(entry->visible);
        field->setValidator(entry->validator);
        control = std::move(field);
        break;
    }
    case GuiListEntry::Kind::LABEL: {
        std::unique_ptr<GuiLabel> label;
        if (alone)
            label = std::make_unique<GuiLabel>(*mc.fontRendererObj, entry->id, x, 0, width - x * 2, 20, -1);
        else
            label = std::make_unique<GuiLabel>(*mc.fontRendererObj, entry->id, x, 0, 150, 20, -1);
        label->visible = entry->visible;
        label->addLine(entry->text);
        label->setCentered();
        control = std::move(label);
        break;
    }
    }
    controls.push_back(std::move(control));
    return controls.back().get();
}

void GuiPageButtonList::setButtonsEnabled(bool enable)
{
    for (const auto& entry : field_178074_u) {
        if (auto* button = dynamic_cast<GuiButton*>(entry->func_178022_a()))
            button->enabled = enable;
        if (auto* button = dynamic_cast<GuiButton*>(entry->func_178021_b()))
            button->enabled = enable;
    }
}

bool GuiPageButtonList::mouseClicked(int mx, int my, int mouseEvent)
{
    const bool flag = GuiListExtended::mouseClicked(mx, my, mouseEvent);
    const int i = getSlotIndexFromScreenCoords(mx, my);
    if (i >= 0) {
        GuiEntry& entry = getListEntry(i);
        if (field_178075_A != entry.field_178028_d && field_178075_A) {
            if (auto* field = dynamic_cast<GuiTextField*>(field_178075_A))
                field->setFocused(false);
        }
        field_178075_A = entry.field_178028_d;
    }
    return flag;
}

void GuiPageButtonList::onKeyPressed(char16_t typedChar, int keyCode)
{
    auto* guitextfield = dynamic_cast<GuiTextField*>(field_178075_A);
    if (!guitextfield)
        return;
    if (!GuiScreen::isKeyComboCtrlV(keyCode)) {
        if (keyCode == 15) { // tab
            guitextfield->setFocused(false);
            int k = static_cast<int>(std::find(field_178072_w.begin(), field_178072_w.end(), guitextfield) - field_178072_w.begin());
            const int size = static_cast<int>(field_178072_w.size());
            if (GuiScreen::isShiftKeyDown()) {
                if (k == 0)
                    k = size - 1;
                else
                    --k;
            } else if (k == size - 1) {
                k = 0;
            } else {
                ++k;
            }
            field_178075_A = field_178072_w[static_cast<size_t>(k)];
            guitextfield = field_178072_w[static_cast<size_t>(k)];
            guitextfield->setFocused(true);
            const int l = guitextfield->yPosition + slotHeight;
            const int i1 = guitextfield->yPosition;
            if (l > bottom)
                amountScrolled += static_cast<float>(l - bottom);
            else if (i1 < top)
                amountScrolled = static_cast<float>(i1);
        } else {
            guitextfield->textboxKeyTyped(typedChar, keyCode);
        }
        return;
    }
    const std::string s = GuiScreen::getClipboardString();
    const std::vector<std::string> astring = splitSemicolons(s);
    const int i = static_cast<int>(std::find(field_178072_w.begin(), field_178072_w.end(), guitextfield) - field_178072_w.begin());
    int j = i;
    for (const std::string& s1 : astring) {
        field_178072_w[static_cast<size_t>(j)]->setText(s1);
        if (j == static_cast<int>(field_178072_w.size()) - 1)
            j = 0;
        else
            ++j;
        if (j == i)
            break;
    }
}

// --- GuiEntry ---

void GuiPageButtonList::GuiEntry::drawEntry(int, int, int y, int, int, int mx, int my, bool)
{
    draw(field_178029_b, y, mx, my, false);
    draw(field_178030_c, y, mx, my, false);
}

void GuiPageButtonList::GuiEntry::draw(Gui* gui, int y, int mx, int my, bool positionOnly)
{
    if (!gui)
        return;
    if (auto* button = dynamic_cast<GuiButton*>(gui)) {
        button->yPosition = y;
        if (!positionOnly)
            button->drawButton(mc, mx, my);
    } else if (auto* field = dynamic_cast<GuiTextField*>(gui)) {
        field->yPosition = y;
        if (!positionOnly)
            field->drawTextBox();
    } else if (auto* label = dynamic_cast<GuiLabel*>(gui)) {
        label->field_146174_h = y;
        if (!positionOnly)
            label->drawLabel(mc, mx, my);
    }
}

void GuiPageButtonList::GuiEntry::setSelected(int, int, int yPos)
{
    draw(field_178029_b, yPos, 0, 0, true);
    draw(field_178030_c, yPos, 0, 0, true);
}

bool GuiPageButtonList::GuiEntry::mousePressed(int, int x, int y, int mouseEvent, int, int)
{
    const bool flag = press(field_178029_b, x, y, mouseEvent);
    const bool flag1 = press(field_178030_c, x, y, mouseEvent);
    return flag || flag1;
}

bool GuiPageButtonList::GuiEntry::press(Gui* gui, int x, int y, int mouseEvent)
{
    if (!gui)
        return false;
    if (auto* button = dynamic_cast<GuiButton*>(gui)) {
        const bool flag = button->mousePressed(mc, x, y);
        if (flag)
            field_178028_d = button;
        return flag;
    }
    if (auto* field = dynamic_cast<GuiTextField*>(gui)) {
        field->mouseClicked(x, y, mouseEvent);
        if (field->isFocused())
            field_178028_d = field;
    }
    return false;
}

void GuiPageButtonList::GuiEntry::mouseReleased(int, int x, int y, int, int, int)
{
    if (auto* button = dynamic_cast<GuiButton*>(field_178029_b))
        button->mouseReleased(x, y);
    if (auto* button = dynamic_cast<GuiButton*>(field_178030_c))
        button->mouseReleased(x, y);
}

} // namespace mc
