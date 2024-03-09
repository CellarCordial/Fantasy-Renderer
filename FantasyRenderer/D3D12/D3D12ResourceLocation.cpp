#include "D3D12ResourceLocation.h"

#include "D3D12ResourceAllocator.h"

D3D12ResourceLocation::D3D12ResourceLocation(D3D12ResourceAllocator* InAllocator, ED3D12ResourceLocationType InType)
    : ResourceAllocator(InAllocator), Type(InType)
{
}

D3D12ResourceLocation::~D3D12ResourceLocation() noexcept
{
    if (NeedRelease && Type != ED3D12ResourceLocationType::ConstantBuffer)
    {
        if (Resource) Resource->Release();
        while (!ResourceAllocator->TryFree(this));
    }
}

void D3D12ResourceLocation::SetResource(ID3D12Resource* InResource)
{
    Resource = InResource;
}

void D3D12ResourceLocation::SetLocation(UINT64 InOffset, UINT64 InSize)
{
    Location.Offset = InOffset;
    Location.Size = InSize;
}

UINT64 D3D12ResourceLocation::GetOffset() const
{
    return Location.Offset;
}

UINT64 D3D12ResourceLocation::GetSize() const
{
    return Location.Size;
}

ID3D12Resource* D3D12ResourceLocation::GetResource() const
{
    return Resource;
}

ED3D12ResourceLocationType D3D12ResourceLocation::GetType() const
{
    return Type;
}

