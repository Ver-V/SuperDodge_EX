#include "AudioManager.hpp"

#include <algorithm>
#include <cstring>
#include <string>

#define NOMINMAX
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mmreg.h>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

namespace
{
    template<typename T>
    void SafeRelease(T*& resource)
    {
        if (resource == nullptr) return;
        resource->Release();
        resource = nullptr;
    }

    bool FileExists(const std::wstring& path)
    {
        const DWORD attributes = GetFileAttributesW(path.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
    }

    std::wstring GetFullPath(const std::wstring& path)
    {
        wchar_t fullPath[MAX_PATH] = {};
        const DWORD length = GetFullPathNameW(path.c_str(), MAX_PATH, fullPath, nullptr);
        if (length == 0 || length >= MAX_PATH)
            return path;

        return fullPath;
    }

    std::wstring ResolveAudioPath(const wchar_t* relativePath)
    {
        const std::wstring path(relativePath);
        const std::wstring candidates[] = {
            path,
            L"SuperDodge\\" + path,
            L"..\\SuperDodge\\" + path,
            L"..\\..\\SuperDodge\\" + path
        };

        for (const std::wstring& candidate : candidates)
        {
            if (FileExists(candidate))
                return GetFullPath(candidate);
        }

        return GetFullPath(path);
    }
}

AudioManager::~AudioManager()
{
    Release();
}

bool AudioManager::Initialize()
{
    Release();

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    _comInitialized = SUCCEEDED(hr);
    if (hr == RPC_E_CHANGED_MODE)
        _comInitialized = false;
    else if (FAILED(hr))
        return false;

    hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
        Release();
        return false;
    }
    _mediaFoundationStarted = true;

    hr = XAudio2Create(&_engine, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr))
    {
        Release();
        return false;
    }

    hr = _engine->CreateMasteringVoice(&_masterVoice);
    if (FAILED(hr))
    {
        Release();
        return false;
    }

    LoadSounds();
    _initialized = true;
    return true;
}

void AudioManager::Update()
{
    for (auto it = _activeVoices.begin(); it != _activeVoices.end();)
    {
        XAUDIO2_VOICE_STATE state = {};
        it->voice->GetState(&state);
        if (state.BuffersQueued == 0)
        {
            it->voice->DestroyVoice();
            it = _activeVoices.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void AudioManager::PlayBgm()
{
    PlayBgm(MusicTrack::Time0To330);
}

void AudioManager::PlayBgm(MusicTrack track)
{
    const SoundData* music = GetMusic(track);
    if (!_initialized || _engine == nullptr || music == nullptr || music->audioBytes.empty()) return;
    if (_bgmVoice != nullptr && _hasCurrentMusicTrack && _currentMusicTrack == track) return;

    StopBgm();

    const WAVEFORMATEX* format = music->GetFormat();
    if (format == nullptr) return;

    HRESULT hr = _engine->CreateSourceVoice(&_bgmVoice, format);
    if (FAILED(hr))
    {
        _bgmVoice = nullptr;
        return;
    }

    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = static_cast<UINT32>(music->audioBytes.size());
    buffer.pAudioData = music->audioBytes.data();
    buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

    if (FAILED(_bgmVoice->SubmitSourceBuffer(&buffer)) || FAILED(_bgmVoice->Start(0)))
        StopBgm();

    if (_bgmVoice != nullptr)
    {
        _currentMusicTrack = track;
        _hasCurrentMusicTrack = true;
        SetBgmVolume(1.0f);
    }
}

void AudioManager::SetBgmVolume(float volume)
{
    if (_bgmVoice == nullptr) return;

    volume = (std::max)(0.0f, (std::min)(1.0f, volume));
    _bgmVoice->SetVolume(volume * _bgmBaseVolume);
}

void AudioManager::StopBgm()
{
    if (_bgmVoice == nullptr) return;

    _bgmVoice->Stop(0);
    _bgmVoice->FlushSourceBuffers();
    _bgmVoice->DestroyVoice();
    _bgmVoice = nullptr;
    _hasCurrentMusicTrack = false;
}

void AudioManager::PlaySfx(SoundEffect effect)
{
    if (!_initialized || _engine == nullptr) return;

    const SoundData* sound = GetSound(effect);
    if (sound == nullptr || sound->audioBytes.empty()) return;

    IXAudio2SourceVoice* voice = nullptr;
    const WAVEFORMATEX* format = sound->GetFormat();
    if (format == nullptr) return;

    HRESULT hr = _engine->CreateSourceVoice(&voice, format);
    if (FAILED(hr) || voice == nullptr) return;

    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = static_cast<UINT32>(sound->audioBytes.size());
    buffer.pAudioData = sound->audioBytes.data();
    buffer.Flags = XAUDIO2_END_OF_STREAM;

    if (FAILED(voice->SubmitSourceBuffer(&buffer)) || FAILED(voice->Start(0)))
    {
        voice->DestroyVoice();
        return;
    }

    _activeVoices.push_back(ActiveVoice{ voice });
}

void AudioManager::Release()
{
    StopBgm();

    for (ActiveVoice& activeVoice : _activeVoices)
    {
        if (activeVoice.voice != nullptr)
        {
            activeVoice.voice->DestroyVoice();
            activeVoice.voice = nullptr;
        }
    }
    _activeVoices.clear();

    if (_masterVoice != nullptr)
    {
        _masterVoice->DestroyVoice();
        _masterVoice = nullptr;
    }

    if (_engine != nullptr)
    {
        _engine->Release();
        _engine = nullptr;
    }

    if (_mediaFoundationStarted)
    {
        MFShutdown();
        _mediaFoundationStarted = false;
    }

    if (_comInitialized)
    {
        CoUninitialize();
        _comInitialized = false;
    }

    _initialized = false;
}

const AudioManager::SoundData* AudioManager::GetSound(SoundEffect effect) const
{
    switch (effect)
    {
    case SoundEffect::Start:
        return &_startSfx;
    case SoundEffect::Graze:
        return &_grazeSfx;
    case SoundEffect::Hit:
        return &_hitSfx;
    case SoundEffect::Bomb:
        return &_bombSfx;
    case SoundEffect::Star:
        return &_starSfx;
    case SoundEffect::Clear:
        return &_clearSfx;
    case SoundEffect::GameOver:
        return &_gameOverSfx;
    default:
        return nullptr;
    }
}

const AudioManager::SoundData* AudioManager::GetMusic(MusicTrack track) const
{
    switch (track)
    {
    case MusicTrack::Time0To330:
        return &_time0To330Bgm;
    case MusicTrack::Time331To530:
        return &_time331To530Bgm;
    case MusicTrack::Time531To730:
        return &_time531To730Bgm;
    case MusicTrack::Time731ToEnd:
        return &_time731ToEndBgm;
    default:
        return nullptr;
    }
}

void AudioManager::LoadSounds()
{
    LoadAudioFile(L"Assets\\Audio\\bgm_0_330.mp3", _time0To330Bgm);
    LoadAudioFile(L"Assets\\Audio\\bgm_331_530.mp3", _time331To530Bgm);
    LoadAudioFile(L"Assets\\Audio\\bgm_531_730.mp3", _time531To730Bgm);
    LoadAudioFile(L"Assets\\Audio\\bgm_731_end.mp3", _time731ToEndBgm);
    LoadAudioFile(L"Assets\\Audio\\start.mp3", _startSfx);
    LoadAudioFile(L"Assets\\Audio\\graze.mp3", _grazeSfx);
    LoadAudioFile(L"Assets\\Audio\\hit.mp3", _hitSfx);
    LoadAudioFile(L"Assets\\Audio\\bomb.mp3", _bombSfx);
    LoadAudioFile(L"Assets\\Audio\\star.mp3", _starSfx);
    LoadAudioFile(L"Assets\\Audio\\clear.mp3", _clearSfx);
    LoadAudioFile(L"Assets\\Audio\\game_over.mp3", _gameOverSfx);
}

bool AudioManager::LoadAudioFile(const wchar_t* path, SoundData& sound)
{
    sound = SoundData{};
    const std::wstring resolvedPath = ResolveAudioPath(path);

    IMFSourceReader* reader = nullptr;
    IMFMediaType* outputType = nullptr;
    IMFMediaType* actualType = nullptr;
    WAVEFORMATEX* waveFormat = nullptr;
    UINT32 waveFormatSize = 0;

    HRESULT hr = MFCreateSourceReaderFromURL(resolvedPath.c_str(), nullptr, &reader);
    if (FAILED(hr)) goto Cleanup;

    hr = MFCreateMediaType(&outputType);
    if (FAILED(hr)) goto Cleanup;

    hr = outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (FAILED(hr)) goto Cleanup;

    hr = outputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    if (FAILED(hr)) goto Cleanup;

    hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, outputType);
    if (FAILED(hr)) goto Cleanup;

    hr = reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &actualType);
    if (FAILED(hr)) goto Cleanup;

    hr = MFCreateWaveFormatExFromMFMediaType(actualType, &waveFormat, &waveFormatSize);
    if (FAILED(hr)) goto Cleanup;

    sound.formatBytes.resize(waveFormatSize);
    std::memcpy(sound.formatBytes.data(), waveFormat, waveFormatSize);

    while (true)
    {
        DWORD flags = 0;
        IMFSample* sample = nullptr;
        hr = reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &sample);
        if (FAILED(hr))
        {
            SafeRelease(sample);
            break;
        }

        if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0)
        {
            SafeRelease(sample);
            break;
        }

        if (sample != nullptr)
        {
            IMFMediaBuffer* buffer = nullptr;
            hr = sample->ConvertToContiguousBuffer(&buffer);
            if (SUCCEEDED(hr) && buffer != nullptr)
            {
                BYTE* audioData = nullptr;
                DWORD maxLength = 0;
                DWORD currentLength = 0;
                hr = buffer->Lock(&audioData, &maxLength, &currentLength);
                if (SUCCEEDED(hr))
                {
                    const size_t oldSize = sound.audioBytes.size();
                    sound.audioBytes.resize(oldSize + currentLength);
                    std::memcpy(sound.audioBytes.data() + oldSize, audioData, currentLength);
                    buffer->Unlock();
                }
            }

            SafeRelease(buffer);
        }

        SafeRelease(sample);
    }

Cleanup:
    if (waveFormat != nullptr)
        CoTaskMemFree(waveFormat);
    SafeRelease(actualType);
    SafeRelease(outputType);
    SafeRelease(reader);

    return SUCCEEDED(hr) && !sound.formatBytes.empty() && !sound.audioBytes.empty();
}
