#include "RenderGraph.h"


RenderGraph::RenderGraph()
{
    Device = std::make_unique<D3D12Device>();
    ResourcePool = std::make_unique<RenderGraphResourcePool>(Device.get());
    GraphicsFence = std::make_unique<D3D12Fence>(Device.get());
    ComputeFence = std::make_unique<D3D12Fence>(Device.get());

    for (UINT32 ix = 0; ix < RenderThreadNum; ++ix)
    {
        FrameResources.emplace_back(std::make_unique<FrameResource>(Device.get()));
        ThreadControls.emplace_back(std::make_unique<ThreadCtrl>(RenderThreadNum, false));
    }
}

RenderGraph::~RenderGraph() noexcept
{
    ReleaseCommandLists();
}

void RenderGraph::Tick()
{
    FrameIndex++;
    ResourcePool->Tick(FrameIndex);
}

void RenderGraph::Setup()
{
    ThreadControls[0]->WakeAll();

    for (auto& Pass :Passes)
    {
        Pass->Setup();
        for (auto& Resource : FrameResources)
        {
            ED3D12CommandType CommandType = ED3D12CommandType::Graphics;
            if (Pass->Desc.Type == ERenderGraphPassType::Compute) CommandType = ED3D12CommandType::Compute;
            Resource->CmdLists.emplace_back(new D3D12CommandList(Device.get(), CommandType));
        }
    }
}

void RenderGraph::Compile()
{
    std::vector<Task> Tasks(Passes.size());
    for (UINT32 ix = 0; ix < Tasks.size(); ++ix)
    {
        Tasks[ix] = ExecuteFlow.Emplace(Passes[ix].get(), &RenderGraphPass::Execute);
    }
    
    for (UINT32 i = 0; i < Passes.size(); ++i)
    {
        RenderGraphPass* CurrentPass = Passes[i].get();

        // Get resource last used pass
        for (auto Buffer : CurrentPass->ReadBuffers) Buffer->LastUsedPass = CurrentPass;
        for (auto Buffer : CurrentPass->WriteBuffers) Buffer->LastUsedPass = CurrentPass;
        for (auto Texture : CurrentPass->ReadTextures) Texture->LastUsedPass = CurrentPass;
        for (auto Texture : CurrentPass->WriteTextures) Texture->LastUsedPass = CurrentPass;

        // Set task flow
        for (UINT32 j = i + 1; j < Passes.size(); ++j)
        {
            RenderGraphPass* OtherPass = Passes[j].get();

            const auto Iterator1 = std::ranges::find_if(
                std::begin(OtherPass->ReadTextures),
                std::end(OtherPass->ReadTextures),
                [CurrentPass](RenderGraphTexture* ReadTexture)
                {
                    for (const auto& WriteTexture : CurrentPass->WriteTextures)
                    {
                        if (WriteTexture == ReadTexture) return true;
                    }
                    return false;
                }
            );

            if (Iterator1 != std::end(OtherPass->ReadTextures))
            {
                // 即一个为Graphics一个为compute或者反过来
                if (CurrentPass->Desc.Type !=  OtherPass->Desc.Type)    
                {
                    CurrentPass->SignalPass = OtherPass;
                }
                Tasks[i].Precede(Tasks[j]); continue;
            }

            const auto Iterator2 = std::ranges::find_if(
                std::begin(OtherPass->ReadBuffers),
                std::end(OtherPass->ReadBuffers),
                [CurrentPass](RenderGraphBuffer* ReadBuffer)
                {
                    for (const auto& WriteBuffer : CurrentPass->WriteBuffers)
                    {
                        if (WriteBuffer == ReadBuffer) return true;
                    }
                    return false;
                }
            );

            if (Iterator2 != std::end(OtherPass->ReadBuffers))
            {
                if (CurrentPass->Desc.Type !=  OtherPass->Desc.Type)    // 即一个为Graphics一个为compute或者反过来
                {
                    CurrentPass->SignalPass = OtherPass;
                }
                Tasks[i].Precede(Tasks[j]);
            }
        }
    }
    
    SetResourcesLastPass();
}

void RenderGraph::Execute(UINT32 InThreadIndex)
{
    ThreadControls[InThreadIndex]->Wait();
			
    Render(InThreadIndex);

    ThreadControls[InThreadIndex]->Wait();
    ThreadControls[(InThreadIndex + 1) % RenderThreadNum]->Wake();

    Submit(InThreadIndex);

    ThreadControls[InThreadIndex]->Wait();
    ThreadControls[(InThreadIndex + 1) % RenderThreadNum]->Wake();

    WaitForGPU(InThreadIndex);

    ThreadControls[(InThreadIndex + 1) % RenderThreadNum]->Wake();
}

void RenderGraph::Render(UINT32 InThreadIndex)
{
    Tick();
    ThreadIndex = InThreadIndex;  // 以便在执行过程中用到ThreadFrameIndex
    
    for (UINT32 ix = 0; ix < Passes.size(); ++ix)
    {
        Passes[ix]->SetCmdList(FrameResources[ThreadIndex]->CmdLists[ix]);
    }
    FrameResources[ThreadIndex]->CmdLists.clear();
    
    Executor.Run(ExecuteFlow);
    Device->FinishFrameAllocation();
}


void RenderGraph::Submit(UINT32 InThreadIndex)
{
    auto& CmdLists = FrameResources[InThreadIndex]->CmdLists;

    std::vector<D3D12CommandList*> GraphicsCmdLists;
    std::vector<D3D12CommandList*> ComputeCmdLists;
    for (const auto& CmdList : CmdLists)
    {
        const ED3D12CommandType CmdListType = CmdList->GetType();
        if (CmdListType == ED3D12CommandType::Graphics)
        {
            if (D3D12CommandList* WaitCmdList = CmdList->GetWaitCmdList())
            {
                Device->ExecuteGraphicsCommandLists(GraphicsCmdLists);
                Device->GetGraphicsCmdQueue()->Wait(ComputeFence.get(), WaitCmdList->GetAsyncFenceValue());

                GraphicsCmdLists.clear();
            }
            
            GraphicsCmdLists.push_back(CmdList);

            if (D3D12CommandList* SignalCmdList = CmdList->GetSignalCmdList())
            {
                Device->ExecuteGraphicsCommandLists(GraphicsCmdLists);
                Device->GetGraphicsCmdQueue()->Signal(ComputeFence.get(), ++ComputeFenceValue);
                SignalCmdList->SetAsyncFenceValue(ComputeFenceValue);

                GraphicsCmdLists.clear();
            }
        }
        else if (CmdListType == ED3D12CommandType::Compute)
        {
            if (D3D12CommandList* WaitCmdList = CmdList->GetWaitCmdList())
            {
                Device->ExecuteComputeCommandLists(ComputeCmdLists);
                Device->GetComputeCmdQueue()->Wait(ComputeFence.get(), WaitCmdList->GetAsyncFenceValue());

                ComputeCmdLists.clear();
            }
            
            ComputeCmdLists.push_back(CmdList);
            
            if (D3D12CommandList* SignalCmdList = CmdList->GetSignalCmdList())
            {
                Device->ExecuteComputeCommandLists(ComputeCmdLists);
                Device->GetComputeCmdQueue()->Signal(ComputeFence.get(), ++ComputeFenceValue);
                SignalCmdList->SetAsyncFenceValue(ComputeFenceValue);

                ComputeCmdLists.clear();
            }
        }
    }

    Device->ExecuteGraphicsCommandLists(GraphicsCmdLists);      // 再怎么样也有CopyToBackBufferPass兜底
    
    Device->GetSwapChain()->Present(InThreadIndex);

    FrameResources[InThreadIndex]->FenceValue = GraphicsFenceValue++;
    Device->GraphicsCmdQueueSignal(GraphicsFence.get(), &FrameResources[InThreadIndex]->FenceValue);
}

void RenderGraph::WaitForGPU(UINT32 InThreadIndex)
{
    Device->WaitForGPU(GraphicsFence.get(), FrameResources[InThreadIndex]->FenceValue);
    Device->ClearFrameResource();
}


void RenderGraph::UpdateConstants(UINT32 InThreadIndex, const CameraConstants* InCameraConstants, const LightConstants* InLightConstants)
{
    auto& FrameResourceData = FrameResources[InThreadIndex];
    FrameResourceData->CameraConstantBuffer->UpdateMappedData(InCameraConstants);
    FrameResourceData->LightConstantBuffer->UpdateMappedData(InLightConstants);
}

void RenderGraph::SetResourcesLastPass()
{
    for (auto& Pass : Passes)
    {
        RenderGraphPass* PassPtr = Pass.get();
            
        ResourcePool->Buffers.ForEach(
            [PassPtr](RenderGraphBuffer* InBuffer)
            {
                if (InBuffer->LastUsedPass == PassPtr)
                {
                    PassPtr->ResourcesToDestroy.push_back(InBuffer);
                }
            }
        );
        ResourcePool->Textures.ForEach(
            [PassPtr](RenderGraphTexture* InTexture)
            {
                if (InTexture->LastUsedPass == PassPtr)
                {
                    PassPtr->ResourcesToDestroy.push_back(InTexture);
                }
            }
        );
    }
}

void RenderGraph::ReleaseCommandLists()
{
    for (UINT32 ix = 0; ix < RenderThreadNum; ++ix)
    {
        for (auto& CmdList : FrameResources[ix]->CmdLists)
        {
            delete CmdList;
            CmdList = nullptr;
        }
    }
}





















