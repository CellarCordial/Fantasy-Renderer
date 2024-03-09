#pragma once
#include <windows.h>
#include <vector>

#include "ConcurrentFreeListAllocator.h"
#include "../Utility/AlignUtil.h"
#include "../Utility/Macros.h"

class ConcurrentSegListAllocator
{
public:
    CLASS_NO_COPY(ConcurrentSegListAllocator)

    ConcurrentSegListAllocator(size_t InTotalSize, size_t InBlockMaxSize, size_t InBlockMinSize)
        : TotalSize(InTotalSize),
          BlockMaxSize(InBlockMaxSize),
          BlockMinSize(InBlockMinSize)
    {
        const size_t MaxIndex = GetBlockIndex(BlockMaxSize);
        for (UINT32 ix = 0; ix <= MaxIndex; ++ix)
        {
            const size_t CurrentBlockSize = BlockMinSize << ix;
            FreeLists.emplace_back(std::make_unique<ConcurrentFreeListAllocator>(TotalSize / CurrentBlockSize));
        }
    }
    ~ConcurrentSegListAllocator() = default;

public:
    UINT32 GetIndexNum() const
    {
        return GetBlockIndex(BlockMaxSize) - GetBlockIndex(BlockMinSize);
    }
    
    bool TryAllocate(size_t* OutAddress, size_t* OutIndex, size_t InSize)
    {
        const size_t BlockIndex = GetBlockIndex(AlignPow2(InSize));
        if (BlockIndex == INVALID_SIZE_64) return false;

        size_t AddressFactor;
        if (!FreeLists[BlockIndex]->TryAllocate(&AddressFactor)) return false;
        *OutAddress = AddressFactor * (BlockMinSize << BlockIndex);
        *OutIndex = BlockIndex;
        return true;
    }

    size_t WaitAndAllocate(size_t InSize)
    {
        const size_t BlockIndex = GetBlockIndex(AlignPow2(InSize));
        if (BlockIndex == INVALID_SIZE_64) return false;

        const size_t AddressFactor = FreeLists[BlockIndex]->WaitAndAllocate();
        return AddressFactor * (BlockMinSize << BlockIndex);
    }

    bool TryFree(size_t InAddress, size_t InSize)
    {
        const size_t BlockIndex = GetBlockIndex(AlignPow2(InSize));
        if (BlockIndex == INVALID_SIZE_64) return false;

        const size_t AddressFactor = InAddress / (BlockMinSize << BlockIndex);
        if (!FreeLists[BlockIndex]->TryFree(AddressFactor)) return false;
        return true;
    }

    void Clear()
    {
        for (auto& FreeList : FreeLists)
        {
            FreeList->Clear();
        }
    }

    constexpr UINT32 GetBlockIndex(size_t InSize) const
    {
        if (InSize == 0 || InSize > BlockMaxSize) assert(false && "Beyond Texture Max Size");
        if (InSize < BlockMinSize) return 0;
        return static_cast<UINT32>(std::ceil(std::log2(InSize)) - std::ceil(std::log2(BlockMinSize)));
    }

private:
    size_t TotalSize;
    size_t BlockMaxSize;
    size_t BlockMinSize;
    std::vector<std::unique_ptr<ConcurrentFreeListAllocator>> FreeLists;
};
