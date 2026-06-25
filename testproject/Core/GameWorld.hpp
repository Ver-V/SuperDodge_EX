#pragma once

#include <vector>
#include <memory>
#include "GameObject.hpp"
#include "../Components/ObstacleStatusComponent.hpp"

class Renderer;

class GameWorld
{
private:
    std::vector<std::unique_ptr<GameObject>> _objects;

public:
    GameObject* AddObject(GameObject* object)
    {
        _objects.push_back(std::unique_ptr<GameObject>(object));
        return object;
    }

    void Clear()
    {
        _objects.clear();
    }

    void ClearActiveObstacles()
    {
        for (auto& obj : _objects)
        {
            if (obj->GetComponent<ObstacleStatusComponent>() != nullptr)
            {
                obj->SetActive(false);
            }
        }
    }

    const std::vector<std::unique_ptr<GameObject>>& GetObjects() const { return _objects; }

    void Start()
    {
        for (size_t i = 0; i < _objects.size(); ++i)
            _objects[i]->Start();
    }

    void Update(float deltaTime)
    {
        for (size_t i = 0; i < _objects.size(); ++i)
        {
            if (!_objects[i]->IsActive()) continue;
            _objects[i]->Update(deltaTime);
        }
    }

    void Render(Renderer& renderer)
    {
        for (size_t i = 0; i < _objects.size(); ++i)
        {
            if (!_objects[i]->IsActive()) continue;
            _objects[i]->Render(renderer);
        }
    }
};
