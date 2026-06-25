#include "PrimitiveRenderer.hpp"
#include "GraphicsContext.hpp"
#include "PrimitiveMeshBuilder.hpp"

#include <cstring>
#include <d3dcompiler.h>
#include <windows.h>

#pragma comment(lib, "d3dcompiler.lib")

namespace
{
    template<typename T>
    void SafeRelease(T*& resource)
    {
        if (resource != nullptr)
        {
            resource->Release();
            resource = nullptr;
        }
    }
}

PrimitiveRenderer::~PrimitiveRenderer()
{
    Release();
}

bool PrimitiveRenderer::Initialize(GraphicsContext* graphics)
{
    _graphics = graphics;

    if (!CreateShaders()) return false;
    if (!CreateVertexBuffer()) return false;

    return true;
}

void PrimitiveRenderer::DrawMesh(const PrimitiveMesh& mesh)
{
    if (mesh.IsEmpty()) return;
    DrawVertices(mesh.GetVertexData(), mesh.GetVertexCount(), mesh.GetTopology());
}

void PrimitiveRenderer::DrawRect(const Vector2& center, const Vector2& size, const Color& color)
{
    DrawMesh(PrimitiveMeshBuilder::CreateRect(center, size, color));
}

void PrimitiveRenderer::DrawCircle(const Vector2& center, float radius, const Color& color)
{
    DrawMesh(PrimitiveMeshBuilder::CreateCircle(center, radius, color));
}

void PrimitiveRenderer::DrawStar(const Vector2& center, float outerRadius, float innerRadius, const Color& color)
{
    DrawMesh(PrimitiveMeshBuilder::CreateStar(center, outerRadius, innerRadius, color));
}

void PrimitiveRenderer::DrawHeart(const Vector2& center, float size, const Color& color)
{
    DrawMesh(PrimitiveMeshBuilder::CreateHeart(center, size, color));
}

void PrimitiveRenderer::DrawVertices(const Vertex* vertices, unsigned int vertexCount, D3D11_PRIMITIVE_TOPOLOGY topology)
{
    if (_graphics == nullptr) return;
    if (vertices == nullptr) return;
    if (vertexCount == 0) return;
    if (vertexCount > VertexBufferCapacity) return;

    ID3D11DeviceContext* context = _graphics->GetContext();
    if (context == nullptr) return;

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = context->Map(_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) return;

    std::memcpy(mapped.pData, vertices, sizeof(Vertex) * vertexCount);
    context->Unmap(_vertexBuffer, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    context->IASetInputLayout(_inputLayout);
    context->IASetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
    context->IASetPrimitiveTopology(topology);
    context->VSSetShader(_vertexShader, nullptr, 0);
    context->PSSetShader(_pixelShader, nullptr, 0);
    context->Draw(vertexCount, 0);
}

bool PrimitiveRenderer::CreateShaders()
{
    if (_graphics == nullptr) return false;

    const char* shaderCode = R"(
        struct VS_INPUT
        {
            float3 position : POSITION;
            float4 color : COLOR;
        };

        struct PS_INPUT
        {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };

        PS_INPUT VSMain(VS_INPUT input)
        {
            PS_INPUT output;
            output.position = float4(input.position, 1.0f);
            output.color = input.color;
            return output;
        }

        float4 PSMain(PS_INPUT input) : SV_TARGET
        {
            return input.color;
        }
    )";

    ID3DBlob* vertexShaderBlob = nullptr;
    ID3DBlob* pixelShaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompile(shaderCode, std::strlen(shaderCode), nullptr, nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vertexShaderBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob != nullptr)
            OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));

        SafeRelease(errorBlob);
        return false;
    }

    SafeRelease(errorBlob);

    hr = D3DCompile(shaderCode, std::strlen(shaderCode), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &pixelShaderBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob != nullptr)
            OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));

        SafeRelease(errorBlob);
        SafeRelease(vertexShaderBlob);
        return false;
    }

    SafeRelease(errorBlob);

    ID3D11Device* device = _graphics->GetDevice();
    if (device == nullptr)
    {
        SafeRelease(vertexShaderBlob);
        SafeRelease(pixelShaderBlob);
        return false;
    }

    hr = device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &_vertexShader);
    if (FAILED(hr))
    {
        SafeRelease(vertexShaderBlob);
        SafeRelease(pixelShaderBlob);
        return false;
    }

    hr = device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &_pixelShader);
    if (FAILED(hr))
    {
        SafeRelease(vertexShaderBlob);
        SafeRelease(pixelShaderBlob);
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = device->CreateInputLayout(layout, 2, vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &_inputLayout);

    SafeRelease(vertexShaderBlob);
    SafeRelease(pixelShaderBlob);

    return SUCCEEDED(hr);
}

bool PrimitiveRenderer::CreateVertexBuffer()
{
    if (_graphics == nullptr) return false;

    ID3D11Device* device = _graphics->GetDevice();
    if (device == nullptr) return false;

    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = sizeof(Vertex) * VertexBufferCapacity;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    return SUCCEEDED(device->CreateBuffer(&desc, nullptr, &_vertexBuffer));
}

void PrimitiveRenderer::Release()
{
    SafeRelease(_vertexBuffer);
    SafeRelease(_inputLayout);
    SafeRelease(_pixelShader);
    SafeRelease(_vertexShader);

    _graphics = nullptr;
}