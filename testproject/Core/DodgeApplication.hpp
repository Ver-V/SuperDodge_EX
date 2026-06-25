#pragma once

#define NOMINMAX
#include <windows.h>
#include "InputManager.hpp"
#include "../Rendering/Renderer.hpp"
#include "../Game/GameManager.hpp"

class DodgeApplication
{
private:
    HWND _hwnd = nullptr;
    InputManager _inputManager;
    Renderer _renderer;
    GameManager _gameManager;

public:
    int Run(HINSTANCE hInstance, int nCmdShow);

private:
    bool CreateMainWindow(HINSTANCE hInstance, int nCmdShow);
    int RunMessageLoop();

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};
