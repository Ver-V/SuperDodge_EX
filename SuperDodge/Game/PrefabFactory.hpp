#pragma once

#include "../Core/GameObject.hpp"
#include "GameConfig.hpp"

#include "../Components/PlayerStatusComponent.hpp"
#include "../Components/PlayerControllerComponent.hpp"
#include "../Components/ObstacleStatusComponent.hpp"

#include "../Components/NormalAIComponent.hpp"
#include "../Components/FastAIComponent.hpp"
#include "../Components/GuidedAIComponent.hpp"

#include "../Components/StarItemComponent.hpp"

#include "../Components/MeshRenderer.hpp"
#include "../Components/FocusHitboxRenderer.hpp"
#include "../Rendering/PlayerBlinkRenderer.hpp"
#include "GameRenderResources.hpp"

class PrefabFactory
{
public:
    static GameObject* CreatePlayer(const GameConfig& config, GameRenderResources& resources, InputManager* input)
    {
        GameObject* player = new GameObject(Vector2(PlayAreaWidth * 0.5f, PlayAreaHeight * 0.5f), config.playerSize);

        player->AddComponent(new MeshRenderer(&resources.playerMesh, &resources.shapeMaterial));
        player->AddComponent(new FocusHitboxRenderer());
        player->AddComponent(new PlayerStatusComponent());
        player->AddComponent(new PlayerBlinkRenderer());
        player->AddComponent(new PlayerControllerComponent(config.playerMoveSpeed, input));

        PlayerStatusComponent* status = player->GetComponent<PlayerStatusComponent>();
        if (status != nullptr)
            status->Reset(config.playerHp);

        return player;
    }

    static GameObject* CreateNormalObstacle(const GameConfig& config, GameRenderResources& resources)
    {
        const ObstacleData& data = config.normalObstacle;

        GameObject* obstacle = new GameObject(Vector2(-1000.0f, -1000.0f), Vector2(data.size, data.size));

        obstacle->AddComponent(new MeshRenderer(&resources.normalObstacleMesh, &resources.shapeMaterial));
        obstacle->AddComponent(new ObstacleStatusComponent());
        obstacle->AddComponent(new NormalAIComponent());
        obstacle->SetActive(false);

        return obstacle;
    }

    static GameObject* CreateFastObstacle(const GameConfig& config, GameRenderResources& resources)
    {
        const ObstacleData& data = config.fastObstacle;

        GameObject* obstacle = new GameObject(Vector2(-1000.0f, -1000.0f), Vector2(data.size, data.size));

        obstacle->AddComponent(new MeshRenderer(&resources.fastObstacleMesh, &resources.shapeMaterial));
        obstacle->AddComponent(new ObstacleStatusComponent());
        obstacle->AddComponent(new FastAIComponent());
        obstacle->SetActive(false);

        return obstacle;
    }

    static GameObject* CreateGuidedObstacle(const GameConfig& config, GameRenderResources& resources)
    {
        const ObstacleData& data = config.guidedObstacle;

        GameObject* obstacle = new GameObject(Vector2(-1000.0f, -1000.0f), Vector2(data.size, data.size));

        obstacle->AddComponent(new MeshRenderer(&resources.guidedObstacleMesh, &resources.shapeMaterial));
        obstacle->AddComponent(new ObstacleStatusComponent());
        obstacle->AddComponent(new GuidedAIComponent());
        obstacle->SetActive(false);

        return obstacle;
    }

    static GameObject* CreateStarItem()
    {
        GameObject* star = new GameObject(
            Vector2(-1000.0f, -1000.0f),
            Vector2(36.0f, 36.0f));

        star->AddComponent(new StarItemComponent());
        star->SetActive(false);

        return star;
    }
};
