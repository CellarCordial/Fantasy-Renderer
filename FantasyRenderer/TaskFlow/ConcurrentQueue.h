#pragma once
#include <memory>
#include <mutex>

template <typename T>
class ConcurrentQueue
{
    struct Node
    {
        std::shared_ptr<T> Value;
        std::unique_ptr<Node> Next;
    };
public:
    CLASS_NO_COPY(ConcurrentQueue)
    
    ConcurrentQueue() : Head(std::make_unique<Node>()), Tail(Head.get()) {}     // 开头的DummyHead可以使Push()只访问尾节点
    ~ConcurrentQueue() = default;

public:
    bool Empty() const
    {
        std::lock_guard LockGuard(HeadMutex);
        return Head.get() == GetTail();
    }

    void Push(T InValue)
    {
        std::shared_ptr<T> Value = std::make_shared<T>(std::forward<T>(InValue));
        std::unique_ptr<Node> NewDummyNode = std::make_unique<Node>();
        Node* NewTail = NewDummyNode.get();
        {
            std::lock_guard LockGuard(TailMutex);
            Tail->Value = Value;
            Tail->Next = std::move(NewDummyNode);
            Tail = NewTail;
        }
        ConditionVariable.notify_one();
    }

    bool TryPop(T& OutValue)
    {
        return TryPopImpl(OutValue) != nullptr;
    }


    std::shared_ptr<T> TryPop()
    {
        std::unique_ptr<Node> PoppedHead = TryPopImpl();
        return PoppedHead ? PoppedHead->Value : nullptr;
    }
    
private:
    Node* GetTail() const
    {
        std::lock_guard LockGuard(TailMutex);
        return Tail;
    }

    std::unique_ptr<Node> Pop()
    {
        std::unique_ptr<Node> Output = std::move(Head);
        Head = std::move(Output->Next);
        return Output;
    }


private:
    std::unique_ptr<Node> TryPopImpl(T& OutValue)
    {
        std::lock_guard LockGuard(HeadMutex);

        if (Head.get() == GetTail())
        {
            return nullptr;
        }

        OutValue = std::move(*Head->Value);

        return Pop();
    }


    std::unique_ptr<Node> TryPopImpl()
    {
        std::lock_guard LockGuard(HeadMutex);

        if (Head.get() == GetTail())
        {
            return nullptr;
        }

        return Pop();
    }


private:
    std::unique_ptr<Node> Head;
    Node* Tail;

    mutable std::mutex HeadMutex;
    mutable std::mutex TailMutex;

    std::condition_variable ConditionVariable;
};
