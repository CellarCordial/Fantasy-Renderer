#include "D3D12Device.h"

#include "../Window/Window.h"

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;}
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = "./D3D12/"; }

D3D12Device::D3D12Device()
{
#if defined(DEBUG) || defined(_DEBUG) 
    // Enable the D3D12 debug layer.
    {
        Microsoft::WRL::ComPtr<ID3D12Debug> DebugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController)));
        DebugController->EnableDebugLayer();
    }
#endif
    
    ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(Factory.GetAddressOf())));
    
    Microsoft::WRL::ComPtr<IDXGIAdapter1> Adapter;
    Factory->EnumAdapters1(0, Adapter.ReleaseAndGetAddressOf());
    
    ThrowIfFailed(D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(Device.GetAddressOf())));

    ResourceAllocator = std::make_unique<D3D12ResourceAllocator>(this);

    D3D12DescriptorHeapDesc CPUDescriptorHeapDesc;
    CPUDescriptorHeapDesc.DescriptorNum = CPU_DESCRIPTOR_NUM;
    CPUDescriptorHeapDesc.ShaderVisible = false;
    for (UINT8 ix = 0; ix < static_cast<UINT8>(ED3D12DescriptorType::Num); ++ix)
    {
        CPUDescriptorHeapDesc.Type = static_cast<ED3D12DescriptorType>(ix);
        CPUDescriptorHeaps.emplace_back(std::make_unique<D3D12CPUDescriptorHeap>(this, CPUDescriptorHeapDesc));
    }

    D3D12DescriptorHeapDesc GPUDescriptorHeapDesc;
    GPUDescriptorHeapDesc.Type = ED3D12DescriptorType::CBV_SRV_UAV;
    GPUDescriptorHeapDesc.DescriptorNum = GPU_DESCRIPTOR_NUM;
    GPUDescriptorHeapDesc.ShaderVisible = true;
    GPUDescriptorHeap = std::make_unique<D3D12GPUDescriptorHeap>(this, GPUDescriptorHeapDesc);

    CreateRootSignature();

    ShaderCache = std::make_unique<D3D12ShaderCache>();
    PipelineStateCache = std::make_unique<D3D12PipelineStateCache>(this, ShaderCache.get());

    GraphicsCmdQueue = std::make_unique<D3D12CommandQueue>(this, ED3D12CommandType::Graphics);
    ComputeCmdQueue = std::make_unique<D3D12CommandQueue>(this, ED3D12CommandType::Compute);
    Fence = std::make_unique<D3D12Fence>(this);
    
    D3D12SwapChainDesc SwapChainDesc;
    SwapChainDesc.Width = Window::GetWidth();
    SwapChainDesc.Height = Window::GetHeight();
    SwapChain = std::make_unique<D3D12SwapChain>(this, SwapChainDesc);
}

void D3D12Device::FlushGraphicsCmdQueue()
{
    GraphicsCmdQueueSignal(Fence.get(), &FenceValue);
    WaitForGPU(Fence.get(), FenceValue);
}

void D3D12Device::GraphicsCmdQueueSignal(D3D12Fence* InFence, UINT64* InOutFenceValue)
{
    (*InOutFenceValue)++;
    GraphicsCmdQueue->Signal(InFence, *InOutFenceValue);
}

void D3D12Device::WaitForGPU(D3D12Fence* InFence, UINT64 InFenceValue)
{
    InFence->Wait(InFenceValue);
}

void D3D12Device::ResizeWindow()
{
    FlushGraphicsCmdQueue();
    SwapChain->ResizeWindow();
}

void D3D12Device::ExecuteGraphicsCommandLists(std::span<D3D12CommandList*> InCmdLists) const
{
    GraphicsCmdQueue->ExecuteCommandLists(InCmdLists);
}

void D3D12Device::ExecuteComputeCommandLists(std::span<D3D12CommandList*> InCmdLists) const
{
    ComputeCmdQueue->ExecuteCommandLists(InCmdLists);
}

void D3D12Device::CreateRootSignature()
{
    CD3DX12_ROOT_PARAMETER1 RootParameter[4];
    RootParameter[0].InitAsConstantBufferView(0);   // CameraConstants
    RootParameter[1].InitAsConstantBufferView(1);   // LightConstants
    RootParameter[2].InitAsConstantBufferView(2);   // 备用
    RootParameter[3].InitAsConstants(6, 3);         // 存放每一个Pass资源在dynamic render中的序号

    CD3DX12_STATIC_SAMPLER_DESC StaticSamplerDesc[6];
    StaticSamplerDesc[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    StaticSamplerDesc[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    StaticSamplerDesc[2].Init(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER);
    StaticSamplerDesc[2].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;

    StaticSamplerDesc[3].Init(3, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    StaticSamplerDesc[4].Init(4, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    StaticSamplerDesc[5].Init(5, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER);
    StaticSamplerDesc[5].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootSignatureDesc;
    RootSignatureDesc.Init_1_1(
        ARRAYSIZE(RootParameter),
        RootParameter,
        ARRAYSIZE(StaticSamplerDesc),
        StaticSamplerDesc,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
    );

    Microsoft::WRL::ComPtr<ID3DBlob> Error;
    Microsoft::WRL::ComPtr<ID3DBlob> RootSignatureBlob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
        &RootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1_1,
        RootSignatureBlob.GetAddressOf(),
        Error.GetAddressOf()
    ));

    ThrowIfFailed(Device->CreateRootSignature(
        0,
        RootSignatureBlob->GetBufferPointer(),
        RootSignatureBlob->GetBufferSize(),
        IID_PPV_ARGS(RootSignature.GetAddressOf())
    ));
}

D3D12Descriptor D3D12Device::AllocateCPUDescriptor(ED3D12DescriptorType InType) const
{
    D3D12Descriptor Output;
    CPUDescriptorHeaps[static_cast<UINT8>(InType)]->TryAllocate(&Output);
    Output.SetType(InType);
    return Output;
}


D3D12Descriptor D3D12Device::GetGPUDescriptor(UINT32 Index) const
{
    return GPUDescriptorHeap->GetDescriptor(Index);
}

D3D12Descriptor D3D12Device::GetPreservedGPUDescriptor() const
{
    return GPUDescriptorHeap->GetPreservedDescriptor();
}

D3D12Descriptor D3D12Device::CreateBufferView(D3D12Buffer* InBuffer)
{
    
    switch (InBuffer->GetDesc()->State)
    {
    case ED3D12ResourceState::UnorderedAccess: return CreateBufferUAV(InBuffer);
    default:
        return CreateBufferSRV(InBuffer);
    }
}


D3D12Descriptor D3D12Device::CreateBufferSRV(D3D12Buffer* InBuffer) const
{
    const D3D12BufferDesc* BufferDesc = InBuffer->GetDesc();

    const D3D12Descriptor Descriptor = AllocateCPUDescriptor(ED3D12DescriptorType::CBV_SRV_UAV);
    
    D3D12_SHADER_RESOURCE_VIEW_DESC SRV{};
    SRV.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    SRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRV.Format = DXGI_FORMAT_UNKNOWN;

    switch (BufferDesc->Flag)
    {
    case ED3D12BufferFlag::Structured:
        ThrowIfFalse(BufferDesc->Stride != 0, "Structured buffer's stride can't be 0.");
        SRV.Buffer.NumElements = static_cast<UINT>(BufferDesc->Size / BufferDesc->Stride);
        SRV.Buffer.StructureByteStride = BufferDesc->Stride;
        break;
    default:
        ThrowIfFalse(false, "Buffer need a flag to be shader resource.");
    }
    Device->CreateShaderResourceView(InBuffer->GetNative(), &SRV, Descriptor.GetCPUHandle());

    return Descriptor;
}

D3D12Descriptor D3D12Device::CreateBufferUAV(D3D12Buffer* InBuffer) const
{


    return D3D12Descriptor{};
}

D3D12Descriptor D3D12Device::CreateTextureView(D3D12Texture* InTexture) const
{
    switch (InTexture->GetDesc()->State)
    {
    case ED3D12ResourceState::DepthRead:
    case ED3D12ResourceState::DepthWrite:
        return CreateTextureDSV(InTexture);
    case ED3D12ResourceState::RenderTarget:
        return CreateTextureRTV(InTexture);
    default:
        return CreateTextureSRV(InTexture);
    }
}

D3D12Descriptor D3D12Device::CreateTextureRTV(D3D12Texture* InTexture) const
{
    const D3D12TextureDesc* TextureDesc = InTexture->GetDesc();

    const D3D12Descriptor Descriptor = AllocateCPUDescriptor(ED3D12DescriptorType::RTV);

    D3D12_RENDER_TARGET_VIEW_DESC RTVDesc{};
    RTVDesc.Format = TextureDesc->Format;
    RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    Device->CreateRenderTargetView(InTexture->GetNative(), &RTVDesc, Descriptor.GetCPUHandle());
    
    return Descriptor;
}

D3D12Descriptor D3D12Device::CreateTextureDSV(D3D12Texture* InTexture) const
{
    const D3D12TextureDesc* TextureDesc = InTexture->GetDesc();

    const D3D12Descriptor Descriptor = AllocateCPUDescriptor(ED3D12DescriptorType::DSV);

    D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc{};
    DSVDesc.Format = TextureDesc->Format;
    DSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    Device->CreateDepthStencilView(InTexture->GetNative(), &DSVDesc, Descriptor.GetCPUHandle());
    
    return Descriptor;
}

D3D12Descriptor D3D12Device::CreateTextureSRV(D3D12Texture* InTexture) const
{
    const D3D12TextureDesc* TextureDesc = InTexture->GetDesc();

    const D3D12Descriptor Descriptor = AllocateCPUDescriptor(ED3D12DescriptorType::CBV_SRV_UAV);

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc{};

    SRVDesc.Format = TextureDesc->Format;
    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Texture2D.MipLevels = 1;

    Device->CreateShaderResourceView(InTexture->GetNative(), &SRVDesc, Descriptor.GetCPUHandle());

    return Descriptor;
}

D3D12Descriptor D3D12Device::AllocateGPUDescriptor(UINT32 InNum/* = 1*/) const
{
    D3D12Descriptor Output;
    GPUDescriptorHeap->TryAllocate(&Output, InNum);
    Output.SetType(ED3D12DescriptorType::CBV_SRV_UAV);
    return Output;
}


void D3D12Device::CopyDescriptor(const D3D12Descriptor& InDst, const D3D12Descriptor& InSrc) const
{
    Device->CopyDescriptorsSimple(1, InDst.GetCPUHandle(), InSrc.GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void D3D12Device::FinishFrameAllocation()
{
    GPUDescriptorHeap->FinishFrameAllocation();
    ResourceAllocator->FinishFrameAllocation();
}

void D3D12Device::ClearFrameResource()
{
    GPUDescriptorHeap->ClearFrameDescriptors();
    ResourceAllocator->ClearFrameResource();
}

void D3D12Device::FreeCPUDescriptor(D3D12Descriptor InDescriptor) const
{
    while (!CPUDescriptorHeaps[static_cast<UINT8>(InDescriptor.GetType())]->TryFree(InDescriptor));
}














