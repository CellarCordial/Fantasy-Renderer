#pragma once

#include "RenderGraphResource.h"

class RenderGraphResourcePool
{
    friend class RenderGraphBuilder;
    friend class RenderGraph;
    
public:
    CLASS_NO_COPY(RenderGraphResourcePool)
    
    RenderGraphResourcePool(D3D12Device* InDevice) : Device(InDevice) {}
    ~RenderGraphResourcePool() = default;

public:
    void Tick(UINT64 InFrameIndex);
    void Clear();
    
    void AllocateBuffer(RenderGraphBuffer* InBuffer, D3D12CommandList* InCmdList);
    void AllocateTexture(RenderGraphTexture* InTexture, D3D12CommandList* InCmdList);

private:
    D3D12Device* Device;

    ConcurrentList<RenderGraphBuffer> Buffers;
    ConcurrentList<RenderGraphTexture> Textures;
};
