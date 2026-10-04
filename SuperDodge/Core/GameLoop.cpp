#include "GameLoop.hpp"
#include "PerfConfig.hpp"

#include <chrono>
#include <cstdio>
#include <fstream>

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

#if ENABLE_FRAME_LOG
    using Clock = std::chrono::high_resolution_clock;
    using Milliseconds = std::chrono::duration<double, std::milli>;

    // 실행마다 덮어쓰지 않도록 시각을 붙임 (예: frame_pool_20261005_012634.csv)
    SYSTEMTIME now = {};
    GetLocalTime(&now);
    char frameLogName[64] = {};
    sprintf_s(frameLogName, "%s_%s_%04d%02d%02d_%02d%02d%02d.csv",
        USE_OBJECT_POOL ? "frame_pool" : "frame_nopool",
        USE_TEXT_LAYOUT_CACHE ? "textcache" : "nocache",
        now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);

    std::ofstream frameLog(frameLogName);
    frameLog << "frame,frameMs,cpuMs,activeObjects,totalObjects,created,destroyed,bossActive,buffersCreated,bufferCreateMs,poolGetCalls,poolScanSteps,poolGetMs,updateMs,drawMs,worldRenderMs,uiMs,presentMs,textDrawCalls,textLayoutsCreated,textEndDrawMs\n";

    long long frameIndex = 0;
    Clock::time_point previousFrameStart = Clock::now();
    GetFrameCounters() = FrameCounters();
#endif

    while (msg.message != WM_QUIT)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

#if ENABLE_FRAME_LOG
        const Clock::time_point frameStart = Clock::now();
#endif

        const float deltaTime = _timer.Tick();
        _inputManager.Update();
        _gameManager.Update(deltaTime);

#if ENABLE_FRAME_LOG
        const Clock::time_point updateEnd = Clock::now();
#endif

        _gameManager.Draw(_renderer, deltaTime);

#if ENABLE_FRAME_LOG
        // cpuMs: Update + Draw 호출에 걸린 시간 (Present의 vsync 대기 제외)
        const Clock::time_point cpuEnd = Clock::now();
#endif

        _renderer.Present();

#if ENABLE_FRAME_LOG
        const Clock::time_point presentEnd = Clock::now();

        // frameMs: 이전 프레임 시작부터 이번 프레임 시작까지 (Present 포함 실제 프레임 간격)
        const FrameCounters& counters = GetFrameCounters();
        frameLog << frameIndex++ << ','
                 << Milliseconds(frameStart - previousFrameStart).count() << ','
                 << Milliseconds(cpuEnd - frameStart).count() << ','
                 << _gameManager.GetActiveObjectCount() << ','
                 << _gameManager.GetTotalObjectCount() << ','
                 << counters.objectsCreated << ','
                 << counters.objectsDestroyed << ','
                 << (_gameManager.IsBossActive() ? 1 : 0) << ','
                 << counters.buffersCreated << ','
                 << counters.bufferCreateMs << ','
                 << counters.poolGetCalls << ','
                 << counters.poolScanSteps << ','
                 << counters.poolGetMs << ','
                 << Milliseconds(updateEnd - frameStart).count() << ','
                 << Milliseconds(cpuEnd - updateEnd).count() << ','
                 << counters.worldRenderMs << ','
                 << counters.uiMs << ','
                 << Milliseconds(presentEnd - cpuEnd).count() << ','
                 << counters.textDrawCalls << ','
                 << counters.textLayoutsCreated << ','
                 << counters.textEndDrawMs << '\n';

        previousFrameStart = frameStart;
        GetFrameCounters() = FrameCounters();
#endif
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
