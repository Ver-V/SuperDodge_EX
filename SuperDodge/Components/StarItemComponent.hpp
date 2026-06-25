#pragma once

#include "../Core/Component.hpp"
#include "../Core/GameObject.hpp"
#include "../Rendering/Renderer.hpp"

class StarItemComponent : public Component
{
private:
    float _pickupRadius = 18.0f;

public:
    float GetPickupRadius() const
    {
        return _pickupRadius;
    }

    void Render(Renderer& renderer) override
    {
        if (owner == nullptr)
            return;

        renderer.DrawStar(
            owner->GetPosition(),
            18.0f,
            8.0f,
            Color(1.0f, 0.9f, 0.15f));
    }
};