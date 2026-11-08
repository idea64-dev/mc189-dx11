#pragma once

// Original: net/minecraft/client/audio/SoundHandler.java, SoundManager.java, SoundList.java (+ serializer),
// SoundRegistry.java. Output goes through platform/AudioDevice (XAudio2) instead of paulscode + OpenAL.
// Not ported yet: listener position and 3D attenuation (stage 13), ITickableSound.

#include "client/audio/Sound.h"
#include "client/resources/IResourceManager.h"
#include "platform/AudioDevice.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {

class GameSettings;
class SoundHandler;

class SoundManager {
public:
    SoundManager(SoundHandler& soundHandler, GameSettings& options, const IResourceManager& resourceManager);

    void reloadSoundSystem();
    void unloadSoundSystem();
    void setSoundCategoryVolume(SoundCategory category, float volume);
    void stopAllSounds();
    void updateAllSounds();
    bool isSoundPlaying(const std::shared_ptr<ISound>& sound) const;
    void stopSound(const std::shared_ptr<ISound>& sound);
    void playSound(const std::shared_ptr<ISound>& sound);
    void playDelayedSound(const std::shared_ptr<ISound>& sound, int delay);
    void pauseAllSounds();
    void resumeAllSounds();

private:
    struct Playing {
        std::shared_ptr<ISound> sound;
        SoundPoolEntry entry;
        SoundCategory category;
        AudioDevice::VoiceId voice;
        int stopTime;
    };

    void loadSoundSystem();
    float getSoundCategoryVolume(SoundCategory category) const;
    static float getNormalizedPitch(const ISound& sound, const SoundPoolEntry& entry);
    float getNormalizedVolume(const ISound& sound, const SoundPoolEntry& entry, SoundCategory category) const;
    std::shared_ptr<const AudioBuffer> loadBuffer(const ResourceLocation& location);

    SoundHandler& sndHandler;
    GameSettings& options;
    const IResourceManager& resourceManager;
    std::unique_ptr<AudioDevice> sndSystem;
    bool loaded = false;
    int playTime = 0;
    std::vector<Playing> playingSounds;
    std::vector<std::pair<std::shared_ptr<ISound>, int>> delayedSounds;
    std::unordered_map<ResourceLocation, std::shared_ptr<const AudioBuffer>> bufferCache;
};

class SoundHandler : public IResourceManagerReloadListener {
public:
    SoundHandler(const IResourceManager& manager, GameSettings& gameSettings);

    static const SoundPoolEntry& missingSound();

    void onResourceManagerReload(const IResourceManager& resourceManager) override;
    SoundEventAccessorComposite* getSound(const ResourceLocation& location);
    void playSound(const std::shared_ptr<ISound>& sound) { sndManager.playSound(sound); }
    void playDelayedSound(const std::shared_ptr<ISound>& sound, int delay) { sndManager.playDelayedSound(sound, delay); }
    void pauseSounds() { sndManager.pauseAllSounds(); }
    void stopSounds() { sndManager.stopAllSounds(); }
    void unloadSounds() { sndManager.unloadSoundSystem(); }
    void update() { sndManager.updateAllSounds(); }
    void resumeSounds() { sndManager.resumeAllSounds(); }
    void setSoundLevel(SoundCategory category, float volume);
    void stopSound(const std::shared_ptr<ISound>& sound) { sndManager.stopSound(sound); }
    SoundEventAccessorComposite* getRandomSoundFromCategories(std::initializer_list<SoundCategory> categories);
    bool isSoundPlaying(const std::shared_ptr<ISound>& sound) const { return sndManager.isSoundPlaying(sound); }

private:
    struct SoundEntry {
        enum class Type { FILE, SOUND_EVENT };
        std::string name;
        float volume = 1.0f;
        float pitch = 1.0f;
        int weight = 1;
        Type type = Type::FILE;
        bool streaming = false;
    };
    struct SoundList {
        std::vector<SoundEntry> sounds;
        bool replaceExisting = false;
        SoundCategory category = SoundCategory::MASTER;
    };

    static std::map<std::string, SoundList> parseSoundMap(const std::vector<uint8_t>& json);
    void loadSoundResource(const ResourceLocation& location, const SoundList& sounds);

    std::unordered_map<ResourceLocation, std::shared_ptr<SoundEventAccessorComposite>> sndRegistry;
    const IResourceManager& mcResourceManager;
    SoundManager sndManager;
};

} // namespace mc
