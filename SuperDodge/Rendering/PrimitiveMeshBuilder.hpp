#pragma once

#include <DirectXMath.h>
#include <cmath>
#include <utility>
#include <vector>

#include "../Core/GameConstants.hpp"
#include "PrimitiveMesh.hpp"

class PrimitiveMeshBuilder
{
private:
    static constexpr float Pi = 3.14159265f;

public:
    static PrimitiveMesh CreateRect(const Vector2& center, const Vector2& size, const Color& color)
    {
        const float left = PixelToNdcX(center.x - size.x * 0.5f);
        const float right = PixelToNdcX(center.x + size.x * 0.5f);
        const float top = PixelToNdcY(center.y - size.y * 0.5f);
        const float bottom = PixelToNdcY(center.y + size.y * 0.5f);
        const DirectX::XMFLOAT4 vertexColor(color.r, color.g, color.b, color.a);

        return PrimitiveMesh({
            { { left, top, 0.0f }, vertexColor },
            { { right, bottom, 0.0f }, vertexColor },
            { { left, bottom, 0.0f }, vertexColor },

            { { left, top, 0.0f }, vertexColor },
            { { right, top, 0.0f }, vertexColor },
            { { right, bottom, 0.0f }, vertexColor }
            });
    }

    static PrimitiveMesh CreateCircle(const Vector2& center, float radius, const Color& color)
    {
        constexpr int SegmentCount = 32;
        const DirectX::XMFLOAT4 vertexColor(color.r, color.g, color.b, color.a);

        std::vector<Vertex> vertices;
        vertices.reserve(SegmentCount * 3);

        for (int i = 0; i < SegmentCount; ++i)
        {
            const float angle1 = 2.0f * Pi * i / SegmentCount;
            const float angle2 = 2.0f * Pi * (i + 1) / SegmentCount;

            const Vector2 point1(center.x + std::cos(angle1) * radius, center.y + std::sin(angle1) * radius);
            const Vector2 point2(center.x + std::cos(angle2) * radius, center.y + std::sin(angle2) * radius);

            AddTriangle(vertices, center, point1, point2, vertexColor);
        }

        return PrimitiveMesh(std::move(vertices));
    }

    static PrimitiveMesh CreateStar(const Vector2& center, float outerRadius, float innerRadius, const Color& color)
    {
        constexpr int PointCount = 5;
        constexpr int EdgeCount = PointCount * 2;

        const float startAngle = -Pi * 0.5f;
        const DirectX::XMFLOAT4 vertexColor(color.r, color.g, color.b, color.a);

        std::vector<Vertex> vertices;
        vertices.reserve(EdgeCount * 3);

        for (int i = 0; i < EdgeCount; ++i)
        {
            const float angle1 = startAngle + Pi * i / PointCount;
            const float angle2 = startAngle + Pi * (i + 1) / PointCount;

            const float radius1 = (i % 2 == 0) ? outerRadius : innerRadius;
            const float radius2 = ((i + 1) % 2 == 0) ? outerRadius : innerRadius;

            const Vector2 point1(center.x + std::cos(angle1) * radius1, center.y + std::sin(angle1) * radius1);
            const Vector2 point2(center.x + std::cos(angle2) * radius2, center.y + std::sin(angle2) * radius2);

            AddTriangle(vertices, center, point1, point2, vertexColor);
        }

        return PrimitiveMesh(std::move(vertices));
    }

    static PrimitiveMesh CreateHeart(const Vector2& center, float size, const Color& color)
    {
        constexpr int SegmentCount = 32;
        const DirectX::XMFLOAT4 vertexColor(color.r, color.g, color.b, color.a);

        std::vector<Vertex> vertices;
        vertices.reserve(SegmentCount * 3);

        for (int i = 0; i < SegmentCount; ++i)
        {
            const float angle1 = 2.0f * Pi * i / SegmentCount;
            const float angle2 = 2.0f * Pi * (i + 1) / SegmentCount;

            AddTriangle(vertices, center, GetHeartPoint(center, size, angle1), GetHeartPoint(center, size, angle2), vertexColor);
        }

        return PrimitiveMesh(std::move(vertices));
    }

private:
    static void AddTriangle(std::vector<Vertex>& vertices, const Vector2& point1, const Vector2& point2, const Vector2& point3, const DirectX::XMFLOAT4& color)
    {
        vertices.push_back({ { PixelToNdcX(point1.x), PixelToNdcY(point1.y), 0.0f }, color });
        vertices.push_back({ { PixelToNdcX(point2.x), PixelToNdcY(point2.y), 0.0f }, color });
        vertices.push_back({ { PixelToNdcX(point3.x), PixelToNdcY(point3.y), 0.0f }, color });
    }

    static Vector2 GetHeartPoint(const Vector2& center, float size, float angle)
    {
        const float sinValue = std::sin(angle);
        const float x = 16.0f * sinValue * sinValue * sinValue;
        const float y = 13.0f * std::cos(angle) - 5.0f * std::cos(2.0f * angle) - 2.0f * std::cos(3.0f * angle) - std::cos(4.0f * angle);

        return Vector2(center.x + x * size / 32.0f, center.y - y * size / 32.0f);
    }

    static float PixelToNdcX(float x)
    {
        return x / ScreenWidth * 2.0f - 1.0f;
    }

    static float PixelToNdcY(float y)
    {
        return 1.0f - y / ScreenHeight * 2.0f;
    }
};