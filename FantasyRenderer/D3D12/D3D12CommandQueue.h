#pragma once

#include "D3D12PipelineStateCache.h"

class D3D12CommandList;

class D3D12CommandQueue
{
public:
    CLASS_NO_COPY(D3D12CommandQueue)

    D3D12CommandQueue(D3D12Device* InDevice, ED3D12CommandType InType);
    ~D3D12CommandQueue() = default;

public:
    void ExecuteCommandLists(std::span<D3D12CommandList*> InCmdLists) const;
    
    void Signal(const D3D12Fence* InFence, UINT64 InFenceValue) const;

    ID3D12CommandQueue* GetNative() const { return CmdQueue.Get(); }

private:
    ED3D12CommandType CommandType;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> CmdQueue;
};
