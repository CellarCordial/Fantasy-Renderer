#pragma once

#include <future>
#include <queue>
#include <vector>
#include <windows.h>

#include "ConcurrentQueue.h"
#include "FunctionWrapper.h"

class ThreadPool
{
public:
    CLASS_NO_COPY(ThreadPool)
        
    ThreadPool(UINT32 InThreadNum = 0)
    {
        size_t MaxThreadNum = std::thread::hardware_concurrency() / 4;
        if (InThreadNum > 0) MaxThreadNum = InThreadNum;
        
        try
        {
            for (size_t ix = 0; ix < MaxThreadNum; ++ix)
            {
                Threads.emplace_back(&ThreadPool::WorkerThread, this, ix);
            }
        
        }
        catch (...)
        {
            FinishPool();
            throw;
        }
    }

    ~ThreadPool()
    {
        FinishPool();
    }

    template <typename F, typename... Args>
    auto Submit(F&& InFunc, Args&&... Arguments) -> std::future<decltype(InFunc(Arguments...))>
    {
        using ReturnType = decltype(InFunc(Arguments...));
        
        auto Task = std::make_shared<std::packaged_task<ReturnType()>>(std::bind(std::forward<F>(InFunc), std::forward<Args>(Arguments)...));
        std::future<ReturnType> Result(Task->get_future());

        PoolTaskQueue.Push([Task]() { (*Task)(); });

        return Result;
    }

private:

    void WorkerThread(size_t InIndex)
    {
        while (!Done)
        {
            FunctionWrapper Task;
            if (PoolTaskQueue.TryPop(Task))
            {
                Task();
            }
            else
            {
                std::this_thread::yield();
            }
        }
    }

    void FinishPool()
    {
        Done = true;
        for (auto& Thread : Threads)
        {
            if (Thread.joinable())
            {
                Thread.join();
            }
        }
    }

private:
    std::atomic<bool> Done = false;

    std::vector<std::thread> Threads;
    ConcurrentQueue<FunctionWrapper> PoolTaskQueue;
};
