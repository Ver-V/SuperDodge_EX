#include "GameLoop.hpp"

int GameLoop::Run(HINSTANCE hInstance, int nCmdShow)
{
    if (!Initialize(hInstance, nCmdShow))
        return 0;

    return RunMessageLoop();
}

bool GameLoop::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    if (!_window.Initialize(hInstance, nCmdShow, GameLoop::WindowProc, ScreenWidth, ScreenHeight, L"Dodge DX11"))
        return false;

    if (!_graphics.Initialize(_window.GetHwnd(), ScreenWidth, ScreenHeight))
    {
        MessageBoxW(_window.GetHwnd(), L"DirectX11 초기화에 실패했습니다.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (!_renderer.Initialize(&_graphics))
    {
        MessageBoxW(_window.GetHwnd(), L"Renderer 초기화에 실패했습니다.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (!_gameManager.Initialize(_renderer, &_inputManager))
    {
        MessageBoxW(_window.GetHwnd(), L"게임 렌더 리소스 초기화에 실패했습니다.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
    _timer.Reset();
    return true;
}

int GameLoop::RunMessageLoop()
{
    MSG msg = {};

    while (msg.message != WM_QUIT)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        const float deltaTime = _timer.Tick();
        _inputManager.Update();
        _gameManager.Update(deltaTime);
        _gameManager.Draw(_renderer, deltaTime);
        _renderer.Present();
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK GameLoop::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
            return 0;
        }
        break;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}
