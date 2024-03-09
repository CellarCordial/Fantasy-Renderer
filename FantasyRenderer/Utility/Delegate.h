#pragma once
#include <functional>

#include "Exception.h"
#include "Macros.h"

#define DeclareDelegateEvent(ClassName, Owner, ...)         \
    class ClassName : MultiDelegate<__VA_ARGS__>            \
    {                                                       \
    public:                                                 \
        using MultiDelegate::AddEvent;                      \
        using MultiDelegate::Broadcast;                     \
    };

template <typename... Args>
class MultiDelegate
{
    using DelegateFunc = std::function<void(Args...)>;
    
public:
    CLASS_NO_COPY(MultiDelegate)

    MultiDelegate() = default;
    ~MultiDelegate() = default;

public:
    template <typename F>
    void AddEvent(F* Instance, void (F::*MemberFunc)(Args...))
    {
        ThrowIfFalse(Instance != nullptr, "Try to use nullptr member function.");

        DelegateQueue.emplace_back(
            [Instance, MemberFunc](Args&&... InArguments)
            {
                (Instance->*MemberFunc)(std::forward<Args>(InArguments)...);
            }
        );
    }

    void Broadcast(Args... InArguments)
    {
        for (const auto& DelegateFunction : DelegateQueue)
        {
            DelegateFunction(std::forward<Args>(InArguments)...);
        }
    }

private:
    std::vector<DelegateFunc> DelegateQueue;
};
