#pragma once

class GameObject;
class Renderer;

class Component
{
protected:
    GameObject* owner = nullptr;

public:
    virtual ~Component() = default;

    void SetOwner(GameObject* newOwner)
    {
        owner = newOwner;
    }

    virtual void Start() {}
    virtual void Update(float deltaTime) {}
    virtual void Render(Renderer& renderer) {}
};