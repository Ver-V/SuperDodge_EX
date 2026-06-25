#pragma once

#include "../Core/MathTypes.hpp"
#include "GraphicsContext.hpp"
#include "PrimitiveRenderer.hpp"
#include "TextRenderer.hpp"

#include <string>

class Renderer
{
private:
    GraphicsContext* _graphics = nullptr;
    PrimitiveRenderer _primitiveRenderer;
    TextRenderer _textRenderer;
    float _flashAmount = 0.0f;

public:
    bool Initialize(GraphicsContext* graphics);

    GraphicsContext* GetGraphicsContext() const { return _graphics; }

    void Clear(float r, float g, float b);
    void Present();

    void DrawRect(const Vector2& center, const Vector2& size, const Color& color);
    void DrawCircle(const Vector2& center, float radius, const Color& color);
    void DrawStar(const Vector2& center, float outerRadius, float innerRadius, const Color& color);
    void DrawHeart(const Vector2& center, float size, const Color& color);

    void BeginText();
    void DrawString(const std::wstring& text, const Vector2& position, float fontSize, const Color& color);
    void DrawCenteredString(const std::wstring& text, float centerY, float fontSize, const Color& color);
    void EndText();

    void SetFlash(float amount) { _flashAmount = amount; }
    void UpdateFlash(float deltaTime);
    float GetFlash() const { return _flashAmount; }
};