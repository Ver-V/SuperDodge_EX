#pragma once

#define NOMINMAX
#include <windows.h>
#include <array>

class InputManager
{
private:
    static constexpr int KeyCount = 256;
    std::array<bool, KeyCount> _currentKeys = {};
    std::array<bool, KeyCount> _previousKeys = {};

public:
    void Update()
    {
        _previousKeys = _currentKeys;

        for (int key = 0; key < KeyCount; ++key)
            _currentKeys[key] = (GetAsyncKeyState(key) & 0x8000) != 0;
    }

    bool IsKeyDown(int key) const
    {
        return IsValidKey(key) && _currentKeys[key];
    }

    bool IsKeyPressed(int key) const
    {
        return IsValidKey(key) && _currentKeys[key] && !_previousKeys[key];
    }

private:
    static bool IsValidKey(int key)
    {
        return key >= 0 && key < KeyCount;
    }
};