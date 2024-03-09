#pragma once
#include "TaskQueue.h"
#include "TaskFlow.h"
#include "ThreadPool.h"
#include "../Utility/Exception.h"

class TaskExecutor
{
public:
    CLASS_NO_COPY(TaskExecutor)
    
    explicit TaskExecutor(UINT32 InThreadNum = 0) : Pool(std::make_unique<ThreadPool>(InThreadNum)) {}
    ~TaskExecutor() = default;

public:
    template <typename... Args>
    void Run(Args&&... Arguments)
    {
        static_assert((std::is_base_of<TaskFlow, std::decay_t<Args>>::value && ...), "All Args must be Task or derived from Task.");
        (Run(Arguments), ...);
    }
    
    void Run(TaskFlow& InFlow)
    {
        ThrowIfFalse(!InFlow.Empty());

        UINT32 TotalUnfinishedTaskNum = InFlow.TotalTaskNum;

        const auto& SrcNodes = InFlow.GetSrcNodes();
        for (const auto& Node : SrcNodes)
        {
            Notify(Node);
        }
        
        while (TotalUnfinishedTaskNum > 0)
        {
            NodeQueue.Wait();
            TotalUnfinishedTaskNum--;

            TaskNode* Node = NodeQueue.Pop();
            for (const auto& Successor : Node->Successors)
            {
                Successor->UnfinishedDependentTaskNum--;
                if (Successor->UnfinishedDependentTaskNum == 0)
                {
                    Notify(Successor);
                    Successor->UnfinishedDependentTaskNum = Successor->UnfinishedDependentTaskNumBackup;    // It keeps the TaskFlow's initial state
                }
            }
        }
    }
    template <typename T, typename... Args>
    void BeginThread(T* Instance, void(T::*MemberFunc)(Args...), Args... Arguments)
    {
        auto Func = [Instance, MemberFunc](Args... FuncArgs) { (Instance->*MemberFunc)(std::forward<Args>(FuncArgs)...); };
        Pool->Submit( [=]() {Func(Arguments...);} );
    }

private:
    void Notify(TaskNode* InNode)
    {
        Pool->Submit(
            [InNode, this]()
            {
                InNode->Run();
                NodeQueue.Push(InNode);     // 入队就说明已经完成了
            }
        );
    }
    
private:
    TaskQueue<TaskNode*> NodeQueue;
    std::unique_ptr<ThreadPool> Pool;
};
