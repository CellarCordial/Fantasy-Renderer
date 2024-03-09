#pragma once

#include <filesystem>

namespace fs = std::filesystem;

inline bool IsFileExist(const char* InPath)
{
    const fs::path Path(InPath);
    return fs::exists(Path);
}

inline fs::file_time_type GetFileLastWriteTime(const char* InPath)
{
    const fs::path Path(InPath);
    return fs::last_write_time(Path);
}

inline std::string GetFileExtension(const char* InPath)
{
    const fs::path Path(InPath);
    std::string Extension = Path.filename().extension().string();
    std::ranges::for_each(
        std::begin(Extension),
        std::end(Extension),
        [](char& InChar) { InChar = std::tolower(InChar); }
    );

    return Extension;
}

inline bool CompareFileWriteTime(const char* FirstFile, const char* SecondFile)
{
    return GetFileLastWriteTime(FirstFile) > GetFileLastWriteTime(SecondFile);
}

inline std::string RemoveFileExtension(const char* InPath)
{
    const fs::path Path(InPath);
    return Path.filename().replace_extension().string();
}




















