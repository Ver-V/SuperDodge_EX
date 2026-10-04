#pragma once

#define NOMINMAX
#include <d2d1.h>
#include <dwrite.h>
#include <map>
#include <string>
#include <tuple>

#include "../Core/MathTypes.hpp"

class GraphicsContext;

class TextRenderer
{
private:
    // key: (text, fontSize, centered)
    using LayoutKey = std::tuple<std::wstring, float, bool>;

    struct CachedLayout
    {
        IDWriteTextLayout* layout = nullptr;
        unsigned long long lastUsedFrame = 0;
    };

    GraphicsContext* _graphics = nullptr;
    ID2D1Factory* _d2dFactory = nullptr;
    ID2D1RenderTarget* _d2dRenderTarget = nullptr;
    IDWriteFactory* _dWriteFactory = nullptr;
    ID2D1SolidColorBrush* _whiteBrush = nullptr;
    std::map<float, IDWriteTextFormat*> _textFormats;
    std::map<LayoutKey, CachedLayout> _layoutCache;
    unsigned long long _textFrame = 0;

public:
    TextRenderer() = default;
    ~TextRenderer();

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    bool Initialize(GraphicsContext* graphics);
    void BeginText();
    void DrawString(const std::wstring& text, const Vector2& position, float fontSize, const Color& color);
    void DrawCenteredString(const std::wstring& text, float centerY, float fontSize, const Color& color);
    void EndText();
    void Release();

private:
    bool CreateD2DTargetResources();
    void ReleaseD2DTargetResources();
    IDWriteTextFormat* GetTextFormat(float fontSize);
    IDWriteTextLayout* GetTextLayout(const std::wstring& text, float fontSize, bool centered);
    void EvictUnusedLayouts();
    void ReleaseLayoutCache();
};
