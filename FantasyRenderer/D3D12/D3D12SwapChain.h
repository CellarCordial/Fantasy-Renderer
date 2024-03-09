#pragma once

#include "D3D12CommandList.h"

struct D3D12SwapChainDesc
{
    UINT32 Width;
    UINT32 Height;
};

class D3D12SwapChain
{
public:
    CLASS_NO_COPY(D3D12SwapChain)

    D3D12SwapChain(D3D12Device* InDevice, const D3D12SwapChainDesc& InDesc);
    ~D3D12SwapChain() = default;

public:
    void Present(UINT32 InThreadIndex);
    void ResizeWindow();
    UINT32 GetCurrentBackBufferIndex() const { return CurrentBackBufferIndex; }
    ID3D12Resource* GetCurrentBackBuffer() const { return BackBuffers[CurrentBackBufferIndex].Get(); }
    ID3D12Resource* GetDepthStencilBuffer() const { return DepthStencilBuffer.Get(); }
    D3D12Descriptor GetCurrentBackBufferView() const { return BackBufferDescriptors[CurrentBackBufferIndex]; }
    D3D12Descriptor GetDepthStencilBufferView() const { return DepthStencilDescriptor; }

    ID3D12Resource* GetBackBuffer(UINT32 InThreadIndex) const { return BackBuffers[InThreadIndex].Get(); }
    D3D12Descriptor GetBackBufferView(UINT32 InThreadIndex) const { return BackBufferDescriptors[InThreadIndex]; }
    
private:
    void CreateBackBuffers();
    void CreateDepthStencilBuffer();

private:
    D3D12Device* Device;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> SwapChain;
    Microsoft::WRL::ComPtr<ID3D12Resource> BackBuffers[SWAP_CHAIN_BUFFER_COUNT];
    D3D12Descriptor BackBufferDescriptors[SWAP_CHAIN_BUFFER_COUNT];

    Microsoft::WRL::ComPtr<ID3D12Resource> DepthStencilBuffer;
    D3D12Descriptor DepthStencilDescriptor;

    UINT8 CurrentBackBufferIndex = 0;
};
