#pragma once

#define NOMINMAX
#include <windows.h>
#include <cmath>

#include "../Core/Component.hpp"
#include "../Core/GameObject.hpp"
#include "../Core/GameConstants.hpp"
#include "../Core/InputManager.hpp"
#include "../Core/MathUtils.hpp"
#include "PlayerStatusComponent.hpp"

class PlayerControllerComponent : public Component
{
private:
    float _moveSpeed = 0.0f;
    bool _isFocusMode = false;
    InputManager* _input = nullptr;

public:
    PlayerControllerComponent(float moveSpeed, InputManager* input)
        : _moveSpeed(moveSpeed), _input(input)
    {
    }

    bool IsFocusMode() const { return _isFocusMode; }

    void Update(float deltaTime) override
    {
        if (owner == nullptr || _input == nullptr) return;

        PlayerStatusComponent* status = owner->GetComponent<PlayerStatusComponent>();
        if (status != nullptr && status->IsDead()) return;

        _isFocusMode = _input->IsKeyDown(VK_SHIFT);
        float currentSpeed = _isFocusMode ? _moveSpeed * 0.4f : _moveSpeed;

        Vector2 input;
        if (_input->IsKeyDown('A') || _input->IsKeyDown(VK_LEFT)) input.x -= 1.0f;
        if (_input->IsKeyDown('D') || _input->IsKeyDown(VK_RIGHT)) input.x += 1.0f;
        if (_input->IsKeyDown('W') || _input->IsKeyDown(VK_UP)) input.y -= 1.0f;
        if (_input->IsKeyDown('S') || _input->IsKeyDown(VK_DOWN)) input.y += 1.0f;

        float length = std::sqrt(input.x * input.x + input.y * input.y);
        if (length > 0.0f)
        {
            input.x /= length;
            input.y /= length;
        }

        Vector2 position = owner->GetPosition();
        Vector2 size = owner->GetSize();

        position.x += input.x * currentSpeed * deltaTime;
        position.y += input.y * currentSpeed * deltaTime;

        float halfW = size.x * 0.5f;
        float halfH = size.y * 0.5f;

        // PlayArea 내로 제한
        position.x = ClampFloat(position.x, halfW, static_cast<float>(PlayAreaWidth) - halfW);
        position.y = ClampFloat(position.y, halfH, static_cast<float>(PlayAreaHeight) - halfH);

        owner->SetPosition(position);
    }
};
