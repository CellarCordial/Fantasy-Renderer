#pragma once
#include <memory>
#include <mutex>

#include "../Utility/Macros.h"

// 这个FreeList在allocate时只会分配一个T
class ConcurrentFreeListAllocator
{
    struct FreeRange
    {
        friend bool operator == (const FreeRange& lhs, const FreeRange& rhs) noexcept
        {
            return lhs.Begin == rhs.Begin && lhs.End == rhs.End;
        }
        
        size_t Begin;
        size_t End;
    };

    struct Node
    {
        Node() = default;
        Node(const FreeRange& rhs) : Data(std::make_shared<FreeRange>(rhs)) {}

        std::mutex Mutex;
        std::shared_ptr<FreeRange> Data;
        std::unique_ptr<Node> Next;
    };
    
public:
    CLASS_NO_COPY(ConcurrentFreeListAllocator)
    
    ConcurrentFreeListAllocator(size_t InCapacity)
    {
        FreeRange InitRange(0, InCapacity);
        Head.Next = std::make_unique<Node>(InitRange);
        Head.Data = std::make_shared<FreeRange>(InitRange);
    }
    ~ConcurrentFreeListAllocator() noexcept { Clear(); }

public:
    bool TryAllocate(size_t* OutValue)
    {
        std::unique_lock<std::mutex> HeadLock(Head.Mutex);
        if (Head.Next == nullptr) return false;
        
        Allocate(std::move(HeadLock), OutValue);

        return true;
    }

    size_t WaitAndAllocate()
    {
        size_t Output;
        Allocate(WaitForData(), &Output);

        return Output;
    }

    bool TryFree(const size_t& InValue)
    {
        return TryFree(InValue, InValue);
    }

    bool TryFree(const size_t& Begin, const size_t& End)
    {
        if (Empty()) return false;
        
        FreeRange RangeToFree(Begin, End);

        std::unique_lock<std::mutex> CurrentNodeLock(Head.Mutex);
        Node* CurrentNode = &Head;

        bool Found = false;
        while (Node* NextNode = CurrentNode->Next.get())
        {
            std::unique_lock<std::mutex> NextNodeLock(NextNode->Mutex);
            CurrentNodeLock.unlock();

            FreeRange& Range = *NextNode->Data;
            if (Range.Begin == End)
            {
                Range.Begin = Begin;
                Found = true; break;
            }
            else if (Range.End == Begin)
            {
                Range.End = End;
                Found = true; break;
            }
            else if (Range.Begin > Begin)
            {
                std::unique_ptr<Node> NewNode = std::make_unique<Node>(RangeToFree);
                NewNode->Next = std::move(NextNode->Next);
                NextNode->Next = std::move(NewNode);
                Found = true; break;
            }
            CurrentNode = NextNode;
            CurrentNodeLock = std::move(NextNodeLock);
        }

        if (!Found)
        {
            PushFront(RangeToFree);
        }

        return true;
    }

    void Clear()
    {
        Node* CurrentNode = &Head;
        std::unique_lock<std::mutex> CurrentNodeLock(CurrentNode->Mutex);

        while (Node* const NextNode = CurrentNode->Next.get())
        {
            std::unique_lock<std::mutex> NextNodeLock(NextNode->Mutex);

            std::unique_ptr<Node> OldNextNode = std::move(CurrentNode->Next);
            CurrentNode->Next = std::move(NextNode->Next);
            NextNodeLock.unlock();
        }
    }

private:
    std::unique_lock<std::mutex> WaitForData()
    {
        std::unique_lock<std::mutex> HeadLock(Head.Mutex);
        ConditionVariable.wait(HeadLock, [this]() { return Head.Next != nullptr; });
        return HeadLock;
    }

    void Allocate(std::unique_lock<std::mutex> InHeadLock, size_t* OutValue)
    {
        std::unique_lock<std::mutex> HeadLock(std::move(InHeadLock));
        
        Node* FirstNode = Head.Next.get();
        
        std::unique_lock<std::mutex> FirstNodeLock(FirstNode->Mutex);

        auto& FirstFreeRange = *FirstNode->Data;
        *OutValue = FirstFreeRange.Begin;
        
        if (FirstFreeRange.Begin == FirstFreeRange.End)
        {
            std::unique_ptr<Node> OldFirstNode = std::move(Head.Next);
            Head.Next = std::move(FirstNode->Next);
            FirstNodeLock.unlock();
        }
        FirstFreeRange.Begin++;
    }

    void PushFront(const FreeRange& InRange)
    {
        std::unique_lock<std::mutex> HeadLock(Head.Mutex);

        std::unique_ptr<Node> NewNode = std::make_unique<Node>(InRange);
        NewNode->Next = std::move(Head.Next);
        Head.Next = std::move(NewNode);

        ConditionVariable.notify_one();   
    }

    bool Empty()
    {
        std::lock_guard<std::mutex> HeadLock(Head.Mutex);
        if (Head.Next)
        {
            std::lock_guard<std::mutex> FirstNodeLock(Head.Next->Mutex);
            return *Head.Data == *Head.Next->Data;
        }
        return false;
    }
    
private:
    Node Head;  // 存储初始化的数据，实际上的用处就是一个空头
    std::condition_variable ConditionVariable;
};
