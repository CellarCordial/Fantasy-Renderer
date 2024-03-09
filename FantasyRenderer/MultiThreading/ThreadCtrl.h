#pragma once
#include <mutex>

#include "../Utility/Macros.h"


class ThreadCtrl
{
public:
    CLASS_NO_COPY(ThreadCtrl)

    ThreadCtrl(UINT32 InFlagNum, bool InitFlag = false) : Flags(InFlagNum)
    {
        UINT32 InitNumber = 0;
        if (InitFlag) InitNumber = 1;
        std::ranges::fill(std::begin(Flags), std::end(Flags), InitNumber);
    }
    ~ThreadCtrl() = default;

public:
    void Wait()
    {
        std::unique_lock UniqueLock(Mutex);
        ConditionVariable.wait(UniqueLock, [this]() { return Flags[WaitIndex] == 1; });
        Flags[WaitIndex] = 0;
        WaitIndex = (WaitIndex + 1) % Flags.size();
    }

    void Wake()
    {
        std::lock_guard LockGuard(Mutex);
        Flags[WakeIndex] = 1;
        WakeIndex = (WakeIndex + 1) % Flags.size();
        ConditionVariable.notify_one();
    }

    void WakeAll()
    {
        std::lock_guard LockGuard(Mutex);
        for (auto& Flag : Flags) Flag = 1;
        WakeIndex = 0; WaitIndex = 0;
        ConditionVariable.notify_all();
    }

    void Reset()
    {
        std::ranges::fill(std::begin(Flags), std::end(Flags), 1);
    }
    
private:
    std::mutex Mutex;
    std::condition_variable ConditionVariable;
    std::vector<UINT32> Flags;
    UINT32 WakeIndex = 0;
    UINT32 WaitIndex = 0;
};
