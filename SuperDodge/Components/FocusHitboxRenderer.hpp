#pragma once

#include "../Core/Component.hpp"
#include "../Core/GameObject.hpp"
#include "../Rendering/Renderer.hpp"
#include "PlayerControllerComponent.hpp"
#include "PlayerStatusComponent.hpp"

class FocusHitboxRenderer : public Component
{
public:
    void Render(Renderer& renderer) override
    {
        if (owner == nullptr) return;

        PlayerControllerComponent* controller = owner->GetComponent<PlayerControllerComponent>();
        PlayerStatusComponent* status = owner->GetComponent<PlayerStatusComponent>();

        if (controller == nullptr) return;
        if (status == nullptr) return;
        if (!controller->IsFocusMode()) return;

        renderer.DrawCircle(owner->GetPosition(), status->GetHitboxRadius(), Color(1.0f, 1.0f, 1.0f));
    }
};