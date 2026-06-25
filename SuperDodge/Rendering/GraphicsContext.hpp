#pragma once

#define NOMINMAX
#include <windows.h>
#include <d3d11.h>

class GraphicsContext
{
private:
    HWND _hwnd = nullptr;
    ID3D11Device* _device = nullptr;
    ID3D11DeviceContext* _context = nullptr;
    IDXGISwapChain* _swapChain = nullptr;
    ID3D11RenderTargetView* _renderTargetView = nullptr;

public:
    GraphicsContext() = default;
    ~GraphicsContext();

    GraphicsContext(const GraphicsContext&) = delete;
    GraphicsContext& operator=(const GraphicsContext&) = delete;

    bool Initialize(HWND hwnd, int width, int height);
    void Clear(float r, float g, float b);
    void Present();
    void Release();

    ID3D11Device* GetDevice() const { return _device; }
    ID3D11DeviceContext* GetContext() const { return _context; }
    IDXGISwapChain* GetSwapChain() const { return _swapChain; }
    ID3D11RenderTargetView* GetRenderTargetView() const { return _renderTargetView; }
};
