#pragma once
#include <future>
#include "../Utility/Macros.h"

class TaskNode
{
    friend class TaskGraph;
    friend class TaskExecutor;
public:
    CLASS_NO_COPY(TaskNode)
    
    TaskNode(FunctionWrapper InFunc) : Func(std::move(InFunc)) {}
    ~TaskNode() = default;

public:
    void Precede(TaskNode* InNode)
    {
        Successors.push_back(InNode);
        InNode->Dependents.push_back(this);
        InNode->UnfinishedDependentTaskNum++;
        InNode->UnfinishedDependentTaskNumBackup++;
    }

    void Run() const
    {
        Func();
    }

    /*void AddChildren(TaskNode* InNode)
    {
        Children.push_back(InNode);
        InNode->Parent = this;
        UnfinishedChildrenTaskNum++;
    }*/
    
private:
    std::vector<TaskNode*> Successors;
    std::vector<TaskNode*> Dependents;
    UINT32 UnfinishedDependentTaskNum = 0;
    UINT32 UnfinishedDependentTaskNumBackup = 0;

    /*TaskNode* Parent = nullptr;
    std::vector<TaskNode*> Children;
    UINT32 UnfinishedChildrenTaskNum = 0;*/

    FunctionWrapper Func;
};

class TaskGraph
{
    friend class TaskFlow;
public:
    TaskGraph() = default;
    ~TaskGraph() = default;

    TaskGraph(const TaskGraph&) = delete;
    TaskGraph(TaskGraph&&) = delete;
    TaskGraph& operator=(const TaskGraph&) = delete;
    TaskGraph& operator=(TaskGraph&&) = delete;

public:
    TaskNode* EmplaceBack(FunctionWrapper InFunc)
    {
        Nodes.emplace_back(std::make_unique<TaskNode>(std::move(InFunc)));
        return Nodes.back().get();
    }

    void Clear()
    {
        SrcNodes.clear();
        Nodes.clear();
    }

private:
    std::vector<TaskNode*>& FindSrcNode()
    {
        SrcNodes.clear();
        for (const auto& Node : Nodes)
        {
            if (Node->Dependents.empty())
            {
                SrcNodes.push_back(Node.get());
            }
        }
        return SrcNodes;
    }

private:
    std::vector<TaskNode*> SrcNodes;
    std::vector<std::unique_ptr<TaskNode>> Nodes;
};

