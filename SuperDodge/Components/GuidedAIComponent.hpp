#pragma once

#include "AIComponent.hpp"
#include "PlayerStatusComponent.hpp"

class GuidedAIComponent : public AIComponent
{
private:
    float _trackingStrength = 3.0f;
    float _speedMultiplier = 0.80f;
    float _timer = 0.0f;
    float _despawnTime = 2.5f;

public:
    void Initialize(const Vector2& targetPosition, float speed, GameObject* targetObject) override
    {
        AIComponent::Initialize(targetPosition, speed, targetObject);
        _timer = 0.0f;
    }

    void Update(float deltaTime) override
    {
        _timer += deltaTime;

        TrackTarget(deltaTime);
        Move(deltaTime, _speedMultiplier);

        if (_timer >= _despawnTime)
            owner->SetActive(false);

        DespawnIfOutOfScreen();
    }

private:
    void TrackTarget(float deltaTime)
    {
        if (owner == nullptr) return;
        if (target == nullptr) return;

        PlayerStatusComponent* playerStatus = target->GetComponent<PlayerStatusComponent>();
        if (playerStatus != nullptr && playerStatus->IsDead()) return;

        Vector2 ownerPosition = owner->GetPosition();
        Vector2 targetPosition = target->GetPosition();

        Vector2 desiredDirection = Normalize(Vector2(targetPosition.x - ownerPosition.x, targetPosition.y - ownerPosition.y));

        moveDirection.x += (desiredDirection.x - moveDirection.x) * _trackingStrength * deltaTime;
        moveDirection.y += (desiredDirection.y - moveDirection.y) * _trackingStrength * deltaTime;

        moveDirection = Normalize(moveDirection);
    }
};