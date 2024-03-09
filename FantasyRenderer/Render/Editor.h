#pragma once

#include "Camera.h"


#include "../External/imgui/imgui.h"
#include "../External/imgui/imgui_impl_win32.h"
#include "../External/imgui/imgui_impl_dx12.h"

class Editor
{
public:
    void Initialize(D3D12Device* InDevice);
    void Destroy();

    void CallEditFuncitons();

    // Use it in RenderGraph Execute function
    static void AddEditFunciton(std::function<void()>&& InFunction);
    static void ResetEditFunction(); 
private:
    inline static std::mutex Mutex;

    inline static std::vector<std::function<void()>> EditFunctions;
};
