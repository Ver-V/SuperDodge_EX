#include "TextRenderer.hpp"
#include "GraphicsContext.hpp"
#include "../Core/GameConstants.hpp"
#include "../Core/PerfConfig.hpp"

#include <chrono>

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

#if ENABLE_FRAME_LOG
    ++GetFrameCounters().textDrawCalls;
#endif

    _whiteBrush->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));

#if USE_TEXT_LAYOUT_CACHE
    IDWriteTextLayout* layout = GetTextLayout(text, fontSize, false);
    if (layout == nullptr) return;

    _d2dRenderTarget->DrawTextLayout(D2D1::Point2F(position.x, position.y), layout, _whiteBrush);
#else
    IDWriteTextFormat* format = GetTextFormat(fontSize);
    if (format == nullptr) return;

    D2D1_RECT_F rect = D2D1::RectF(position.x, position.y, position.x + 800.0f, position.y + 150.0f);
    _d2dRenderTarget->DrawTextW(text.c_str(), static_cast<UINT32>(text.length()), format, rect, _whiteBrush);
#endif
}

void TextRenderer::DrawCenteredString(const std::wstring& text, float centerY, float fontSize, const Color& color)
{
    if (_d2dRenderTarget == nullptr || _dWriteFactory == nullptr || _whiteBrush == nullptr) return;

#if ENABLE_FRAME_LOG
    ++GetFrameCounters().textDrawCalls;
#endif

    _whiteBrush->SetColor(D2D1::ColorF(color.r, color.g, color.b, color.a));
    const float textHeight = fontSize * 1.5f;

#if USE_TEXT_LAYOUT_CACHE
    IDWriteTextLayout* layout = GetTextLayout(text, fontSize, true);
    if (layout == nullptr) return;

    _d2dRenderTarget->DrawTextLayout(D2D1::Point2F(0.0f, centerY - textHeight * 0.5f), layout, _whiteBrush);
#else
    IDWriteTextFormat* format = GetTextFormat(fontSize);
    if (format == nullptr) return;

    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    D2D1_RECT_F rect = D2D1::RectF(0.0f, centerY - textHeight * 0.5f, static_cast<float>(PlayAreaWidth), centerY + textHeight * 0.5f);
    _d2dRenderTarget->DrawTextW(text.c_str(), static_cast<UINT32>(text.length()), format, rect, _whiteBrush);

    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
#endif
}

void TextRenderer::EndText()
{
    if (_d2dRenderTarget == nullptr) return;

#if ENABLE_FRAME_LOG
    const auto endStart = std::chrono::high_resolution_clock::now();
#endif

    HRESULT hr = _d2dRenderTarget->EndDraw();

#if ENABLE_FRAME_LOG
    GetFrameCounters().textEndDrawMs = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - endStart).count();
#endif

    if (hr == D2DERR_RECREATE_TARGET)
    {
        ReleaseD2DTargetResources();
        CreateD2DTargetResources();
    }

    EvictUnusedLayouts();
    ++_textFrame;
}

IDWriteTextLayout* TextRenderer::GetTextLayout(const std::wstring& text, float fontSize, bool centered)
{
    LayoutKey key(text, fontSize, centered);

    auto it = _layoutCache.find(key);
    if (it != _layoutCache.end())
    {
        it->second.lastUsedFrame = _textFrame;
        return it->second.layout;
    }

    IDWriteTextFormat* format = GetTextFormat(fontSize);
    if (format == nullptr) return nullptr;

    // 레이아웃 크기는 기존 DrawTextW의 사각형 크기와 동일 (800x150 / PlayAreaWidth x 1.5*fontSize)
    const float maxWidth = centered ? static_cast<float>(PlayAreaWidth) : 800.0f;
    const float maxHeight = centered ? fontSize * 1.5f : 150.0f;

    IDWriteTextLayout* layout = nullptr;
    HRESULT hr = _dWriteFactory->CreateTextLayout(
        text.c_str(), static_cast<UINT32>(text.length()), format, maxWidth, maxHeight, &layout);
    if (FAILED(hr)) return nullptr;

    if (centered)
    {
        layout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        layout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

#if ENABLE_FRAME_LOG
    ++GetFrameCounters().textLayoutsCreated;
#endif

    CachedLayout cached;
    cached.layout = layout;
    cached.lastUsedFrame = _textFrame;
    _layoutCache.emplace(std::move(key), cached);
    return layout;
}

// 이번 프레임에 쓰이지 않은 레이아웃 해제 (점수처럼 매번 바뀌는 문자열로 캐시가 커지는 것 방지)
void TextRenderer::EvictUnusedLayouts()
{
    for (auto it = _layoutCache.begin(); it != _layoutCache.end();)
    {
        if (it->second.lastUsedFrame != _textFrame)
        {
            SafeRelease(it->second.layout);
            it = _layoutCache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void TextRenderer::ReleaseLayoutCache()
{
    for (auto& entry : _layoutCache)
        SafeRelease(entry.second.layout);
    _layoutCache.clear();
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
    ReleaseLayoutCache();

    for (auto& entry : _textFormats)
        SafeRelease(entry.second);
    _textFormats.clear();

    ReleaseD2DTargetResources();
    SafeRelease(_dWriteFactory);
    SafeRelease(_d2dFactory);
    _graphics = nullptr;
}
