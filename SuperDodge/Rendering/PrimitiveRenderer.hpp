#pragma once

#define NOMINMAX
#include <d3d11.h>

#include "../Core/MathTypes.hpp"
#include "PrimitiveMesh.hpp"

class GraphicsContext;

// PrimitiveRenderer는 UI, 오버레이, 디버그 표시처럼
// 매 프레임 간단한 도형을 즉시 그리는 용도로만 사용한다.
// Player, Obstacle, Boss 같은 게임 오브젝트 본체 렌더링에는 사용하지 않는다.
class PrimitiveRenderer
{
private:
    static constexpr unsigned int VertexBufferCapacity = 4096;

    GraphicsContext* _graphics = nullptr;
    ID3D11VertexShader* _vertexShader = nullptr;
    ID3D11PixelShader* _pixelShader = nullptr;
    ID3D11InputLayout* _inputLayout = nullptr;
    ID3D11Buffer* _vertexBuffer = nullptr;

public:
    PrimitiveRenderer() = default;
    ~PrimitiveRenderer();

    PrimitiveRenderer(const PrimitiveRenderer&) = delete;
    PrimitiveRenderer& operator=(const PrimitiveRenderer&) = delete;

    bool Initialize(GraphicsContext* graphics);

    void DrawMesh(const PrimitiveMesh& mesh);
    void DrawRect(const Vector2& center, const Vector2& size, const Color& color);
    void DrawCircle(const Vector2& center, float radius, const Color& color);
    void DrawStar(const Vector2& center, float outerRadius, float innerRadius, const Color& color);
    void DrawHeart(const Vector2& center, float size, const Color& color);

    void Release();

private:
    bool CreateShaders();
    bool CreateVertexBuffer();
    void DrawVertices(const Vertex* vertices, unsigned int vertexCount, D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
};