#pragma once

// Original: net/minecraft/client/audio/ISound.java, PositionedSound.java, PositionedSoundRecord.java,
// SoundPoolEntry.java, ISoundEventAccessor.java, SoundEventAccessor.java, SoundEventAccessorComposite.java

#include "client/audio/SoundCategory.h"
#include "util/Random.h"
#include "util/ResourceLocation.h"

#include <functional>
#include <memory>
#include <vector>

namespace mc {

class ISound {
public:
    enum class AttenuationType { NONE = 0, LINEAR = 2 };

    virtual ~ISound() = default;
    virtual const ResourceLocation& getSoundLocation() const = 0;
    virtual bool canRepeat() const = 0;
    virtual int getRepeatDelay() const = 0;
    virtual float getVolume() const = 0;
    virtual float getPitch() const = 0;
    virtual float getXPosF() const = 0;
    virtual float getYPosF() const = 0;
    virtual float getZPosF() const = 0;
    virtual AttenuationType getAttenuationType() const = 0;
};

class PositionedSound : public ISound {
public:
    const ResourceLocation& getSoundLocation() const override { return positionedSoundLocation; }
    bool canRepeat() const override { return repeat; }
    int getRepeatDelay() const override { return repeatDelay; }
    float getVolume() const override { return volume; }
    float getPitch() const override { return pitch; }
    float getXPosF() const override { return xPosF; }
    float getYPosF() const override { return yPosF; }
    float getZPosF() const override { return zPosF; }
    AttenuationType getAttenuationType() const override { return attenuationType; }

protected:
    explicit PositionedSound(ResourceLocation soundResource)
        : positionedSoundLocation(std::move(soundResource))
    {
    }

    ResourceLocation positionedSoundLocation;
    float volume = 1.0f;
    float pitch = 1.0f;
    float xPosF = 0.0f;
    float yPosF = 0.0f;
    float zPosF = 0.0f;
    bool repeat = false;
    int repeatDelay = 0;
    AttenuationType attenuationType = AttenuationType::LINEAR;
};

class PositionedSoundRecord : public PositionedSound {
public:
    // GUI sounds: volume 0.25, no attenuation.
    static std::shared_ptr<PositionedSoundRecord> create(ResourceLocation soundResource, float pitch);
    static std::shared_ptr<PositionedSoundRecord> create(ResourceLocation soundResource);
    static std::shared_ptr<PositionedSoundRecord> create(ResourceLocation soundResource, float x, float y, float z);

    PositionedSoundRecord(ResourceLocation soundResource, float volume, float pitch, float x, float y, float z);
    PositionedSoundRecord(ResourceLocation soundResource, float volume, float pitch, bool repeat, int repeatDelay,
        AttenuationType attenuationType, float x, float y, float z);
};

class SoundPoolEntry {
public:
    SoundPoolEntry(ResourceLocation locationIn, double pitchIn, double volumeIn, bool streamingSoundIn)
        : location(std::move(locationIn))
        , streamingSound(streamingSoundIn)
        , pitch(pitchIn)
        , volume(volumeIn)
    {
    }

    const ResourceLocation& getSoundPoolEntryLocation() const { return location; }
    double getPitch() const { return pitch; }
    void setPitch(double pitchIn) { pitch = pitchIn; }
    double getVolume() const { return volume; }
    void setVolume(double volumeIn) { volume = volumeIn; }
    bool isStreamingSound() const { return streamingSound; }
    bool isMissingSound() const { return location == ResourceLocation("meta:missing_sound"); }

private:
    ResourceLocation location;
    bool streamingSound;
    double pitch;
    double volume;
};

class ISoundEventAccessor {
public:
    virtual ~ISoundEventAccessor() = default;
    virtual int getWeight() const = 0;
    virtual SoundPoolEntry cloneEntry() = 0;
};

class SoundEventAccessor : public ISoundEventAccessor {
public:
    SoundEventAccessor(SoundPoolEntry entryIn, int weightIn)
        : entry(std::move(entryIn))
        , weight(weightIn)
    {
    }
    int getWeight() const override { return weight; }
    SoundPoolEntry cloneEntry() override { return entry; }

private:
    SoundPoolEntry entry;
    int weight;
};

class SoundEventAccessorComposite : public ISoundEventAccessor {
public:
    SoundEventAccessorComposite(ResourceLocation soundLocation, double pitch, double volume, SoundCategory category);

    int getWeight() const override;
    // Picks a variant by weight; returns SoundHandler::missingSound() when there is none.
    SoundPoolEntry cloneEntry() override;
    void addSoundToEventPool(std::shared_ptr<ISoundEventAccessor> sound) { soundPool.push_back(std::move(sound)); }
    const ResourceLocation& getSoundEventLocation() const { return soundLocation; }
    SoundCategory getSoundCategory() const { return category; }

private:
    std::vector<std::shared_ptr<ISoundEventAccessor>> soundPool;
    Random rnd;
    ResourceLocation soundLocation;
    SoundCategory category;
    double eventPitch;
    double eventVolume;
};

} // namespace mc
