#pragma once

#define NOMINMAX
#include <d3d11.h>
#include <vector>

#include "../Core/MathTypes.hpp"
#include "GraphicsContext.hpp"

class Mesh
{
private:
    ID3D11Buffer* _vertexBuffer = nullptr;
    unsigned int _vertexCount = 0;
    D3D11_PRIMITIVE_TOPOLOGY _topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

public:
    Mesh() = default;

    ~Mesh()
    {
        Release();
    }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    bool Create(GraphicsContext* graphics, const std::vector<Vertex>& vertices, D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
    {
        if (graphics == nullptr) return false;
        if (vertices.empty()) return false;

        Release();

        _vertexCount = static_cast<unsigned int>(vertices.size());
        _topology = topology;

        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.ByteWidth = sizeof(Vertex) * _vertexCount;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA data = {};
        data.pSysMem = vertices.data();

        HRESULT hr = graphics->GetDevice()->CreateBuffer(&desc, &data, &_vertexBuffer);
        return SUCCEEDED(hr);
    }

    void Release()
    {
        if (_vertexBuffer != nullptr)
        {
            _vertexBuffer->Release();
            _vertexBuffer = nullptr;
        }

        _vertexCount = 0;
        _topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }

    ID3D11Buffer* GetVertexBuffer() const
    {
        return _vertexBuffer;
    }

    unsigned int GetVertexCount() const
    {
        return _vertexCount;
    }

    D3D11_PRIMITIVE_TOPOLOGY GetTopology() const
    {
        return _topology;
    }
};