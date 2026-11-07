#include "client/audio/SoundHandler.h"

#include "client/resources/IResourcePack.h"
#include "client/settings/GameSettings.h"
#include "core/Log.h"
#include "util/MathHelper.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <stdexcept>

namespace mc {

// --- Sound records and accessors ---

std::shared_ptr<PositionedSoundRecord> PositionedSoundRecord::create(ResourceLocation soundResource, float pitch)
{
    return std::make_shared<PositionedSoundRecord>(std::move(soundResource), 0.25f, pitch, false, 0, AttenuationType::NONE, 0.0f, 0.0f, 0.0f);
}

std::shared_ptr<PositionedSoundRecord> PositionedSoundRecord::create(ResourceLocation soundResource)
{
    return std::make_shared<PositionedSoundRecord>(std::move(soundResource), 1.0f, 1.0f, false, 0, AttenuationType::NONE, 0.0f, 0.0f, 0.0f);
}

std::shared_ptr<PositionedSoundRecord> PositionedSoundRecord::create(ResourceLocation soundResource, float x, float y, float z)
{
    return std::make_shared<PositionedSoundRecord>(std::move(soundResource), 4.0f, 1.0f, false, 0, AttenuationType::LINEAR, x, y, z);
}

PositionedSoundRecord::PositionedSoundRecord(ResourceLocation soundResource, float volumeIn, float pitchIn, float x, float y, float z)
    : PositionedSoundRecord(std::move(soundResource), volumeIn, pitchIn, false, 0, AttenuationType::LINEAR, x, y, z)
{
}

PositionedSoundRecord::PositionedSoundRecord(ResourceLocation soundResource, float volumeIn, float pitchIn, bool repeatIn, int repeatDelayIn,
    AttenuationType attenuationTypeIn, float x, float y, float z)
    : PositionedSound(std::move(soundResource))
{
    volume = volumeIn;
    pitch = pitchIn;
    xPosF = x;
    yPosF = y;
    zPosF = z;
    repeat = repeatIn;
    repeatDelay = repeatDelayIn;
    attenuationType = attenuationTypeIn;
}

SoundEventAccessorComposite::SoundEventAccessorComposite(ResourceLocation location, double pitch, double volume, SoundCategory categoryIn)
    : soundLocation(std::move(location))
    , category(categoryIn)
    , eventPitch(pitch)
    , eventVolume(volume)
{
}

int SoundEventAccessorComposite::getWeight() const
{
    int i = 0;
    for (const auto& accessor : soundPool)
        i += accessor->getWeight();
    return i;
}

SoundPoolEntry SoundEventAccessorComposite::cloneEntry()
{
    const int i = getWeight();
    if (!soundPool.empty() && i != 0) {
        int j = rnd.nextInt(i);
        for (const auto& accessor : soundPool) {
            j -= accessor->getWeight();
            if (j < 0) {
                SoundPoolEntry entry = accessor->cloneEntry();
                entry.setPitch(entry.getPitch() * eventPitch);
                entry.setVolume(entry.getVolume() * eventVolume);
                return entry;
            }
        }
    }
    return SoundHandler::missingSound();
}

// --- SoundManager ---

SoundManager::SoundManager(SoundHandler& handler, GameSettings& gameSettings, const IResourceManager& resources)
    : sndHandler(handler)
    , options(gameSettings)
    , resourceManager(resources)
{
}

void SoundManager::reloadSoundSystem()
{
    unloadSoundSystem();
    loadSoundSystem();
}

void SoundManager::loadSoundSystem()
{
    if (loaded)
        return;
    sndSystem = std::make_unique<AudioDevice>();
    if (!sndSystem->isAvailable()) {
        // Java: "Error starting SoundSystem. Turning off sounds & music"
        options.setSoundLevel(SoundCategory::MASTER, 0.0f);
        options.saveOptions();
    }
    loaded = true;
    sndSystem->setMasterGain(options.getSoundLevel(SoundCategory::MASTER));
}

void SoundManager::unloadSoundSystem()
{
    if (loaded) {
        stopAllSounds();
        sndSystem.reset();
        bufferCache.clear(); // resource packs may have changed the files
        loaded = false;
    }
}

float SoundManager::getSoundCategoryVolume(SoundCategory category) const
{
    return category != SoundCategory::MASTER ? options.getSoundLevel(category) : 1.0f;
}

void SoundManager::setSoundCategoryVolume(SoundCategory category, float volume)
{
    if (!loaded)
        return;
    if (category == SoundCategory::MASTER) {
        sndSystem->setMasterGain(volume);
        return;
    }
    for (Playing& playing : playingSounds) {
        if (playing.category != category)
            continue;
        const float f = getNormalizedVolume(*playing.sound, playing.entry, category);
        if (f <= 0.0f)
            sndSystem->stop(playing.voice);
        else
            sndSystem->setGain(playing.voice, f);
    }
}

void SoundManager::stopAllSounds()
{
    if (!loaded)
        return;
    sndSystem->stopAll();
    playingSounds.clear();
    delayedSounds.clear();
}

void SoundManager::updateAllSounds()
{
    ++playTime;
    if (!loaded)
        return;

    // Java: tickable sounds update their volume/pitch/position here (stage 13).

    for (auto it = playingSounds.begin(); it != playingSounds.end();) {
        if (!sndSystem->isPlaying(it->voice) && it->stopTime <= playTime) {
            const int j = it->sound->getRepeatDelay();
            if (it->sound->canRepeat() && j > 0)
                delayedSounds.emplace_back(it->sound, playTime + j);
            sndSystem->stop(it->voice);
            it = playingSounds.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = delayedSounds.begin(); it != delayedSounds.end();) {
        if (playTime >= it->second) {
            const std::shared_ptr<ISound> sound = it->first;
            it = delayedSounds.erase(it);
            playSound(sound);
        } else {
            ++it;
        }
    }
    sndSystem->update();
}

bool SoundManager::isSoundPlaying(const std::shared_ptr<ISound>& sound) const
{
    if (!loaded)
        return false;
    for (const Playing& playing : playingSounds) {
        if (playing.sound == sound)
            return sndSystem->isPlaying(playing.voice) || playing.stopTime <= playTime;
    }
    return false;
}

void SoundManager::stopSound(const std::shared_ptr<ISound>& sound)
{
    if (!loaded)
        return;
    for (const Playing& playing : playingSounds) {
        if (playing.sound == sound)
            sndSystem->stop(playing.voice);
    }
}

std::shared_ptr<const AudioBuffer> SoundManager::loadBuffer(const ResourceLocation& location)
{
    if (const auto it = bufferCache.find(location); it != bufferCache.end())
        return it->second;
    const std::shared_ptr<IResource> resource = resourceManager.getResource(location);
    std::shared_ptr<const AudioBuffer> buffer = decodeOggVorbis(resource->getInputStream());
    bufferCache.emplace(location, buffer);
    return buffer;
}

void SoundManager::playSound(const std::shared_ptr<ISound>& sound)
{
    if (!loaded)
        return;
    if (sndSystem->getMasterGain() <= 0.0f)
        return; // "Skipped playing soundEvent: master volume was zero"

    SoundEventAccessorComposite* composite = sndHandler.getSound(sound->getSoundLocation());
    if (!composite) {
        Log::warn("Unable to play unknown soundEvent: {}", sound->getSoundLocation().toString());
        return;
    }

    const SoundPoolEntry entry = composite->cloneEntry();
    if (entry.isMissingSound()) {
        Log::warn("Unable to play empty soundEvent: {}", composite->getSoundEventLocation().toString());
        return;
    }

    // Java computes the attenuation distance here (16, scaled by volumes above 1) for 3D sounds (stage 13).
    const SoundCategory category = composite->getSoundCategory();
    const float f2 = getNormalizedVolume(*sound, entry, category);
    const float pitch = getNormalizedPitch(*sound, entry);
    if (f2 == 0.0f)
        return;

    const bool loop = sound->canRepeat() && sound->getRepeatDelay() == 0;
    AudioDevice::VoiceId voice = 0;
    try {
        voice = sndSystem->play(loadBuffer(entry.getSoundPoolEntryLocation()), f2, pitch, loop);
    } catch (const std::exception& e) {
        Log::error("Error in class 'SoundManager': can't play {} ({})", entry.getSoundPoolEntryLocation().toString(), e.what());
        return;
    }
    playingSounds.push_back(Playing { sound, entry, category, voice, playTime + 20 });
}

void SoundManager::playDelayedSound(const std::shared_ptr<ISound>& sound, int delay)
{
    delayedSounds.emplace_back(sound, playTime + delay);
}

float SoundManager::getNormalizedPitch(const ISound& sound, const SoundPoolEntry& entry)
{
    return static_cast<float>(MathHelper::clamp_double(static_cast<double>(sound.getPitch()) * entry.getPitch(), 0.5, 2.0));
}

float SoundManager::getNormalizedVolume(const ISound& sound, const SoundPoolEntry& entry, SoundCategory category) const
{
    return static_cast<float>(MathHelper::clamp_double(static_cast<double>(sound.getVolume()) * entry.getVolume(), 0.0, 1.0))
        * getSoundCategoryVolume(category);
}

void SoundManager::pauseAllSounds()
{
    if (loaded)
        sndSystem->pauseAll();
}

void SoundManager::resumeAllSounds()
{
    if (loaded)
        sndSystem->resumeAll();
}

// --- SoundHandler ---

SoundHandler::SoundHandler(const IResourceManager& manager, GameSettings& gameSettings)
    : mcResourceManager(manager)
    , sndManager(*this, gameSettings, manager)
{
}

const SoundPoolEntry& SoundHandler::missingSound()
{
    static const SoundPoolEntry missing(ResourceLocation("meta:missing_sound"), 0.0, 0.0, false);
    return missing;
}

std::map<std::string, SoundHandler::SoundList> SoundHandler::parseSoundMap(const std::vector<uint8_t>& data)
{
    const nlohmann::json root = nlohmann::json::parse(data.begin(), data.end());
    std::map<std::string, SoundList> map;
    for (const auto& [name, element] : root.items()) {
        // SoundListSerializer
        if (!element.is_object())
            throw std::runtime_error("Expected entry to be a JsonObject");
        SoundList list;
        list.replaceExisting = element.value("replace", false);
        const auto category = getSoundCategoryByName(element.value("category", std::string(getCategoryName(SoundCategory::MASTER))));
        if (!category)
            throw std::runtime_error("Invalid category");
        list.category = *category;

        if (const auto sounds = element.find("sounds"); sounds != element.end()) {
            for (const auto& soundElement : *sounds) {
                SoundEntry entry;
                if (soundElement.is_string()) {
                    entry.name = soundElement.get<std::string>();
                } else {
                    entry.name = soundElement.at("name").get<std::string>();
                    if (soundElement.contains("type")) {
                        const std::string type = soundElement["type"].get<std::string>();
                        if (type == "file")
                            entry.type = SoundEntry::Type::FILE;
                        else if (type == "event")
                            entry.type = SoundEntry::Type::SOUND_EVENT;
                        else
                            throw std::runtime_error("Invalid type");
                    }
                    if (soundElement.contains("volume")) {
                        entry.volume = soundElement["volume"].get<float>();
                        if (!(entry.volume > 0.0f))
                            throw std::runtime_error("Invalid volume");
                    }
                    if (soundElement.contains("pitch")) {
                        entry.pitch = soundElement["pitch"].get<float>();
                        if (!(entry.pitch > 0.0f))
                            throw std::runtime_error("Invalid pitch");
                    }
                    if (soundElement.contains("weight")) {
                        entry.weight = soundElement["weight"].get<int>();
                        if (entry.weight <= 0)
                            throw std::runtime_error("Invalid weight");
                    }
                    if (soundElement.contains("stream"))
                        entry.streaming = soundElement["stream"].get<bool>();
                }
                list.sounds.push_back(std::move(entry));
            }
        }
        map.emplace(name, std::move(list));
    }
    return map;
}

void SoundHandler::onResourceManagerReload(const IResourceManager& resourceManager)
{
    sndManager.reloadSoundSystem();
    sndRegistry.clear();

    for (const std::string& domain : resourceManager.getResourceDomains()) {
        try {
            for (const auto& resource : resourceManager.getAllResources(ResourceLocation(domain, "sounds.json"))) {
                try {
                    for (const auto& [name, list] : parseSoundMap(resource->getInputStream()))
                        loadSoundResource(ResourceLocation(domain, name), list);
                } catch (const std::exception& e) {
                    Log::warn("Invalid sounds.json ({})", e.what());
                }
            }
        } catch (const ResourceNotFoundException&) {
            // Java: IOException ignored
        }
    }
}

void SoundHandler::loadSoundResource(const ResourceLocation& location, const SoundList& sounds)
{
    const bool absent = !sndRegistry.contains(location);
    std::shared_ptr<SoundEventAccessorComposite> composite;
    if (!absent && !sounds.replaceExisting) {
        composite = sndRegistry[location];
    } else {
        composite = std::make_shared<SoundEventAccessorComposite>(location, 1.0, 1.0, sounds.category);
        sndRegistry[location] = composite;
    }

    for (const SoundEntry& entry : sounds.sounds) {
        const ResourceLocation resourcelocation(entry.name);
        const std::string domain = entry.name.find(':') != std::string::npos ? resourcelocation.getResourceDomain() : location.getResourceDomain();
        std::shared_ptr<ISoundEventAccessor> accessor;

        if (entry.type == SoundEntry::Type::FILE) {
            const ResourceLocation file(domain, "sounds/" + resourcelocation.getResourcePath() + ".ogg");
            // Java opens the file to check it exists; only existence matters, so don't read it.
            if (!mcResourceManager.resourceExists(file)) {
                Log::warn("File {} does not exist, cannot add it to event {}", file.toString(), location.toString());
                continue;
            }
            accessor = std::make_shared<SoundEventAccessor>(SoundPoolEntry(file, entry.pitch, entry.volume, entry.streaming), entry.weight);
        } else {
            // Resolved on use: the target event may be registered later.
            class EventReference : public ISoundEventAccessor {
            public:
                EventReference(SoundHandler& handlerIn, ResourceLocation targetIn)
                    : handler(handlerIn)
                    , target(std::move(targetIn))
                {
                }
                int getWeight() const override
                {
                    const SoundEventAccessorComposite* event = handler.getSound(target);
                    return event ? event->getWeight() : 0;
                }
                SoundPoolEntry cloneEntry() override
                {
                    SoundEventAccessorComposite* event = handler.getSound(target);
                    return event ? event->cloneEntry() : SoundHandler::missingSound();
                }

            private:
                SoundHandler& handler;
                ResourceLocation target;
            };
            accessor = std::make_shared<EventReference>(*this, ResourceLocation(domain, entry.name));
        }
        composite->addSoundToEventPool(std::move(accessor));
    }
}

SoundEventAccessorComposite* SoundHandler::getSound(const ResourceLocation& location)
{
    const auto it = sndRegistry.find(location);
    return it == sndRegistry.end() ? nullptr : it->second.get();
}

void SoundHandler::setSoundLevel(SoundCategory category, float volume)
{
    if (category == SoundCategory::MASTER && volume <= 0.0f)
        stopSounds();
    sndManager.setSoundCategoryVolume(category, volume);
}

SoundEventAccessorComposite* SoundHandler::getRandomSoundFromCategories(std::initializer_list<SoundCategory> categories)
{
    std::vector<SoundEventAccessorComposite*> list;
    for (const auto& [location, composite] : sndRegistry) {
        if (std::find(categories.begin(), categories.end(), composite->getSoundCategory()) != categories.end())
            list.push_back(composite.get());
    }
    if (list.empty())
        return nullptr;
    return list[static_cast<size_t>(Random().nextInt(static_cast<int32_t>(list.size())))];
}

} // namespace mc
