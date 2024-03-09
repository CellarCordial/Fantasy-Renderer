#pragma once
#include <DirectXCollision.h>
#include <math.h>

#include "../Pass/PassDefines.h"
#include "../Utility/Macros.h"
#include "../Utility/CommonMath.h"
#include "../Window/Window.h"

class Camera
{
public:
    CLASS_NO_COPY(Camera)

    Camera();
    ~Camera() = default;

public:
    void MouseAction(EMouseActionType InType, INT32 X, INT32 Y, bool Down);

    void UpdateViewMatrix();
    void UpdateMousePosition(UINT32 X, UINT32 Y);
    void HandleKeyboardInput(float InDeltaTime);

    void SetLens(float InFOV, float InAspect, float InNearZ, float InFarZ);
    void SetPosition(float X, float Y, float Z);
    void SetPosition(const DirectX::XMFLOAT3& InPosition);
    
    void Strafe(float InSize);      // 前后移动
    void Walk(float InSize);        // 左右平移
    void Vertical(float InSize);    // 上下移动  

    void Pitch(float InAngle);  // 上下俯仰
    void Yall(float InAngle);   // 左右转向
    void Pitch(INT32 X);  // 上下俯仰
    void Yall(INT32 Y);   // 左右转向

    DirectX::XMFLOAT3 GetPosition() const;
    DirectX::XMFLOAT3 GetRight() const;
    DirectX::XMFLOAT3 GetUp() const;
    DirectX::XMFLOAT3 GetLook() const;
    
    float GetNearZ() const;
    float GetFarZ() const;
    float GetAspect() const;
    float GetFOVX() const;
    float GetFOVY() const;
    
    float GetNearWindowWidth() const;
    float GetNearWindowHeight() const;
    float GetFarWindowWidth() const;
    float GetFarWindowHeight() const;
    
    DirectX::XMMATRIX GetView() const;
    DirectX::XMMATRIX GetProj() const;

private:
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 Look = { 0.0f, 0.0f, 1.0f };
    DirectX::XMFLOAT3 Up = { 0.0f, 1.0f, 0.0f };
    DirectX::XMFLOAT3 Right = { 1.0f, 0.0f, 0.0f };

    float NearZ = 0.0f;
    float FarZ = 0.0f;

    float Aspect = 0.0f;
    float FOVY = 0.0f;

    float NearWindowHeight = 0.0f;
    float FarWindowHeight = 0.0f;

    bool ViewDirty = true;

    DirectX::XMFLOAT4X4 ViewMatrix = Identity4x4Matrix();
    DirectX::XMFLOAT4X4 ProjMatrix = Identity4x4Matrix();

    POINT MousePosition;
};
