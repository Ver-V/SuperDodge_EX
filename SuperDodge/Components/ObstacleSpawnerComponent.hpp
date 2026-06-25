#pragma once

#include <random>
#include <chrono>
#include <algorithm>
#include <vector>

#include "../Core/Component.hpp"
#include "../Core/GameObject.hpp"
#include "../Core/GameWorld.hpp"
#include "../Core/GameConstants.hpp"
#include "../Core/MathUtils.hpp"

#include "../Game/GameConfig.hpp"
#include "../Game/GameEnums.hpp"
#include "../Game/ObstacleData.hpp"
#include "../Game/ObjectPool.hpp"
#include "../Game/GameRenderResources.hpp"
#include "../Game/PrefabFactory.hpp"

#include "AIComponent.hpp"
#include "ObstacleStatusComponent.hpp"

class ObstacleSpawnerComponent : public Component
{
private:
    GameWorld* _world = nullptr;
    GameObject* _target = nullptr;
    GameConfig _config;
    ObjectPool _objectPool;
    std::vector<GameObject*> _starPool;
    GameRenderResources* _resources = nullptr;

    float _spawnTimer = 0.0f;
    float _currentSpawnInterval = 1.00f;
    float _currentObstacleSpeed = 180.0f;
    float _spawnIntervalMultiplier = 1.0f;
    bool _isSpawning = false;

    std::mt19937 _randomEngine;

public:
    ObstacleSpawnerComponent(GameWorld* world, const GameConfig& config, GameObject* target, GameRenderResources* resources)
        : _world(world), _target(target), _config(config), _resources(resources)
    {
        _randomEngine.seed(static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count()));

        _objectPool.Initialize(_world, _config, _resources, 150);
        InitializeStarPool(20);

        Reset();
    }

    void Reset()
    {
        _spawnTimer = 0.0f;
        _currentSpawnInterval = _config.spawnStartInterval;
        _currentObstacleSpeed = _config.obstacleStartSpeed;
        _spawnIntervalMultiplier = 1.0f;
        _isSpawning = false;

        _objectPool.Reset();
        ResetStars();
    }

    void StartSpawn()
    {
        _isSpawning = true;
    }

    void StopSpawn()
    {
        _isSpawning = false;
        ResetStars();
    }

    void SetSpawnCountScale(float scale)
    {
        _spawnIntervalMultiplier = scale > 0.0f ? 1.0f / scale : 1.0f;
    }

    void Update(float deltaTime) override
    {
        if (!_isSpawning) return;
        if (_target == nullptr) return;

        _spawnTimer += deltaTime;

        const float effectiveSpawnInterval = _currentSpawnInterval * _spawnIntervalMultiplier;
        if (_spawnTimer >= effectiveSpawnInterval)
        {
            _spawnTimer = 0.0f;
            SpawnObstacle();
            TrySpawnStar();
        }

        IncreaseDifficulty(deltaTime);
    }

private:
    void SpawnObstacle()
    {
        std::uniform_real_distribution<float> xDist(0.0f, static_cast<float>(PlayAreaWidth));
        std::uniform_real_distribution<float> yDist(0.0f, static_cast<float>(PlayAreaHeight));
        std::uniform_real_distribution<float> aimOffsetDist(-80.0f, 80.0f);
        std::uniform_int_distribution<int> sideDist(0, 3);

        ObstacleType obstacleType = PickObstacleType();
        const ObstacleData& obstacleData = GetObstacleData(obstacleType);

        float size = obstacleData.size;
        float halfSize = size * 0.5f;
        int side = sideDist(_randomEngine);

        Vector2 spawnPosition;

        if (side == 0) // Top
            spawnPosition = Vector2(xDist(_randomEngine), -halfSize);
        else if (side == 1) // Bottom
            spawnPosition = Vector2(xDist(_randomEngine), PlayAreaHeight + halfSize);
        else if (side == 2) // Left
            spawnPosition = Vector2(-halfSize, yDist(_randomEngine));
        else // Right
            spawnPosition = Vector2(PlayAreaWidth + halfSize, yDist(_randomEngine));

        Vector2 targetPosition = _target->GetPosition();
        targetPosition.x = ClampFloat(targetPosition.x + aimOffsetDist(_randomEngine), 0.0f, static_cast<float>(PlayAreaWidth));
        targetPosition.y = ClampFloat(targetPosition.y + aimOffsetDist(_randomEngine), 0.0f, static_cast<float>(PlayAreaHeight));

        GameObject* obstacle = _objectPool.GetObject(obstacleType);
        if (obstacle == nullptr) return;

        obstacle->SetPosition(spawnPosition);
        obstacle->SetSize(Vector2(size, size));

        ObstacleStatusComponent* obstacleStatus = obstacle->GetComponent<ObstacleStatusComponent>();
        if (obstacleStatus != nullptr)
            obstacleStatus->ResetGraze();

        obstacle->SetActive(true);

        AIComponent* ai = obstacle->GetComponent<AIComponent>();
        if (ai != nullptr)
            ai->Initialize(targetPosition, _currentObstacleSpeed, _target);
    }

    ObstacleType PickObstacleType()
    {
        std::uniform_int_distribution<int> typeDist(0, 99);
        int typeValue = typeDist(_randomEngine);

        if (typeValue < 70)
            return ObstacleType::Normal;

        if (typeValue < 90)
            return ObstacleType::Fast;

        return ObstacleType::Guided;
    }

    const ObstacleData& GetObstacleData(ObstacleType type) const
    {
        if (type == ObstacleType::Normal)
            return _config.normalObstacle;

        if (type == ObstacleType::Fast)
            return _config.fastObstacle;

        return _config.guidedObstacle;
    }

    void IncreaseDifficulty(float deltaTime)
    {
        _currentSpawnInterval = std::max(_config.spawnMinInterval, _currentSpawnInterval - 0.005f * deltaTime);
        _currentObstacleSpeed = std::min(_config.obstacleMaxSpeed, _currentObstacleSpeed + 1.2f * deltaTime);
    }

    void InitializeStarPool(int capacity)
    {
        if (_world == nullptr)
            return;

        _starPool.reserve(capacity);
        for (int i = 0; i < capacity; ++i)
            _starPool.push_back(_world->AddObject(PrefabFactory::CreateStarItem()));
    }

    void ResetStars()
    {
        for (GameObject* star : _starPool)
        {
            if (star != nullptr)
                star->SetActive(false);
        }
    }

    void TrySpawnStar()
    {
        std::uniform_int_distribution<int> chanceDist(0, 99);
        if (chanceDist(_randomEngine) >= 5)
            return;

        GameObject* availableStar = nullptr;
        for (GameObject* star : _starPool)
        {
            if (star != nullptr && !star->IsActive())
            {
                availableStar = star;
                break;
            }
        }

        if (availableStar == nullptr)
            return;

        const float margin = 30.0f;
        std::uniform_real_distribution<float> xDist(
            margin,
            static_cast<float>(PlayAreaWidth) - margin);
        std::uniform_real_distribution<float> yDist(
            margin,
            static_cast<float>(PlayAreaHeight) - margin);

        availableStar->SetPosition(Vector2(xDist(_randomEngine), yDist(_randomEngine)));
        availableStar->SetActive(true);
    }
};
