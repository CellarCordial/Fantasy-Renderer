#pragma once
#include <cassert>
#include <type_traits>
#include <windows.h>
#include <vector>
#include <atomic>

#include "../Utility/Macros.h"

/*
 * This class has not been used.
 */

template <typename T>
requires std::is_pointer_v<T>
class LockFreeQueue
{
    class Array
    {
    public:
        CLASS_NO_COPY(Array)

        explicit Array(UINT64 InCapacity)
            : Capacity(InCapacity),
              IndexMask(InCapacity - 1),
              Data(new std::atomic<T>[InCapacity])
        {}
        ~Array()
        {
            delete[] Data;
        }

    public:
        UINT64 GetCapacity() const
        {
            return Capacity;
        }

        void SetIndexElement(UINT64 Index, T InValue)
        {
            Data[Index & IndexMask].store(InValue, std::memory_order_relaxed);
        }
        
        T GetIndexElement(UINT64 Index)
        {
            return Data[Index & IndexMask].load(std::memory_order_relaxed);
        }

        // 会改变本身的存储地址
        Array* Resize(UINT64 InBottom, UINT64 InTop)
        {
            Array* NewArray = new Array(Capacity * 2);
            for (UINT64 ix = InBottom; ix != InTop; ++ix)
            {
                NewArray->SetIndexElement(ix, GetIndexElement(ix));
            }
            return NewArray;
        }


    private:
        UINT64 Capacity;
        UINT64 IndexMask;   // 有了掩码，数组就成了环
        std::atomic<T>* Data;
    };
    
public:
    CLASS_NO_COPY(LockFreeQueue)

    LockFreeQueue(UINT64 InCapacity = 512)
    {
        assert(InCapacity > 0);

        Top.store(0, std::memory_order_relaxed);
        Bottom.store(0, std::memory_order_relaxed);
        Arrays.store(new Array(InCapacity), std::memory_order_relaxed);
        GarbageBin.reserve(32);
    }
    ~LockFreeQueue()
    {
        for (auto Garbage : GarbageBin)
        {
            delete Garbage;
        }
        delete Arrays.load();
    }

public:
    bool Empty() const
    {
        const UINT64 BottomTemp = Bottom.load(std::memory_order_relaxed);
        const UINT64 TopTemp = Top.load(std::memory_order_relaxed);
        return BottomTemp <= TopTemp;
    }

    void Push(T InValue)
    {
        const UINT64 BottomTemp = Bottom.load(std::memory_order_acquire);
        const UINT64 TopTemp = Top.load(std::memory_order_relaxed);
        Array* ArrayTemp = Arrays.load(std::memory_order_relaxed);

        if (ArrayTemp->GetCapacity() < TopTemp - BottomTemp)
        {
            Resize(ArrayTemp, BottomTemp, TopTemp);
        }

        ArrayTemp->SetIndexElement(TopTemp, InValue);

        std::atomic_thread_fence(std::memory_order_release);
        Top.store(TopTemp + 1, std::memory_order_relaxed);
    }

    T Pop()
    {
        Array* ArrayTemp = Arrays.load(std::memory_order_relaxed);
            
        UINT64 TopTemp = Top.load(std::memory_order_relaxed) - 1;
        Top.store(TopTemp, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
            
        UINT64 BottomTemp = Bottom.load(std::memory_order_relaxed);
        T Output = nullptr;

        if (TopTemp >= BottomTemp)
        {
            Output = ArrayTemp->GetIndexElement(TopTemp);

            if (TopTemp == BottomTemp)
            {
                if (!Bottom.compare_exchange_strong(BottomTemp, BottomTemp + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
                {
                    // 如果刚刚被steal了
                    Output = nullptr;
                }
                Top.store(TopTemp + 1, std::memory_order_relaxed);
            }
        }
        else
        {
            Top.store(TopTemp + 1, std::memory_order_relaxed);
        }
            
        return Output;
    }

    T Steal()
    {
        UINT64 BottomTemp = Bottom.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        UINT64 TopTemp = Top.load(std::memory_order_acquire);

        T Output = nullptr;

        if (TopTemp > BottomTemp)
        {
            Array* ArrayTemp = Arrays.load(std::memory_order_consume);
            Output = ArrayTemp->GetIndexElement(TopTemp);

            if (!Bottom.compare_exchange_strong(BottomTemp, BottomTemp + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
            {
                // 如果刚刚被steal了
                return nullptr;
            }
        }
            
        return Output;
    }

private:
    void Resize(Array* InArray, UINT64 InBottom, UINT64 InTop)
    {
        Array* NewArray = InArray->Resize(InBottom, InTop);     // 改变了InArray本身的存储地址，atomic需要重新store一次
        GarbageBin.push_back(InArray);
        std::swap(NewArray, InArray);
        Arrays.store(InArray, std::memory_order_release);
    }
    
private:
    std::atomic<UINT64> Top;
    std::atomic<UINT64> Bottom;
    std::atomic<Array*> Arrays;
    std::vector<Array*> GarbageBin;
};
