#include "Editor.h"

void Editor::Initialize(D3D12Device* InDevice)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    D3D12Descriptor ImguiDescriptor = InDevice->GetPreservedGPUDescriptor();    
    
    ImGui_ImplWin32_Init(Window::GetHWND());
    ImGui_ImplDX12_Init(InDevice->GetNative(), SWAP_CHAIN_BUFFER_COUNT,
        SWAP_CHAIN_FORMAT, InDevice->GetGPUDescriptorHeap(),
        ImguiDescriptor.GetCPUHandle(),
        ImguiDescriptor.GetGPUHandle());
}

void Editor::Destroy()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Editor::CallEditFuncitons()
{
    for (const auto& Function : EditFunctions) { Function(); }
    EditFunctions.clear();
}

void Editor::AddEditFunciton(std::function<void()>&& InFunction)
{
    {
        std::lock_guard Lock_Guard(Mutex);
        EditFunctions.push_back(std::move(InFunction));
    }
}

void Editor::ResetEditFunction()
{
    {
        std::lock_guard LockGuard(Mutex);
        EditFunctions.clear();
    }
}
