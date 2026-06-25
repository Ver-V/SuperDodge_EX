#pragma once

#define NOMINMAX
#include <d3d11.h>
#include <DirectXMath.h>

struct ConstantBuffer
{
    DirectX::XMMATRIX matWorld;
};

struct ShaderSet
{
    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;

    void Release()
    {
        if (vertexShader != nullptr)
        {
            vertexShader->Release();
            vertexShader = nullptr;
        }

        if (pixelShader != nullptr)
        {
            pixelShader->Release();
            pixelShader = nullptr;
        }

        if (inputLayout != nullptr)
        {
            inputLayout->Release();
            inputLayout = nullptr;
        }
    }
};

class Texture
{
public:
    ID3D11ShaderResourceView* shaderResourceView = nullptr;
    ID3D11SamplerState* samplerState = nullptr;

    ~Texture()
    {
        Release();
    }

    Texture() = default;
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void Release()
    {
        if (shaderResourceView != nullptr)
        {
            shaderResourceView->Release();
            shaderResourceView = nullptr;
        }

        if (samplerState != nullptr)
        {
            samplerState->Release();
            samplerState = nullptr;
        }
    }
};