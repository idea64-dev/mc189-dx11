#pragma once

// Sound output on XAudio2 (part of Windows), replacing paulscode SoundSystem + LWJGL OpenAL. Plays decoded
// PCM buffers with per-voice gain and pitch; everything Minecraft-specific lives in SoundManager.

#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

struct IXAudio2;
struct IXAudio2MasteringVoice;
struct IXAudio2SourceVoice;

namespace mc {

struct AudioBuffer {
    std::vector<int16_t> samples; // interleaved
    int channels = 1;
    int sampleRate = 44100;
};

// Decodes an Ogg Vorbis file; throws std::runtime_error on bad data.
std::shared_ptr<const AudioBuffer> decodeOggVorbis(std::span<const uint8_t> data);

class AudioDevice {
public:
    using VoiceId = uint64_t;

    AudioDevice();
    ~AudioDevice();

    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;

    // False when no output device could be opened; every call is then a no-op, as with paulscode's "no sound".
    bool isAvailable() const { return xaudio != nullptr; }

    VoiceId play(std::shared_ptr<const AudioBuffer> buffer, float gain, float pitch, bool loop);
    void setGain(VoiceId voice, float gain);
    void setPitch(VoiceId voice, float pitch);
    void stop(VoiceId voice);
    bool isPlaying(VoiceId voice) const;
    void pauseAll();
    void resumeAll();
    void stopAll();
    void setMasterGain(float gain);
    float getMasterGain() const { return masterGain; }
    // Releases voices that finished playing.
    void update();

private:
    struct Voice {
        IXAudio2SourceVoice* source = nullptr;
        std::shared_ptr<const AudioBuffer> buffer;
    };

    void destroyVoice(Voice& voice);

    IXAudio2* xaudio = nullptr;
    IXAudio2MasteringVoice* mastering = nullptr;
    bool comInitialized = false;
    float masterGain = 1.0f;
    VoiceId nextId = 1;
    std::unordered_map<VoiceId, Voice> voices;
};

} // namespace mc
