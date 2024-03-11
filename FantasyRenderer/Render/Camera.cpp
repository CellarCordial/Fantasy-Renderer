#include "Camera.h"

Camera::Camera()
{
    Window::GetMouseActionEvent()->AddEvent(this, &Camera::MouseAction);

    SetLens(0.25f * DirectX::XM_PI, static_cast<float>(Window::GetWidth()) / static_cast<float>(Window::GetHeight()), 1.0f, 1000.0f);
}

void Camera::MouseAction(EMouseActionType InType, INT32 X, INT32 Y, bool RightDown)
{
    switch (InType)
    {
    case EMouseActionType::Down:
        if (RightDown) ShowCursor(false);
        UpdateMousePosition(X, Y);
        SetCapture(Window::GetHWND());
        break;
    case EMouseActionType::Up:
        if (RightDown) ShowCursor(true);
        ReleaseCapture();
        break;
    case EMouseActionType::Move:
        if(RightDown)
        {
            Pitch(Y);
            Yall(X);
            UpdateViewMatrix();

            CursorCycle(X, Y);
        }
        UpdateMousePosition(X, Y);
        break;
    }
}

void Camera::UpdateViewMatrix()
{
    if(ViewDirty)
    {
        DirectX::XMVECTOR R = DirectX::XMLoadFloat3(&Right);
        DirectX::XMVECTOR U = DirectX::XMLoadFloat3(&Up);
        DirectX::XMVECTOR L = DirectX::XMLoadFloat3(&Look);
        DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&Position);

        L = DirectX::XMVector3Normalize(L);
        U = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(L, R));
        R = DirectX::XMVector3Cross(U, L);

        float x = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, R));
        float y = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, U));
        float z = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, L));

        DirectX::XMStoreFloat3(&Right, R);
        DirectX::XMStoreFloat3(&Up, U);
        DirectX::XMStoreFloat3(&Look, L);

        ViewMatrix(0, 0) = Right.x;
        ViewMatrix(1, 0) = Right.y;
        ViewMatrix(2, 0) = Right.z;
        ViewMatrix(3, 0) = x;

        ViewMatrix(0, 1) = Up.x;
        ViewMatrix(1, 1) = Up.y;
        ViewMatrix(2, 1) = Up.z;
        ViewMatrix(3, 1) = y;

        ViewMatrix(0, 2) = Look.x;
        ViewMatrix(1, 2) = Look.y;
        ViewMatrix(2, 2) = Look.z;
        ViewMatrix(3, 2) = z;

        ViewMatrix(0, 3) = 0.0f;
        ViewMatrix(1, 3) = 0.0f;
        ViewMatrix(2, 3) = 0.0f;
        ViewMatrix(3, 3) = 1.0f;

        ViewDirty = false;
    }
}

void Camera::UpdateMousePosition(UINT32 X, UINT32 Y)
{
    MousePosition = { static_cast<LONG>(X), static_cast<LONG>(Y) };    
}

void Camera::HandleKeyboardInput(float InDeltaTime)
{
    if(GetAsyncKeyState('W') & 0x8000) Walk(8.0f * InDeltaTime);
    if(GetAsyncKeyState('S') & 0x8000) Walk(-8.0f * InDeltaTime);
    if(GetAsyncKeyState('D') & 0x8000) Strafe(8.0f * InDeltaTime);
    if(GetAsyncKeyState('A') & 0x8000) Strafe(-8.0f * InDeltaTime);
    if(GetAsyncKeyState('E') & 0x8000) Vertical(8.0f * InDeltaTime);
    if(GetAsyncKeyState('Q') & 0x8000) Vertical(-8.0f * InDeltaTime);

    UpdateViewMatrix();
}

void Camera::SetLens(float InFOV, float InAspect, float InNearZ, float InFarZ)
{
    FOVY = InFOV; Aspect = InAspect; NearZ = InNearZ; FarZ = InFarZ;
    NearWindowHeight = NearZ * static_cast<float>(tan(FOVY)) * 2.0f;
    FarWindowHeight = FarZ * static_cast<float>(tan(FOVY)) * 2.0f;

     const DirectX::XMMATRIX Proj = DirectX::XMMatrixPerspectiveFovLH(FOVY, Aspect, NearZ, FarZ);     // LH is left hand
    DirectX::XMStoreFloat4x4(&ProjMatrix, Proj);
}

void Camera::SetPosition(float X, float Y, float Z)
{
    Position = DirectX::XMFLOAT3{ X, Y, Z };
    ViewDirty = true;
}

void Camera::SetPosition(const DirectX::XMFLOAT3& InPosition)
{
    Position = InPosition;
    ViewDirty = true;
}

void Camera::Walk(float InSize)
{
    const DirectX::XMVECTOR Size = DirectX::XMVectorReplicate(InSize);
    const DirectX::XMVECTOR LookVector = DirectX::XMLoadFloat3(&Look);
    const DirectX::XMVECTOR PositionVector = DirectX::XMLoadFloat3(&Position);
    DirectX::XMStoreFloat3(&Position, DirectX::XMVectorMultiplyAdd(Size, LookVector, PositionVector));  // v1 * v2 + v3

    ViewDirty = true;
}

void Camera::Strafe(float InSize)
{
    const DirectX::XMVECTOR Size = DirectX::XMVectorReplicate(InSize);
    const DirectX::XMVECTOR RightVector = DirectX::XMLoadFloat3(&Right);
    const DirectX::XMVECTOR PositionVector = DirectX::XMLoadFloat3(&Position);
    DirectX::XMStoreFloat3(&Position, DirectX::XMVectorMultiplyAdd(Size, RightVector, PositionVector));  // v1 * v2 + v3

    ViewDirty = true;
}

void Camera::Vertical(float InSize)
{
    const DirectX::XMVECTOR Size = DirectX::XMVectorReplicate(InSize);
    const DirectX::XMVECTOR UpVector = DirectX::XMLoadFloat3(&Up);
    const DirectX::XMVECTOR PositionVector = DirectX::XMLoadFloat3(&Position);
    DirectX::XMStoreFloat3(&Position, DirectX::XMVectorMultiplyAdd(Size, UpVector, PositionVector));  // v1 * v2 + v3

    ViewDirty = true;
}

void Camera::Pitch(float InAngle)
{
    const DirectX::XMMATRIX Rotation = DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(&Right), InAngle);

    DirectX::XMStoreFloat3(&Up, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&Up), Rotation));
    DirectX::XMStoreFloat3(&Look, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&Look), Rotation));

    ViewDirty = true;
}

void Camera::Pitch(INT32 Y)
{
    const float DeltaY = DirectX::XMConvertToRadians(0.20f * static_cast<float>(Y - MousePosition.y));
    Pitch(DeltaY);
}

void Camera::Yall(float InAngle)
{
    const DirectX::XMMATRIX Rotation = DirectX::XMMatrixRotationY(InAngle);

    DirectX::XMStoreFloat3(&Right, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&Right), Rotation));
    DirectX::XMStoreFloat3(&Up, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&Up), Rotation));
    DirectX::XMStoreFloat3(&Look, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&Look), Rotation));

    ViewDirty = true;
}

void Camera::Yall(INT32 X)
{
    const float DeltaX = DirectX::XMConvertToRadians(0.20f * static_cast<float>(X - MousePosition.x));
    Yall(DeltaX);
}


DirectX::XMMATRIX Camera::GetProj() const
{
    return DirectX::XMLoadFloat4x4(&ProjMatrix);
}

DirectX::XMMATRIX Camera::GetView() const
{
    return DirectX::XMLoadFloat4x4(&ViewMatrix);
}

DirectX::XMFLOAT3 Camera::GetPosition() const
{
    return Position;
}

DirectX::XMFLOAT3 Camera::GetRight() const
{
    return Right;
}

DirectX::XMFLOAT3 Camera::GetUp() const
{
    return Up;
}

DirectX::XMFLOAT3 Camera::GetLook() const
{
    return Look;
}

float Camera::GetNearZ() const
{
    return NearZ;
}

float Camera::GetFarZ() const
{
    return FarZ;
}

float Camera::GetAspect() const
{
    return Aspect;
}

float Camera::GetFOVY() const
{
    return FOVY;
}

float Camera::GetFOVX() const
{
    return 2.0f * static_cast<float>(atan((0.5f * GetNearWindowWidth()) / NearZ));
}

float Camera::GetNearWindowWidth() const
{
    return Aspect * NearWindowHeight;
}

float Camera::GetNearWindowHeight() const
{
    return NearWindowHeight;
}

float Camera::GetFarWindowWidth() const
{
    return Aspect * FarWindowHeight;
}

float Camera::GetFarWindowHeight() const
{
    return FarWindowHeight;
}

void Camera::CursorCycle(INT32& X, INT32& Y)
{
    RECT ClientRect;
    GetClientRect(Window::GetHWND(), &ClientRect);
    if (X > ClientRect.right)
    {
        POINT CursorPos = { X, Y };
        POINT ClientPoint = { ClientRect.left, ClientRect.top };
        ClientToScreen(Window::GetHWND(), &ClientPoint);
        ClientToScreen(Window::GetHWND(), &CursorPos);
        SetCursorPos(ClientPoint.x, CursorPos.y);
        X = ClientRect.left;
    }
    if (X < ClientRect.left)
    {
        POINT CursorPos = { X, Y };
        POINT ClientPoint = { ClientRect.right, ClientRect.bottom };
        ClientToScreen(Window::GetHWND(), &ClientPoint);
        ClientToScreen(Window::GetHWND(), &CursorPos);
        SetCursorPos(ClientPoint.x, CursorPos.y);
        X = ClientRect.right;
    }
    if (Y > ClientRect.bottom)
    {
        POINT CursorPos = { X, Y };
        POINT ClientPoint = { ClientRect.left, ClientRect.top };
        ClientToScreen(Window::GetHWND(), &ClientPoint);
        ClientToScreen(Window::GetHWND(), &CursorPos);
        SetCursorPos(CursorPos.x, ClientPoint.y);
        Y = ClientRect.top;
    }
    if (Y < ClientRect.top)
    {
        POINT CursorPos = { X, Y };
        POINT ClientPoint = { ClientRect.right, ClientRect.bottom };
        ClientToScreen(Window::GetHWND(), &ClientPoint);
        ClientToScreen(Window::GetHWND(), &CursorPos);
        SetCursorPos(CursorPos.x, ClientPoint.y);
        Y = ClientRect.bottom;
    }
}
