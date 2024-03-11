#pragma once

#include "D3D12SwapChain.h"

class D3D12Device
{
public:
    CLASS_NO_COPY(D3D12Device)
    
    D3D12Device();
    ~D3D12Device() = default;

public:
    void FlushGraphicsCmdQueue();   // 使用内置的Fence
    void GraphicsCmdQueueSignal(D3D12Fence* InFence, UINT64* InOutFenceValue);
    void WaitForGPU(D3D12Fence* InFence, UINT64 InFenceValue);
    void ResizeWindow();
    
    void ExecuteGraphicsCommandLists(std::span<D3D12CommandList*>) const;
    void ExecuteComputeCommandLists(std::span<D3D12CommandList*>) const;
    D3D12Descriptor AllocateCPUDescriptor(ED3D12DescriptorType InType) const;
    D3D12Descriptor AllocateGPUDescriptor(UINT32 InNum = 1) const;
    void FinishFrameAllocation();
    void ClearFrameResource();
    void FreeCPUDescriptor(D3D12Descriptor InDescriptor) const;
    D3D12Descriptor GetGPUDescriptor(UINT32 Index) const;
    D3D12Descriptor GetPreservedGPUDescriptor() const;
    

    void CopyDescriptor(const D3D12Descriptor& InDst, const D3D12Descriptor& InSrc) const;
    D3D12Descriptor CreateBufferView(D3D12Buffer* InBuffer);
    D3D12Descriptor CreateTextureView(D3D12Texture* InTexture) const;
    

    ID3D12Device* GetNative() const { return Device.Get(); }
    IDXGIFactory4* GetFactory() const { return Factory.Get(); }
    D3D12CommandQueue* GetGraphicsCmdQueue() const { return GraphicsCmdQueue.get(); }
    D3D12CommandQueue* GetComputeCmdQueue() const { return ComputeCmdQueue.get(); }
    D3D12ResourceAllocator* GetResourceAllocator() const { return ResourceAllocator.get(); }
    ID3D12DescriptorHeap* GetGPUDescriptorHeap() const { return GPUDescriptorHeap->GetDescriptorHeap(); }
    ID3D12RootSignature* GetRootSignature() const { return RootSignature.Get(); }
    ID3D12PipelineState* GetPipelineState(ED3D12PipelineStateID InID) const { return PipelineStateCache->GetPipelineState(InID); }
    D3D12SwapChain* GetSwapChain() const { return SwapChain.get(); }
    
private:
    void CreateRootSignature();
    D3D12Descriptor CreateBufferSRV(D3D12Buffer* InBuffer) const;
    D3D12Descriptor CreateBufferUAV(D3D12Buffer* InBuffer) const;
    D3D12Descriptor CreateTextureRTV(D3D12Texture* InTexture) const;
    D3D12Descriptor CreateTextureDSV(D3D12Texture* InTexture) const;
    D3D12Descriptor CreateTextureSRV(D3D12Texture* InTexture) const;

private:
    Microsoft::WRL::ComPtr<ID3D12Device5> Device;
    Microsoft::WRL::ComPtr<IDXGIFactory4> Factory;

    UINT64 FenceValue = 0;
    std::unique_ptr<D3D12Fence> Fence;
    
    std::unique_ptr<D3D12CommandQueue> GraphicsCmdQueue;
    std::unique_ptr<D3D12CommandQueue> ComputeCmdQueue;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
    std::unique_ptr<D3D12SwapChain> SwapChain;

    std::vector<std::unique_ptr<D3D12CPUDescriptorHeap>> CPUDescriptorHeaps;
    std::unique_ptr<D3D12GPUDescriptorHeap> GPUDescriptorHeap;

    std::unique_ptr<D3D12ResourceAllocator> ResourceAllocator;

    std::unique_ptr<D3D12ShaderCache> ShaderCache;
    std::unique_ptr<D3D12PipelineStateCache> PipelineStateCache;
};
