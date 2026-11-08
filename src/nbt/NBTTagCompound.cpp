#include "nbt/NBTTagCompound.h"

#include "core/Java.h"
#include "core/JavaString.h"
#include "core/Log.h"

namespace mc {

namespace {

std::shared_ptr<NBTBase> createChecked(int8_t id)
{
    auto tag = NBTBase::createNewByType(id);
    if (tag == nullptr)
        throw std::runtime_error("java.lang.NullPointerException: unknown NBT tag type " + std::to_string(id));
    return tag;
}

} // namespace

// --- NBTTagCompound ---

void NBTTagCompound::write(DataOutput& output) const
{
    for (const auto& [key, tag] : tagMap) {
        output.writeByte(tag->getId());
        if (tag->getId() != 0) {
            output.writeUTF(key);
            tag->write(output);
        }
    }
    output.writeByte(0);
}

void NBTTagCompound::read(DataInput& input, int depth, NBTSizeTracker& sizeTracker)
{
    sizeTracker.read(384);
    if (depth > 512)
        throw std::runtime_error("Tried to read NBT tag with too high complexity, depth > 512");

    tagMap.clear();
    int8_t b0;
    while ((b0 = input.readByte()) != 0) {
        const std::string s = input.readUTF();
        sizeTracker.read(java::add(224, java::mul(16, static_cast<int32_t>(java::utf16Length(s)))));
        auto nbtbase = readNBT(b0, s, input, depth + 1, sizeTracker);
        if (!tagMap.put(s, std::move(nbtbase)))
            sizeTracker.read(288);
    }
}

std::shared_ptr<NBTBase> NBTTagCompound::readNBT(int8_t id, const std::string& key, DataInput& input, int depth, NBTSizeTracker& sizeTracker)
{
    auto nbtbase = createChecked(id);
    try {
        nbtbase->read(input, depth, sizeTracker);
    } catch (const IOException& e) {
        // Java wraps it in a crash report "Loading NBT data" naming the tag.
        throw IOException(std::string(e.what()) + " (loading NBT tag '" + key + "' of type " + std::to_string(id) + ")");
    }
    return nbtbase;
}

std::vector<std::string> NBTTagCompound::getKeySet() const
{
    std::vector<std::string> keys;
    keys.reserve(tagMap.size());
    for (const auto& [key, tag] : tagMap)
        keys.push_back(key);
    return keys;
}

void NBTTagCompound::setTag(const std::string& key, std::shared_ptr<NBTBase> value)
{
    tagMap.put(key, std::move(value));
}

void NBTTagCompound::setByte(const std::string& key, int8_t value)
{
    tagMap.put(key, std::make_shared<NBTTagByte>(value));
}

void NBTTagCompound::setShort(const std::string& key, int16_t value)
{
    tagMap.put(key, std::make_shared<NBTTagShort>(value));
}

void NBTTagCompound::setInteger(const std::string& key, int32_t value)
{
    tagMap.put(key, std::make_shared<NBTTagInt>(value));
}

void NBTTagCompound::setLong(const std::string& key, int64_t value)
{
    tagMap.put(key, std::make_shared<NBTTagLong>(value));
}

void NBTTagCompound::setFloat(const std::string& key, float value)
{
    tagMap.put(key, std::make_shared<NBTTagFloat>(value));
}

void NBTTagCompound::setDouble(const std::string& key, double value)
{
    tagMap.put(key, std::make_shared<NBTTagDouble>(value));
}

void NBTTagCompound::setString(const std::string& key, const std::string& value)
{
    tagMap.put(key, std::make_shared<NBTTagString>(value));
}

void NBTTagCompound::setByteArray(const std::string& key, std::vector<uint8_t> value)
{
    tagMap.put(key, std::make_shared<NBTTagByteArray>(std::move(value)));
}

void NBTTagCompound::setIntArray(const std::string& key, std::vector<int32_t> value)
{
    tagMap.put(key, std::make_shared<NBTTagIntArray>(std::move(value)));
}

std::shared_ptr<NBTBase> NBTTagCompound::getTag(const std::string& key) const
{
    const auto* tag = tagMap.find(key);
    return tag ? *tag : nullptr;
}

int8_t NBTTagCompound::getTagId(const std::string& key) const
{
    const auto* tag = tagMap.find(key);
    return tag ? (*tag)->getId() : 0;
}

bool NBTTagCompound::hasKey(const std::string& key, int type) const
{
    const int i = getTagId(key);
    if (i == type)
        return true;
    if (type != 99)
        return false;
    return i == 1 || i == 2 || i == 3 || i == 4 || i == 5 || i == 6;
}

int8_t NBTTagCompound::getByte(const std::string& key) const
{
    return hasKey(key, 99) ? static_cast<const NBTPrimitive&>(**tagMap.find(key)).getByte() : 0;
}

int16_t NBTTagCompound::getShort(const std::string& key) const
{
    return hasKey(key, 99) ? static_cast<const NBTPrimitive&>(**tagMap.find(key)).getShort() : 0;
}

int32_t NBTTagCompound::getInteger(const std::string& key) const
{
    return hasKey(key, 99) ? static_cast<const NBTPrimitive&>(**tagMap.find(key)).getInt() : 0;
}

int64_t NBTTagCompound::getLong(const std::string& key) const
{
    return hasKey(key, 99) ? static_cast<const NBTPrimitive&>(**tagMap.find(key)).getLong() : 0;
}

float NBTTagCompound::getFloat(const std::string& key) const
{
    return hasKey(key, 99) ? static_cast<const NBTPrimitive&>(**tagMap.find(key)).getFloat() : 0.0f;
}

double NBTTagCompound::getDouble(const std::string& key) const
{
    return hasKey(key, 99) ? static_cast<const NBTPrimitive&>(**tagMap.find(key)).getDouble() : 0.0;
}

std::string NBTTagCompound::getString(const std::string& key) const
{
    return hasKey(key, 8) ? (*tagMap.find(key))->getString() : "";
}

std::vector<uint8_t> NBTTagCompound::getByteArray(const std::string& key) const
{
    return hasKey(key, 7) ? static_cast<const NBTTagByteArray&>(**tagMap.find(key)).getByteArray() : std::vector<uint8_t> {};
}

std::vector<int32_t> NBTTagCompound::getIntArray(const std::string& key) const
{
    return hasKey(key, 11) ? static_cast<const NBTTagIntArray&>(**tagMap.find(key)).getIntArray() : std::vector<int32_t> {};
}

std::shared_ptr<NBTTagCompound> NBTTagCompound::getCompoundTag(const std::string& key) const
{
    return hasKey(key, 10) ? std::static_pointer_cast<NBTTagCompound>(*tagMap.find(key)) : std::make_shared<NBTTagCompound>();
}

std::shared_ptr<NBTTagList> NBTTagCompound::getTagList(const std::string& key, int type) const
{
    if (getTagId(key) != 9)
        return std::make_shared<NBTTagList>();
    auto list = std::static_pointer_cast<NBTTagList>(*tagMap.find(key));
    return list->tagCount() > 0 && list->getTagType() != type ? std::make_shared<NBTTagList>() : list;
}

std::string NBTTagCompound::toString() const
{
    std::string s = "{";
    for (const auto& [key, tag] : tagMap) {
        if (s.size() != 1)
            s += ',';
        s += key + ':' + tag->toString();
    }
    return s + '}';
}

std::shared_ptr<NBTBase> NBTTagCompound::copy() const
{
    auto compound = std::make_shared<NBTTagCompound>();
    for (const auto& [key, tag] : tagMap)
        compound->setTag(key, tag->copy());
    return compound;
}

bool NBTTagCompound::equals(const NBTBase& other) const
{
    if (!NBTBase::equals(other))
        return false;
    const auto& compound = static_cast<const NBTTagCompound&>(other);
    if (compound.tagMap.size() != tagMap.size())
        return false;
    for (const auto& [key, tag] : tagMap) {
        const auto* found = compound.tagMap.find(key);
        if (found == nullptr || !tag->equals(**found))
            return false;
    }
    return true;
}

int32_t NBTTagCompound::hashCode() const
{
    int32_t mapHash = 0;
    for (const auto& [key, tag] : tagMap)
        mapHash = java::add(mapHash, java::hashCode(key) ^ tag->hashCode());
    return NBTBase::hashCode() ^ mapHash;
}

void NBTTagCompound::merge(const NBTTagCompound& other)
{
    for (const auto& [key, tag] : other.tagMap) {
        if (tag->getId() == 10 && hasKey(key, 10))
            getCompoundTag(key)->merge(static_cast<const NBTTagCompound&>(*tag));
        else
            setTag(key, tag->copy());
    }
}

// --- NBTTagList ---

void NBTTagList::write(DataOutput& output) const
{
    tagType = tagList.empty() ? 0 : tagList.front()->getId();
    output.writeByte(tagType);
    output.writeInt(static_cast<int32_t>(tagList.size()));
    for (const auto& tag : tagList)
        tag->write(output);
}

void NBTTagList::read(DataInput& input, int depth, NBTSizeTracker& sizeTracker)
{
    sizeTracker.read(296);
    if (depth > 512)
        throw std::runtime_error("Tried to read NBT tag with too high complexity, depth > 512");

    tagType = input.readByte();
    const int32_t i = input.readInt();
    if (tagType == 0 && i > 0)
        throw std::runtime_error("Missing type on ListTag");
    sizeTracker.read(32LL * i);
    if (i < 0)
        throw std::runtime_error("java.lang.IllegalArgumentException: Illegal Capacity: " + std::to_string(i));
    tagList.clear();
    for (int32_t j = 0; j < i; ++j) {
        auto nbtbase = createChecked(tagType);
        nbtbase->read(input, depth + 1, sizeTracker);
        tagList.push_back(std::move(nbtbase));
    }
}

std::string NBTTagList::toString() const
{
    std::string s = "[";
    for (size_t i = 0; i < tagList.size(); ++i) {
        if (i != 0)
            s += ',';
        s += std::to_string(i) + ':' + tagList[i]->toString();
    }
    return s + ']';
}

void NBTTagList::appendTag(std::shared_ptr<NBTBase> nbt)
{
    if (nbt->getId() == 0) {
        Log::warn("Invalid TagEnd added to ListTag");
        return;
    }
    if (tagType == 0) {
        tagType = nbt->getId();
    } else if (tagType != nbt->getId()) {
        Log::warn("Adding mismatching tag types to tag list");
        return;
    }
    tagList.push_back(std::move(nbt));
}

void NBTTagList::set(int idx, std::shared_ptr<NBTBase> nbt)
{
    if (nbt->getId() == 0) {
        Log::warn("Invalid TagEnd added to ListTag");
    } else if (idx >= 0 && idx < tagCount()) {
        if (tagType == 0) {
            tagType = nbt->getId();
        } else if (tagType != nbt->getId()) {
            Log::warn("Adding mismatching tag types to tag list");
            return;
        }
        tagList[static_cast<size_t>(idx)] = std::move(nbt);
    } else {
        Log::warn("index out of bounds to set tag in tag list");
    }
}

std::shared_ptr<NBTBase> NBTTagList::removeTag(int i)
{
    if (i < 0 || i >= tagCount())
        throw std::out_of_range("Index: " + std::to_string(i) + ", Size: " + std::to_string(tagCount()));
    auto tag = tagList[static_cast<size_t>(i)];
    tagList.erase(tagList.begin() + i);
    return tag;
}

std::shared_ptr<NBTTagCompound> NBTTagList::getCompoundTagAt(int i) const
{
    if (i >= 0 && i < tagCount() && tagList[static_cast<size_t>(i)]->getId() == 10)
        return std::static_pointer_cast<NBTTagCompound>(tagList[static_cast<size_t>(i)]);
    return std::make_shared<NBTTagCompound>();
}

std::vector<int32_t> NBTTagList::getIntArrayAt(int i) const
{
    if (i >= 0 && i < tagCount() && tagList[static_cast<size_t>(i)]->getId() == 11)
        return static_cast<const NBTTagIntArray&>(*tagList[static_cast<size_t>(i)]).getIntArray();
    return {};
}

double NBTTagList::getDoubleAt(int i) const
{
    if (i >= 0 && i < tagCount() && tagList[static_cast<size_t>(i)]->getId() == 6)
        return static_cast<const NBTTagDouble&>(*tagList[static_cast<size_t>(i)]).getDouble();
    return 0.0;
}

float NBTTagList::getFloatAt(int i) const
{
    if (i >= 0 && i < tagCount() && tagList[static_cast<size_t>(i)]->getId() == 5)
        return static_cast<const NBTTagFloat&>(*tagList[static_cast<size_t>(i)]).getFloat();
    return 0.0f;
}

std::string NBTTagList::getStringTagAt(int i) const
{
    if (i >= 0 && i < tagCount()) {
        const NBTBase& tag = *tagList[static_cast<size_t>(i)];
        return tag.getId() == 8 ? tag.getString() : tag.toString();
    }
    return "";
}

std::shared_ptr<NBTBase> NBTTagList::get(int idx) const
{
    return idx >= 0 && idx < tagCount() ? tagList[static_cast<size_t>(idx)] : std::make_shared<NBTTagEnd>();
}

std::shared_ptr<NBTBase> NBTTagList::copy() const
{
    auto list = std::make_shared<NBTTagList>();
    list->tagType = tagType;
    for (const auto& tag : tagList)
        list->tagList.push_back(tag->copy());
    return list;
}

bool NBTTagList::equals(const NBTBase& other) const
{
    if (!NBTBase::equals(other))
        return false;
    const auto& list = static_cast<const NBTTagList&>(other);
    if (tagType != list.tagType || tagList.size() != list.tagList.size())
        return false;
    for (size_t i = 0; i < tagList.size(); ++i) {
        if (!tagList[i]->equals(*list.tagList[i]))
            return false;
    }
    return true;
}

int32_t NBTTagList::hashCode() const
{
    int32_t listHash = 1;
    for (const auto& tag : tagList)
        listHash = java::add(java::mul(31, listHash), tag->hashCode());
    return NBTBase::hashCode() ^ listHash;
}

} // namespace mc
