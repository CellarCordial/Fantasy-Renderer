#pragma once

#include "D3D12ResourceAllocator.h"

class D3D12CommandList;

struct D3D12BufferDesc
{
    bool CanAlias(const D3D12BufferDesc* rhs) const
    {
        ThrowIfFalse(rhs != nullptr, "Try to use nullptr buffer desc to compatible.");
        // ToDo: It needs the same Size and BufferViewDesc which will be Created
        return Size == rhs->Size;
    }
    
    ED3D12BufferType Type = ED3D12BufferType::Default;
    ED3D12BufferFlag Flag = ED3D12BufferFlag::None;
    ED3D12ResourceState State;
    UINT64 Size = 0;
    UINT32 Stride = 0;
    D3D12ClearValue ClearValue;
};


class D3D12Buffer
{
public:
    CLASS_NO_COPY(D3D12Buffer)

    D3D12Buffer(D3D12Device* InDevice, const D3D12BufferDesc& InDesc, D3D12Buffer* InAliasingBuffer = nullptr);
    explicit D3D12Buffer(D3D12Buffer* InBuffer);
    ~D3D12Buffer() noexcept;

public:
    void UploadData(D3D12CommandList* InCmdList, const void* InData) const;
    
    void CreateOwnCPUDescriptor();
    
    D3D12Descriptor GetCPUDescriptor() const { return CPUDescriptor; }
    ID3D12Resource* GetNative() const { return ResourceLocation.GetResource(); }
    D3D12BufferDesc* GetDesc() { return &Desc; }

    void NoNeedToRelease() { ResourceLocation.NeedRelease = false; }
    void NeedToRelease() { ResourceLocation.NeedRelease = true; }

    // Only for upload buffer
    void UpdateMappedData(void* InData, UINT32 InSize);


    void SetName(const wchar_t* InName) { ResourceLocation.GetResource()->SetName(InName); }

private:
    D3D12Device* Device;
    D3D12BufferDesc Desc;
    D3D12ResourceLocation ResourceLocation;
    D3D12Descriptor CPUDescriptor;

    /*UINT8* MappedData;*/
};
