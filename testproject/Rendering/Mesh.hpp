#pragma once

#include <d3d11.h>
#include <cstddef>
#include <utility>
#include <vector>

#include "../Core/MathTypes.hpp"

class Mesh
{
private:
    std::vector<Vertex> _vertices;
    D3D11_PRIMITIVE_TOPOLOGY _topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

public:
    Mesh() = default;

    explicit Mesh(
        std::vector<Vertex> vertices,
        D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
        : _vertices(std::move(vertices)), _topology(topology)
    {
    }

    const Vertex* GetVertexData() const
    {
        return _vertices.data();
    }

    UINT GetVertexCount() const
    {
        return static_cast<UINT>(_vertices.size());
    }

    size_t GetVertexDataSize() const
    {
        return sizeof(Vertex) * _vertices.size();
    }

    D3D11_PRIMITIVE_TOPOLOGY GetTopology() const
    {
        return _topology;
    }

    bool IsEmpty() const
    {
        return _vertices.empty();
    }
};
