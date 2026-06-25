#pragma once

#include <cmath>

#include "../Core/GameConstants.hpp"
#include "../Core/GameObject.hpp"
#include "../Core/GameWorld.hpp"
#include "../Components/AIComponent.hpp"
#include "../Components/ObstacleStatusComponent.hpp"
#include "../Components/MeshRenderer.hpp"
#include "GameConfig.hpp"
#include "GameEnums.hpp"
#include "ObjectPool.hpp"
#include "GameRenderResources.hpp"

enum class BossEvent
{
    None,
    Started,
    Ended,
    FinalCleared
};

enum class BossState
{
    Waiting,
    Active
};

enum class BossPhase
{
    None,
    Phase1,
    Phase2,
    Phase3,
    Phase4,
    Phase5,
    Phase6,
    Phase7,
    Phase8,
    Phase9,
    Final
};

struct BossPhaseConfig
{
    BossPhase phase;
    float spawnTime;
    float duration;
    float shotInterval;
    float projectileSpeed;
    int projectileCount;
    int requiredGraze;
    bool allowGrazeClear;
};

class BossManager
{
private:
    static constexpr float Pi = 3.14159265f;
    static constexpr int PhaseCount = 10;

    GameWorld* _world = nullptr;
    GameObject* _target = nullptr;
    GameObject* _bossObject = nullptr;
    ObjectPool _projectilePool;
    GameRenderResources* _resources = nullptr;

    BossState _state = BossState::Waiting;
    BossPhase _phase = BossPhase::None;
    int _nextPhaseIndex = 0;
    float _bossElapsed = 0.0f;
    float _shotTimer = 0.0f;
    int _burstIndex = 0;
    int _grazeCount = 0;

public:
    void Initialize(GameWorld* world, const GameConfig& config, GameObject* target, GameRenderResources* resources)
    {
        _world = world;
        _target = target;
        _resources = resources;
        _projectilePool.Initialize(world, config, _resources, 540);

        _bossObject = new GameObject(Vector2(PlayAreaWidth * 0.5f, 90.0f), Vector2(140.0f, 70.0f));

        if (_resources != nullptr)
            _bossObject->AddComponent(new MeshRenderer(&_resources->bossMesh, &_resources->shapeMaterial));

        _bossObject->SetActive(false);
        _world->AddObject(_bossObject);

        Reset();
    }

    void Reset()
    {
        _state = BossState::Waiting;
        _phase = BossPhase::None;
        _nextPhaseIndex = 0;
        _bossElapsed = 0.0f;
        _shotTimer = 0.0f;
        _burstIndex = 0;
        _grazeCount = 0;

        _projectilePool.Reset();
        if (_bossObject != nullptr)
            _bossObject->SetActive(false);
    }

    void Stop()
    {
        _state = BossState::Waiting;
        _phase = BossPhase::None;
        _projectilePool.Reset();

        if (_bossObject != nullptr)
            _bossObject->SetActive(false);
    }

    BossEvent Update(float deltaTime, float survivalTime)
    {
        if (_state == BossState::Waiting)
        {
            const BossPhaseConfig* phaseConfigs = GetPhaseConfigs();
            if (_nextPhaseIndex < PhaseCount &&
                survivalTime >= phaseConfigs[_nextPhaseIndex].spawnTime)
            {
                StartPhase(phaseConfigs[_nextPhaseIndex]);
                ++_nextPhaseIndex;
                return BossEvent::Started;
            }

            return BossEvent::None;
        }

        const BossPhaseConfig& config = GetCurrentConfig();
        _bossElapsed += deltaTime;
        _shotTimer += deltaTime;

        UpdateBossPosition();

        while (_shotTimer >= config.shotInterval)
        {
            _shotTimer -= config.shotInterval;
            ExecutePattern(config);
        }

        if (_bossElapsed >= config.duration)
        {
            const bool wasFinal = _phase == BossPhase::Final;
            EndPhase();
            return wasFinal ? BossEvent::FinalCleared : BossEvent::Ended;
        }

        return BossEvent::None;
    }

    bool RegisterGraze()
    {
        if (_state != BossState::Active) return false;

        const BossPhaseConfig& config = GetCurrentConfig();
        if (!config.allowGrazeClear)
            return false;

        ++_grazeCount;
        if (_grazeCount < config.requiredGraze)
            return false;

        EndPhase();
        return true;
    }

    bool IsActive() const
    {
        return _state == BossState::Active;
    }

    bool IsFinalPhase() const
    {
        return _phase == BossPhase::Final;
    }

    BossPhase GetPhase() const
    {
        return _phase;
    }

    int GetPhaseNumber() const
    {
        switch (_phase)
        {
        case BossPhase::Phase1: return 1;
        case BossPhase::Phase2: return 2;
        case BossPhase::Phase3: return 3;
        case BossPhase::Phase4: return 4;
        case BossPhase::Phase5: return 5;
        case BossPhase::Phase6: return 6;
        case BossPhase::Phase7: return 7;
        case BossPhase::Phase8: return 8;
        case BossPhase::Phase9: return 9;
        case BossPhase::Final: return 10;
        default: return 0;
        }
    }

    int GetGrazeCount() const
    {
        return _grazeCount;
    }

    int GetGrazeTarget() const
    {
        return IsActive() ? GetCurrentConfig().requiredGraze : 0;
    }

    float GetRemainingTime() const
    {
        if (!IsActive()) return 0.0f;
        return (std::max)(0.0f, GetCurrentConfig().duration - _bossElapsed);
    }

    float GetNextPhaseRemainingTime(float survivalTime) const
    {
        if (_state != BossState::Waiting || _nextPhaseIndex >= PhaseCount)
            return -1.0f;

        const BossPhaseConfig* phaseConfigs = GetPhaseConfigs();
        return phaseConfigs[_nextPhaseIndex].spawnTime - survivalTime;
    }

    bool IsWarningTime(float survivalTime) const
    {
        const float remainingTime = GetNextPhaseRemainingTime(survivalTime);
        return remainingTime > 0.0f && remainingTime <= 5.0f;
    }

    void ForceStartPhase(int phaseIndex)
    {
        if (phaseIndex < 0 || phaseIndex >= PhaseCount)
            return;

        const BossPhaseConfig* phaseConfigs = GetPhaseConfigs();
        StartPhase(phaseConfigs[phaseIndex]);
        _nextPhaseIndex = phaseIndex + 1;
    }

private:
    static const BossPhaseConfig* GetPhaseConfigs()
    {
        static const BossPhaseConfig phaseConfigs[PhaseCount] = {
            { BossPhase::Phase1,  60.0f, 30.0f, 0.82f, 230.0f, 10, 30, true },
            { BossPhase::Phase2, 120.0f, 30.0f, 0.68f, 250.0f, 12, 30, true },
            { BossPhase::Phase3, 180.0f, 30.0f, 0.58f, 275.0f,  7, 30, true },
            { BossPhase::Phase4, 240.0f, 30.0f, 0.50f, 300.0f, 12, 30, true },
            { BossPhase::Phase5, 300.0f, 30.0f, 0.46f, 320.0f, 14, 30, true },
            { BossPhase::Phase6, 360.0f, 30.0f, 0.42f, 330.0f, 14, 30, true },
            { BossPhase::Phase7, 420.0f, 30.0f, 0.38f, 345.0f, 16, 30, true },
            { BossPhase::Phase8, 480.0f, 30.0f, 0.35f, 360.0f, 18, 30, true },
            { BossPhase::Phase9, 540.0f, 30.0f, 0.32f, 375.0f, 18, 30, true },
            { BossPhase::Final,  600.0f, 30.0f, 0.28f, 395.0f, 20,  0, false }
        };

        return phaseConfigs;
    }

    const BossPhaseConfig& GetCurrentConfig() const
    {
        const BossPhaseConfig* phaseConfigs = GetPhaseConfigs();
        for (int i = 0; i < PhaseCount; ++i)
        {
            if (phaseConfigs[i].phase == _phase)
                return phaseConfigs[i];
        }

        return phaseConfigs[0];
    }

    void StartPhase(const BossPhaseConfig& config)
    {
        _state = BossState::Active;
        _phase = config.phase;
        _bossElapsed = 0.0f;
        _shotTimer = config.shotInterval;
        _burstIndex = 0;
        _grazeCount = 0;
        _projectilePool.Reset();

        if (_bossObject != nullptr)
        {
            _bossObject->SetPosition(Vector2(PlayAreaWidth * 0.5f, 90.0f));
            _bossObject->SetSize(
                _phase == BossPhase::Final
                    ? Vector2(180.0f, 90.0f)
                    : Vector2(140.0f, 70.0f));
            _bossObject->SetActive(true);
        }
    }

    void EndPhase()
    {
        _state = BossState::Waiting;
        _phase = BossPhase::None;
        _projectilePool.Reset();

        if (_bossObject != nullptr)
            _bossObject->SetActive(false);
    }

    void UpdateBossPosition()
    {
        if (_bossObject == nullptr) return;

        const float centerX = PlayAreaWidth * 0.5f;
        float frequency = 1.0f;
        float moveRange = PlayAreaWidth * 0.30f;

        if (_phase == BossPhase::Phase3)
            frequency = 1.7f;
        else if (_phase == BossPhase::Phase4)
            frequency = 2.0f;
        else if (_phase == BossPhase::Phase5)
        {
            frequency = 2.2f;
            moveRange = PlayAreaWidth * 0.32f;
        }
        else if (_phase == BossPhase::Phase6)
        {
            frequency = 2.4f;
            moveRange = PlayAreaWidth * 0.33f;
        }
        else if (_phase == BossPhase::Phase7)
        {
            frequency = 2.7f;
            moveRange = PlayAreaWidth * 0.34f;
        }
        else if (_phase == BossPhase::Phase8)
        {
            frequency = 3.0f;
            moveRange = PlayAreaWidth * 0.35f;
        }
        else if (_phase == BossPhase::Phase9)
        {
            frequency = 3.2f;
            moveRange = PlayAreaWidth * 0.36f;
        }
        else if (_phase == BossPhase::Final)
        {
            frequency = 3.4f;
            moveRange = PlayAreaWidth * 0.36f;
        }

        const float x = centerX + std::sin(_bossElapsed * frequency) * moveRange;
        _bossObject->SetPosition(Vector2(x, 90.0f));
    }

    void ExecutePattern(const BossPhaseConfig& config)
    {
        constexpr float BossSpeedScale = 2.0f / 3.0f;

        switch (config.phase)
        {
        case BossPhase::Phase1:
            SpawnRadial(config.projectileCount, config.projectileSpeed, AlternatingOffset(config.projectileCount));
            break;

        case BossPhase::Phase2:
            SpawnRadial(config.projectileCount, config.projectileSpeed, _burstIndex * 0.16f);
            break;

        case BossPhase::Phase3:
            SpawnAimedFan(config.projectileCount, config.projectileSpeed, 0.16f);
            break;

        case BossPhase::Phase4:
            if (_burstIndex % 2 == 0)
                SpawnRadial(config.projectileCount, config.projectileSpeed, _burstIndex * 0.12f);
            else
                SpawnAimedFan(7, config.projectileSpeed + 20.0f, 0.13f);
            break;

        case BossPhase::Phase5:
            SpawnRadial(config.projectileCount, config.projectileSpeed, _burstIndex * 0.14f);
            if (_burstIndex % 2 == 0)
                SpawnAimedFan(7, config.projectileSpeed + 30.0f, 0.12f, ObstacleType::Fast);
            break;

        case BossPhase::Phase6:
        {
            const float speed = config.projectileSpeed * BossSpeedScale;
            SpawnRadial(config.projectileCount, speed, _burstIndex * 0.15f);
            SpawnAimedFan(7, (config.projectileSpeed + 25.0f) * BossSpeedScale, 0.12f, ObstacleType::Fast);
            if (_burstIndex % 2 == 0)
                SpawnGuidedFan(5, (config.projectileSpeed - 30.0f) * BossSpeedScale, 0.18f);
            break;
        }

        case BossPhase::Phase7:
        {
            const float speed = config.projectileSpeed * BossSpeedScale;
            SpawnRadial(config.projectileCount, speed, _burstIndex * 0.18f);
            SpawnAimedFan(9, (config.projectileSpeed + 30.0f) * BossSpeedScale, 0.11f, ObstacleType::Fast);
            SpawnGuidedFan(5, (config.projectileSpeed - 20.0f) * BossSpeedScale, 0.16f);
            break;
        }

        case BossPhase::Phase8:
        {
            const float speed = config.projectileSpeed * BossSpeedScale;
            SpawnRadial(config.projectileCount, speed, _burstIndex * 0.20f);
            SpawnAimedFan(9, (config.projectileSpeed + 35.0f) * BossSpeedScale, 0.10f, ObstacleType::Fast);
            SpawnGuidedFan(7, (config.projectileSpeed - 25.0f) * BossSpeedScale, 0.14f);
            break;
        }

        case BossPhase::Phase9:
        {
            const float speed = config.projectileSpeed * BossSpeedScale;
            SpawnRadial(config.projectileCount, speed, _burstIndex * 0.22f);
            SpawnAimedFan(9, (config.projectileSpeed + 40.0f) * BossSpeedScale, 0.10f, ObstacleType::Fast);
            SpawnGuidedFan(7, (config.projectileSpeed - 20.0f) * BossSpeedScale, 0.13f);
            if (_burstIndex % 2 == 0)
                SpawnCrossBurst(4, 3, (config.projectileSpeed + 15.0f) * BossSpeedScale, _burstIndex * 0.10f, BossSpeedScale);
            break;
        }

        case BossPhase::Final:
        {
            const float speed = config.projectileSpeed * BossSpeedScale;
            SpawnRadial(config.projectileCount, speed, _burstIndex * 0.20f);
            SpawnAimedFan(11, (config.projectileSpeed + 45.0f) * BossSpeedScale, 0.09f, ObstacleType::Fast);
            SpawnGuidedFan(7, (config.projectileSpeed - 25.0f) * BossSpeedScale, 0.12f);
            SpawnCrossBurst(4, 3, (config.projectileSpeed + 20.0f) * BossSpeedScale, _burstIndex * 0.12f, BossSpeedScale);
            break;
        }

        default:
            break;
        }

        ++_burstIndex;
    }

    float AlternatingOffset(int projectileCount) const
    {
        return (_burstIndex % 2 == 0) ? 0.0f : Pi / projectileCount;
    }

    void SpawnRadial(int projectileCount, float speed, float angleOffset)
    {
        SpawnRadial(projectileCount, speed, angleOffset, ObstacleType::Normal);
    }

    void SpawnRadial(int projectileCount, float speed, float angleOffset, ObstacleType type)
    {
        for (int i = 0; i < projectileCount; ++i)
        {
            const float angle = angleOffset + (2.0f * Pi * i / projectileCount);
            SpawnProjectile(type, Vector2(std::cos(angle), std::sin(angle)), speed);
        }
    }

    void SpawnAimedFan(int projectileCount, float speed, float angleStep)
    {
        SpawnAimedFan(projectileCount, speed, angleStep, ObstacleType::Normal);
    }

    void SpawnAimedFan(int projectileCount, float speed, float angleStep, ObstacleType type)
    {
        if (_bossObject == nullptr || _target == nullptr) return;

        const Vector2 spawnPosition = _bossObject->GetPosition();
        const Vector2 targetPosition = _target->GetPosition();
        const float centerAngle = std::atan2(
            targetPosition.y - spawnPosition.y,
            targetPosition.x - spawnPosition.x);
        const float centerIndex = (projectileCount - 1) * 0.5f;

        for (int i = 0; i < projectileCount; ++i)
        {
            const float angle = centerAngle + (i - centerIndex) * angleStep;
            SpawnProjectile(type, Vector2(std::cos(angle), std::sin(angle)), speed);
        }
    }

    void SpawnGuidedFan(int projectileCount, float speed, float angleStep)
    {
        SpawnAimedFan(projectileCount, speed, angleStep, ObstacleType::Guided);
    }

    void SpawnCrossBurst(int rayCount, int bulletsPerRay, float baseSpeed, float angleOffset)
    {
        SpawnCrossBurst(rayCount, bulletsPerRay, baseSpeed, angleOffset, 1.0f);
    }

    void SpawnCrossBurst(int rayCount, int bulletsPerRay, float baseSpeed, float angleOffset, float speedStepScale)
    {
        for (int ray = 0; ray < rayCount; ++ray)
        {
            const float angle = angleOffset + (2.0f * Pi * ray / rayCount);
            const Vector2 direction(std::cos(angle), std::sin(angle));

            for (int bullet = 0; bullet < bulletsPerRay; ++bullet)
                SpawnProjectile(ObstacleType::Fast, direction, baseSpeed + bullet * 42.0f * speedStepScale);
        }
    }

    void SpawnProjectile(ObstacleType type, const Vector2& direction, float speed)
    {
        if (_bossObject == nullptr) return;

        const Vector2 spawnPosition = _bossObject->GetPosition();
        GameObject* projectile = _projectilePool.GetObject(type);
        if (projectile == nullptr) return;

        Vector2 projectileSize(24.0f, 24.0f);
        if (type == ObstacleType::Fast)
            projectileSize = Vector2(18.0f, 18.0f);
        else if (type == ObstacleType::Guided)
            projectileSize = Vector2(28.0f, 28.0f);

        if (_phase == BossPhase::Final)
        {
            projectileSize.x -= 2.0f;
            projectileSize.y -= 2.0f;
        }

        projectile->SetPosition(spawnPosition);
        projectile->SetSize(projectileSize);

        ObstacleStatusComponent* status = projectile->GetComponent<ObstacleStatusComponent>();
        if (status != nullptr)
        {
            status->ResetGraze();
            status->SetBossProjectile(true);
        }

        AIComponent* ai = projectile->GetComponent<AIComponent>();
        if (ai != nullptr)
        {
            const Vector2 targetPosition(
                spawnPosition.x + direction.x * 100.0f,
                spawnPosition.y + direction.y * 100.0f);
            ai->Initialize(targetPosition, speed, _target);
        }

        projectile->SetActive(true);
    }
};
