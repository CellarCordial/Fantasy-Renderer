#pragma once

#include <windows.h>
#include <string>

static std::string WStringToString(const std::wstring& WString)
{
    if (WString.empty())
    {
        return std::string("");
    }

    int Size = WideCharToMultiByte(CP_UTF8, 0, &WString[0], (int)WString.size(), NULL, 0, NULL, NULL);
    std::string String(Size, 0);
    WideCharToMultiByte(CP_UTF8, 0, &WString[0], (int)WString.size(), &String[0], Size, NULL, NULL);
    return String;
}

static std::wstring StringToWString(const std::string& String)
{
    if (String.empty())
    {
        return std::wstring(L"");
    }

    int Size = MultiByteToWideChar(CP_UTF8, 0, &String[0], (int)String.size(), NULL, 0);
    std::wstring WString(Size, 0);
    MultiByteToWideChar(CP_UTF8, 0, &String[0], (int)WString.size(), &WString[0], Size);
    return WString;
}