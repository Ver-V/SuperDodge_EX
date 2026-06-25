#pragma once

#include "../Rendering/Mesh.hpp"
#include "../Rendering/Material.hpp"
#include "../Rendering/ShaderTypes.hpp"

struct GameRenderResources
{
    ShaderSet shapeShader;
    Material shapeMaterial;

    Mesh playerMesh;
    Mesh normalObstacleMesh;
    Mesh fastObstacleMesh;
    Mesh guidedObstacleMesh;
    Mesh bossMesh;

    GameRenderResources()
    {
        shapeMaterial.SetShaderSet(&shapeShader);
    }

    ~GameRenderResources()
    {
        Release();
    }

    GameRenderResources(const GameRenderResources&) = delete;
    GameRenderResources& operator=(const GameRenderResources&) = delete;

    void Release()
    {
        playerMesh.Release();
        normalObstacleMesh.Release();
        fastObstacleMesh.Release();
        guidedObstacleMesh.Release();
        bossMesh.Release();

        shapeShader.Release();
    }
};