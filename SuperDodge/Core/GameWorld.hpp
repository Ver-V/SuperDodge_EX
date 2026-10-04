#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include "GameObject.hpp"

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

    // Update/Render 루프 밖에서만 호출 (순회 중 erase하면 인덱스가 밀림)
    void DestroyObjects(const std::vector<GameObject*>& targets)
    {
        if (targets.empty()) return;

        _objects.erase(
            std::remove_if(_objects.begin(), _objects.end(),
                [&targets](const std::unique_ptr<GameObject>& object)
                {
                    return std::find(targets.begin(), targets.end(), object.get()) != targets.end();
                }),
            _objects.end());
    }

    void Clear()
    {
        _objects.clear();
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