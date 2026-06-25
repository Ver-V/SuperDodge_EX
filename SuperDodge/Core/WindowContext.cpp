#include "WindowContext.hpp"

WindowContext::~WindowContext()
{
    if (_hwnd != nullptr)
    {
        DestroyWindow(_hwnd);
        _hwnd = nullptr;
    }

    if (_hInstance != nullptr)
        UnregisterClassW(_className, _hInstance);
}

bool WindowContext::Initialize(HINSTANCE hInstance, int nCmdShow, WNDPROC wndProc, int width, int height, const wchar_t* title)
{
    _hInstance = hInstance;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = _className;

    if (!RegisterClassExW(&wc))
        return false;

    RECT windowRect = { 0, 0, width, height };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    _hwnd = CreateWindowExW(
        0,
        _className,
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (_hwnd == nullptr)
        return false;

    ShowWindow(_hwnd, nCmdShow);
    return true;
}
