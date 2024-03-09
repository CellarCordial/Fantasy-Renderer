#pragma once
#include <string>
#include <windows.h>
#include <comdef.h>
#include <stdexcept>

#include "FormatConvert.h"

inline std::string ToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

class Exception
{
public:
    Exception(HRESULT HR) : Reason(ToString(HR))
    {}

    Exception(std::string InReason = "")
        : Reason(InReason)
    {}

    std::string GetErrorMessage() const { return Reason; }
    
private:
    std::string Reason;
};

inline void ThrowIfFalse(bool In, std::string Reason = "")
{
    if (!In)
    {
        printf_s("%s", Reason.c_str());
        throw Exception(Reason);
    }
}

inline void ThrowIfFailed(HRESULT In)
{
    if (FAILED(In))
    {
        throw Exception(In);
    }
}
