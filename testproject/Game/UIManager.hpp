#pragma once

#define NOMINMAX
#include <windows.h>

class UIManager
{
private:
    HWND _hwnd = nullptr;

public:
    void Initialize(HWND hwnd)
    {
        _hwnd = hwnd;
    }

    void ShowStartUI()
    {
        SetWindowTextW(_hwnd, L"Dodge DX11 - Space: Start | WASD/Arrow: Move | ESC: Quit");
    }
};
