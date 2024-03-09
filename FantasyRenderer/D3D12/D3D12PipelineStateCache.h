#pragma once
#include "D3D12PipelineState.h"

enum class ED3D12PipelineStateID : UINT32;

class D3D12PipelineStateCache
{
public:
    D3D12PipelineStateCache(D3D12Device* InDevice, D3D12ShaderCache* InShaderCache);
    ~D3D12PipelineStateCache() = default;

public:
    ID3D12PipelineState* GetPipelineState(ED3D12PipelineStateID InID) const;

private:
    void CreateAllPipelineState();
    static ED3D12PipelineStateType CheckPipelineStateType(ED3D12PipelineStateID InID);

    static UINT32 GetGraphicsIndex(ED3D12PipelineStateID InID);
    static UINT32 GetComputeIndex(ED3D12PipelineStateID InID);
    
private:
    D3D12Device* Device;
    D3D12ShaderCache* ShaderCache;
    std::vector<std::unique_ptr<D3D12GraphicsPipelineState>> GraphicsIDData;
    std::vector<std::unique_ptr<D3D12ComputePipelineState>> ComputeIDData;
};




/*--------------------------- PipelineState Defines -------------------------------*/

enum class ED3D12PipelineStateID : UINT32
{
    Invalid,
    Simple,
    Sample,
    GraphicsNum,
    ComputeNum
};

inline ED3D12PipelineStateType D3D12PipelineStateCache::CheckPipelineStateType(ED3D12PipelineStateID InID)
{
    switch (InID)
    {
    case ED3D12PipelineStateID::Sample:
        return ED3D12PipelineStateType::Graphics;
    case ED3D12PipelineStateID::Invalid:
    default:
        return ED3D12PipelineStateType::Invalid;
    }
}

inline void D3D12PipelineStateCache::CreateAllPipelineState()
{
    D3D12GraphicsPipelineStateDesc GraphicsDesc{};
    
    GraphicsDesc.RenderTargetNum = 1;
    GraphicsDesc.RTVFormat[0] = SWAP_CHAIN_FORMAT;
    GraphicsDesc.DSVFormat = SWAP_CHAIN_DEPTH_STENCIL_FORMAT;
    GraphicsDesc.VS = ED3D12ShaderID::VS_Sample;
    GraphicsDesc.PS = ED3D12ShaderID::PS_Sample;

    GraphicsIDData[GetGraphicsIndex(ED3D12PipelineStateID::Sample)] = std::make_unique<D3D12GraphicsPipelineState>(Device, GraphicsDesc, ShaderCache);

    
    
    D3D12ComputePipelineStateDesc ComputeDesc{};
    {
        
    }
}
