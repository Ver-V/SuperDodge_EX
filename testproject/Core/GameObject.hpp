#pragma once

#include <vector>
#include <memory>
#include "MathTypes.hpp"
#include "Component.hpp"

class Renderer;

class GameObject
{
private:
    Vector2 _position;
    Vector2 _size;
    bool _isActive = true;
    std::vector<std::unique_ptr<Component>> _components;

public:
    GameObject(const Vector2& position = Vector2(), const Vector2& size = Vector2(10.0f, 10.0f))
        : _position(position), _size(size)
    {
    }

    template<typename T>
    T* AddComponent(T* component)
    {
        component->SetOwner(this);
        _components.push_back(std::unique_ptr<Component>(component));
        return component;
    }

    template<typename T>
    T* GetComponent()
    {
        for (auto& component : _components)
        {
            T* result = dynamic_cast<T*>(component.get());
            if (result != nullptr) return result;
        }

        return nullptr;
    }

    void Start()
    {
        for (auto& component : _components)
            component->Start();
    }

    void Update(float deltaTime)
    {
        for (auto& component : _components)
            component->Update(deltaTime);
    }

    void Render(Renderer& renderer)
    {
        for (auto& component : _components)
            component->Render(renderer);
    }

    void SetPosition(const Vector2& position)
    {
        _position = position;
    }

    void SetSize(const Vector2& size)
    {
        _size = size;
    }

    void SetActive(bool active)
    {
        _isActive = active;
    }

    bool IsActive() const
    {
        return _isActive;
    }

    Vector2 GetPosition() const
    {
        return _position;
    }

    Vector2 GetSize() const
    {
        return _size;
    }

    Rect GetBounds() const
    {
        return Rect{ _position, _size };
    }

    void Translate(const Vector2& delta)
    {
        _position.x += delta.x;
        _position.y += delta.y;
    }
};