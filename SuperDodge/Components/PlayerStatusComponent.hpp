#pragma once

#include <algorithm>

#include "../Core/Component.hpp"

class PlayerStatusComponent : public Component
{
private:
    static constexpr int MaxHp = 5;
    static constexpr int StarsPerLife = 3;
    static constexpr float InvincibilityDuration = 1.0f;

    int _hp = 3;
    int _bombCount = 3;
    int _collectedStars = 0;
    bool _isDead = false;
    float _invincibilityRemaining = 0.0f;
    float _hitboxRadius = 5.0f;

public:
    void Reset(int hp)
    {
        _hp = (std::max)(1, (std::min)(hp, MaxHp));
        _bombCount = 3;
        _collectedStars = 0;
        _isDead = false;
        _invincibilityRemaining = 0.0f;
    }

    void Update(float deltaTime) override
    {
        _invincibilityRemaining = (std::max)(0.0f, _invincibilityRemaining - deltaTime);
    }

    bool TakeDamage(int damage)
    {
        if (_isDead || IsInvincible())
            return false;

        _hp = (std::max)(0, _hp - damage);
        _isDead = _hp <= 0;
        if (!_isDead)
            _invincibilityRemaining = InvincibilityDuration;

        return true;
    }

    void CollectStar()
    {
        ++_collectedStars;
        if (_collectedStars < StarsPerLife)
            return;

        _collectedStars = 0;
        if (_hp < MaxHp)
            ++_hp;
    }

    bool UseBomb()
    {
        if (_bombCount > 0 && !_isDead)
        {
            _bombCount--;
            return true;
        }
        return false;
    }

    int GetHp() const { return _hp; }
    int GetMaxHp() const { return MaxHp; }
    int GetBombCount() const { return _bombCount; }
    int GetCollectedStars() const { return _collectedStars; }
    float GetHitboxRadius() const { return _hitboxRadius; }
    bool IsDead() const { return _isDead; }
    bool IsInvincible() const { return _invincibilityRemaining > 0.0f; }
    bool IsVisible() const
    {
        if (!IsInvincible())
            return true;

        return static_cast<int>(_invincibilityRemaining * 12.0f) % 2 == 0;
    }
};
