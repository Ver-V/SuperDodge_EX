#pragma once

#include "../Core/Component.hpp"
#include "../Core/GameObject.hpp"
#include "../Core/MathUtils.hpp"
#include "../Rendering/Renderer.hpp"
#include "PlayerControllerComponent.hpp"
#include "PlayerStatusComponent.hpp"

class RectRendererComponent : public Component
{
private:
    Color _color;

public:
    RectRendererComponent(const Color& color) : _color(color)
    {
    }

    void SetColor(const Color& color)
    {
        _color = color;
    }

    void Render(Renderer& renderer) override
    {
        if (owner == nullptr) return;

        auto* status = owner->GetComponent<PlayerStatusComponent>();
        if (status != nullptr && !status->IsVisible())
            return;

        // 기본 기체 그리기
        const float radius = GetCircumscribedRadius(owner->GetSize());
        renderer.DrawCircle(owner->GetPosition(), radius, _color);

        // 피탄점 표시
        auto* controller = owner->GetComponent<PlayerControllerComponent>();

        if (controller != nullptr && status != nullptr && controller->IsFocusMode())
        {
            renderer.DrawCircle(
                owner->GetPosition(),
                status->GetHitboxRadius(),
                Color(1.0f, 1.0f, 1.0f));
        }
    }
};
