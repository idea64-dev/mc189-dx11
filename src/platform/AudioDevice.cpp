#include "platform/AudioDevice.h"

#include "core/Log.h"
#include "core/WinError.h"

#include <Windows.h>
#include <objbase.h>
#include <xaudio2.h>

#define STB_VORBIS_HEADER_ONLY
#define STB_VORBIS_NO_STDIO
#include <stb/stb_vorbis.c> // implementation compiled in the stb_vorbis library (CMakeLists.txt)

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace mc {

std::shared_ptr<const AudioBuffer> decodeOggVorbis(std::span<const uint8_t> data)
{
    int channels = 0;
    int sampleRate = 0;
    short* output = nullptr;
    const int frames = stb_vorbis_decode_memory(data.data(), static_cast<int>(data.size()), &channels, &sampleRate, &output);
    if (frames < 0 || !output)
        throw std::runtime_error("Couldn't decode Ogg Vorbis data");

    auto buffer = std::make_shared<AudioBuffer>();
    buffer->channels = channels;
    buffer->sampleRate = sampleRate;
    buffer->samples.assign(output, output + static_cast<size_t>(frames) * static_cast<size_t>(channels));
    std::free(output);
    return buffer;
}

AudioDevice::AudioDevice()
{
    const HRESULT co = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    comInitialized = SUCCEEDED(co);

    HRESULT hr = XAudio2Create(&xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (SUCCEEDED(hr))
        hr = xaudio->CreateMasteringVoice(&mastering);
    if (FAILED(hr)) {
        Log::error("Error starting SoundSystem. Turning off sounds & music ({})", hresultToString(hr));
        if (xaudio) {
            xaudio->Release();
            xaudio = nullptr;
        }
        return;
    }

    XAUDIO2_VOICE_DETAILS details {};
    mastering->GetVoiceDetails(&details);
    Log::info("Sound engine started (XAudio2, {} Hz, {} channels)", details.InputSampleRate, details.InputChannels);
}

AudioDevice::~AudioDevice()
{
    stopAll();
    if (mastering)
        mastering->DestroyVoice();
    if (xaudio)
        xaudio->Release();
    if (comInitialized)
        CoUninitialize();
}

void AudioDevice::destroyVoice(Voice& voice)
{
    if (voice.source) {
        voice.source->Stop(0);
        voice.source->FlushSourceBuffers();
        voice.source->DestroyVoice();
        voice.source = nullptr;
    }
}

AudioDevice::VoiceId AudioDevice::play(std::shared_ptr<const AudioBuffer> buffer, float gain, float pitch, bool loop)
{
    if (!xaudio || !buffer || buffer->samples.empty())
        return 0;

    WAVEFORMATEX format {};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = static_cast<WORD>(buffer->channels);
    format.nSamplesPerSec = static_cast<DWORD>(buffer->sampleRate);
    format.wBitsPerSample = 16;
    format.nBlockAlign = static_cast<WORD>(format.nChannels * 2);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

    Voice voice;
    voice.buffer = std::move(buffer);
    // Minecraft clamps pitch to [0.5, 2]; XAudio2's default ceiling is 2.
    HRESULT hr = xaudio->CreateSourceVoice(&voice.source, &format, 0, XAUDIO2_DEFAULT_FREQ_RATIO);
    if (FAILED(hr)) {
        Log::warn("Couldn't create a sound voice ({})", hresultToString(hr));
        return 0;
    }

    XAUDIO2_BUFFER submit {};
    submit.AudioBytes = static_cast<UINT32>(voice.buffer->samples.size() * sizeof(int16_t));
    submit.pAudioData = reinterpret_cast<const BYTE*>(voice.buffer->samples.data());
    submit.Flags = XAUDIO2_END_OF_STREAM;
    submit.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;
    hr = voice.source->SubmitSourceBuffer(&submit);
    if (FAILED(hr)) {
        destroyVoice(voice);
        return 0;
    }

    voice.source->SetVolume(gain);
    voice.source->SetFrequencyRatio(std::clamp(pitch, XAUDIO2_MIN_FREQ_RATIO, XAUDIO2_DEFAULT_FREQ_RATIO));
    voice.source->Start(0);

    const VoiceId id = nextId++;
    voices.emplace(id, std::move(voice));
    return id;
}

void AudioDevice::setGain(VoiceId id, float gain)
{
    if (const auto it = voices.find(id); it != voices.end())
        it->second.source->SetVolume(gain);
}

void AudioDevice::setPitch(VoiceId id, float pitch)
{
    if (const auto it = voices.find(id); it != voices.end())
        it->second.source->SetFrequencyRatio(std::clamp(pitch, XAUDIO2_MIN_FREQ_RATIO, XAUDIO2_DEFAULT_FREQ_RATIO));
}

void AudioDevice::stop(VoiceId id)
{
    if (const auto it = voices.find(id); it != voices.end()) {
        destroyVoice(it->second);
        voices.erase(it);
    }
}

bool AudioDevice::isPlaying(VoiceId id) const
{
    const auto it = voices.find(id);
    if (it == voices.end())
        return false;
    XAUDIO2_VOICE_STATE state {};
    it->second.source->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
    return state.BuffersQueued > 0;
}

void AudioDevice::pauseAll()
{
    for (auto& [id, voice] : voices)
        voice.source->Stop(0);
}

void AudioDevice::resumeAll()
{
    for (auto& [id, voice] : voices)
        voice.source->Start(0);
}

void AudioDevice::stopAll()
{
    for (auto& [id, voice] : voices)
        destroyVoice(voice);
    voices.clear();
}

void AudioDevice::setMasterGain(float gain)
{
    masterGain = gain;
    if (mastering)
        mastering->SetVolume(gain);
}

void AudioDevice::update()
{
    for (auto it = voices.begin(); it != voices.end();) {
        XAUDIO2_VOICE_STATE state {};
        it->second.source->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
        if (state.BuffersQueued == 0) {
            destroyVoice(it->second);
            it = voices.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace mc
