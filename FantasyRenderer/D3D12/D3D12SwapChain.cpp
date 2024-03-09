#include "D3D12SwapChain.h"
#include "../Window/Window.h"
#include "D3D12Device.h"

D3D12SwapChain::D3D12SwapChain(D3D12Device* InDevice, const D3D12SwapChainDesc& InDesc)
    : Device(InDevice)
{
    Microsoft::WRL::ComPtr<IDXGISwapChain1> TempSwapChain;
    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc{};
    SwapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
    SwapChainDesc.Width = InDesc.Width;
    SwapChainDesc.Height = InDesc.Height;
    SwapChainDesc.Format = SWAP_CHAIN_FORMAT;
    SwapChainDesc.Scaling = DXGI_SCALING_NONE;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    SwapChainDesc.SampleDesc.Count = 1;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC FullScreenDesc;
    FullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    FullScreenDesc.Windowed = true;
    FullScreenDesc.RefreshRate = { 1, 60 };
    FullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

    ThrowIfFailed(Device->GetFactory()->CreateSwapChainForHwnd(
        Device->GetGraphicsCmdQueue()->GetNative(),
        Window::GetHWND(),
        &SwapChainDesc,
        &FullScreenDesc,
        nullptr,
        TempSwapChain.GetAddressOf()
    ));

    ThrowIfFailed(TempSwapChain.As(&SwapChain));

    CreateBackBuffers();
    CreateDepthStencilBuffer();
}

void D3D12SwapChain::CreateBackBuffers()
{
    for (UINT32 ix = 0; ix < SWAP_CHAIN_BUFFER_COUNT; ++ix)
    {
        if (BackBufferDescriptors[ix].IsValid()) Device->FreeCPUDescriptor(BackBufferDescriptors[ix]);
        BackBufferDescriptors[ix] = Device->AllocateCPUDescriptor(ED3D12DescriptorType::RTV);
        ThrowIfFailed(SwapChain->GetBuffer(ix, IID_PPV_ARGS(BackBuffers[ix].GetAddressOf())));
        Device->GetNative()->CreateRenderTargetView(BackBuffers[ix].Get(), nullptr, BackBufferDescriptors[ix].GetCPUHandle());
    }
}

void D3D12SwapChain::CreateDepthStencilBuffer()
{
    if (DepthStencilDescriptor.IsValid()) Device->FreeCPUDescriptor(DepthStencilDescriptor);
    
    D3D12_RESOURCE_DESC DepthStencilDesc;
    DepthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    DepthStencilDesc.Alignment = 0;
    DepthStencilDesc.Width = Window::GetWidth();
    DepthStencilDesc.Height = Window::GetHeight();
    DepthStencilDesc.DepthOrArraySize = 1;
    DepthStencilDesc.MipLevels = 1;
    DepthStencilDesc.Format = SWAP_CHAIN_DEPTH_STENCIL_FORMAT;
    DepthStencilDesc.SampleDesc = { 1, 0 };
    DepthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    DepthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE ClearValue;
    ClearValue.Format = SWAP_CHAIN_DEPTH_STENCIL_FORMAT;
    ClearValue.DepthStencil.Depth = 1.0f;
    ClearValue.DepthStencil.Stencil = 0;
    
    const auto HeapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    
    ThrowIfFailed(Device->GetNative()->CreateCommittedResource(
        &HeapProperty,
        D3D12_HEAP_FLAG_NONE,
        &DepthStencilDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &ClearValue,
        IID_PPV_ARGS(DepthStencilBuffer.GetAddressOf())
    ));

    DepthStencilDescriptor = Device->AllocateCPUDescriptor(ED3D12DescriptorType::DSV);
    Device->GetNative()->CreateDepthStencilView(DepthStencilBuffer.Get(), nullptr, DepthStencilDescriptor.GetCPUHandle());
}


void D3D12SwapChain::Present(UINT32 InThreadIndex)
{
    SwapChain->Present(0, 0);
    CurrentBackBufferIndex = (CurrentBackBufferIndex + 1) % SWAP_CHAIN_BUFFER_COUNT;
}

void D3D12SwapChain::ResizeWindow()
{
    CurrentBackBufferIndex = 0;
    for (auto& BackBuffer : BackBuffers) BackBuffer.Reset();
    DepthStencilBuffer.Reset();

    ThrowIfFailed(SwapChain->ResizeBuffers(
        SWAP_CHAIN_BUFFER_COUNT,
        Window::GetWidth(),
        Window::GetHeight(),
        SWAP_CHAIN_FORMAT,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
    ));

    CreateBackBuffers();
    CreateDepthStencilBuffer();
}








