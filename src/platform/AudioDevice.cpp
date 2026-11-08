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
#include <chrono>
#include <cstdlib>
#include <stdexcept>

namespace mc {

namespace {

// Each streamed chunk holds this many frames (~0.19 s at 44.1 kHz); a few are kept queued ahead.
constexpr int kStreamChunkFrames = 8192;
constexpr UINT32 kStreamQueuedChunks = 4;

float clampPitch(float pitch)
{
    // Minecraft clamps pitch to [0.5, 2]; XAudio2's default ceiling is 2.
    return std::clamp(pitch, XAUDIO2_MIN_FREQ_RATIO, XAUDIO2_DEFAULT_FREQ_RATIO);
}

} // namespace

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
    streamThread = std::thread([this] { streamThreadMain(); });
}

AudioDevice::~AudioDevice()
{
    {
        const std::lock_guard lock(mutex);
        stopping = true;
    }
    wake.notify_all();
    if (streamThread.joinable())
        streamThread.join();

    stopAll();
    if (mastering)
        mastering->DestroyVoice();
    if (xaudio)
        xaudio->Release();
    if (comInitialized)
        CoUninitialize();
}

IXAudio2SourceVoice* AudioDevice::createSourceVoice(int channels, int sampleRate)
{
    WAVEFORMATEX format {};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = static_cast<WORD>(channels);
    format.nSamplesPerSec = static_cast<DWORD>(sampleRate);
    format.wBitsPerSample = 16;
    format.nBlockAlign = static_cast<WORD>(format.nChannels * 2);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

    IXAudio2SourceVoice* source = nullptr;
    const HRESULT hr = xaudio->CreateSourceVoice(&source, &format, 0, XAUDIO2_DEFAULT_FREQ_RATIO);
    if (FAILED(hr)) {
        Log::warn("Couldn't create a sound voice ({})", hresultToString(hr));
        return nullptr;
    }
    return source;
}

void AudioDevice::destroyVoice(Voice& voice)
{
    if (voice.source) {
        voice.source->Stop(0);
        voice.source->FlushSourceBuffers();
        voice.source->DestroyVoice(); // blocks until XAudio2 no longer reads the buffers
        voice.source = nullptr;
    }
    if (voice.stream && voice.stream->decoder) {
        stb_vorbis_close(voice.stream->decoder);
        voice.stream->decoder = nullptr;
    }
}

AudioDevice::VoiceId AudioDevice::play(std::shared_ptr<const AudioBuffer> buffer, float gain, float pitch, bool loop)
{
    if (!xaudio || !buffer || buffer->samples.empty())
        return 0;

    const std::lock_guard lock(mutex);
    Voice voice;
    voice.buffer = std::move(buffer);
    voice.source = createSourceVoice(voice.buffer->channels, voice.buffer->sampleRate);
    if (!voice.source)
        return 0;

    XAUDIO2_BUFFER submit {};
    submit.AudioBytes = static_cast<UINT32>(voice.buffer->samples.size() * sizeof(int16_t));
    submit.pAudioData = reinterpret_cast<const BYTE*>(voice.buffer->samples.data());
    submit.Flags = XAUDIO2_END_OF_STREAM;
    submit.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;
    if (FAILED(voice.source->SubmitSourceBuffer(&submit))) {
        destroyVoice(voice);
        return 0;
    }

    voice.source->SetVolume(gain);
    voice.source->SetFrequencyRatio(clampPitch(pitch));
    voice.source->Start(0);

    const VoiceId id = nextId++;
    voices.emplace(id, std::move(voice));
    return id;
}

AudioDevice::VoiceId AudioDevice::playStream(std::shared_ptr<const std::vector<uint8_t>> oggData, float gain, float pitch, bool loop)
{
    if (!xaudio || !oggData || oggData->empty())
        return 0;

    auto stream = std::make_unique<Stream>();
    stream->data = std::move(oggData);
    stream->loop = loop;
    int error = 0;
    stream->decoder = stb_vorbis_open_memory(stream->data->data(), static_cast<int>(stream->data->size()), &error, nullptr);
    if (!stream->decoder)
        throw std::runtime_error("Couldn't decode Ogg Vorbis data (stb_vorbis error " + std::to_string(error) + ")");
    const stb_vorbis_info info = stb_vorbis_get_info(stream->decoder);
    stream->channels = info.channels;

    const std::lock_guard lock(mutex);
    Voice voice;
    voice.stream = std::move(stream);
    voice.source = createSourceVoice(info.channels, static_cast<int>(info.sample_rate));
    if (!voice.source) {
        destroyVoice(voice);
        return 0;
    }

    refill(voice); // the first chunks, so playback starts right away
    voice.source->SetVolume(gain);
    voice.source->SetFrequencyRatio(clampPitch(pitch));
    voice.source->Start(0);

    const VoiceId id = nextId++;
    voices.emplace(id, std::move(voice));
    return id;
}

void AudioDevice::refill(Voice& voice)
{
    Stream& stream = *voice.stream;
    XAUDIO2_VOICE_STATE state {};
    voice.source->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
    while (stream.chunks.size() > state.BuffersQueued)
        stream.chunks.pop_front();

    UINT32 queued = state.BuffersQueued;
    while (!stream.finished && queued < kStreamQueuedChunks) {
        std::vector<int16_t> chunk(static_cast<size_t>(kStreamChunkFrames) * static_cast<size_t>(stream.channels));
        int frames = stb_vorbis_get_samples_short_interleaved(stream.decoder, stream.channels, chunk.data(), static_cast<int>(chunk.size()));
        if (frames == 0 && stream.loop) {
            stb_vorbis_seek_start(stream.decoder);
            frames = stb_vorbis_get_samples_short_interleaved(stream.decoder, stream.channels, chunk.data(), static_cast<int>(chunk.size()));
        }
        if (frames == 0) {
            stream.finished = true;
            voice.source->Discontinuity(); // the last queued buffer ends the stream
            break;
        }
        chunk.resize(static_cast<size_t>(frames) * static_cast<size_t>(stream.channels));

        XAUDIO2_BUFFER submit {};
        submit.AudioBytes = static_cast<UINT32>(chunk.size() * sizeof(int16_t));
        submit.pAudioData = reinterpret_cast<const BYTE*>(chunk.data());
        stream.chunks.push_back(std::move(chunk)); // the vector's storage doesn't move with it
        if (FAILED(voice.source->SubmitSourceBuffer(&submit))) {
            stream.chunks.pop_back();
            stream.finished = true;
            break;
        }
        ++queued;
    }
}

void AudioDevice::streamThreadMain()
{
    std::unique_lock lock(mutex);
    while (!stopping) {
        for (auto& [id, voice] : voices) {
            if (voice.stream && voice.source)
                refill(voice);
        }
        wake.wait_for(lock, std::chrono::milliseconds(30));
    }
}

void AudioDevice::setGain(VoiceId id, float gain)
{
    const std::lock_guard lock(mutex);
    if (const auto it = voices.find(id); it != voices.end())
        it->second.source->SetVolume(gain);
}

void AudioDevice::setPitch(VoiceId id, float pitch)
{
    const std::lock_guard lock(mutex);
    if (const auto it = voices.find(id); it != voices.end())
        it->second.source->SetFrequencyRatio(clampPitch(pitch));
}

void AudioDevice::stop(VoiceId id)
{
    const std::lock_guard lock(mutex);
    if (const auto it = voices.find(id); it != voices.end()) {
        destroyVoice(it->second);
        voices.erase(it);
    }
}

bool AudioDevice::isPlayingLocked(const Voice& voice) const
{
    XAUDIO2_VOICE_STATE state {};
    voice.source->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
    return state.BuffersQueued > 0 || (voice.stream && !voice.stream->finished);
}

bool AudioDevice::isPlaying(VoiceId id) const
{
    const std::lock_guard lock(mutex);
    const auto it = voices.find(id);
    return it != voices.end() && isPlayingLocked(it->second);
}

void AudioDevice::pauseAll()
{
    const std::lock_guard lock(mutex);
    for (auto& [id, voice] : voices)
        voice.source->Stop(0);
}

void AudioDevice::resumeAll()
{
    const std::lock_guard lock(mutex);
    for (auto& [id, voice] : voices)
        voice.source->Start(0);
}

void AudioDevice::stopAll()
{
    const std::lock_guard lock(mutex);
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
    const std::lock_guard lock(mutex);
    for (auto it = voices.begin(); it != voices.end();) {
        if (!isPlayingLocked(it->second)) {
            destroyVoice(it->second);
            it = voices.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace mc
