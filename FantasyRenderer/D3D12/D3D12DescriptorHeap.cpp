#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"

D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12Device* InDevice, const D3D12DescriptorHeapDesc& InDesc)
    : Device(InDevice),
      Desc(InDesc)
{
    ThrowIfFalse(Desc.DescriptorNum < D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_1, "Descriptor Num is too big.");

    D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc;
    DescriptorHeapDesc.Flags = Desc.ShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DescriptorHeapDesc.Type = ConvertToD3D12DescriptorHeapType(Desc.Type);
    DescriptorHeapDesc.NumDescriptors = Desc.DescriptorNum;
    DescriptorHeapDesc.NodeMask = 0;    // 对于单Adapter操作，请将此设置为零

    ThrowIfFailed(Device->GetNative()->CreateDescriptorHeap(&DescriptorHeapDesc, IID_PPV_ARGS(DescriptorHeap.GetAddressOf())));

    DescriptorSize = Device->GetNative()->GetDescriptorHandleIncrementSize(ConvertToD3D12DescriptorHeapType(Desc.Type));
    HeadDescriptor.SetIndex(0);
    HeadDescriptor.SetCPUDescriptorHandle(DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    if (Desc.ShaderVisible) HeadDescriptor.SetGPUDescriptorHandle(DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

D3D12Descriptor D3D12DescriptorHeap::GetDescriptor(UINT64 InDescriptorIndex) const
{
    ThrowIfFalse(InDescriptorIndex < Desc.DescriptorNum, "Index is beyond the descriptor's Num in heap.");

    D3D12Descriptor Output = HeadDescriptor;
    Output.Increase(DescriptorSize, InDescriptorIndex);
    return Output;
}

D3D12CPUDescriptorHeap::D3D12CPUDescriptorHeap(D3D12Device* InDevice, const D3D12DescriptorHeapDesc& InDesc)
    : D3D12DescriptorHeap(InDevice, InDesc),
      FreeList(InDesc.DescriptorNum)
{
}

bool D3D12CPUDescriptorHeap::TryAllocate(D3D12Descriptor* OutDescriptor)
{
    UINT64 Index;
    if (FreeList.TryAllocate(&Index))
    {
        *OutDescriptor = GetDescriptor(Index);
        return true;
    }
    return false;
}

bool D3D12CPUDescriptorHeap::TryFree(D3D12Descriptor InDescriptor)
{
    const UINT32 Index = InDescriptor.GetIndex();
    return FreeList.TryFree(Index);
}

void D3D12CPUDescriptorHeap::ClearFrameDescriptors()
{
    FreeList.Clear();
}

D3D12GPUDescriptorHeap::D3D12GPUDescriptorHeap(D3D12Device* InDevice, const D3D12DescriptorHeapDesc& InDesc)
    : D3D12DescriptorHeap(InDevice, InDesc),
      RingBuffer(InDesc.DescriptorNum - PRESERVED_GPU_DESCRIPTOR_NUM)
{
    const UINT32 LastIndex = InDesc.DescriptorNum - PRESERVED_GPU_DESCRIPTOR_NUM;
    PreservedDescriptor = GetDescriptor(LastIndex);
}

bool D3D12GPUDescriptorHeap::TryAllocate(D3D12Descriptor* OutDescriptor, UINT32 InNum/* = 1*/)
{
    size_t Index;
    if (RingBuffer.TryAllocate(&Index, InNum))
    {
        *OutDescriptor = GetDescriptor(Index);
        CurrentFrameAllocation += InNum;
        return true;
    }
    return false;
}

void D3D12GPUDescriptorHeap::FinishFrameAllocation()
{
    std::lock_guard LockGuard(Mutex);

    FrameAllocations.push(CurrentFrameAllocation);
    CurrentFrameAllocation = 0;
}

void D3D12GPUDescriptorHeap::ClearFrameDescriptors()
{
    std::lock_guard LockGuard(Mutex);

    const UINT32 Size = FrameAllocations.front();
    FrameAllocations.pop();
    RingBuffer.TryFree(Size);
}
