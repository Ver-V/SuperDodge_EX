#pragma once

#define NOMINMAX
#include <windows.h>

#include "../Core/GameWorld.hpp"
#include "GameEnums.hpp"
#include "GameConfig.hpp"
#include "BossManager.hpp"
#include "ScoreManager.hpp"
#include "GameRenderResources.hpp"
#include "../Core/AudioManager.hpp"

#include <vector>

class Renderer;
class GameObject;
class ObstacleSpawnerComponent;
class GraphicsContext;
class InputManager;

class GameManager
{
private:
    GameState _currentState = GameState::Ready;
    GameConfig _config;
    GameWorld _world;

    GameObject* _player = nullptr;
    ObstacleSpawnerComponent* _spawner = nullptr;
    InputManager* _inputManager = nullptr;

    ScoreManager _scoreManager;
    BossManager _bossManager;
    GameRenderResources _renderResources;
    AudioManager _audioManager;

    struct GrazeParticle
    {
        Vector2 position;
        Vector2 velocity;
        float age = 0.0f;
        float lifetime = 0.0f;
        float radius = 0.0f;
    };

    std::vector<GrazeParticle> _grazeParticles;
    unsigned int _particleSeed = 0x1234abcd;

    bool _bombFlashRequest = false;
    int _debugBossPhaseIndex = 0;
    int _debugMusicTrackIndex = -1;

public:
    bool Initialize(Renderer& renderer, InputManager* inputManager);
    void Update(float deltaTime);
    void Draw(Renderer& renderer, float deltaTime);

    void StartGame();
    void GameOver();
    void GameClear();
    void RestartGame();

    int GetActiveObjectCount() const;
    int GetTotalObjectCount() const;
    bool IsBossActive() const;

private:
    void FinalizeScore(bool awardClearBonus);
    void ResetPlayer();
    void DrawUI(Renderer& renderer);
    void ClearActiveObstacles();
    void SpawnGrazeParticles(const Vector2& center);
    void UpdateGrazeParticles(float deltaTime);
    void DrawGrazeParticles(Renderer& renderer);
    float NextParticleRandom(float minValue, float maxValue);
    MusicTrack GetGameplayMusicTrack(float survivalTime) const;
    float GetGameplayMusicVolume(float survivalTime) const;
    MusicTrack GetDebugMusicTrack(int trackIndex) const;
    bool InitializeRenderResources(Renderer& renderer);
    bool LoadShapeShader(GraphicsContext* graphics, ShaderSet& shaderSet);
};
