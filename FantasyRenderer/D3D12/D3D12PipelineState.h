#pragma once
#include "D3D12ShaderCache.h"

struct D3D12GraphicsPipelineStateDesc
{
    UINT32 RenderTargetNum = 1;
    DXGI_FORMAT RTVFormat[8];
    DXGI_FORMAT DSVFormat;
    
    ED3D12ShaderID VS = ED3D12ShaderID::Invalid;
    ED3D12ShaderID HS = ED3D12ShaderID::Invalid;
    ED3D12ShaderID DS = ED3D12ShaderID::Invalid;
    ED3D12ShaderID GS = ED3D12ShaderID::Invalid;
    ED3D12ShaderID PS = ED3D12ShaderID::Invalid;
};

struct D3D12ComputePipelineStateDesc
{
    ED3D12ShaderID CS;
};


class D3D12GraphicsPipelineState
{
public:
    CLASS_NO_COPY(D3D12GraphicsPipelineState)

    D3D12GraphicsPipelineState(D3D12Device* InDevice, const D3D12GraphicsPipelineStateDesc& InDesc, D3D12ShaderCache* InShaderCache);
    ~D3D12GraphicsPipelineState() = default;

public:
    ID3D12PipelineState* GetNative() const { return PipelineState.Get(); }
    
private:
    D3D12GraphicsPipelineStateDesc Desc;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;
};


class D3D12ComputePipelineState
{
public:
    CLASS_NO_COPY(D3D12ComputePipelineState)

    D3D12ComputePipelineState(D3D12Device* InDevice, const D3D12ComputePipelineStateDesc& InDesc, D3D12ShaderCache* InShaderCache);
    ~D3D12ComputePipelineState() = default;

public:
    ID3D12PipelineState* GetNative() const { return PipelineState.Get(); }
    
private:
    D3D12ComputePipelineStateDesc Desc;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;
};