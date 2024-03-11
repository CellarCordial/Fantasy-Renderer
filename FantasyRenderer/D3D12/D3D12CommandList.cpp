#include "D3D12CommandList.h"

#include "D3D12Device.h"

D3D12CommandList::D3D12CommandList(D3D12Device* InDevice, ED3D12CommandType InType)
    : Device(InDevice),
      Type(InType)
{
    ThrowIfFailed(Device->GetNative()->CreateCommandAllocator(ConvertToCommandListType(Type), IID_PPV_ARGS(CmdAllocator.GetAddressOf())));
    ThrowIfFailed(Device->GetNative()->CreateCommandList(0, ConvertToCommandListType(Type), CmdAllocator.Get(), nullptr, IID_PPV_ARGS(CmdList.GetAddressOf())));

    ThrowIfFailed(CmdList->Close());
}

D3D12CommandList::~D3D12CommandList() noexcept
{
    FreeUploadBuffers();
}


void D3D12CommandList::Begin()
{
    FreeUploadBuffers();

    WaitCmdList = nullptr; SignalCmdList = nullptr;

    CmdAllocator->Reset();
    CmdList->Reset(CmdAllocator.Get(), nullptr);

    CmdList->SetGraphicsRootSignature(Device->GetRootSignature());

    ID3D12DescriptorHeap* DescriptorHeaps[] = { Device->GetGPUDescriptorHeap() };
    CmdList->SetDescriptorHeaps(1, DescriptorHeaps);    // 试图使用Bindless render
}

void D3D12CommandList::Close()
{
    FlushBarriers();
    CmdList->Close();
}

void D3D12CommandList::CreateBufferTransitionBarrier(D3D12Buffer* InBuffer, ED3D12ResourceState InNewState)
{
    ED3D12ResourceState& State = InBuffer->GetDesc()->State;

    if (State == InNewState) return;

    CreateTransitionBarrier(InBuffer->GetNative(), State, InNewState);
    State = InNewState;
}

void D3D12CommandList::CreateTextureTransitionBarrier(D3D12Texture* InTexture, ED3D12ResourceState InNewState)
{
    ED3D12ResourceState& State = InTexture->GetDesc()->State;

    if (State == InNewState) return;

    CreateTransitionBarrier(InTexture->GetNative(), State, InNewState);
    State = InNewState;
}

void D3D12CommandList::CreateTransitionBarrier(ID3D12Resource* InResource, ED3D12ResourceState InOldState, ED3D12ResourceState InNewState)
{
    if (InOldState == InNewState) return;

    D3D12_RESOURCE_BARRIER Barrier;
    Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    Barrier.Transition.pResource = InResource;
    Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    Barrier.Transition.StateBefore = ConvertToD3D12ResourceStates(InOldState);
    Barrier.Transition.StateAfter = ConvertToD3D12ResourceStates(InNewState);
    PendingBarriers.push_back(Barrier);
}

void D3D12CommandList::FlushBarriers()
{
    if (!PendingBarriers.empty())
    {
        CmdList->ResourceBarrier(static_cast<UINT>(PendingBarriers.size()), PendingBarriers.data());
    }
    PendingBarriers.clear();
}

void D3D12CommandList::FreeUploadBuffers()
{
    if (!PendingUploadBuffers.empty())
    {
        for (auto Buffer : PendingUploadBuffers)
        {
            delete Buffer;
            Buffer = nullptr;
        }
    }
    PendingUploadBuffers.clear();
}

void D3D12CommandList::CreateBufferAliasingBarrier(D3D12Buffer* InOldBuffer, D3D12Buffer* InNewBuffer)
{
    InOldBuffer->NoNeedToRelease(); InNewBuffer->NeedToRelease();
    CreateAliasingBarrier(InOldBuffer->GetNative(), InNewBuffer->GetNative());
}

void D3D12CommandList::CreateTextureAliasingBarrier(D3D12Texture* InOldTexture, D3D12Texture* InNewTexture)
{
    InOldTexture->NoNeedToRelease(); InNewTexture->NeedToRelease();
    CreateAliasingBarrier(InOldTexture->GetNative(), InNewTexture->GetNative());

    /*const ED3D12TextureFlag Flag = InOldTexture->GetDesc()->Flag;
    if (Flag == ED3D12TextureFlag::AllowRenderTarget || Flag == ED3D12TextureFlag::AllowDepthStencil)
    {
        CmdList->DiscardResource(InOldTexture->GetNative(), nullptr);
    }*/
}

void D3D12CommandList::CreateAliasingBarrier(ID3D12Resource* InOldResource, ID3D12Resource* InNewResource)
{
    D3D12_RESOURCE_BARRIER Barrier;
    Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    Barrier.Aliasing.pResourceBefore = InOldResource;
    Barrier.Aliasing.pResourceAfter = InNewResource;

    PendingBarriers.push_back(Barrier);
}

void D3D12CommandList::BeginRenderPass(const D3D12RenderPassDesc& InDesc) const
{
    if (HasFlag(InDesc.Flags, ED3D12RenderPassFlags::UseLegacy))
    {
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RTVs;
        for (UINT32 ix = 0; ix < InDesc.RenderTargets.size(); ++ix)
        {
            const auto RenderTarget = InDesc.RenderTargets[ix];

            RTVs.push_back(RenderTarget->GetCPUDescriptor().GetCPUHandle());

            if (InDesc.RenderTargetAccessTypes[ix] == ED3D12AccessType::Clear_Preserve)
            {
                const auto ClearValue = RenderTarget->GetDesc()->ClearValue.GetNative();
                CmdList->ClearRenderTargetView(RTVs.back(), ClearValue->Color, 0, nullptr);
            }
        }

        D3D12_CPU_DESCRIPTOR_HANDLE DSV;
        if (InDesc.DepthStencil)
        {
            const auto DepthStencil = InDesc.DepthStencil;

            DSV = DepthStencil->GetCPUDescriptor().GetCPUHandle();

            if (InDesc.DepthAccessType == ED3D12AccessType::Clear_Preserve)
            {
                D3D12_CLEAR_FLAGS ClearFlag = D3D12_CLEAR_FLAG_DEPTH;
                if (InDesc.StencilAccessType != ED3D12AccessType::InValid_InValid)
                {
                    ClearFlag |= D3D12_CLEAR_FLAG_STENCIL;
                }

                const auto ClearValue = DepthStencil->GetDesc()->ClearValue.GetNative();
                CmdList->ClearDepthStencilView(DSV, ClearFlag, ClearValue->DepthStencil.Depth, ClearValue->DepthStencil.Stencil, 0, nullptr);
            }
        }
        CmdList->OMSetRenderTargets(static_cast<UINT>(RTVs.size()), RTVs.data(), false, InDesc.DepthStencil ? &DSV : nullptr);
    }
    else
    {
        std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> RenderTargetDescs;
        for (UINT32 ix = 0; ix < InDesc.RenderTargets.size(); ++ix)
        {
            const auto RenderTarget = InDesc.RenderTargets[ix];
            
            D3D12_RENDER_PASS_RENDER_TARGET_DESC RenderTargetDesc{};
            RenderTargetDesc.cpuDescriptor = RenderTarget->GetCPUDescriptor().GetCPUHandle();
            RenderTargetDesc.BeginningAccess.Clear = { *RenderTarget->GetDesc()->ClearValue.GetNative() };
            SplitD3D12AccessType(
                InDesc.RenderTargetAccessTypes[ix],
                &RenderTargetDesc.BeginningAccess.Type,
                &RenderTargetDesc.EndingAccess.Type
            );

            RenderTargetDescs.push_back(RenderTargetDesc);
        }

        D3D12_RENDER_PASS_DEPTH_STENCIL_DESC DepthStencilDesc{};
        if (InDesc.DepthStencil)
        {
            const auto DepthStencil = InDesc.DepthStencil;

            DepthStencilDesc.cpuDescriptor = DepthStencil->GetCPUDescriptor().GetCPUHandle();

            DepthStencilDesc.DepthBeginningAccess.Clear = { *DepthStencil->GetDesc()->ClearValue.GetNative() };
            SplitD3D12AccessType(
                InDesc.DepthAccessType,
                &DepthStencilDesc.DepthBeginningAccess.Type,
                &DepthStencilDesc.DepthEndingAccess.Type
            );
            DepthStencilDesc.StencilBeginningAccess.Clear = { *InDesc.DepthStencil->GetDesc()->ClearValue.GetNative() };
            SplitD3D12AccessType(
                InDesc.StencilAccessType,
                &DepthStencilDesc.StencilBeginningAccess.Type,
                &DepthStencilDesc.StencilEndingAccess.Type
            );
        }
        CmdList->BeginRenderPass(
            static_cast<UINT>(RenderTargetDescs.size()),
            RenderTargetDescs.data(),
            InDesc.DepthStencil ? &DepthStencilDesc : nullptr,
            ConvertToD3D12RenderPassFlags(InDesc.Flags)
        );
    }
}

void D3D12CommandList::EndRenderPass() const
{
    CmdList->EndRenderPass();
}

void D3D12CommandList::SetPipelineState(ID3D12PipelineState* InPipelineState) const
{
    CmdList->SetPipelineState(InPipelineState);
}


void D3D12CommandList::SetViewport(UINT32 InWidth, UINT32 InHeight) const
{
    const D3D12_VIEWPORT ScreenViewport{ 0.0f, 0.0f, static_cast<FLOAT>(InWidth), static_cast<FLOAT>(InHeight), 0.0f, 1.0f }; 
    const D3D12_RECT ScissorRect{ 0, 0, static_cast<LONG>(InWidth), static_cast<LONG>(InHeight)};

    CmdList->RSSetViewports(1, &ScreenViewport);
    CmdList->RSSetScissorRects(1, &ScissorRect);
}

void D3D12CommandList::DrawIndexedInstance(D3D12Buffer* InVertexBuffer, D3D12Buffer* InIndexBuffer, UINT32 InVertexSize, UINT32 InIndexNum, UINT32 InInstanceNum/* = 1 */) const
{
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
    VertexBufferView.BufferLocation = InVertexBuffer->GetNative()->GetGPUVirtualAddress();
    VertexBufferView.SizeInBytes = static_cast<UINT>(InVertexBuffer->GetDesc()->Size);
    VertexBufferView.StrideInBytes = InVertexSize;

    D3D12_INDEX_BUFFER_VIEW IndexBufferView;
    IndexBufferView.BufferLocation = InIndexBuffer->GetNative()->GetGPUVirtualAddress();
    IndexBufferView.SizeInBytes = static_cast<UINT>(InIndexBuffer->GetDesc()->Size);
    IndexBufferView.Format = DXGI_FORMAT_R16_UINT;

    CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    CmdList->IASetIndexBuffer(&IndexBufferView);
    CmdList->IASetVertexBuffers(0, 1, &VertexBufferView);
    CmdList->DrawIndexedInstanced(InIndexNum, InInstanceNum, 0, 0, 0);
}





























