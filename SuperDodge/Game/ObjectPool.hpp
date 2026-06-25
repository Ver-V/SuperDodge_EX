#pragma once

#include <vector>
#include <algorithm>

#include "../Core/GameWorld.hpp"
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

        int perTypeCount = std::max(1, capacity / 3);

        for (int i = 0; i < perTypeCount; ++i)
        {
            AddPooledObject(ObstacleType::Normal);
            AddPooledObject(ObstacleType::Fast);
            AddPooledObject(ObstacleType::Guided);
        }
    }

    GameObject* GetObject(ObstacleType type)
    {
        for (size_t i = 0; i < _pool.size(); ++i)
        {
            if (_pool[i].object == nullptr) continue;
            if (_pool[i].type != type) continue;
            if (_pool[i].object->IsActive()) continue;

            return _pool[i].object;
        }

        return AddPooledObject(type);
    }

    void Reset()
    {
        for (size_t i = 0; i < _pool.size(); ++i)
        {
            if (_pool[i].object != nullptr)
                _pool[i].object->SetActive(false);
        }
    }

private:
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

        PooledObject pooledObject;
        pooledObject.object = object;
        pooledObject.type = type;
        _pool.push_back(pooledObject);

        return object;
    }
};
