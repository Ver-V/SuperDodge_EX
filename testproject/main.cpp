#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")

#define NOMINMAX
#include <windows.h>
#include "Core/DodgeApplication.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    DodgeApplication app;
    return app.Run(hInstance, nCmdShow);
}