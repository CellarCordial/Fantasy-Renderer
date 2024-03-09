#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class TaskQueue
{
public:
    void Push(const T& task)
    {
        std::unique_lock<std::mutex> lock(Mutex);
        Queue.push(task);
        ConditionVariable.notify_one();
    }
    
    T Pop()
    {
        std::unique_lock<std::mutex> lock(Mutex);
        T msg = Queue.front();
        Queue.pop();
        return msg;
    }
    
    void Wait()
    {
        std::unique_lock<std::mutex> lock(Mutex);
        ConditionVariable.wait(lock, [this]() {
            return (!Queue.empty());
            });
    }

private:
    std::condition_variable ConditionVariable;
    std::mutex Mutex;
    std::queue<T> Queue;
};



