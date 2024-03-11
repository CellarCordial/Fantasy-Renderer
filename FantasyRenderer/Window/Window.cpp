#include "Window.h"

#include <windowsx.h>

constexpr UINT32 WINDOW_WIDTH = 1620;
constexpr UINT32 WINDOW_HEIGHT = 1080;
constexpr WCHAR WINDOW_TITLE[] = L"Fantasy Renderer";

Window::Window(HINSTANCE Instance, UINT32 ShowCmd)
{
    WNDCLASSEX wndClass{};
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW | WS_OVERLAPPED ;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = Instance;
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
    wndClass.lpszMenuName = nullptr;
    wndClass.lpszClassName = L"ForTheDreamOfGameDevelop";
        
    if (!RegisterClassEx(&wndClass)) assert(false && "Register Window Class Failed.");
    hwnd = CreateWindow(L"ForTheDreamofGameDevelop", WINDOW_TITLE, WS_OVERLAPPEDWINDOW^WS_THICKFRAME, CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);
    nShowCmd = ShowCmd;
    Width = WINDOW_WIDTH;
    Height = WINDOW_HEIGHT;
    
    ShowWindow(hwnd, static_cast<int>(nShowCmd));
    UpdateWindow(hwnd);


}


LRESULT Window::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_PAINT:
        ValidateRect(hwnd, NULL);
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            DestroyWindow(hwnd);
        return 0;

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
        MouseEvent.Broadcast(EMouseActionType::Down, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), false);
        return 0;
    case WM_RBUTTONDOWN:
        MouseEvent.Broadcast(EMouseActionType::Down, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), true);
        return 0;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
        MouseEvent.Broadcast(EMouseActionType::Down, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), false);
        return 0;
    case WM_RBUTTONUP:
        MouseEvent.Broadcast(EMouseActionType::Up, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), true);
        return 0;
        
    case WM_MOUSEMOVE:
        MouseEvent.Broadcast(EMouseActionType::Move, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (wParam & MK_RBUTTON) != 0);
        return 0;
        
    case WM_DESTROY:
        QuitEvent.Broadcast();
        PostQuitMessage(0);
        return 0;
        
    default:
        break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}