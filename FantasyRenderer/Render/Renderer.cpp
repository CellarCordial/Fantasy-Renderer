#include "Renderer.h"

Renderer::Renderer()
{
    RenderGraphImpl = std::make_unique<RenderGraph>();
    UploadDataCmdList = std::make_unique<D3D12CommandList>(RenderGraphImpl->GetDevice(), ED3D12CommandType::Graphics);
    Window::GetQuitWindowEvent()->AddEvent(this, &Renderer::FinishRenderThreads);

    SamplePassImpl.Init(RenderGraphImpl.get(), &ModelLoaderImpl, &LightManagerImpl);
}


void Renderer::Init()
{
    ModelLoadDesc ModelDesc{};
    ModelDesc.ModelFilePath = "Asset/GLTFModel/Sponza/glTF/Sponza.gltf";
    ModelDesc.TextureFilePath = "Asset/GLTFModel/Sponza/glTF/";
    SamplePassImpl.Setup(ModelDesc, UploadDataCmdList.get());
    SetupEditorPass();
    CopyToBackBufferPass();
}


void Renderer::Update(UINT32 InThreadIndex)
{
    CameraConstants CameraConstantData;
    DirectX::XMMATRIX Proj;
    DirectX::XMMATRIX View;
    
    {
        std::lock_guard CameraLock(CameraMutex);
        CameraData.HandleKeyboardInput(Time.Tick());
        Proj = CameraData.GetProj();
        View = CameraData.GetView();
    }
    
    DirectX::XMStoreFloat4x4(&CameraConstantData.View, DirectX::XMMatrixTranspose(View));
    DirectX::XMStoreFloat4x4(&CameraConstantData.Proj, DirectX::XMMatrixTranspose(Proj));
    DirectX::XMStoreFloat4x4(&CameraConstantData.ViewProj, XMMatrixMultiply(View, Proj));

    LightConstants LightConstantData;

    DirectLight* DirectLightData = LightManagerImpl.GetDirectLightData();
    LightConstantData.DirectLightNum = LightManagerImpl.GetDirectLightNum();
    for (UINT32 ix = 0; ix < LightConstantData.DirectLightNum; ++ix) LightConstantData.DirectLights[ix] = DirectLightData[ix];

    PointLight* PointLightData = LightManagerImpl.GetPointLightData();
    LightConstantData.PointLightNum = LightManagerImpl.GetPointLightNum();
    for (UINT32 ix = 0; ix < LightConstantData.PointLightNum; ++ix) LightConstantData.PointLights[ix] = PointLightData[ix];

    SpotLight* SpotLightData = LightManagerImpl.GetSpotLightData();
    LightConstantData.SpotLightNum = LightManagerImpl.GetSpotLightNum();
    for (UINT32 ix = 0; ix < LightConstantData.SpotLightNum; ++ix) LightConstantData.SpotLights[ix] = SpotLightData[ix];
    
    RenderGraphImpl->UpdateConstants(InThreadIndex, &CameraConstantData, &LightConstantData);
}


void Renderer::Run()
{
    RenderGraphImpl->Setup();
    RenderGraphImpl->Compile();

    RenderFinish = false;
    constexpr UINT32 BeginThreadIndex = 0, MidThreadIndex = 1, EndThreadIndex = 2;
    ThreadExecutor.BeginThread(this, &Renderer::Render, BeginThreadIndex);
    ThreadExecutor.BeginThread(this, &Renderer::Render, MidThreadIndex);
    ThreadExecutor.BeginThread(this, &Renderer::Render, EndThreadIndex);
}


void Renderer::CalculateFPS() const
{
    static int FrameCount = 0;
    static float TimeElapsed = 0.0f;

    FrameCount++;

    if(Time.Elapsed() - TimeElapsed >= 1.0f)
    {
        const std::wstring MainWndCaption = L"D3D12Test";
        const std::wstring FrameCountString = std::to_wstring(RenderGraphImpl->GetFrameIndex());
        const std::wstring FPSString = std::to_wstring(FrameCount);

        const std::wstring windowText = MainWndCaption +
            L"   Frame: " + FrameCountString +
            L"   FPS: "	  + FPSString;

        SetWindowText(Window::GetHWND(), windowText.c_str());
		
        FrameCount = 0;
        TimeElapsed += 1.0f;
    }
}

void Renderer::Render(UINT32 InThreadIndex)
{
    ThreadActive[InThreadIndex] = true;
    while (!RenderFinish)
    {
        Update(InThreadIndex);
        RenderGraphImpl->Execute(InThreadIndex);
    }
    ThreadActive[InThreadIndex] = false;
}

void Renderer::SetupEditorPass()
{
    struct EditorPassConstants
    {
    };

    RenderGraphPassDesc Desc{};
    Desc.Flags = ED3D12RenderPassFlags::None;
    Desc.Height = Window::GetHeight();
    Desc.Width = Window::GetWidth();
    Desc.Type = ERenderGraphPassType::None;
    
    RenderGraphImpl->AddPass<EditorPassConstants>(
        Desc,
        [](EditorPassConstants& OutData, RenderGraphBuilder* InBuilder)
        {
        },
        [this](const EditorPassConstants& InData, RenderGraphBuilder* InBuilder, D3D12CommandList* InCmdList)
        {
              EditorImpl.CallEditFuncitons();
        }
    );
}

void Renderer::CopyToBackBufferPass()
{
    D3D12Device* Device = RenderGraphImpl->GetDevice();
    
    struct CopyBackBufferPassData
    {
        RGTextureRef Src1;
    };

    RenderGraphPassDesc PassDesc{};
    PassDesc.Name = "CopyToBackBuffer";
    PassDesc.Type = ERenderGraphPassType::Copy;
    RenderGraphImpl->AddPass<CopyBackBufferPassData>(
        PassDesc,
        [=](CopyBackBufferPassData& OutData, RenderGraphBuilder* InBuilder)
        {
            OutData.Src1 = InBuilder->TransitionReadTexture("FinalTexture", ED3D12ResourceState::CopySrc);
        },
        [=](const CopyBackBufferPassData& InData, RenderGraphBuilder* InBuilder, D3D12CommandList* InCmdList)
        {
            const UINT32 ThreadFrameIndex = InBuilder->GetThreadIndex();
            InCmdList->CreateTransitionBarrier(Device->GetSwapChain()->GetBackBuffer(ThreadFrameIndex), ED3D12ResourceState::Present, ED3D12ResourceState::CopyDst);
            InCmdList->FlushBarriers();
            InCmdList->GetNative()->CopyResource(Device->GetSwapChain()->GetBackBuffer(ThreadFrameIndex), InData.Src1->Texture.get()->GetNative());
            InCmdList->CreateTransitionBarrier(Device->GetSwapChain()->GetBackBuffer(ThreadFrameIndex), ED3D12ResourceState::CopyDst, ED3D12ResourceState::Present);
            InCmdList->FlushBarriers();
        }
    );
}

void Renderer::FinishRenderThreads()
{
    RenderFinish = true;
    while (true)
    {
        UINT32 ActiveThreadNum = 0;
        for (const auto& Active : ThreadActive)
        {
            if (Active) ActiveThreadNum++;
        }
        if (ActiveThreadNum == 0) break;
        else std::this_thread::yield();
    }
}























