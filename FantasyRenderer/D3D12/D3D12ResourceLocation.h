#pragma once
#include <memory>

#include "../Utility/Macros.h"
#include "D3D12DescriptorHeap.h"

class D3D12ResourceAllocator;

struct D3D12ResourceLocationDesc
{
    D3D12_RESOURCE_DESC* ResourceDesc = nullptr;
    D3D12_RESOURCE_STATES ResourceState = D3D12_RESOURCE_STATE_COMMON;
    D3D12_CLEAR_VALUE* ClearValue;
    UINT64 Size = 0;
};

class D3D12ResourceLocation
{
    friend class D3D12Buffer;
    friend class D3D12Texture;

    struct LocationData
    {
        UINT64 Offset = 0;
        UINT64 Size = 0;
    };
    
public:
    CLASS_NO_COPY(D3D12ResourceLocation)

    D3D12ResourceLocation(D3D12ResourceAllocator* InAllocator, ED3D12ResourceLocationType InType);
    ~D3D12ResourceLocation() noexcept;

public:
    void SetResource(ID3D12Resource* InResource);
    void SetLocation(UINT64 InOffset, UINT64 InSize);
    
    UINT64 GetOffset() const;
    UINT64 GetSize() const;
    ID3D12Resource* GetResource() const;
    ED3D12ResourceLocationType GetType() const;
    
private:
    D3D12ResourceAllocator* ResourceAllocator;
    ED3D12ResourceLocationType Type;
    ID3D12Resource* Resource = nullptr;
    LocationData Location;
    bool NeedRelease = true;
};
