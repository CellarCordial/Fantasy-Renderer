#pragma once

#include "D3D12Fence.h"

class D3D12Descriptor
{
public:
    CLASS_DEFAULT_COPY(D3D12Descriptor)

    D3D12Descriptor() = default;
    ~D3D12Descriptor() = default;

    friend bool operator==(const D3D12Descriptor& lhs, const D3D12Descriptor& rhs) noexcept
    {
        return lhs.CPUHandle.ptr == rhs.CPUHandle.ptr || lhs.GPUHandle.ptr == rhs.GPUHandle.ptr;
    }

public:
    void Increase(UINT64 IncrementSize, UINT64 IncrementNum)
    {
        const size_t TotalIncrement = IncrementSize * IncrementNum;
        
        CPUHandle.ptr += TotalIncrement;
        if (GPUHandle.ptr) GPUHandle.ptr += TotalIncrement;
        Index += IncrementNum;
    }
    
    bool IsValid() const
    {
        return CPUHandle.ptr != NULL;
    }
    
    void Reset() { Index = 0; CPUHandle.ptr = NULL; GPUHandle.ptr = NULL; }
    void SetIndex(UINT32 InHeapIndex) { Index = InHeapIndex; }
    void SetType(ED3D12DescriptorType InType) { Type = InType; }
    void SetCPUDescriptorHandle(const D3D12_CPU_DESCRIPTOR_HANDLE& InHandle) { CPUHandle = InHandle; }
    void SetGPUDescriptorHandle(const D3D12_GPU_DESCRIPTOR_HANDLE& InHandle) { GPUHandle = InHandle; }

    UINT32 GetIndex() const { return static_cast<UINT32>(Index); }
    ED3D12DescriptorType GetType() const { return Type; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return CPUHandle; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const { return GPUHandle; }
    

private:
    UINT64 Index = 0;
    ED3D12DescriptorType Type;
    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle{ NULL };
    D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle{ NULL };
};
