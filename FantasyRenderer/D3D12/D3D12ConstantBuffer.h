#pragma once

#include "D3D12Device.h"

template <typename T>
class D3D12ConstantBuffer
{
public:
    CLASS_NO_COPY(D3D12ConstantBuffer)

    D3D12ConstantBuffer(D3D12Device* InDevice, UINT32 InConstantNum = 1, const T* InMappedData = nullptr)
        : ResourceLocation(InDevice->GetResourceAllocator(), ED3D12ResourceLocationType::ConstantBuffer)
    {
        D3D12ResourceLocationDesc LocationDesc{};
        LocationDesc.Size = sizeof(T) * InConstantNum;
        D3D12ResourceAllocator* ResourceAllocator = InDevice->GetResourceAllocator();
        ThrowIfFalse(ResourceAllocator->TryAllocate(&ResourceLocation, &LocationDesc), "Memory allocate failed.");

        MappedData = ResourceAllocator->GetConstantMappedData();

        if (InMappedData) UpdateMappedData(InMappedData);
    }
    ~D3D12ConstantBuffer() = default;

public:
    void UpdateMappedData(const T* InMappedData) const
    {
        memcpy(MappedData + GetOffset(), InMappedData, sizeof(T));
    }
    
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const { return GetNative()->GetGPUVirtualAddress() + GetOffset(); }

private:
    UINT64 GetOffset() const { return ResourceLocation.GetOffset(); }
    ID3D12Resource* GetNative() const { return ResourceLocation.GetResource(); }

private:
    D3D12ResourceLocation ResourceLocation;
    UINT8* MappedData = nullptr;
};
