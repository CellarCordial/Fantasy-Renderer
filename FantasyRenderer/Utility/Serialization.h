#pragma once
#include <fstream>
#include <vector>

#include "../Utility/Exception.h"
#include "../Utility/Macros.h"

/*
 * Only for Integer, std::vector and std::string serialization
 */

class BinaryOutput 
{
public:
    CLASS_NO_COPY(BinaryOutput)

    BinaryOutput(const std::string& InFileName) : Output(InFileName, std::ios::binary) {}
    ~BinaryOutput() { Output.close(); }

    template <typename... Args>
    void operator()(Args&&... Arguments)
    {
        Process(std::forward<Args>(Arguments)...);
    }

public:
    void SaveBinaryData(const void* InData, INT64 InSize)
    {
        if (Output.is_open())
        {
            Output.write(static_cast<const char*>(InData), InSize);
        }
    }

private:
    template <typename T>
    void Process(T&& InValue)
    {
        ProcessImpl(InValue);
    }

    template <typename T>
    void ProcessImpl(const std::vector<T>& InValue)
    {
        UINT64 Size = InValue.size();
        SaveBinaryData(&Size, sizeof(UINT64));
        for (const T& Element : InValue)
        {
            ProcessImpl(Element);
        }
    }

    void ProcessImpl(const std::string& InValue)
    {
        UINT64 Size = InValue.size();
        SaveBinaryData(&Size, sizeof(UINT64));
        SaveBinaryData(InValue.data(), static_cast<INT64>(Size));
    }

    void ProcessImpl(UINT64 InValue)
    {
        SaveBinaryData(&InValue, sizeof(UINT64));
    }

private:
    std::ofstream Output;
};


class BinaryInput
{
public:
    CLASS_NO_COPY(BinaryInput)

    BinaryInput(const std::string& InFileName) : Input(InFileName, std::ios::binary) {}
    ~BinaryInput() noexcept { Input.close(); }

    template <typename... Args>
    void operator()(Args&&... Arguments)
    {
        Process(std::forward<Args>(Arguments)...);
    }

public:
    void LoadBinaryData(void* OutData, INT64 InSize)
    {
        if (Input.is_open())
        {
            Input.read(static_cast<char*>(OutData), InSize);
        }
    }

private:
    template <typename T>
    void Process(T&& InValue)
    {
        ProcessImpl(InValue);
    }

    template <typename T>
    void ProcessImpl(std::vector<T>& OutValue)
    {
        UINT64 Size = 0;
        LoadBinaryData(&Size, sizeof(UINT64));

        OutValue.resize(Size);
        for (T& Element : OutValue)
        {
            ProcessImpl(Element);
        }
    }

    void ProcessImpl(std::string& OutValue)
    {
        UINT64 Size = 0;
        LoadBinaryData(&Size, sizeof(UINT64));

        OutValue.resize(Size);
        LoadBinaryData(OutValue.data(), static_cast<INT64>(Size));
    }
    
    void ProcessImpl(UINT64& OutValue)
    {
        LoadBinaryData(&OutValue, sizeof(UINT64));
    }
    
private:
    std::ifstream Input;
};