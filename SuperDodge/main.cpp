#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")

#define NOMINMAX
#include <windows.h>
#include "Core/GameLoop.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    GameLoop gameLoop;
    return gameLoop.Run(hInstance, nCmdShow);
}
