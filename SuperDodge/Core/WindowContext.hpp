#pragma once

#define NOMINMAX
#include <windows.h>

class WindowContext
{
private:
    HINSTANCE _hInstance = nullptr;
    HWND _hwnd = nullptr;
    const wchar_t* _className = L"DodgeDX11WindowClass";

public:
    WindowContext() = default;
    ~WindowContext();

    WindowContext(const WindowContext&) = delete;
    WindowContext& operator=(const WindowContext&) = delete;

    bool Initialize(HINSTANCE hInstance, int nCmdShow, WNDPROC wndProc, int width, int height, const wchar_t* title);
    HWND GetHwnd() const { return _hwnd; }
};
