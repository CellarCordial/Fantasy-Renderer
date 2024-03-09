#pragma once

#include <functional>

#include "Task.h"

class TaskFlow
{
    friend class TaskExecutor;
public:
    CLASS_NO_COPY(TaskFlow)
    
    TaskFlow() = default;
    ~TaskFlow() = default;

public:

    template <typename F, typename... Args>
    Task Emplace(F&& InFunc, Args&&... Arguments)
    {
        TotalTaskNum++;
        
        auto Func = std::bind(std::forward<F>(InFunc), std::forward<Args>(Arguments)...);
        return Task(Graph.EmplaceBack([Func]() { Func(); }));
    }

    template <typename T, typename... Args>
    Task Emplace(T* Instance, void(T::*MemberFunc)(Args...), Args... Arguments)
    {
        TotalTaskNum++;
        
        auto Func = [Instance, MemberFunc](Args... FuncArgs) { (Instance->*MemberFunc)(std::forward<Args>(FuncArgs)...); };
        return Task(Graph.EmplaceBack([=]() { Func(Arguments...); }));
    }

    void Reset()
    {
        Graph.Clear();
        TotalTaskNum = 0;
    }

private:
    std::vector<TaskNode*>& GetSrcNodes()
    {
        return Graph.FindSrcNode();
    }
    
    bool Empty() const
    {
        return TotalTaskNum == 0;
    }

private:
    TaskGraph Graph;

    UINT32 TotalTaskNum = 0;
};
