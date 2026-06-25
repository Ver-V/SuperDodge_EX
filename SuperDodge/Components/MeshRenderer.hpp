#pragma once

#define NOMINMAX
#include <d3d11.h>
#include <DirectXMath.h>

#include "../Core/Component.hpp"
#include "../Core/GameObject.hpp"
#include "../Core/GameConstants.hpp"
#include "../Rendering/Renderer.hpp"
#include "../Rendering/GraphicsContext.hpp"
#include "../Rendering/Mesh.hpp"
#include "../Rendering/Material.hpp"
#include "../Rendering/ShaderTypes.hpp"

class MeshRenderer : public Component
{
private:
    Mesh* _mesh = nullptr;
    Material* _material = nullptr;
    ID3D11Buffer* _constantBuffer = nullptr;
    float _rotation = 0.0f;
    bool _isVisible = true;

public:
    MeshRenderer(Mesh* mesh, Material* material)
        : _mesh(mesh), _material(material)
    {
    }

    ~MeshRenderer()
    {
        if (_constantBuffer != nullptr)
        {
            _constantBuffer->Release();
            _constantBuffer = nullptr;
        }
    }

    MeshRenderer(const MeshRenderer&) = delete;
    MeshRenderer& operator=(const MeshRenderer&) = delete;

    void SetMesh(Mesh* mesh)
    {
        _mesh = mesh;
    }

    void SetMaterial(Material* material)
    {
        _material = material;
    }

    void SetRotation(float rotation)
    {
        _rotation = rotation;
    }

    void SetVisible(bool visible)
    {
        _isVisible = visible;
    }

    bool IsVisible() const
    {
        return _isVisible;
    }

    void Render(Renderer& renderer) override
    {
        if (!_isVisible) return;
        if (owner == nullptr) return;
        if (_mesh == nullptr) return;
        if (_material == nullptr) return;

        GraphicsContext* graphics = renderer.GetGraphicsContext();
        if (graphics == nullptr) return;

        ID3D11Device* device = graphics->GetDevice();
        ID3D11DeviceContext* context = graphics->GetContext();
        if (device == nullptr || context == nullptr) return;

        if (_constantBuffer == nullptr)
        {
            D3D11_BUFFER_DESC desc = {};
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.ByteWidth = sizeof(ConstantBuffer);
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

            HRESULT hr = device->CreateBuffer(&desc, nullptr, &_constantBuffer);
            if (FAILED(hr)) return;
        }

        Vector2 position = owner->GetPosition();
        Vector2 size = owner->GetSize();

        float ndcX = (position.x / (ScreenWidth * 0.5f)) - 1.0f;
        float ndcY = 1.0f - (position.y / (ScreenHeight * 0.5f));

        float scaleX = size.x * 2.0f / ScreenWidth;
        float scaleY = size.y * 2.0f / ScreenHeight;

        DirectX::XMMATRIX world = DirectX::XMMatrixScaling(scaleX, scaleY, 1.0f) * DirectX::XMMatrixRotationZ(_rotation) * DirectX::XMMatrixTranslation(ndcX, ndcY, 0.0f);

        ConstantBuffer constantBuffer;
        constantBuffer.matWorld = DirectX::XMMatrixTranspose(world);

        context->UpdateSubresource(_constantBuffer, 0, nullptr, &constantBuffer, 0, 0);
        context->VSSetConstantBuffers(0, 1, &_constantBuffer);

        _material->Bind(context);

        ID3D11Buffer* vertexBuffer = _mesh->GetVertexBuffer();
        unsigned int stride = sizeof(Vertex);
        unsigned int offset = 0;

        context->IASetPrimitiveTopology(_mesh->GetTopology());
        context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        context->Draw(_mesh->GetVertexCount(), 0);
    }
};