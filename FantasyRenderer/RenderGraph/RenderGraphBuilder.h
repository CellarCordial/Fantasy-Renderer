#pragma once

#include "RenderGraphPass.h"


class RenderGraphBuilder
{
    friend class RenderGraph;
    friend class RenderGraphPass;    
public:
    CLASS_NO_COPY(RenderGraphBuilder)
    RenderGraphBuilder(RenderGraph* InGraph) : Graph(InGraph) {}
    ~RenderGraphBuilder() = default;

public:

    // Import resource must has its own cpu descriptor
    RenderGraphBuffer* ImportBuffer(const char* InName, D3D12Buffer* InBuffer, bool NeedDescriptor) const;
    RenderGraphTexture* ImportTexture(const char* InName, D3D12Texture* InTexture, bool NeedDescriptor) const;
    
    RenderGraphBuffer* TransitionReadBuffer(const char* InName, ED3D12ResourceState InState) const;
    RenderGraphTexture* TransitionReadTexture(const char* InName, ED3D12ResourceState InState) const;
    RenderGraphBuffer* TransitionWriteBuffer(const char* InName, ED3D12ResourceState InState) const;
    RenderGraphTexture* TransitionWriteTexture(const char* InName, ED3D12ResourceState InState) const;
    
    RenderGraphBuffer* DeclareReadBuffer(const char* InName, const D3D12BufferDesc& InDesc, void* InData = nullptr) const;
    RenderGraphBuffer* DeclareWriteBuffer(const char* InName, const D3D12BufferDesc& InDesc) const;
    RenderGraphTexture* DeclareReadTexture(const char* InName, const D3D12TextureDesc& InDesc, void* InData = nullptr) const;
    RenderGraphTexture* DeclareWriteTexture(const char* InName, const D3D12TextureDesc& InDesc) const;

    void AllocateBuffer(RenderGraphBuffer* InBuffer, D3D12CommandList* InCmdList) const;
    void AllocateTexture(RenderGraphTexture* InTexture, D3D12CommandList* InCmdList) const;

    UINT32 GetThreadIndex() const;
    UINT64 GetFrameIndex() const;
    FrameResource* GetFrameResource();
    void SubmitCmdList(D3D12CommandList* InCmdList);

private:
    void SetPass(RenderGraphPass* InPass) { Pass = InPass; }
    
private:
    RenderGraph* Graph = nullptr;
    RenderGraphPass* Pass = nullptr;
};
