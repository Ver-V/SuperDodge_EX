#pragma once

#define NOMINMAX
#include <windows.h>

#include "GameConstants.hpp"
#include "Timer.hpp"
#include "WindowContext.hpp"
#include "../Rendering/GraphicsContext.hpp"
#include "../Rendering/Renderer.hpp"
#include "../Game/GameManager.hpp"

class GameLoop
{
private:
    WindowContext _window;
    GraphicsContext _graphics;
    Renderer _renderer;
    GameManager _gameManager;
    InputManager _inputManager;
    Timer _timer;

public:
    int Run(HINSTANCE hInstance, int nCmdShow);

private:
    bool Initialize(HINSTANCE hInstance, int nCmdShow);
    int RunMessageLoop();
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};
