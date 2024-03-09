#include "RenderGraphPass.h"

#include "RenderGraph.h"
#include "RenderGraphBuilder.h"

RenderGraphPass::RenderGraphPass(RenderGraphBuilder* InBuilder, const RenderGraphPassDesc& InDesc)
    : Builder(InBuilder) , Desc(InDesc)
{
    Builder->SetPass(this);
}

RenderGraphPass::~RenderGraphPass() noexcept
{
    if (Builder)
    {
        delete Builder;
        Builder = nullptr;
    }
}

void RenderGraphPass::ResourceAllocationAndTransition()
{
    auto AllocateAndTransitionBuffer = [this](RenderGraphBuffer* InBuffer)
    {
        Builder->AllocateBuffer(InBuffer, CmdList);
        CmdList->CreateBufferTransitionBarrier(InBuffer->Buffer.get(), ResourceStateMap[InBuffer]);
        InBuffer->LastUsedFrame = Builder->GetFrameIndex();

        const D3D12Descriptor CPUDescriptor = InBuffer->Buffer->GetCPUDescriptor();
        if (CPUDescriptor.IsValid())
        {
            D3D12Device* Device = CmdList->GetDevice();
            InBuffer->GPUDescriptor = Device->AllocateGPUDescriptor();
            Device->CopyDescriptor(InBuffer->GPUDescriptor, CPUDescriptor);
        }
    };

    auto AllocateAndTransitionTexture = [this](RenderGraphTexture* InTexture)
    {
        const ED3D12ResourceState ResourceState = ResourceStateMap[InTexture];
        
        Builder->AllocateTexture(InTexture, CmdList);
        CmdList->CreateTextureTransitionBarrier(InTexture->Texture.get(), ResourceState);
        InTexture->LastUsedFrame = Builder->GetFrameIndex();

        const D3D12Descriptor CPUDescriptor = InTexture->Texture->GetCPUDescriptor();
        if (CPUDescriptor.IsValid())
        {
            const ED3D12DescriptorType DescriptorType = CPUDescriptor.GetType();
            switch (ResourceState)
            {
            case ED3D12ResourceState::RenderTarget:
                if (DescriptorType != ED3D12DescriptorType::RTV) InTexture->Texture->CreateOwnCPUDescriptor(); break;
            case ED3D12ResourceState::DepthWrite:
                if (DescriptorType != ED3D12DescriptorType::DSV) InTexture->Texture->CreateOwnCPUDescriptor(); break;
            case ED3D12ResourceState::PixelShader:
                if (DescriptorType != ED3D12DescriptorType::CBV_SRV_UAV) InTexture->Texture->CreateOwnCPUDescriptor();
                
                {
                    D3D12Device* Device = CmdList->GetDevice();
                    InTexture->GPUDescriptor = Device->AllocateGPUDescriptor();
                    Device->CopyDescriptor(InTexture->GPUDescriptor, CPUDescriptor);
                }

            default: break;
            }
        }
    };

    
    {
        FrameResource* FrameResourceData = Builder->GetFrameResource();
        std::lock_guard LockGuard(FrameResourceData->CmdListsMutex);
        FrameResourceData->CmdLists.push_back(CmdList);     // 为了防止并行时资源状态转换的混乱, 需锁住并抢占一个位置

        std::ranges::for_each(std::begin(ReadBuffers), std::end(ReadBuffers), AllocateAndTransitionBuffer);
        std::ranges::for_each(std::begin(WriteBuffers), std::end(WriteBuffers), AllocateAndTransitionBuffer);
        std::ranges::for_each(std::begin(ReadTextures), std::end(ReadTextures), AllocateAndTransitionTexture);
        std::ranges::for_each(std::begin(WriteTextures), std::end(WriteTextures), AllocateAndTransitionTexture);
    }
    
    
    CmdList->FlushBarriers();
}

void RenderGraphPass::BeginRenderPass()
{
    if (Desc.Type == ERenderGraphPassType::Graphics)
    {
        CmdList->SetViewport(Desc.Width, Desc.Height);
    
        D3D12RenderPassDesc RenderPassDesc{};

        for (const RenderGraphTexture* Texture : WriteTextures)
        {
            D3D12TextureDesc* Desc = Texture->Texture->GetDesc();
        
            if (Desc->State == ED3D12ResourceState::RenderTarget)
            {
                RenderPassDesc.RenderTargets.push_back(Texture->Texture.get());
                RenderPassDesc.RenderTargetAccessTypes.push_back(Texture->RenderTargetAccessType);
            }
            else if (Desc->State == ED3D12ResourceState::DepthWrite)
            {
                RenderPassDesc.DepthStencil = Texture->Texture.get();
                RenderPassDesc.DepthAccessType = Texture->DepthStencilAccessType.first;
                RenderPassDesc.StencilAccessType = Texture->DepthStencilAccessType.second;
            }
        }
        RenderPassDesc.Flags = Desc.Flags;
        CmdList->BeginRenderPass(RenderPassDesc);
    }
}

void RenderGraphPass::EndRenderPass()
{
    if (Desc.Type == ERenderGraphPassType::Graphics &&
        !HasFlag(Desc.Flags, ED3D12RenderPassFlags::UseLegacy))
    {
        CmdList->EndRenderPass();
    }
}


void RenderGraphPass::InActiveResources()
{
    std::ranges::for_each(
        std::begin(ResourcesToDestroy),
        std::end(ResourcesToDestroy),
        [](RenderGraphResource* InResource) { InResource->TryInActive(); }
    );
}





















