#pragma once

#include <algorithm>
#include <chrono>

class Timer
{
private:
    std::chrono::high_resolution_clock::time_point _previousTime;
    float _maxDeltaTime = 0.033f;

public:
    Timer()
    {
        Reset();
    }

    void Reset()
    {
        _previousTime = std::chrono::high_resolution_clock::now();
    }

    float Tick()
    {
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<float> elapsed = currentTime - _previousTime;
        _previousTime = currentTime;
        return (std::min)(elapsed.count(), _maxDeltaTime);
    }
};
