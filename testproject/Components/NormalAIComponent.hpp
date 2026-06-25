#pragma once

#include "AIComponent.hpp"

class NormalAIComponent : public AIComponent
{
public:
    void Update(float deltaTime) override
    {
        Move(deltaTime, 1.0f);
        DespawnIfOutOfScreen();
    }
};