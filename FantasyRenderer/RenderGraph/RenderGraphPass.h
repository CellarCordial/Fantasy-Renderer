#pragma once

#include "RenderGraphPool.h"

struct RenderGraphPassDesc
{
    std::string Name;
    ERenderGraphPassType Type = ERenderGraphPassType::Graphics;
    ED3D12RenderPassFlags Flags = ED3D12RenderPassFlags::None;

    UINT32 Width = 0;
    UINT32 Height = 0;
};

class RenderGraphPass
{
    friend class RenderGraphBuilder;
    friend class RenderGraph;

public:
    CLASS_NO_COPY(RenderGraphPass)

    RenderGraphPass(RenderGraphBuilder* InBuilder, const RenderGraphPassDesc& InDesc);
    virtual ~RenderGraphPass() noexcept;

public:
    virtual void Setup() = 0;
    virtual void Execute() = 0;
    virtual void SetCmdList(D3D12CommandList* InCmdList) = 0;

protected:
    void ResourceAllocationAndTransition();
    void BeginRenderPass();
    void EndRenderPass();
    void InActiveResources();
    
protected:
    RenderGraphPassDesc Desc;
    RenderGraphBuilder* Builder;
    D3D12CommandList* CmdList;

    std::vector<RenderGraphBuffer*> ReadBuffers;
    std::vector<RenderGraphBuffer*> WriteBuffers;
    std::vector<RenderGraphTexture*> ReadTextures;
    std::vector<RenderGraphTexture*> WriteTextures;
    std::unordered_map<RenderGraphResource*, ED3D12ResourceState> ResourceStateMap;
    
    std::vector<RenderGraphResource*> ResourcesToDestroy;
};


template <typename T>
class RenderGraphDataPass : public RenderGraphPass
{
    using SetupFunction = std::function<void(T&, RenderGraphBuilder*)>;
    using ExecuteFunction = std::function<void(const T&, RenderGraphBuilder*, D3D12CommandList*)>;
    
public:
    CLASS_NO_COPY(RenderGraphDataPass)

    RenderGraphDataPass(RenderGraphBuilder* InBuilder, const RenderGraphPassDesc& InDesc, SetupFunction&& InSetupFunction, ExecuteFunction&& InExecuteFunction)
        : RenderGraphPass(InBuilder, InDesc), SetupFunc(std::move(InSetupFunction)), ExecuteFunc(std::move(InExecuteFunction))
    {
    }
    ~RenderGraphDataPass() override = default;

public:
    void Setup() override
    {
        SetupFunc(Data, Builder);
    }
    void Execute() override
    {
        ThrowIfFalse(CmdList != nullptr, "Try to use nullptr RenderGraphBuilder | D3D12CommandList.");

        CmdList->Begin();
        
        ResourceAllocationAndTransition();

        BeginRenderPass();
        ExecuteFunc(Data, Builder, CmdList);
        EndRenderPass();

        CmdList->Close();

        InActiveResources();
    }

    void SetCmdList(D3D12CommandList* InCmdList) override{ CmdList = InCmdList; }
    
private:
    T Data;

    SetupFunction SetupFunc;
    ExecuteFunction ExecuteFunc;
};
