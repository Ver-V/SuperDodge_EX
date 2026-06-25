#pragma once

#define NOMINMAX
#include <d3d11.h>
#include <vector>
#include <cstdint>
#include <cstring>

#include "ShaderTypes.hpp"

class Material
{
private:
    ShaderSet* _shaderSet = nullptr;
    std::vector<Texture*> _textures;
    std::vector<std::vector<unsigned char>> _constantDataList;
    std::vector<ID3D11Buffer*> _constantBuffers;

public:
    Material() = default;

    Material(ShaderSet* shaderSet)
        : _shaderSet(shaderSet)
    {
    }

    ~Material()
    {
        for (ID3D11Buffer* buffer : _constantBuffers)
        {
            if (buffer != nullptr)
                buffer->Release();
        }

        _constantBuffers.clear();
    }

    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    void SetShaderSet(ShaderSet* shaderSet)
    {
        _shaderSet = shaderSet;
    }

    ShaderSet* GetShaderSet() const
    {
        return _shaderSet;
    }

    void AddTexture(Texture* texture)
    {
        _textures.push_back(texture);
    }

    template<typename T>
    void AddConstantData(const T& data)
    {
        size_t originalSize = sizeof(T);
        size_t alignedSize = (originalSize + 15) & ~15;

        std::vector<unsigned char> byteBuffer(alignedSize, 0);
        std::memcpy(byteBuffer.data(), &data, originalSize);

        _constantDataList.push_back(byteBuffer);
        _constantBuffers.push_back(nullptr);
    }

    void Bind(ID3D11DeviceContext* context)
    {
        if (context == nullptr) return;
        if (_shaderSet == nullptr) return;

        for (int i = 0; i < static_cast<int>(_constantBuffers.size()); ++i)
        {
            if (_constantBuffers[i] != nullptr) continue;

            ID3D11Device* device = nullptr;
            context->GetDevice(&device);
            if (device == nullptr) continue;

            D3D11_BUFFER_DESC desc = {};
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.ByteWidth = static_cast<unsigned int>(_constantDataList[i].size());
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

            D3D11_SUBRESOURCE_DATA data = {};
            data.pSysMem = _constantDataList[i].data();

            device->CreateBuffer(&desc, &data, &_constantBuffers[i]);
            device->Release();
        }

        context->IASetInputLayout(_shaderSet->inputLayout);
        context->VSSetShader(_shaderSet->vertexShader, nullptr, 0);
        context->PSSetShader(_shaderSet->pixelShader, nullptr, 0);

        for (int i = 0; i < static_cast<int>(_textures.size()); ++i)
        {
            if (_textures[i] == nullptr) continue;

            ID3D11ShaderResourceView* srv = _textures[i]->shaderResourceView;
            context->VSSetShaderResources(i, 1, &srv);
            context->PSSetShaderResources(i, 1, &srv);

            if (_textures[i]->samplerState != nullptr)
            {
                ID3D11SamplerState* sampler = _textures[i]->samplerState;
                context->VSSetSamplers(0, 1, &sampler);
                context->PSSetSamplers(0, 1, &sampler);
            }
        }

        for (int i = 0; i < static_cast<int>(_constantBuffers.size()); ++i)
        {
            ID3D11Buffer* buffer = _constantBuffers[i];
            context->VSSetConstantBuffers(i + 1, 1, &buffer);
            context->PSSetConstantBuffers(i + 1, 1, &buffer);
        }
    }

    template<typename T>
    void UpdateConstantData(ID3D11DeviceContext* context, int index, const T& data)
    {
        if (context == nullptr) return;
        if (index < 0) return;
        if (index >= static_cast<int>(_constantBuffers.size())) return;
        if (_constantBuffers[index] == nullptr) return;

        context->UpdateSubresource(_constantBuffers[index], 0, nullptr, &data, 0, 0);
    }
};