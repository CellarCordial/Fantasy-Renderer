#pragma once
#include <memory>

#include "RenderGraphBuilder.h"
#include "../MultiThreading/ThreadCtrl.h"

struct FrameResource
{
    FrameResource(D3D12Device* InDevice)
        : CameraConstantBuffer(std::make_unique<D3D12ConstantBuffer<CameraConstants>>(InDevice)),
          LightConstantBuffer(std::make_unique<D3D12ConstantBuffer<LightConstants>>(InDevice))
    {}
    
    UINT64 FenceValue = 0;
    
    std::mutex CmdListsMutex;
    std::vector<D3D12CommandList*> CmdLists;
    std::unique_ptr<D3D12ConstantBuffer<CameraConstants>> CameraConstantBuffer;
    std::unique_ptr<D3D12ConstantBuffer<LightConstants>> LightConstantBuffer;

};

class RenderGraph
{
    friend class RenderGraphBuilder;
    
public:
    CLASS_NO_COPY(RenderGraph)
    
    RenderGraph();
    ~RenderGraph() noexcept;

public:
    template <typename T, typename... Args>
    requires std::is_constructible_v<RenderGraphDataPass<T>, RenderGraphBuilder*, Args...>
    void AddPass(Args&&... Arguments)
    {
        RenderGraphBuilder* Builder = new RenderGraphBuilder(this);
        Passes.emplace_back(std::make_unique<RenderGraphDataPass<T>>(Builder, std::forward<Args>(Arguments)...));
    }

    void Setup();
    void Compile();
    void Execute(UINT32 InThreadIndex);

    void UpdateConstants(UINT32 InThreadIndex, const CameraConstants* InCameraConstants, const LightConstants* InLightConstants);

public:
    UINT64 GetFrameIndex() const { return FrameIndex; }
    D3D12Device* GetDevice() const { return Device.get(); }
    
private:
    void Tick();
    
    void Render(UINT32 InThreadIndex);
    void Submit(UINT32 InThreadIndex);
    void WaitForGPU(UINT32 InThreadIndex);

    void SetResourcesLastPass();
    void ReleaseCommandLists();
    
private:
    UINT64 FrameIndex = 0;
    
    std::unique_ptr<D3D12Device> Device;
    std::vector<std::unique_ptr<RenderGraphPass>> Passes;
    std::unique_ptr<RenderGraphResourcePool> ResourcePool;

    TaskFlow ExecuteFlow;
    TaskExecutor Executor;
    
    std::unique_ptr<D3D12Fence> Fence;
    UINT64 FenceValue = 0;

    std::vector<std::unique_ptr<FrameResource>> FrameResources;
    UINT32 ThreadIndex = 0;

    std::vector<std::unique_ptr<ThreadCtrl>> ThreadControls;
};
