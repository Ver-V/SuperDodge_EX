#pragma once

#include <vector>
#include <algorithm>
#include <chrono>

#include "../Core/GameWorld.hpp"
#include "../Core/PerfConfig.hpp"
#include "GameConfig.hpp"
#include "GameEnums.hpp"
#include "PrefabFactory.hpp"
#include "GameRenderResources.hpp"

struct PooledObject
{
    GameObject* object = nullptr;
    ObstacleType type = ObstacleType::Normal;
};

class ObjectPool
{
private:
    std::vector<PooledObject> _pool;
    GameWorld* _world = nullptr;
    GameConfig _config;
    GameRenderResources* _resources = nullptr;

public:
    void Initialize(GameWorld* world, const GameConfig& config, GameRenderResources* resources, int capacity)
    {
        _world = world;
        _config = config;
        _resources = resources;
        _pool.clear();

#if USE_OBJECT_POOL
        int perTypeCount = std::max(1, capacity / 3);

        for (int i = 0; i < perTypeCount; ++i)
        {
            AddPooledObject(ObstacleType::Normal);
            AddPooledObject(ObstacleType::Fast);
            AddPooledObject(ObstacleType::Guided);
        }
#else
        (void)capacity;
#endif
    }

    GameObject* GetObject(ObstacleType type)
    {
#if ENABLE_FRAME_LOG
        const auto getStart = std::chrono::high_resolution_clock::now();
        GameObject* result = FindOrCreate(type);
        FrameCounters& counters = GetFrameCounters();
        ++counters.poolGetCalls;
        counters.poolGetMs += std::chrono::duration<double, std::milli>(
            std::chrono::high_resolution_clock::now() - getStart).count();
        return result;
#else
        return FindOrCreate(type);
#endif
    }

    void Reset()
    {
        for (size_t i = 0; i < _pool.size(); ++i)
        {
            if (_pool[i].object != nullptr)
                _pool[i].object->SetActive(false);
        }
    }

    // 풀 미사용 모드(USE_OBJECT_POOL 0)에서만 비활성 오브젝트를 월드에서 delete
    void ReleaseInactive()
    {
#if !USE_OBJECT_POOL
        if (_world == nullptr) return;

        std::vector<GameObject*> inactiveObjects;

        auto newEnd = std::remove_if(_pool.begin(), _pool.end(),
            [&inactiveObjects](const PooledObject& pooledObject)
            {
                if (pooledObject.object == nullptr) return true;
                if (pooledObject.object->IsActive()) return false;

                inactiveObjects.push_back(pooledObject.object);
                return true;
            });
        _pool.erase(newEnd, _pool.end());

        _world->DestroyObjects(inactiveObjects);
        GetFrameCounters().objectsDestroyed += static_cast<int>(inactiveObjects.size());
#endif
    }

private:
    GameObject* FindOrCreate(ObstacleType type)
    {
#if USE_OBJECT_POOL
        for (size_t i = 0; i < _pool.size(); ++i)
        {
#if ENABLE_FRAME_LOG
            ++GetFrameCounters().poolScanSteps;
#endif
            if (_pool[i].object == nullptr) continue;
            if (_pool[i].type != type) continue;
            if (_pool[i].object->IsActive()) continue;

            return _pool[i].object;
        }
#endif

        return AddPooledObject(type);
    }

    GameObject* AddPooledObject(ObstacleType type)
    {
        if (_world == nullptr) return nullptr;
        if (_resources == nullptr) return nullptr;

        GameObject* object = nullptr;

        if (type == ObstacleType::Normal)
            object = PrefabFactory::CreateNormalObstacle(_config, *_resources);
        else if (type == ObstacleType::Fast)
            object = PrefabFactory::CreateFastObstacle(_config, *_resources);
        else
            object = PrefabFactory::CreateGuidedObstacle(_config, *_resources);

        object = _world->AddObject(object);
        ++GetFrameCounters().objectsCreated;

        PooledObject pooledObject;
        pooledObject.object = object;
        pooledObject.type = type;
        _pool.push_back(pooledObject);

        return object;
    }
};
