#include "Renderer.hpp"
#include "PrimitiveMeshBuilder.hpp"

#include "../Core/GameConstants.hpp"

#include <d3dcompiler.h>
#include <cmath>
#include <cstring>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

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

Renderer::~Renderer()
{
    Release();
}

bool Renderer::Initialize(HWND hwnd)
{
    _hwnd = hwnd;

    DXGI_SWAP_CHAIN_DESC swapDesc = {};
    swapDesc.BufferCount = 1;
    swapDesc.BufferDesc.Width = ScreenWidth;
    swapDesc.BufferDesc.Height = ScreenHeight;
    swapDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDesc.OutputWindow = hwnd;
    swapDesc.SampleDesc.Count = 1;
    swapDesc.Windowed = TRUE;

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, &swapDesc, &_swapChain, &_device, &featureLevel, &_context);


    if (FAILED(hr)) return false;

    ID3D11Texture2D* backBuffer = nullptr;
    hr = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    if (FAILED(hr)) return false;

    hr = _device->CreateRenderTargetView(backBuffer, nullptr, &_renderTargetView);
    backBuffer->Release();
    if (FAILED(hr)) return false;

    _context->OMSetRenderTargets(1, &_renderTargetView, nullptr);

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    ID3D11BlendState* blendState = nullptr;
    hr = _device->CreateBlendState(&blendDesc, &blendState);
    if (FAILED(hr)) return false;

    _context->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);
    blendState->Release();

    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(ScreenWidth);
    viewport.Height = static_cast<float>(ScreenHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    _context->RSSetViewports(1, &viewport);

    return CreateShaders() && CreateVertexBuffer() && InitDirectWrite();
}

bool Renderer::InitDirectWrite()
{
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &_d2dFactory);
    if (FAILED(hr)) return false;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&_dWriteFactory));
    if (FAILED(hr)) return false;

    return CreateD2DTargetResources();
}

bool Renderer::CreateD2DTargetResources()
{
    IDXGISurface* surface = nullptr;
    HRESULT hr = _swapChain->GetBuffer(0, __uuidof(IDXGISurface), reinterpret_cast<void**>(&surface));
    if (FAILED(hr)) return false;

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT, 
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
    
    hr = _d2dFactory->CreateDxgiSurfaceRenderTarget(surface, &props, &_d2dRenderTarget);
    surface->Release();
    if (FAILED(hr)) return false;

    hr = _d2dRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_whiteBrush);
    if (FAILED(hr))
    {
        ReleaseD2DTargetResources();
        return false;
    }

    return true;
}

void Renderer::Clear(float r, float g, float b)
{
    float finalR = r + _flashAmount;
    float finalG = g + _flashAmount;
    float finalB = b + _flashAmount;
    float clearColor[4] = { (std::min)(1.0f, finalR), (std::min)(1.0f, finalG), (std::min)(1.0f, finalB), 1.0f };
    _context->ClearRenderTargetView(_renderTargetView, clearColor);
}

void Renderer::Present()
{
    _swapChain->Present(1, 0);
}

void Renderer::DrawMesh(const Mesh& mesh)
{
    if (mesh.IsEmpty() || mesh.GetVertexCount() > VertexBufferCapacity)
        return;

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = _context->Map(_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) return;

    memcpy(mapped.pData, mesh.GetVertexData(), mesh.GetVertexDataSize());
    _context->Unmap(_vertexBuffer, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    _context->IASetInputLayout(_inputLayout);
    _context->IASetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
    _context->IASetPrimitiveTopology(mesh.GetTopology());
    _context->VSSetShader(_vertexShader, nullptr, 0);
    _context->PSSetShader(_pixelShader, nullptr, 0);
    _context->Draw(mesh.GetVertexCount(), 0);
}

void Renderer::DrawRect(const Vector2& center, const Vector2& size, const Color& color)
{
    DrawMesh(PrimitiveMeshBuilder::CreateRect(center, size, color));
}

void Renderer::DrawCircle(const Vector2& center, float radius, const Color& color)
{
    DrawMesh(PrimitiveMeshBuilder::CreateCircle(center, radius, color));
}

void Renderer::DrawStar(
    const Vector2& center,
    float outerRadius,
    float innerRadius,
    const Color& color)
{
    DrawMesh(PrimitiveMeshBuilder::CreateStar(center, outerRadius, innerRadius, color));
}

void Renderer::DrawHeart(const Vector2& center, float size, const Color& color)
{
    DrawMesh(PrimitiveMeshBuilder::CreateHeart(center, size, color));
}

void Renderer::BeginText()
{
    if (_d2dRenderTarget) _d2dRenderTarget->BeginDraw();
}

void Renderer::DrawString(const std::wstring& text, const Vector2& position, float fontSize, const Color& color)
{
    if (!_d2dRenderTarget || !_dWriteFactory || !_whiteBrush) return;

    IDWriteTextFormat* format = GetTextFormat(fontSize);
    if (format == nullptr) return;

    _whiteBrush->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
    D2D1_RECT_F rect = D2D1::RectF(position.x, position.y, position.x + 800.0f, position.y + 150.0f);
    _d2dRenderTarget->DrawTextW(text.c_str(), static_cast<UINT32>(text.length()), format, rect, _whiteBrush);
}

void Renderer::DrawCenteredString(
    const std::wstring& text,
    float centerY,
    float fontSize,
    const Color& color)
{
    if (!_d2dRenderTarget || !_dWriteFactory || !_whiteBrush) return;

    IDWriteTextFormat* format = GetTextFormat(fontSize);
    if (format == nullptr) return;

    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    _whiteBrush->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));

    const float textHeight = fontSize * 1.5f;
    D2D1_RECT_F rect = D2D1::RectF(
        0.0f,
        centerY - textHeight * 0.5f,
        static_cast<float>(PlayAreaWidth),
        centerY + textHeight * 0.5f);
    _d2dRenderTarget->DrawTextW(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        format,
        rect,
        _whiteBrush);

    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
}

void Renderer::EndText()
{
    if (_d2dRenderTarget == nullptr) return;

    HRESULT hr = _d2dRenderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
    {
        ReleaseD2DTargetResources();
        CreateD2DTargetResources();
    }
}

bool Renderer::CreateShaders()
{
    const char* shaderCode = R"(
        struct VS_INPUT { float3 position : POSITION; float4 color : COLOR; };
        struct PS_INPUT { float4 position : SV_POSITION; float4 color : COLOR; };
        PS_INPUT VSMain(VS_INPUT input) {
            PS_INPUT output;
            output.position = float4(input.position, 1.0f);
            output.color = input.color;
            return output;
        }
        float4 PSMain(PS_INPUT input) : SV_TARGET { return input.color; }
    )";

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob != nullptr)
            OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
        SafeRelease(errorBlob);
        return false;
    }
    SafeRelease(errorBlob);

    hr = D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob != nullptr)
            OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
        SafeRelease(errorBlob);
        SafeRelease(vsBlob);
        return false;
    }
    SafeRelease(errorBlob);

    hr = _device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &_vertexShader);
    if (FAILED(hr))
    {
        SafeRelease(vsBlob);
        SafeRelease(psBlob);
        return false;
    }

    hr = _device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &_pixelShader);
    if (FAILED(hr))
    {
        SafeRelease(vsBlob);
        SafeRelease(psBlob);
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = _device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &_inputLayout);

    SafeRelease(vsBlob);
    SafeRelease(psBlob);
    return SUCCEEDED(hr);
}

bool Renderer::CreateVertexBuffer()
{
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = sizeof(Vertex) * VertexBufferCapacity;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    return SUCCEEDED(_device->CreateBuffer(&desc, nullptr, &_vertexBuffer));
}

IDWriteTextFormat* Renderer::GetTextFormat(float fontSize)
{
    auto it = _textFormats.find(fontSize);
    if (it != _textFormats.end())
        return it->second;

    IDWriteTextFormat* format = nullptr;
    HRESULT hr = _dWriteFactory->CreateTextFormat(
        L"Consolas",
        nullptr,
        DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        fontSize,
        L"en-us",
        &format);

    if (FAILED(hr)) return nullptr;

    _textFormats.emplace(fontSize, format);
    return format;
}

void Renderer::ReleaseD2DTargetResources()
{
    SafeRelease(_whiteBrush);
    SafeRelease(_d2dRenderTarget);
}

void Renderer::Release()
{
    for (auto& entry : _textFormats)
        SafeRelease(entry.second);
    _textFormats.clear();

    ReleaseD2DTargetResources();
    SafeRelease(_dWriteFactory);
    SafeRelease(_d2dFactory);
    SafeRelease(_vertexBuffer);
    SafeRelease(_inputLayout);
    SafeRelease(_pixelShader);
    SafeRelease(_vertexShader);
    SafeRelease(_renderTargetView);
    SafeRelease(_swapChain);
    SafeRelease(_context);
    SafeRelease(_device);
}
