#pragma once
#include <chrono>

#include "Macros.h"

// Timer in seconds
class Timer
{
    using clock = std::chrono::steady_clock;
    using seconds = std::chrono::seconds;
    using microseconds = std::chrono::microseconds;

public:
    CLASS_NO_COPY(Timer)
    
    Timer() { PrevTime = StartTime; }
    ~Timer() = default;

    // ��ȡ��һ��Tick()�����ڵ�ʱ�䣬����PrevTime����Ϊ���ڵ�ʱ��
    float Tick()
    {
        const clock::time_point CurrentTime = clock::now();
        
        const auto DeltaTime = std::chrono::duration_cast<microseconds>(CurrentTime - PrevTime).count();
        
        PrevTime = CurrentTime;
        
        return static_cast<float>(DeltaTime) / ClockSecondRatio;
    }

    // ����ȡ��һ��Tick()�����ڵ�ʱ��
    float Peek() const
    {
        const clock::time_point CurrentTime = clock::now();
        
        const auto DeltaTime = std::chrono::duration_cast<microseconds>(CurrentTime - PrevTime).count();

        return static_cast<float>(DeltaTime) / ClockSecondRatio;
    }

    // ����ȡ��ʵ����ʼ�������ڵ�ʱ��
    float Elapsed() const
    {
        const clock::time_point CurrentTime = clock::now();
        const auto ElapsedTime = std::chrono::duration_cast<microseconds>(CurrentTime - StartTime).count();
        return static_cast<float>(ElapsedTime) / ClockSecondRatio;
    }

private:
    static constexpr float ClockSecondRatio = seconds(1) * 1.0f / microseconds(1);

    const clock::time_point StartTime = clock::now();

    clock::time_point PrevTime;  
};
