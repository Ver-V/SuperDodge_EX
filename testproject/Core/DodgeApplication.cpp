#include "DodgeApplication.hpp"
#include "GameConstants.hpp"

#include <chrono>
#include <algorithm>

int DodgeApplication::Run(HINSTANCE hInstance, int nCmdShow)
{
    if (!CreateMainWindow(hInstance, nCmdShow))
        return 0;

    if (!_renderer.Initialize(_hwnd))
    {
        MessageBoxW(_hwnd, L"DirectX11 초기화에 실패했습니다.", L"Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    _gameManager.Initialize(_hwnd, &_inputManager);

    return RunMessageLoop();
}

bool DodgeApplication::CreateMainWindow(HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DodgeApplication::WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"DodgeDX11WindowClass";

    RegisterClassExW(&wc);

    RECT windowRect = { 0, 0, ScreenWidth, ScreenHeight };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    _hwnd = CreateWindowExW(0, wc.lpszClassName, L"Dodge DX11", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, hInstance, nullptr);

    if (!_hwnd)
        return false;

    ShowWindow(_hwnd, nCmdShow);
    return true;
}

int DodgeApplication::RunMessageLoop()
{
    MSG msg = {};
    auto prevTime = std::chrono::high_resolution_clock::now();

    while (msg.message != WM_QUIT)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentTime - prevTime;
        prevTime = currentTime;

        float deltaTime = std::min(elapsed.count(), 0.033f);

        _inputManager.Update();
        _gameManager.Update(deltaTime);
        _gameManager.Draw(_renderer, deltaTime);
        _renderer.Present();
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK DodgeApplication::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
