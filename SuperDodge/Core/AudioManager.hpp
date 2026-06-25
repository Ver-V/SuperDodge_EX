#pragma once

#define NOMINMAX
#include <xaudio2.h>

#include <cstdint>
#include <string>
#include <vector>

enum class SoundEffect
{
    Start,
    Graze,
    Hit,
    Bomb,
    Star,
    Clear,
    GameOver
};

enum class MusicTrack
{
    Time0To330,
    Time331To530,
    Time531To730,
    Time731ToEnd
};

class AudioManager
{
private:
    struct SoundData
    {
        std::vector<std::uint8_t> formatBytes;
        std::vector<std::uint8_t> audioBytes;

        const WAVEFORMATEX* GetFormat() const
        {
            return formatBytes.empty()
                ? nullptr
                : reinterpret_cast<const WAVEFORMATEX*>(formatBytes.data());
        }
    };

    struct ActiveVoice
    {
        IXAudio2SourceVoice* voice = nullptr;
    };

    IXAudio2* _engine = nullptr;
    IXAudio2MasteringVoice* _masterVoice = nullptr;
    IXAudio2SourceVoice* _bgmVoice = nullptr;
    float _bgmBaseVolume = 0.31622776f;
    MusicTrack _currentMusicTrack = MusicTrack::Time0To330;
    bool _hasCurrentMusicTrack = false;
    bool _initialized = false;
    bool _comInitialized = false;
    bool _mediaFoundationStarted = false;

    SoundData _time0To330Bgm;
    SoundData _time331To530Bgm;
    SoundData _time531To730Bgm;
    SoundData _time731ToEndBgm;
    SoundData _startSfx;
    SoundData _grazeSfx;
    SoundData _hitSfx;
    SoundData _bombSfx;
    SoundData _starSfx;
    SoundData _clearSfx;
    SoundData _gameOverSfx;
    std::vector<ActiveVoice> _activeVoices;

public:
    AudioManager() = default;
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    bool Initialize();
    void Update();
    void PlayBgm();
    void PlayBgm(MusicTrack track);
    void SetBgmVolume(float volume);
    void StopBgm();
    void PlaySfx(SoundEffect effect);
    void Release();

private:
    const SoundData* GetSound(SoundEffect effect) const;
    const SoundData* GetMusic(MusicTrack track) const;
    void LoadSounds();
    bool LoadAudioFile(const wchar_t* path, SoundData& sound);
};
