#pragma once

#include "Editor.h"
#include "../Utility/Timer.h"
#include "../Pass/Sample/SamplePass.h"

class Renderer
{
public:
    CLASS_NO_COPY(Renderer)

    Renderer();
    ~Renderer() = default;

public:
    void Init();
    void Run();
    
    void Render(UINT32 InThreadIndex);

private:
    void Update(UINT32 InThreadIndex);
    void SetupEditorPass();
    void CopyToBackBufferPass();

    void CalculateFPS() const;
    void FinishRenderThreads();

private:
    std::unique_ptr<RenderGraph> RenderGraphImpl;
    std::unique_ptr<D3D12CommandList> UploadDataCmdList;

    
    Editor EditorImpl;
    LightManager LightManagerImpl;
    ModelLoader ModelLoaderImpl;


    Timer Time;
    Camera CameraData;
    std::mutex CameraMutex;
    
    
    // Pass
    SamplePass SamplePassImpl;


    
    bool RenderFinish = false;
    bool ThreadActive[RenderThreadNum] = { false };
    TaskExecutor ThreadExecutor;
};
