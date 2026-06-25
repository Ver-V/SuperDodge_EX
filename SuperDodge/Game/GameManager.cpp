#include "GameManager.hpp"

#include "../Rendering/Renderer.hpp"
#include "../Core/GameObject.hpp"
#include "../Core/GameConstants.hpp"
#include "../Core/InputManager.hpp"
#include "../Core/MathUtils.hpp"

#include "PrefabFactory.hpp"
#include "../Components/ObstacleSpawnerComponent.hpp"
#include "../Components/PlayerStatusComponent.hpp"
#include "../Components/ObstacleStatusComponent.hpp"
#include "../Components/StarItemComponent.hpp"


#include "../Rendering/GraphicsContext.hpp"

#include <d3dcompiler.h>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")
#include <cmath>
#include <algorithm>
#include <string>

namespace
{
    template<typename T>
    void SafeRelease(T*& resource)
    {
        if (resource == nullptr) return;
        resource->Release();
        resource = nullptr;
    }

    void OutputShaderError(ID3DBlob* errorBlob)
    {
        if (errorBlob == nullptr) return;
        OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
    }

    DirectX::XMFLOAT4 ToFloat4(const Color& color)
    {
        return DirectX::XMFLOAT4(color.r, color.g, color.b, color.a);
    }

    std::vector<Vertex> CreateCircleVertices(int segmentCount, const Color& color)
    {
        std::vector<Vertex> vertices;
        vertices.reserve(segmentCount * 3);

        constexpr float Pi = 3.1415926535f;
        const float radius = 0.5f;
        DirectX::XMFLOAT4 vertexColor = ToFloat4(color);

        for (int i = 0; i < segmentCount; ++i)
        {
            float angle0 = (static_cast<float>(i) / segmentCount) * Pi * 2.0f;
            float angle1 = (static_cast<float>(i + 1) / segmentCount) * Pi * 2.0f;

            vertices.push_back(Vertex{ DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), vertexColor });
            vertices.push_back(Vertex{ DirectX::XMFLOAT3(std::cos(angle1) * radius, std::sin(angle1) * radius, 0.0f), vertexColor });
            vertices.push_back(Vertex{ DirectX::XMFLOAT3(std::cos(angle0) * radius, std::sin(angle0) * radius, 0.0f), vertexColor });
        }

        return vertices;
    }

    std::vector<Vertex> CreateRectVertices(const Color& color)
    {
        DirectX::XMFLOAT4 vertexColor = ToFloat4(color);

        return {
            Vertex{ DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f), vertexColor },
            Vertex{ DirectX::XMFLOAT3(-0.5f,  0.5f, 0.0f), vertexColor },
            Vertex{ DirectX::XMFLOAT3(0.5f,  0.5f, 0.0f), vertexColor },

            Vertex{ DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f), vertexColor },
            Vertex{ DirectX::XMFLOAT3(0.5f,  0.5f, 0.0f), vertexColor },
            Vertex{ DirectX::XMFLOAT3(0.5f, -0.5f, 0.0f), vertexColor },
        };
    }
}


bool GameManager::Initialize(Renderer& renderer, InputManager* inputManager)
{
    if (!InitializeRenderResources(renderer)) return false;
    _inputManager = inputManager;
    _world.Clear();

    _player = _world.AddObject(PrefabFactory::CreatePlayer(_config, _renderResources, inputManager));

    GameObject* spawnerObject = new GameObject(Vector2(0.0f, 0.0f), Vector2(1.0f, 1.0f));
    _spawner = spawnerObject->AddComponent(new ObstacleSpawnerComponent(&_world, _config, _player, &_renderResources));
    _world.AddObject(spawnerObject);

    _bossManager.Initialize(&_world, _config, _player, &_renderResources);

    _world.Start();
    _scoreManager.Initialize();
    _scoreManager.ResetScore();
    _currentState = GameState::Ready;

    return true;
}

void GameManager::Update(float deltaTime)
{
    if (_inputManager == nullptr)
        return;

    if (_currentState == GameState::Ready && _inputManager->IsKeyPressed(VK_SPACE))
    {
        StartGame();
        return;
    }

    if ((_currentState == GameState::GameOver || _currentState == GameState::GameClear) &&
        (_inputManager->IsKeyPressed(VK_SPACE) || _inputManager->IsKeyPressed('R')))
    {
        RestartGame();
        return;
    }

    if (_currentState != GameState::Playing)
        return;

    if (_inputManager->IsKeyPressed(VK_F5))
    {
        _bossManager.ForceStartPhase(_debugBossPhaseIndex);
        _debugBossPhaseIndex = (_debugBossPhaseIndex + 1) % 10;

        if (_spawner != nullptr)
            _spawner->SetSpawnCountScale(1.0f / 3.0f);
    }

    // 폭탄 사용
    if (_inputManager->IsKeyPressed('X'))
    {
        PlayerStatusComponent* status = _player->GetComponent<PlayerStatusComponent>();
        if (status != nullptr && status->UseBomb())
        {
            ClearActiveObstacles();
            _bombFlashRequest = true;
        }
    }

    _scoreManager.UpdateScore(deltaTime);
    BossEvent bossEvent = _bossManager.Update(deltaTime, _scoreManager.GetSurvivalTime());
    if (bossEvent == BossEvent::Started)
    {
        if (_spawner != nullptr)
            _spawner->SetSpawnCountScale(1.0f / 3.0f);
    }
    else if (bossEvent == BossEvent::Ended)
    {
        if (_spawner != nullptr)
            _spawner->SetSpawnCountScale(1.0f);
    }
    else if (bossEvent == BossEvent::FinalCleared)
    {
        GameClear();
        return;
    }

    if (_scoreManager.IsTimeUp() && !_bossManager.IsActive())
    {
        GameClear();
        return;
    }

    _world.Update(deltaTime);

    // 죽음 or 그레이즈
    PlayerStatusComponent* playerStatus = _player->GetComponent<PlayerStatusComponent>();
    if (playerStatus != nullptr && !playerStatus->IsDead())
    {
        Vector2 playerPos = _player->GetPosition();
        Vector2 playerSize = _player->GetSize();

        const auto& objects = _world.GetObjects();
        for (const auto& obj : objects)
        {
            if (!obj->IsActive() || obj.get() == _player)
                continue;

            StarItemComponent* starItem = obj->GetComponent<StarItemComponent>();
            if (starItem != nullptr)
            {
                if (IsCircleOverlap(playerPos, GetCircumscribedRadius(playerSize), obj->GetPosition(), starItem->GetPickupRadius()))
                {
                    playerStatus->CollectStar();
                    obj->SetActive(false);
                }
                continue;
            }

            ObstacleStatusComponent* obstacleStatus = obj->GetComponent<ObstacleStatusComponent>();
            if (obstacleStatus == nullptr)
                continue;

            Vector2 obsPos = obj->GetPosition();
            Vector2 obsSize = obj->GetSize();

            const float playerHitboxRadius = playerStatus->GetHitboxRadius();
            const float playerGrazeRadius = GetCircumscribedRadius(playerSize);
            const float obstacleHitboxRadius = obstacleStatus->GetHitboxRadius(obsSize);

            if (IsCircleOverlap(playerPos, playerHitboxRadius, obsPos, obstacleHitboxRadius))
            {
                bool damaged = playerStatus->TakeDamage(obstacleStatus->GetDamage());

                if (!damaged) continue;

                obj->SetActive(false);

                if (playerStatus->IsDead())
                {
                    GameOver();
                    return;
                }

                continue;
            }

            if (!obstacleStatus->HasGrazed() &&
                IsCircleOverlap(
                    playerPos,
                    playerGrazeRadius,
                    obsPos,
                    obstacleHitboxRadius))
            {
                _scoreManager.AddGrazeScore();
                obstacleStatus->MarkGrazed();

                if (obstacleStatus->IsBossProjectile() && _bossManager.RegisterGraze())
                {
                    if (_spawner != nullptr)
                        _spawner->SetSpawnCountScale(1.0f);
                    break;
                }
            }
        }
    }

}

void GameManager::Draw(Renderer& renderer, float deltaTime)
{
    if (_bombFlashRequest)
    {
        renderer.SetFlash(1.0f);
        _bombFlashRequest = false;
    }

    renderer.UpdateFlash(deltaTime);

    renderer.Clear(0.05f, 0.05f, 0.1f);

    _world.Render(renderer);

    if (_currentState == GameState::GameOver)
    {
        renderer.DrawRect(Vector2(PlayAreaWidth * 0.5f, PlayAreaHeight * 0.5f), 
                          Vector2((float)PlayAreaWidth, (float)PlayAreaHeight), 
                          Color(1.0f, 0.0f, 0.0f, 0.4f));
    }
    else if (_currentState == GameState::GameClear)
    {
        renderer.DrawRect(Vector2(PlayAreaWidth * 0.5f, PlayAreaHeight * 0.5f),
                          Vector2((float)PlayAreaWidth, (float)PlayAreaHeight),
                          Color(0.1f, 0.4f, 1.0f, 0.35f));
    }

    DrawUI(renderer);
}

void GameManager::StartGame()
{
    _currentState = GameState::Playing;
    _debugBossPhaseIndex = 0;
    ClearActiveObstacles();
    _bossManager.Reset();
    ResetPlayer();
    if (_spawner != nullptr)
    {
        _spawner->Reset();
        _spawner->StartSpawn();
    }
    _scoreManager.ResetScore();
}

void GameManager::GameOver()
{
    _currentState = GameState::GameOver;
    FinalizeScore(false);

    if (_spawner != nullptr)
        _spawner->StopSpawn();

    _bossManager.Stop();

    if (_player != nullptr)
        _player->SetActive(false);
}

void GameManager::GameClear()
{
    _currentState = GameState::GameClear;
    FinalizeScore(true);

    if (_spawner != nullptr)
        _spawner->StopSpawn();

    _bossManager.Stop();
    ClearActiveObstacles();
}

void GameManager::RestartGame()
{
    StartGame();
}

void GameManager::FinalizeScore(bool awardClearBonus)
{
    int remainingBombs = 0;
    int remainingLives = 0;

    if (_player != nullptr)
    {
        PlayerStatusComponent* status = _player->GetComponent<PlayerStatusComponent>();
        if (status != nullptr) 
        {
            remainingBombs = status->GetBombCount();
            remainingLives = status->GetHp();
        }
            
    }

    _scoreManager.FinalizeScore(remainingBombs, remainingLives, awardClearBonus);
}

void GameManager::ResetPlayer()
{
    if (_player == nullptr) return;
    _player->SetPosition(Vector2(PlayAreaWidth * 0.5f, PlayAreaHeight * 0.5f));
    _player->SetSize(_config.playerSize);
    _player->SetActive(true);
    PlayerStatusComponent* status = _player->GetComponent<PlayerStatusComponent>();
    if (status != nullptr) status->Reset(_config.playerHp);
}

void GameManager::DrawUI(Renderer& renderer)
{
    // UI
    renderer.DrawRect(Vector2(PlayAreaWidth + UIAreaWidth * 0.5f, ScreenHeight * 0.5f), 
                      Vector2((float)UIAreaWidth, (float)ScreenHeight), 
                      Color(0.1f, 0.1f, 0.12f));

    // 구분선
    renderer.DrawRect(Vector2(PlayAreaWidth, ScreenHeight * 0.5f), 
                      Vector2(4.0f, (float)ScreenHeight), 
                      Color(0.25f, 0.25f, 0.3f));

    renderer.BeginText();

    float uiLeft = PlayAreaWidth + 25.0f;

    // Score
    renderer.DrawString(L"SCORE", Vector2(uiLeft, 40.0f), 18.0f, Color(0.7f, 0.7f, 0.7f));
    renderer.DrawString(std::to_wstring(_scoreManager.GetScore()), Vector2(uiLeft, 65.0f), 32.0f, Color(1.0f, 1.0f, 1.0f));

    // High score
    renderer.DrawString(L"HIGH SCORE", Vector2(uiLeft, 115.0f), 18.0f, Color(0.7f, 0.7f, 0.7f));
    renderer.DrawString(std::to_wstring(_scoreManager.GetHighScore()), Vector2(uiLeft, 140.0f), 28.0f, Color(1.0f, 0.85f, 0.2f));

    // Timer
    renderer.DrawString(L"TIME", Vector2(uiLeft, 195.0f), 18.0f, Color(0.7f, 0.7f, 0.7f));
    int totalSeconds = static_cast<int>(_scoreManager.GetSurvivalTime());
    std::wstring timeStr = std::to_wstring(totalSeconds / 60) + L":" + (totalSeconds % 60 < 10 ? L"0" : L"") + std::to_wstring(totalSeconds % 60);
    renderer.DrawString(timeStr, Vector2(uiLeft, 220.0f), 32.0f, Color(1.0f, 1.0f, 1.0f));

    PlayerStatusComponent* status = _player->GetComponent<PlayerStatusComponent>();
    if (status != nullptr)
    {
        // Life
        renderer.DrawString(L"LIFE", Vector2(uiLeft, 275.0f), 18.0f, Color(0.9f, 0.35f, 0.35f));
        for (int i = 0; i < status->GetHp(); ++i)
        {
            renderer.DrawHeart(
                Vector2(uiLeft + 13.0f + i * 30.0f, 315.0f),
                25.0f,
                Color(1.0f, 0.15f, 0.2f));
        }

        renderer.DrawString(
            L"STAR " + std::to_wstring(status->GetCollectedStars()) + L"/3",
            Vector2(uiLeft, 340.0f),
            16.0f,
            Color(1.0f, 0.85f, 0.2f));

        // Bombs
        renderer.DrawString(L"BOMBS", Vector2(uiLeft, 385.0f), 18.0f, Color(0.7f, 0.7f, 0.7f));
        int bombs = status->GetBombCount();
        for (int i = 0; i < bombs; ++i)
        {
            renderer.DrawStar(
                Vector2(uiLeft + 12.0f + i * 30.0f, 425.0f),
                12.0f,
                5.5f,
                Color(0.2f, 0.9f, 0.2f));
        }
    }

    if (_bossManager.IsActive())
    {
        renderer.DrawString(
            L"BOSS PHASE " + std::to_wstring(_bossManager.GetPhaseNumber()),
            Vector2(uiLeft, 480.0f),
            18.0f,
            Color(1.0f, 0.3f, 0.8f));
        renderer.DrawString(
            std::to_wstring(static_cast<int>(std::ceil(_bossManager.GetRemainingTime()))),
            Vector2(uiLeft, 505.0f),
            32.0f,
            Color(1.0f, 0.4f, 0.8f));

        if (_bossManager.IsFinalPhase())
        {
            renderer.DrawString(
                L"SURVIVE",
                Vector2(uiLeft, 550.0f),
                18.0f,
                Color(1.0f, 0.7f, 0.9f));
        }
        else
        {
            renderer.DrawString(
                L"GRAZE " + std::to_wstring(_bossManager.GetGrazeCount()) +
                    L"/" + std::to_wstring(_bossManager.GetGrazeTarget()),
                Vector2(uiLeft, 440.0f),
                18.0f,
                Color(1.0f, 0.7f, 0.9f));
        }
    }

    if (_currentState == GameState::Playing &&
        _bossManager.IsWarningTime(_scoreManager.GetSurvivalTime()) &&
        static_cast<int>(_scoreManager.GetSurvivalTime() * 4.0f) % 2 == 0)
    {
        const int remainingTime = static_cast<int>(
            std::ceil(_bossManager.GetNextPhaseRemainingTime(_scoreManager.GetSurvivalTime())));
        renderer.DrawCenteredString(L"WARNING", 70.0f, 54.0f, Color(1.0f, 0.0f, 0.0f));
        renderer.DrawCenteredString(
            L"BOSS IN " + std::to_wstring(remainingTime),
            120.0f,
            24.0f,
            Color(1.0f, 0.35f, 0.35f));
    }
    else if (_currentState == GameState::GameOver || _currentState == GameState::GameClear)
    {
        const bool isGameClear = _currentState == GameState::GameClear;
        const Color titleColor = isGameClear
            ? Color(0.2f, 0.8f, 1.0f)
            : Color(1.0f, 0.1f, 0.1f);

        renderer.DrawCenteredString(
            isGameClear ? L"GAME CLEAR" : L"GAME OVER",
            70.0f,
            56.0f,
            titleColor);


        renderer.DrawCenteredString(L"FINAL SCORE  " + std::to_wstring(_scoreManager.GetScore()),
            130.0f,
            30.0f,
            Color(1.0f, 1.0f, 1.0f));
        
        const int lastRank = _scoreManager.GetLastRank();
        renderer.DrawCenteredString(
            lastRank > 0
            ? L"YOUR RANK  #" + std::to_wstring(lastRank)
            : L"YOUR RANK  OUT OF TOP 10",
            170.0f,
            24.0f,
            lastRank > 0 ? Color(1.0f, 0.85f, 0.2f) : Color(0.75f, 0.75f, 0.75f));

        renderer.DrawCenteredString(L"TOP 10", 220.0f, 30.0f, Color(1.0f, 0.85f, 0.2f));

        const int rankingCount = _scoreManager.GetRankingCount();
        for (int i = 0; i < 10; ++i)
        {
            const int rank = i + 1;
            const bool isCurrentScore = rank == lastRank;
            const std::wstring scoreText = i < rankingCount
                ? std::to_wstring(_scoreManager.GetRankingScore(i))
                : L"---";
            const std::wstring rankingText =
                (isCurrentScore ? L"> " : L"  ") +
                std::to_wstring(rank) + L".  " + scoreText +
                (isCurrentScore ? L" <" : L"");

            renderer.DrawCenteredString(
                rankingText,
                265.0f + i * 55.0f,
                isCurrentScore ? 28.0f : 24.0f,
                isCurrentScore
                ? Color(1.0f, 0.85f, 0.2f)
                : Color(0.9f, 0.9f, 0.9f));
        }

        renderer.DrawCenteredString(
            L"SURVIVAL +" + std::to_wstring(_scoreManager.GetSurvivalScore()) +
            L"   BONUS +" + std::to_wstring(_scoreManager.GetBonusScore()),
            790.0f,
            20.0f,
            Color(0.85f, 0.85f, 0.85f));

        if (isGameClear)
        {
            renderer.DrawCenteredString(
                L"BOMB BONUS +" + std::to_wstring(_scoreManager.GetBombBonusScore()),
                820.0f,
                22.0f,
                Color(0.4f, 0.9f, 1.0f));
            renderer.DrawCenteredString(
                L"LIFE BONUS +" + std::to_wstring(_scoreManager.GetLifeBonusScore()),
                850.0f,
                22.0f,
                Color(1.0f, 0.35f, 0.4f));
        }

        renderer.DrawCenteredString(
            L"FINAL SCORE  " + std::to_wstring(_scoreManager.GetScore()),
            isGameClear ? 885.0f : 835.0f,
            24.0f, Color(1.0f, 0.85f, 0.2f));

        renderer.DrawCenteredString(
            L"PRESS SPACE TO RESTART",
            950.0f,
            26.0f,
            Color(1.0f, 1.0f, 1.0f));
    }
    else if (_currentState == GameState::Ready)
    {
        renderer.DrawCenteredString(L"SUPER DODGE", 360.0f, 64.0f, Color(0.25f, 0.75f, 1.0f));
        renderer.DrawCenteredString(L"PRESS SPACE TO START", 455.0f, 32.0f, Color(1.0f, 1.0f, 1.0f));
        renderer.DrawCenteredString(L"MOVE  WASD / ARROW", 530.0f, 22.0f, Color(0.85f, 0.85f, 0.85f));
        renderer.DrawCenteredString(L"FOCUS  SHIFT     BOMB  X", 565.0f, 22.0f, Color(0.85f, 0.85f, 0.85f));
        renderer.DrawCenteredString(L"COLLECT 3 STARS TO GAIN 1 LIFE", 600.0f, 20.0f, Color(1.0f, 0.85f, 0.2f));
        renderer.DrawCenteredString(L"GRAZE BOSS SHOTS TO CLEAR PHASES", 635.0f, 20.0f, Color(1.0f, 0.7f, 0.9f));
    }

    renderer.EndText();
}

void GameManager::ClearActiveObstacles()
{
    const auto& objects = _world.GetObjects();

    for (const auto& obj : objects)
    {
        if (obj->GetComponent<ObstacleStatusComponent>() == nullptr) continue;
        obj->SetActive(false);
    }
}

bool GameManager::InitializeRenderResources(Renderer& renderer)
{
    GraphicsContext* graphics = renderer.GetGraphicsContext();
    if (graphics == nullptr) return false;

    _renderResources.Release();
    _renderResources.shapeMaterial.SetShaderSet(&_renderResources.shapeShader);

    if (!LoadShapeShader(graphics, _renderResources.shapeShader)) return false;

    const int circleSegmentCount = 48;

    if (!_renderResources.playerMesh.Create(graphics, CreateCircleVertices(circleSegmentCount, Color(0.25f, 0.75f, 1.0f)))) return false;
    if (!_renderResources.normalObstacleMesh.Create(graphics, CreateCircleVertices(circleSegmentCount, _config.normalObstacle.color))) return false;
    if (!_renderResources.fastObstacleMesh.Create(graphics, CreateCircleVertices(circleSegmentCount, _config.fastObstacle.color))) return false;
    if (!_renderResources.guidedObstacleMesh.Create(graphics, CreateCircleVertices(circleSegmentCount, _config.guidedObstacle.color))) return false;
    if (!_renderResources.bossMesh.Create(graphics, CreateRectVertices(Color(0.9f, 0.15f, 0.75f)))) return false;

    return true;
}

bool GameManager::LoadShapeShader(GraphicsContext* graphics, ShaderSet& shaderSet)
{
    if (graphics == nullptr) return false;

    ID3D11Device* device = graphics->GetDevice();
    if (device == nullptr) return false;

    shaderSet.Release();

    ID3DBlob* vertexShaderBlob = nullptr;
    ID3DBlob* pixelShaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#ifdef _DEBUG
    compileFlags |= D3DCOMPILE_DEBUG;
#endif

    HRESULT hr = D3DCompileFromFile(L"vs.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", compileFlags, 0, &vertexShaderBlob, &errorBlob);
    if (FAILED(hr))
    {
        OutputShaderError(errorBlob);
        SafeRelease(errorBlob);
        SafeRelease(vertexShaderBlob);
        return false;
    }

    hr = D3DCompileFromFile(L"ps.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", compileFlags, 0, &pixelShaderBlob, &errorBlob);
    if (FAILED(hr))
    {
        OutputShaderError(errorBlob);
        SafeRelease(errorBlob);
        SafeRelease(vertexShaderBlob);
        SafeRelease(pixelShaderBlob);
        return false;
    }

    hr = device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &shaderSet.vertexShader);
    if (FAILED(hr))
    {
        SafeRelease(vertexShaderBlob);
        SafeRelease(pixelShaderBlob);
        shaderSet.Release();
        return false;
    }

    hr = device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &shaderSet.pixelShader);
    if (FAILED(hr))
    {
        SafeRelease(vertexShaderBlob);
        SafeRelease(pixelShaderBlob);
        shaderSet.Release();
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = device->CreateInputLayout(inputLayoutDesc, 2, vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &shaderSet.inputLayout);
    if (FAILED(hr))
    {
        SafeRelease(vertexShaderBlob);
        SafeRelease(pixelShaderBlob);
        shaderSet.Release();
        return false;
    }

    SafeRelease(vertexShaderBlob);
    SafeRelease(pixelShaderBlob);
    SafeRelease(errorBlob);

    return true;
}
