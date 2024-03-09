#pragma once

#include <vector>
#include <functional>
#include <unordered_set>

#include "../D3D12/D3D12Interface.h"
#include "../TaskFlow/TaskExecutor.h"
#include "../Model/LightManager.h"
#include "../Utility/CommonMath.h"

class RenderGraph;
class RenderGraphPass;
struct FrameResource;


enum class ERenderGraphPassType
{
    None,
    Graphics,
    Compute,
    Copy
};

enum class ERenderGraphResourceType
{
    Invalid,
    Read,
    Write,
};


inline constexpr UINT32 MaxLightNum = 64;


struct CameraConstants
{
    DirectX::XMFLOAT4X4 View = Identity4x4Matrix();
    DirectX::XMFLOAT4X4 Proj = Identity4x4Matrix();
    DirectX::XMFLOAT4X4 ViewProj = Identity4x4Matrix();
};

struct LightConstants
{
    UINT32 DirectLightNum;
    DirectLight DirectLights[MaxLightNum];
    UINT32 PointLightNum;
    PointLight PointLights[MaxLightNum];
    UINT32 SpotLightNum;
    SpotLight SpotLights[MaxLightNum];
};
