#pragma once
#include <mutex>

#include "../Utility/Macros.h"

template <typename T>
class ConcurrentList
{
    struct Node
    {
        std::mutex Mutex;
        std::unique_ptr<T> Data;
        std::unique_ptr<Node> Next;
    };
    
public:
    CLASS_NO_COPY(ConcurrentList)
    
    ConcurrentList() { Tail = &Head; }
    ~ConcurrentList() { Clear(); }

public:
    bool Empty() const
    {
        std::lock_guard HeadLock(Head.Mutex);
        return Head.Next != nullptr;
    }

    void Clear()
    {
        RemoveIf([](const T*) { return true; });
        Tail = &Head;
    }

    template <typename... Args>
    requires std::is_constructible_v<T, Args...>
    T* PushFront(Args&&... Arguments)
    {
        std::unique_ptr<Node> NewNode = std::make_unique<Node>();
        NewNode->Data = std::make_unique<T>(std::forward<Args>(Arguments)...);
        
        std::lock_guard HeadLock(Head.Mutex);
        NewNode->Next = std::move(Head.Next);

        if (Head.Next.get() == nullptr)
        {
            std::lock_guard TailLock(TailMutex);
            Tail = NewNode.get();
        }
        Head.Next = std::move(NewNode);

        return Head.Next->Data.get();
    }

    template <typename... Args>
    requires std::is_constructible_v<T, Args...>
    T* PushBack(Args&&... Arguments)
    {
        std::unique_ptr<Node> NewNode = std::make_unique<Node>();
        NewNode->Data = std::make_unique<T>(std::forward<Args>(Arguments)...);
        
        std::lock_guard TailLock(TailMutex);
        std::lock_guard HeadLock(Head.Mutex);
        Tail->Next = std::move(NewNode);
        Tail = Tail->Next.get();

        return Tail->Data.get();
    }
    
    template <typename F>
    void ForEach(F Func)
    {
        Node* CurrentNode = &Head;
        std::unique_lock CurrentNodeLock(CurrentNode->Mutex);

        while (Node* const NextNode = CurrentNode->Next.get())
        {
            std::unique_lock NextNodeLock(NextNode->Mutex);
            CurrentNodeLock.unlock();

            Func(NextNode->Data.get());

            CurrentNode = NextNode;
            CurrentNodeLock = std::move(NextNodeLock);
        }
    }

    template <typename F>
    T* FindFirstIf(F Func)
    {
        Node* CurrentNode = &Head;
        std::unique_lock CurrentNodeLock(CurrentNode->Mutex);

        while (Node* const NextNode = CurrentNode->Next.get())
        {
            std::unique_lock NextNodeLock(NextNode->Mutex);
            CurrentNodeLock.unlock();

            if (Func(NextNode->Data.get()))
            {
                return NextNode->Data.get();
            }
            CurrentNode = NextNode;
            CurrentNodeLock = std::move(NextNodeLock);
        }
        return nullptr;
    }

    template <typename F>
    bool RemoveIf(F Func)
    {
        bool Success = false;
        
        Node* CurrentNode = &Head;
        std::unique_lock CurrentNodeLock(CurrentNode->Mutex);

        while (Node* const NextNode = CurrentNode->Next.get())
        {
            std::unique_lock NextNodeLock(NextNode->Mutex);

            if (Func(NextNode->Data.get()))
            {
                std::unique_ptr<Node> OldNextNode = std::move(CurrentNode->Next);
                CurrentNode->Next = std::move(NextNode->Next);
                NextNodeLock.unlock();

                Success = true;
            }
            else
            {
                CurrentNodeLock.unlock();
                CurrentNode = NextNode;
                CurrentNodeLock = std::move(NextNodeLock);
            }
        }

        return Success;
    }

private:
    Node Head;      // Empty Head
    Node* Tail;
    std::mutex TailMutex;
};







