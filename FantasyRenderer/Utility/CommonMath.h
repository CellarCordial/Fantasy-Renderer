#pragma once
#include <DirectXCollision.h>
#include <DirectXMath.h>
#include <windows.h>


inline DirectX::XMFLOAT4X4 Identity4x4Matrix()
{
    static DirectX::XMFLOAT4X4 I(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    return I;
}
