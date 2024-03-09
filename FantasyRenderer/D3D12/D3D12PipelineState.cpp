#include "D3D12PipelineState.h"
#include "D3D12Device.h"

D3D12GraphicsPipelineState::D3D12GraphicsPipelineState(D3D12Device* InDevice, const D3D12GraphicsPipelineStateDesc& InDesc, D3D12ShaderCache* InShaderCache)
    : Desc(InDesc)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsDesc{};

    GraphicsDesc.pRootSignature = InDevice->GetRootSignature();
    GraphicsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    GraphicsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    
    GraphicsDesc.InputLayout = InShaderCache->GetInputLayout(Desc.VS);
    GraphicsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    GraphicsDesc.NumRenderTargets = Desc.RenderTargetNum;
    std::copy(std::begin(Desc.RTVFormat), std::end(Desc.RTVFormat), GraphicsDesc.RTVFormats);
    GraphicsDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    GraphicsDesc.DSVFormat = Desc.DSVFormat;

    GraphicsDesc.VS = InShaderCache->GetShaderData(Desc.VS)->GetByteCode();
    GraphicsDesc.HS = InShaderCache->GetShaderData(Desc.HS)->GetByteCode();
    GraphicsDesc.DS = InShaderCache->GetShaderData(Desc.DS)->GetByteCode();
    GraphicsDesc.GS = InShaderCache->GetShaderData(Desc.GS)->GetByteCode();
    GraphicsDesc.PS = InShaderCache->GetShaderData(Desc.PS)->GetByteCode();
    
    GraphicsDesc.SampleDesc = { 1, 0 };
    GraphicsDesc.SampleMask = UINT_MAX;

    ThrowIfFailed(InDevice->GetNative()->CreateGraphicsPipelineState(&GraphicsDesc, IID_PPV_ARGS(PipelineState.GetAddressOf())));
}

D3D12ComputePipelineState::D3D12ComputePipelineState(D3D12Device* InDevice, const D3D12ComputePipelineStateDesc& InDesc, D3D12ShaderCache* InShaderCache)
    : Desc(InDesc)
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC ComputeDesc{};
    ComputeDesc.pRootSignature = InDevice->GetRootSignature();
    ComputeDesc.CS = InShaderCache->GetShaderData(Desc.CS)->GetByteCode();

    ThrowIfFailed(InDevice->GetNative()->CreateComputePipelineState(&ComputeDesc, IID_PPV_ARGS(PipelineState.GetAddressOf())));
}





















