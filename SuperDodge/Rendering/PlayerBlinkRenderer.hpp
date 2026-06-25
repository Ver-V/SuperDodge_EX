#pragma once

#include "../Core/Component.hpp"
#include "../Core/GameObject.hpp"
#include "../Components/MeshRenderer.hpp"
#include "../Components/PlayerStatusComponent.hpp"

class PlayerBlinkRenderer : public Component
{
public:
    void Update(float deltaTime) override
    {
        if (owner == nullptr) return;

        PlayerStatusComponent* status = owner->GetComponent<PlayerStatusComponent>();
        MeshRenderer* meshRenderer = owner->GetComponent<MeshRenderer>();

        if (status == nullptr) return;
        if (meshRenderer == nullptr) return;

        meshRenderer->SetVisible(status->IsVisible());
    }
};