#include "Renderer.hpp"

bool Renderer::Initialize(GraphicsContext* graphics)
{
    _graphics = graphics;

    if (!_primitiveRenderer.Initialize(graphics)) return false;
    if (!_textRenderer.Initialize(graphics)) return false;

    return true;
}

void Renderer::Clear(float r, float g, float b)
{
    if (_graphics == nullptr) return;

    float finalR = r + _flashAmount;
    float finalG = g + _flashAmount;
    float finalB = b + _flashAmount;

    if (finalR > 1.0f) finalR = 1.0f;
    if (finalG > 1.0f) finalG = 1.0f;
    if (finalB > 1.0f) finalB = 1.0f;

    _graphics->Clear(finalR, finalG, finalB);
}

void Renderer::Present()
{
    if (_graphics == nullptr) return;
    _graphics->Present();
}

void Renderer::DrawRect(const Vector2& center, const Vector2& size, const Color& color)
{
    _primitiveRenderer.DrawRect(center, size, color);
}

void Renderer::DrawCircle(const Vector2& center, float radius, const Color& color)
{
    _primitiveRenderer.DrawCircle(center, radius, color);
}

void Renderer::DrawStar(const Vector2& center, float outerRadius, float innerRadius, const Color& color)
{
    _primitiveRenderer.DrawStar(center, outerRadius, innerRadius, color);
}

void Renderer::DrawHeart(const Vector2& center, float size, const Color& color)
{
    _primitiveRenderer.DrawHeart(center, size, color);
}

void Renderer::BeginText()
{
    _textRenderer.BeginText();
}

void Renderer::DrawString(const std::wstring& text, const Vector2& position, float fontSize, const Color& color)
{
    _textRenderer.DrawString(text, position, fontSize, color);
}

void Renderer::DrawCenteredString(const std::wstring& text, float centerY, float fontSize, const Color& color)
{
    _textRenderer.DrawCenteredString(text, centerY, fontSize, color);
}

void Renderer::EndText()
{
    _textRenderer.EndText();
}

void Renderer::UpdateFlash(float deltaTime)
{
    _flashAmount -= deltaTime * 2.0f;

    if (_flashAmount < 0.0f)
        _flashAmount = 0.0f;
}