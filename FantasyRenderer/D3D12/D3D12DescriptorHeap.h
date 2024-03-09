#pragma once

#include <queue>

#include "D3D12Descriptor.h"
#include "../MultiThreading/ConcurrentFreeListAllocator.h"
#include "../MultiThreading/ConcurrentRingAllocator.h"

struct D3D12DescriptorHeapDesc
{
    ED3D12DescriptorType Type;
    UINT32 DescriptorNum;
    bool ShaderVisible;
};

class D3D12DescriptorHeap
{
public:
    CLASS_NO_COPY(D3D12DescriptorHeap)

    D3D12DescriptorHeap(D3D12Device* InDevice, const D3D12DescriptorHeapDesc& InDesc);
    ~D3D12DescriptorHeap() = default;

public:
    D3D12Descriptor GetDescriptor(UINT64 InDescriptorIndex) const;
    ID3D12DescriptorHeap* GetDescriptorHeap() const { return DescriptorHeap.Get(); }

private:
    D3D12Device* Device;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeap;
    D3D12DescriptorHeapDesc Desc;

    D3D12Descriptor HeadDescriptor;
    UINT32 DescriptorSize;
};

class D3D12CPUDescriptorHeap : public D3D12DescriptorHeap
{
public:
    CLASS_NO_COPY(D3D12CPUDescriptorHeap)

    D3D12CPUDescriptorHeap(D3D12Device* InDevice, const D3D12DescriptorHeapDesc& InDesc);
    ~D3D12CPUDescriptorHeap() = default;

public:
    bool TryAllocate(D3D12Descriptor* OutDescriptor);
    bool TryFree(D3D12Descriptor InDescriptor);
    void ClearFrameDescriptors();
    
private:
    ConcurrentFreeListAllocator FreeList;
};

class D3D12GPUDescriptorHeap : public D3D12DescriptorHeap
{
public:
    CLASS_NO_COPY(D3D12GPUDescriptorHeap)
    
    D3D12GPUDescriptorHeap(D3D12Device* InDevice, const D3D12DescriptorHeapDesc& InDesc);
    ~D3D12GPUDescriptorHeap() = default;

public:
    bool TryAllocate(D3D12Descriptor* OutDescriptor, UINT32 InNum = 1);
    void FinishFrameAllocation();
    void ClearFrameDescriptors();

public:
    D3D12Descriptor GetPreservedDescriptor() const { return PreservedDescriptor; }

private:
    ConcurrentRingAllocator RingBuffer;

    std::mutex Mutex;
    std::queue<UINT32> FrameAllocations;
    UINT32 CurrentFrameAllocation = 0;

    D3D12Descriptor PreservedDescriptor;
};