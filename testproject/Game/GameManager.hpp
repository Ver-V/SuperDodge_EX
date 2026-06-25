#pragma once

#define NOMINMAX
#include <windows.h>

#include "../Core/GameWorld.hpp"
#include "GameEnums.hpp"
#include "GameConfig.hpp"
#include "BossManager.hpp"
#include "ScoreManager.hpp"
#include "UIManager.hpp"

class Renderer;
class GameObject;
class ObstacleSpawnerComponent;
class InputManager;

class GameManager
{
private:
    GameState _currentState = GameState::Ready;
    GameConfig _config;
    GameWorld _world;

    GameObject* _player = nullptr;
    ObstacleSpawnerComponent* _spawner = nullptr;
    InputManager* _input = nullptr;

    ScoreManager _scoreManager;
    BossManager _bossManager;
    UIManager _uiManager;

    bool _bombFlashRequest = false;
    int _debugBossPhaseIndex = 0;

public:
    void Initialize(HWND hwnd, InputManager* input);
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
};
