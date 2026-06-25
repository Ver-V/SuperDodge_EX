#include "TextRenderer.hpp"
#include "GraphicsContext.hpp"
#include "../Core/GameConstants.hpp"

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

TextRenderer::~TextRenderer()
{
    Release();
}

bool TextRenderer::Initialize(GraphicsContext* graphics)
{
    _graphics = graphics;

    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &_d2dFactory);
    if (FAILED(hr)) return false;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&_dWriteFactory));
    if (FAILED(hr)) return false;

    return CreateD2DTargetResources();
}

bool TextRenderer::CreateD2DTargetResources()
{
    if (_graphics == nullptr) return false;

    IDXGISurface* surface = nullptr;
    HRESULT hr = _graphics->GetSwapChain()->GetBuffer(0, __uuidof(IDXGISurface), reinterpret_cast<void**>(&surface));
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

void TextRenderer::BeginText()
{
    if (_d2dRenderTarget != nullptr)
        _d2dRenderTarget->BeginDraw();
}

void TextRenderer::DrawString(const std::wstring& text, const Vector2& position, float fontSize, const Color& color)
{
    if (_d2dRenderTarget == nullptr || _dWriteFactory == nullptr || _whiteBrush == nullptr) return;

    IDWriteTextFormat* format = GetTextFormat(fontSize);
    if (format == nullptr) return;

    _whiteBrush->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
    D2D1_RECT_F rect = D2D1::RectF(position.x, position.y, position.x + 800.0f, position.y + 150.0f);
    _d2dRenderTarget->DrawTextW(text.c_str(), static_cast<UINT32>(text.length()), format, rect, _whiteBrush);
}

void TextRenderer::DrawCenteredString(const std::wstring& text, float centerY, float fontSize, const Color& color)
{
    if (_d2dRenderTarget == nullptr || _dWriteFactory == nullptr || _whiteBrush == nullptr) return;

    IDWriteTextFormat* format = GetTextFormat(fontSize);
    if (format == nullptr) return;

    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    _whiteBrush->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));

    const float textHeight = fontSize * 1.5f;
    D2D1_RECT_F rect = D2D1::RectF(0.0f, centerY - textHeight * 0.5f, static_cast<float>(PlayAreaWidth), centerY + textHeight * 0.5f);
    _d2dRenderTarget->DrawTextW(text.c_str(), static_cast<UINT32>(text.length()), format, rect, _whiteBrush);

    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
}

void TextRenderer::EndText()
{
    if (_d2dRenderTarget == nullptr) return;

    HRESULT hr = _d2dRenderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
    {
        ReleaseD2DTargetResources();
        CreateD2DTargetResources();
    }
}

IDWriteTextFormat* TextRenderer::GetTextFormat(float fontSize)
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

void TextRenderer::ReleaseD2DTargetResources()
{
    SafeRelease(_whiteBrush);
    SafeRelease(_d2dRenderTarget);
}

void TextRenderer::Release()
{
    for (auto& entry : _textFormats)
        SafeRelease(entry.second);
    _textFormats.clear();

    ReleaseD2DTargetResources();
    SafeRelease(_dWriteFactory);
    SafeRelease(_d2dFactory);
    _graphics = nullptr;
}
