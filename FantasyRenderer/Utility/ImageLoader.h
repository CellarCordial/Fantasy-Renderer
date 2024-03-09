#pragma once
#include <wincodec.h>
#include <windows.h>
#include <memory>


#include "Exception.h"
#include "Macros.h"
#include "FileUtil.h"

inline constexpr UINT32 IMAGE_MAX_SIZE = 2048;

struct ImageData
{
    UINT32 Width;
    UINT32 Height;
    DXGI_FORMAT Format;
    std::unique_ptr<UINT8[]> Data;
};

class ImageLoader
{
public:
    CLASS_NO_COPY(ImageLoader)

    ImageLoader() = default;
    ~ImageLoader() = default;

public:
    static void LoadBitMapFromFile(const char* InFileName, ImageData* OutData);
};























