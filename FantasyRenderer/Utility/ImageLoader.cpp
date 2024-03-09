#include "ImageLoader.h"

#include "../External/stbimage/stb_image.h"

void ImageLoader::LoadBitMapFromFile(const char* InFileName, ImageData* OutData)
{
    INT32 Width = 0, Height = 0;
    auto Data = stbi_load(InFileName, &Width, &Height, 0, STBI_rgb_alpha);

    ThrowIfFalse(Data != nullptr, "Load image failed.");
    UINT32 Size = Width * Height * STBI_rgb_alpha;
        
    OutData->Width = Width;
    OutData->Height = Height;
    OutData->Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    OutData->Data = std::make_unique<UINT8[]>(Size);
    memcpy(OutData->Data.get(), Data, Size);

    stbi_image_free(Data);
}