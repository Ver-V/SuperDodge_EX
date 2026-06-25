#pragma once

#include <algorithm>

#include "../Core/Component.hpp"
#include "../Core/GameObject.hpp"
#include "../Core/GameConstants.hpp"
#include "../Core/MathUtils.hpp"

class AIComponent : public Component
{
protected:
    GameObject* target = nullptr;
    Vector2 moveDirection;
    float moveSpeed = 0.0f;

public:
    virtual void Initialize(const Vector2& targetPosition, float speed, GameObject* targetObject)
    {
        target = targetObject;
        moveSpeed = speed;

        if (owner == nullptr) return;

        Vector2 position = owner->GetPosition();
        moveDirection = Normalize(Vector2(targetPosition.x - position.x, targetPosition.y - position.y));
    }

protected:
    void Move(float deltaTime, float speedMultiplier)
    {
        if (owner == nullptr) return;

        Vector2 delta;
        delta.x = moveDirection.x * moveSpeed * speedMultiplier * deltaTime;
        delta.y = moveDirection.y * moveSpeed * speedMultiplier * deltaTime;

        owner->Translate(delta);
    }

    void DespawnIfOutOfScreen()
    {
        if (owner == nullptr) return;

        Vector2 position = owner->GetPosition();
        Vector2 size = owner->GetSize();

        float margin = std::max(size.x, size.y) + 80.0f;

        if (position.x < -margin || position.x > PlayAreaWidth + margin || position.y < -margin || position.y > PlayAreaHeight + margin)
            owner->SetActive(false);
    }
};