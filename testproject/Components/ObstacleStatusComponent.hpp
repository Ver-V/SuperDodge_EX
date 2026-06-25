#pragma once

#include "../Core/Component.hpp"
#include "../Core/MathTypes.hpp"
#include "../Core/MathUtils.hpp"

class ObstacleStatusComponent : public Component
{
private:
    int _damage = 1;
    bool _hasGrazed = false;
    bool _isBossProjectile = false;

public:
    explicit ObstacleStatusComponent(int damage = 1)
        : _damage(damage)
    {
    }

    int GetDamage() const { return _damage; }
    bool HasGrazed() const { return _hasGrazed; }
    void MarkGrazed() { _hasGrazed = true; }
    void ResetGraze() { _hasGrazed = false; }
    void SetBossProjectile(bool isBossProjectile) { _isBossProjectile = isBossProjectile; }
    bool IsBossProjectile() const { return _isBossProjectile; }
    float GetHitboxRadius(const Vector2& size) const
    {
        return GetCircumscribedRadius(size);
    }
};
