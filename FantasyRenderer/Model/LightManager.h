#pragma once
#include <DirectXMath.h>
#include <vector>
#include <windows.h>
#include "../Utility/Macros.h"

enum class ELightType
{
    Point,
    Direct,
    Spot
};


struct DirectLight
{
    DirectX::XMFLOAT3 Color = { 0.5f, 0.5f, 0.5f };
    float Strength = 10.0f;
    DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };
    float Pad1 = 0.0f;
};

struct PointLight
{
    DirectX::XMFLOAT3 Color = { 0.5f, 0.5f, 0.5f };
    float Strength = 10.0f;
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
    float FallOffStart = 1.0f;
    
    float FallOffEnd = 10.0f;
};

struct SpotLight
{
    DirectX::XMFLOAT3 Color = { 0.5f, 0.5f, 0.5f };
    float Strength = 10.0f;
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
    float FallOffStart = 1.0f;
    DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };
    float FallOffEnd = 10.0f;

    float SpotPower;
};


class LightManager
{
public:
    CLASS_NO_COPY(LightManager)
        
    LightManager();
    ~LightManager() = default;

public:
    void AddLight(ELightType InType);
    
    UINT32 GetDirectLightNum() const { return static_cast<UINT32>(DirectLights.size()); }
    UINT32 GetPointLightNum() const { return static_cast<UINT32>(PointLights.size()); }
    UINT32 GetSpotLightNum() const { return static_cast<UINT32>(SpotLights.size()); }
    DirectLight* GetDirectLightData() { return DirectLights.data(); }
    PointLight* GetPointLightData() { return PointLights.data(); }
    SpotLight* GetSpotLightData() { return SpotLights.data(); }


private:
    std::vector<DirectLight> DirectLights;
    std::vector<PointLight> PointLights;
    std::vector<SpotLight> SpotLights;
};
