#pragma once
#include <windows.h>
#include <atomic>
#include <thread>
#include <vector>

#include "../Utility/Exception.h"
#include "../Utility/Macros.h"

// For GPU descriptor allocate.
class ConcurrentRingAllocator
{
public:
    CLASS_NO_COPY(ConcurrentRingAllocator)
    
    explicit ConcurrentRingAllocator(size_t InCapacity)
        : Capacity(InCapacity)
    {
    }

    ~ConcurrentRingAllocator() = default;

public:
    bool TryAllocate(size_t* OutValue, size_t InSize = 1)
    {
        const size_t CurrentHead = Head.load(std::memory_order_acquire);
        const size_t CurrentTail = Tail.load(std::memory_order_acquire);

        const size_t AvailableSize = GetAvailableSize(CurrentHead, CurrentTail);
        if (AvailableSize < InSize) return false;

        const size_t NewTail = (CurrentTail + InSize) % Capacity; 
        *OutValue = CurrentTail;

        Tail.store(NewTail, std::memory_order_release);
        
        return true;
    }
    
    bool TryFree(size_t InSize)
    {
        const size_t CurrentHead = Head.load(std::memory_order_acquire);
        const size_t CurrentTail = Tail.load(std::memory_order_acquire);

        const size_t UsedSize = Capacity - GetAvailableSize(CurrentHead, CurrentTail);
        if (InSize > UsedSize) return false;

        const size_t NewHead = (CurrentHead + InSize) % Capacity;

        Head.store(NewHead, std::memory_order_release);

        return true;
    }
    
    void Clear()
    {
        Head.store(0, std::memory_order_release);
        Tail.store(0, std::memory_order_release);
    }

private:
    size_t GetAvailableSize(size_t Head, size_t Tail) const
    {
        if (Head < Tail) return Capacity - (Tail - Head + 1);
        else if (Head > Tail) return Head - Tail - 1;
        else return Capacity;
    }

private:
    std::atomic<size_t> Head = 0;   
    std::atomic<size_t> Tail = 0;

    const size_t Capacity;
};
