#pragma once

#include "D3D12CommandQueue.h"

struct D3D12RenderPassDesc
{
    std::vector<D3D12Texture*> RenderTargets;
    std::vector<ED3D12AccessType> RenderTargetAccessTypes;
    D3D12Texture* DepthStencil = nullptr;
    ED3D12AccessType DepthAccessType;
    ED3D12AccessType StencilAccessType;
    
    ED3D12RenderPassFlags Flags = ED3D12RenderPassFlags::None;
};

class D3D12CommandList
{
public:
    CLASS_NO_COPY(D3D12CommandList)

    D3D12CommandList(D3D12Device* InDevice, ED3D12CommandType InType);
    ~D3D12CommandList() noexcept;

public:
    void Begin();
    void Close();

    void BeginRenderPass(const D3D12RenderPassDesc& InDesc) const;
    void EndRenderPass() const;

    void SetViewport(UINT32 InWidth, UINT32 InHeight) const;
    void SetPipelineState(ID3D12PipelineState* InPipelineState) const;
    void DrawIndexedInstance(D3D12Buffer* InVertexBuffer, D3D12Buffer* InIndexBuffer, UINT32 InVertexSize, UINT32 InIndexNum, UINT32 InInstanceNum = 1) const;
    
    void FlushBarriers();

    void CreateBufferAliasingBarrier(D3D12Buffer* InOldBuffer, D3D12Buffer* InNewBuffer);
    void CreateTextureAliasingBarrier(D3D12Texture* InOldTexture, D3D12Texture* InNewTexture);
    void CreateAliasingBarrier(ID3D12Resource* InOldResource, ID3D12Resource* InNewResource);
    
    void CreateBufferTransitionBarrier(D3D12Buffer* InBuffer, ED3D12ResourceState InNewState);
    void CreateTextureTransitionBarrier(D3D12Texture* InTexture, ED3D12ResourceState InNewState);
    void CreateTransitionBarrier(ID3D12Resource* InResource, ED3D12ResourceState InOldState, ED3D12ResourceState InNewState);

    ID3D12GraphicsCommandList* GetNative() const { return CmdList.Get(); }
    D3D12Device* GetDevice() const { return Device;}

    void AddUploadBuffer(D3D12Buffer* InBuffer) { PendingUploadBuffers.push_back(InBuffer); }
    void FreeUploadBuffers();

private:

private:
    D3D12Device* Device;
    
    ED3D12CommandType Type;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> CmdList;

    std::vector<D3D12_RESOURCE_BARRIER> PendingBarriers;
    std::vector<D3D12Buffer*> PendingUploadBuffers;
};
