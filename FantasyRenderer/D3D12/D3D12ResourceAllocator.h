#pragma once

#include "D3D12ResourceLocation.h"
#include "../MultiThreading/ConcurrentBuddyAllocator.h"
#include "../MultiThreading/ConcurrentRingAllocator.h"
#include "../MultiThreading/ConcurrentSegListAllocator.h"

class D3D12BufferAllocator
{
public:
    CLASS_NO_COPY(D3D12BufferAllocator)

    D3D12BufferAllocator(D3D12Device* InDevice, D3D12_HEAP_TYPE HeapType, UINT64 InCapacity);
    ~D3D12BufferAllocator() = default;

public:
    bool TryAllocate(D3D12ResourceLocation* OutLocation, const D3D12ResourceLocationDesc* InLocationDesc, UINT64 InOffset = INVALID_SIZE_64);
    bool TryFree(const D3D12ResourceLocation* InLocation);

    void Clear();

private:
    D3D12Device* Device;
    Microsoft::WRL::ComPtr<ID3D12Heap> Heap;
    ConcurrentBuddyAllocator Allocator;
};

class D3D12TextureAllocator
{
public:
    CLASS_NO_COPY(D3D12TextureAllocator)

    D3D12TextureAllocator(D3D12Device* InDevice, UINT64 InCapacity, UINT64 InTextureMaxSize, UINT64 InTextureMinSize);
    ~D3D12TextureAllocator() = default;

public:
    bool TryAllocate(D3D12ResourceLocation* OutLocation, const D3D12ResourceLocationDesc* InLocationDesc, UINT64 InOffset = INVALID_SIZE_64);
    bool TryFree(const D3D12ResourceLocation* InLocation);

    void Clear();

private:
    D3D12Device* Device;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Heap>> Heaps;
    ConcurrentSegListAllocator Allocator;
};

class D3D12ConstantAllocator
{
public:
    CLASS_NO_COPY(D3D12ConstantAllocator)

    D3D12ConstantAllocator(D3D12Device* InDevice, UINT64 InCapacity);
    ~D3D12ConstantAllocator() noexcept;

public:
    bool TryAllocate(D3D12ResourceLocation* OutLocation, const D3D12ResourceLocationDesc* InDesc);
    void FinishFrameAllocation();
    void ClearFrameResource();

    UINT8* GetMappedData() const { return MappedData; }
    
private:
    D3D12Device* Device;
    Microsoft::WRL::ComPtr<ID3D12Resource> ResourceHeap;
    ConcurrentRingAllocator Allocator;
    UINT8* MappedData = nullptr;

    std::mutex Mutex;
    std::queue<UINT32> FrameAllocations;
    UINT32 CurrentFrameAllocation = 0;
};


class D3D12ResourceAllocator
{
public:
    CLASS_NO_COPY(D3D12ResourceAllocator)

    explicit D3D12ResourceAllocator(D3D12Device* InDevice);
    ~D3D12ResourceAllocator() = default;

public:
    bool TryAllocate(D3D12ResourceLocation* OutLocation, const D3D12ResourceLocationDesc* InDesc, UINT64 InOffset = INVALID_SIZE_64);
    bool TryFree(const D3D12ResourceLocation* InLocation);

    void FinishFrameAllocation();
    void ClearFrameResource();
    UINT8* GetConstantMappedData() const { return ConstantAllocator.GetMappedData(); }

private:
    D3D12ConstantAllocator ConstantAllocator;
    D3D12TextureAllocator TextureAllocator;
    D3D12BufferAllocator DefaultBufferAllocator;
    D3D12BufferAllocator UploadBufferAllocator;
};

