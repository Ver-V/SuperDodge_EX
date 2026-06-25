#pragma once

#define NOMINMAX
#include <windows.h>

#include "../Core/GameWorld.hpp"
#include "GameEnums.hpp"
#include "GameConfig.hpp"
#include "BossManager.hpp"
#include "ScoreManager.hpp"
#include "GameRenderResources.hpp"

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

    bool _bombFlashRequest = false;
    int _debugBossPhaseIndex = 0;

public:
    bool Initialize(Renderer& renderer, InputManager* inputManager);
    void Update(float deltaTime);
    void Draw(Renderer& renderer, float deltaTime);

    void StartGame();
    void GameOver();
    void GameClear();
    void RestartGame();

private:
    void FinalizeScore(bool awardClearBonus);
    void ResetPlayer();
    void DrawUI(Renderer& renderer);
    void ClearActiveObstacles();
    bool InitializeRenderResources(Renderer& renderer);
    bool LoadShapeShader(GraphicsContext* graphics, ShaderSet& shaderSet);
};
