#include "D3D12ResourceAllocator.h"

#include "D3D12Device.h"

D3D12BufferAllocator::D3D12BufferAllocator(D3D12Device* InDevice, D3D12_HEAP_TYPE HeapType, UINT64 InCapacity)
    : Device(InDevice),
      Allocator(InCapacity, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)
{
    D3D12_HEAP_DESC HeapDesc;
    HeapDesc.Alignment = 0;
    HeapDesc.Flags = D3D12_HEAP_FLAG_NONE;
    HeapDesc.Properties = CD3DX12_HEAP_PROPERTIES(HeapType);
    HeapDesc.SizeInBytes = InCapacity;
    ThrowIfFailed(Device->GetNative()->CreateHeap(&HeapDesc, IID_PPV_ARGS(Heap.GetAddressOf())));
}

bool D3D12BufferAllocator::TryAllocate(D3D12ResourceLocation* OutLocation, const D3D12ResourceLocationDesc* InLocationDesc, UINT64 InOffset/* = INVALID_SIZE_64*/)
{
    UINT64 Offset;
    const UINT64 Size = InLocationDesc->Size;

    if (InOffset != INVALID_SIZE_64) Offset = InOffset;
    else if (!Allocator.TryAllocate(&Offset, Size)) return false;

    Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
    ThrowIfFailed(Device->GetNative()->CreatePlacedResource(
            Heap.Get(),
            Offset,
            InLocationDesc->ResourceDesc,
            InLocationDesc->ResourceState,
            InLocationDesc->ClearValue,
            IID_PPV_ARGS(Resource.GetAddressOf())
    ));

    OutLocation->SetLocation(Offset, Size);
    OutLocation->SetResource(Resource.Detach());

    return true;
}

bool D3D12BufferAllocator::TryFree(const D3D12ResourceLocation* InLocation)
{
    if (!Allocator.TryFree(InLocation->GetOffset(), InLocation->GetSize())) return false;
    return true;
}

void D3D12BufferAllocator::Clear()
{
    Allocator.Clear();
}

D3D12TextureAllocator::D3D12TextureAllocator(D3D12Device* InDevice, UINT64 InCapacity, UINT64 InTextureMaxSize, UINT64 InTextureMinSize)
    : Device(InDevice),
      Allocator(InCapacity, InTextureMaxSize, InTextureMinSize)
{
    const UINT32 IndexNum = Allocator.GetIndexNum();
    
    D3D12_HEAP_DESC HeapDesc;
    HeapDesc.Alignment = 0;
    HeapDesc.Flags = D3D12_HEAP_FLAG_NONE;
    HeapDesc.Properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    HeapDesc.SizeInBytes = InCapacity;

    Heaps.resize(IndexNum + 1);
    for (UINT32 ix = 0; ix <= IndexNum; ++ix)
    {
        ThrowIfFailed(Device->GetNative()->CreateHeap(&HeapDesc, IID_PPV_ARGS(Heaps[ix].GetAddressOf())));
    }
}


bool D3D12TextureAllocator::TryAllocate(D3D12ResourceLocation* OutLocation, const D3D12ResourceLocationDesc* InLocationDesc, UINT64 InOffset/* = INVALID_SIZE_64*/)
{
    UINT64 Offset;
    UINT64 Index;
    const UINT64 Size = InLocationDesc->Size;
    
    if (InOffset != INVALID_SIZE_64)
    {
        Offset = InOffset;
        Index = Allocator.GetBlockIndex(Size);
    }
    else if (!Allocator.TryAllocate(&Offset, &Index, Size)) return false;

    Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
    ThrowIfFailed(Device->GetNative()->CreatePlacedResource(
            Heaps[Index].Get(),
            Offset,
            InLocationDesc->ResourceDesc,
            InLocationDesc->ResourceState,
            InLocationDesc->ClearValue,
            IID_PPV_ARGS(Resource.GetAddressOf())
    ));

    OutLocation->SetLocation(Offset, Size);
    OutLocation->SetResource(Resource.Detach());

    return true;
}

bool D3D12TextureAllocator::TryFree(const D3D12ResourceLocation* InLocation)
{
    if (!Allocator.TryFree(InLocation->GetOffset(), InLocation->GetSize())) return false;
    return true;
}

void D3D12TextureAllocator::Clear()
{
    Allocator.Clear();
}

D3D12ConstantAllocator::D3D12ConstantAllocator(D3D12Device* InDevice, UINT64 InCapacity)
    : Device(InDevice),
      Allocator(InCapacity)
{
    const CD3DX12_HEAP_PROPERTIES HeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    const D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(InCapacity);
    ThrowIfFailed(Device->GetNative()->CreateCommittedResource(
        &HeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &ResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(ResourceHeap.GetAddressOf()))
    );

    const CD3DX12_RANGE Range(0, 0);
    ResourceHeap->Map(0, &Range, reinterpret_cast<void**>(&MappedData));
}

D3D12ConstantAllocator::~D3D12ConstantAllocator() noexcept
{
    if (MappedData)
    {
        ResourceHeap->Unmap(0, nullptr);
        MappedData = nullptr;
    }
}

bool D3D12ConstantAllocator::TryAllocate(D3D12ResourceLocation* OutLocation, const D3D12ResourceLocationDesc* InLocationDesc)
{
    UINT64 Offset;
    const UINT64 AlignedSize = Align(InLocationDesc->Size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    if (!Allocator.TryAllocate(&Offset, AlignedSize)) return false;

    CurrentFrameAllocation += static_cast<UINT32>(AlignedSize);
    OutLocation->SetLocation(Offset, AlignedSize);
    OutLocation->SetResource(ResourceHeap.Get());
    return true;
}

void D3D12ConstantAllocator::FinishFrameAllocation()
{
    std::lock_guard LockGuard(Mutex);
    FrameAllocations.push(CurrentFrameAllocation);
    CurrentFrameAllocation = 0;
}

void D3D12ConstantAllocator::ClearFrameResource()
{
    std::lock_guard LockGuard(Mutex);
         
    const UINT32 Size = FrameAllocations.front();
    FrameAllocations.pop();
    Allocator.TryFree(Size);
}

D3D12ResourceAllocator::D3D12ResourceAllocator(D3D12Device* InDevice)
    : ConstantAllocator(InDevice, HEAP_DEFAULT_SIZE),
      TextureAllocator(InDevice, HEAP_DEFAULT_SIZE, TEXTURE_MAX_SIZE, TEXTURE_MIN_SIZE),
      DefaultBufferAllocator(InDevice, D3D12_HEAP_TYPE_DEFAULT, HEAP_DEFAULT_SIZE),
      UploadBufferAllocator(InDevice, D3D12_HEAP_TYPE_UPLOAD, HEAP_DEFAULT_SIZE)
{
}

bool D3D12ResourceAllocator::TryAllocate(D3D12ResourceLocation* OutLocation, const D3D12ResourceLocationDesc* InDesc/* = nullptr*/, UINT64 InOffset/* = INVALID_SIZE_64*/)
{
    switch (OutLocation->GetType())
    {
    case ED3D12ResourceLocationType::Texture:
        return TextureAllocator.TryAllocate(OutLocation, InDesc, InOffset);
    case ED3D12ResourceLocationType::ConstantBuffer:
        return ConstantAllocator.TryAllocate(OutLocation, InDesc);
    case ED3D12ResourceLocationType::DefaultBuffer:
        return DefaultBufferAllocator.TryAllocate(OutLocation, InDesc, InOffset);
    case ED3D12ResourceLocationType::UploadBuffer:
        return UploadBufferAllocator.TryAllocate(OutLocation, InDesc);
    case ED3D12ResourceLocationType::Invalid:
    default:
        return false;
    }
}

bool D3D12ResourceAllocator::TryFree(const D3D12ResourceLocation* InLocation)
{
    
    switch (InLocation->GetType())
    {
    case ED3D12ResourceLocationType::Texture:
        return TextureAllocator.TryFree(InLocation);
    case ED3D12ResourceLocationType::DefaultBuffer:
        return DefaultBufferAllocator.TryFree(InLocation);
    case ED3D12ResourceLocationType::UploadBuffer:
        return UploadBufferAllocator.TryFree(InLocation);
    case ED3D12ResourceLocationType::Invalid:
    default:
        return false;
    }
}

void D3D12ResourceAllocator::FinishFrameAllocation()
{
    ConstantAllocator.FinishFrameAllocation();
}

void D3D12ResourceAllocator::ClearFrameResource()
{
    ConstantAllocator.ClearFrameResource();
}
