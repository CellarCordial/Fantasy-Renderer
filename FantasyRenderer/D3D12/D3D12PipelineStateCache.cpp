#include "D3D12PipelineStateCache.h"

D3D12PipelineStateCache::D3D12PipelineStateCache(D3D12Device* InDevice, D3D12ShaderCache* InShaderCache)
    : Device(InDevice), ShaderCache(InShaderCache)
{
    GraphicsIDData.resize(GetGraphicsIndex(ED3D12PipelineStateID::GraphicsNum));
    ComputeIDData.resize(GetComputeIndex(ED3D12PipelineStateID::ComputeNum));
    CreateAllPipelineState();
}

ID3D12PipelineState* D3D12PipelineStateCache::GetPipelineState(ED3D12PipelineStateID InID) const
{
    if (CheckPipelineStateType(InID) == ED3D12PipelineStateType::Graphics)
    {
        return GraphicsIDData[GetGraphicsIndex(InID)]->GetNative();
    }
    else if (CheckPipelineStateType(InID) == ED3D12PipelineStateType::Compute)
    {
        return ComputeIDData[GetComputeIndex(InID)]->GetNative();
    }
    else
    {
        return nullptr;
    }
}

UINT32 D3D12PipelineStateCache::GetComputeIndex(ED3D12PipelineStateID InID)
{
    return static_cast<UINT32>(InID) - static_cast<UINT32>(ED3D12PipelineStateID::GraphicsNum) - 1;
}

UINT32 D3D12PipelineStateCache::GetGraphicsIndex(ED3D12PipelineStateID InID)
{
    return static_cast<UINT32>(InID) - 1;
}
