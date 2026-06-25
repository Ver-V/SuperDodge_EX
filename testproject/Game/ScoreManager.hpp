#pragma once

#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <array>
#include <functional>
#include <string>

class ScoreManager
{
private:
    static constexpr int RankingCapacity = 10;
    static constexpr DWORD RankingFileMagic = 0x4B4E4152; // "RANK"
    static constexpr DWORD RankingFileVersion = 1;

    float _survivalTime = 0.0f;
    int _bonusScore = 0;
    int _bombBonusScore = 0;
    int _lifeBonusScore = 0;
    int _highScore = 0;
    std::array<int, RankingCapacity> _rankings = {};
    int _rankingCount = 0;
    int _lastRank = 0;
    bool _isFinalized = false;

public:
    static constexpr int BombBonusPerCount = 10000;
    static constexpr int LifeBonusPerCount = 10000;

    void Initialize()
    {
        LoadHighScore();
    }

    void ResetScore()
    {
        _survivalTime = 0.0f;
        _bonusScore = 0;
        _bombBonusScore = 0;
        _lifeBonusScore = 0;
        _lastRank = 0;
        _isFinalized = false;
    }

    void UpdateScore(float deltaTime)
    {
        _survivalTime = (std::min)(_survivalTime + deltaTime, GetGameDuration());
    }

    void AddGrazeScore()
    {
        _bonusScore += 50;
    }

    void AddScore(int amount)
    {
        _bonusScore += amount;
    }

    void FinalizeScore(int remainingBombs, int remainingLives, bool awardClearBonus)
    {
        if (_isFinalized) return;

        _bombBonusScore = awardClearBonus
            ? (std::max)(0, remainingBombs) * BombBonusPerCount
            : 0;
        _lifeBonusScore = awardClearBonus
            ? (std::max)(0, remainingLives) * LifeBonusPerCount
            : 0;
        _isFinalized = true;

        RegisterRankingScore(GetScore());
    }

    int GetScore() const
    {
        return static_cast<int>(_survivalTime * 10.0f) +
            _bonusScore +
            _bombBonusScore +
            _lifeBonusScore;
    }

    int GetSurvivalScore() const
    {
        return static_cast<int>(_survivalTime * 10.0f);
    }

    int GetBonusScore() const
    {
        return _bonusScore;
    }

    float GetSurvivalTime() const
    {
        return _survivalTime;
    }

    int GetBombBonusScore() const
    {
        return _bombBonusScore;
    }

    int GetLifeBonusScore() const
    {
        return _lifeBonusScore;
    }

    int GetHighScore() const
    {
        return _highScore;
    }

    int GetRankingCount() const
    {
        return _rankingCount;
    }

    int GetRankingScore(int index) const
    {
        if (index < 0 || index >= _rankingCount)
            return 0;

        return _rankings[index];
    }

    int GetLastRank() const
    {
        return _lastRank;
    }

    bool IsTimeUp() const
    {
        return _survivalTime >= GetGameDuration();
    }

    static constexpr float GetGameDuration()
    {
        return 630.0f;
    }

private:
    static std::wstring GetHighScorePath()
    {
        wchar_t modulePath[MAX_PATH] = {};
        DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
        std::wstring path(modulePath, length);

        size_t separator = path.find_last_of(L"\\/");
        if (separator != std::wstring::npos)
            path.resize(separator + 1);
        else
            path.clear();

        return path + L"highscore.dat";
    }

    void LoadHighScore()
    {
        HANDLE file = CreateFileW(
            GetHighScorePath().c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (file == INVALID_HANDLE_VALUE)
            return;

        struct RankingFileData
        {
            DWORD magic;
            DWORD version;
            DWORD count;
            int scores[RankingCapacity];
        };

        RankingFileData data = {};
        DWORD bytesRead = 0;
        BOOL success = ReadFile(file, &data, sizeof(data), &bytesRead, nullptr);
        CloseHandle(file);

        if (!success)
            return;

        // Old versions stored only one integer in highscore.dat.
        if (bytesRead == sizeof(int))
        {
            int storedScore = 0;
            CopyMemory(&storedScore, &data, sizeof(storedScore));
            if (storedScore >= 0)
            {
                _rankings[0] = storedScore;
                _rankingCount = 1;
                _highScore = storedScore;
            }
            return;
        }

        if (bytesRead != sizeof(data) ||
            data.magic != RankingFileMagic ||
            data.version != RankingFileVersion ||
            data.count > RankingCapacity)
        {
            return;
        }

        _rankingCount = static_cast<int>(data.count);
        for (int i = 0; i < _rankingCount; ++i)
            _rankings[i] = (std::max)(0, data.scores[i]);

        std::sort(_rankings.begin(), _rankings.begin() + _rankingCount, std::greater<int>());
        _highScore = _rankingCount > 0 ? _rankings[0] : 0;
    }

    void SaveHighScore() const
    {
        HANDLE file = CreateFileW(
            GetHighScorePath().c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (file == INVALID_HANDLE_VALUE)
            return;

        struct RankingFileData
        {
            DWORD magic;
            DWORD version;
            DWORD count;
            int scores[RankingCapacity];
        };

        RankingFileData data = {};
        data.magic = RankingFileMagic;
        data.version = RankingFileVersion;
        data.count = static_cast<DWORD>(_rankingCount);
        for (int i = 0; i < _rankingCount; ++i)
            data.scores[i] = _rankings[i];

        DWORD bytesWritten = 0;
        WriteFile(file, &data, sizeof(data), &bytesWritten, nullptr);
        CloseHandle(file);
    }

    void RegisterRankingScore(int score)
    {
        int insertIndex = 0;
        while (insertIndex < _rankingCount && _rankings[insertIndex] >= score)
            ++insertIndex;

        if (insertIndex >= RankingCapacity)
        {
            _lastRank = 0;
            return;
        }

        int newCount = (std::min)(_rankingCount + 1, RankingCapacity);
        for (int i = newCount - 1; i > insertIndex; --i)
            _rankings[i] = _rankings[i - 1];

        _rankings[insertIndex] = score;
        _rankingCount = newCount;
        _lastRank = insertIndex + 1;
        _highScore = _rankings[0];
        SaveHighScore();
    }
};
