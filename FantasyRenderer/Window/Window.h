#pragma once
#include <cassert>
#include <windows.h>
#include "../Utility/Delegate.h"
#include "../Utility/Timer.h"

enum class EMouseActionType
{
    Up,
    Down,
    Move
};

enum class EResizeState
{
    Begin,
    End
};

DeclareDelegateEvent(ResizeWindowEvent, Window, EResizeState)
DeclareDelegateEvent(QuitWindowEvent, Window)
DeclareDelegateEvent(MouseActionEvent, Window, EMouseActionType, INT32, INT32, bool)

class Window
{
public:
    Window(HINSTANCE Instance, UINT32 ShowCmd);
    ~Window() = default;
    
    Window(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(const Window&) = delete;
    Window& operator=(Window&&) = delete;

public:
    static UINT32 GetWidth() { return Width; }
    static UINT32 GetHeight() { return Height; }
    static HWND GetHWND() { return hwnd; }
    static void Resize(UINT32 InWidth, UINT32 InHeight) { Width = InWidth; Height = InHeight; }

    static ResizeWindowEvent* GetResizeEvent() { return &ResizeEvent; }
    static QuitWindowEvent* GetQuitWindowEvent() { return &QuitEvent; }
    static MouseActionEvent* GetMouseActionEvent() { return &MouseEvent; }

protected:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    inline static HWND hwnd;
    inline static HINSTANCE hInstance;
    inline static UINT32 nShowCmd;

    inline static UINT32 Width;
    inline static UINT32 Height;

    inline static ResizeWindowEvent ResizeEvent;
    inline static QuitWindowEvent QuitEvent;
    inline static MouseActionEvent MouseEvent;
};
