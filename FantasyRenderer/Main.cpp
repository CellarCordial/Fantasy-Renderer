#include "Render/Renderer.h"
#include "Window/Window.h"
#include "Utility/Exception.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    Window WinApp(hInstance, nShowCmd);
    
    Renderer Render;
    Render.Init();
    Render.Run();
    try
    {
        static Timer Time;
        Time.Tick();
        MSG msg{};
        while (msg.message != WM_QUIT)
        {
            if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    catch (const Exception& e)
    {
        MessageBoxA(nullptr, e.GetErrorMessage().c_str(), nullptr, 0);
    }

}
