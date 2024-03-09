#pragma once
#include <complex>

constexpr uint64_t INVALID_SIZE_64 = static_cast<uint64_t>(-1);
constexpr uint32_t INVALID_SIZE_32 = static_cast<uint32_t>(-1);

constexpr size_t Align(size_t Size, size_t AlignSize)
{
    if (AlignSize == 0 || AlignSize == 1) return Size;
    
    size_t Remainder = Size % AlignSize; 

    return Remainder ? Size + (AlignSize - Remainder) : Size;
}


inline size_t AlignPow2(size_t InSize, size_t InAlignSize = 2)
{
    while (InSize > InAlignSize) InAlignSize *= 2;
    return InAlignSize;
}