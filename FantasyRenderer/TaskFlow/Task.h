#pragma once
#include <vector>
#include <windows.h>

#include "FunctionWrapper.h"
#include "TaskGraph.h"

class Task
{
public:
    CLASS_DEFAULT_COPY(Task)
    
    Task() = default;
    ~Task() = default;

    Task(TaskNode* InNode) : Node(InNode) {}

public:
    template <typename... Args>
    void Succeed(Args&&... Arguments)
    {
        static_assert((std::is_base_of<Task, std::decay_t<Args>>::value && ...), "All Args must be Task or derived from Task.");
        (Arguments.Node->Precede(Node), ...);
    }

    template <typename... Args>
    void Precede(Args&&... Arguments)
    {
        static_assert((std::is_base_of<Task, std::decay_t<Args>>::value && ...), "All Args must be Task or derived from Task.");
        (Node->Precede(Arguments.Node), ...);
    }

private:
    TaskNode* Node = nullptr;
};
