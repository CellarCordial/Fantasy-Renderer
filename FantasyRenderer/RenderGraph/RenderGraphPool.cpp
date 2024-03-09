#include "RenderGraphPool.h"


void RenderGraphResourcePool::Tick(UINT64 InFrameIndex)
{
    Buffers.RemoveIf(
        [this, InFrameIndex](RenderGraphBuffer* InBuffer)
        {
            if (InFrameIndex - InBuffer->LastUsedFrame > 6)
                return true;
            return false;
        }
    );
    Textures.RemoveIf(
        [this, InFrameIndex](RenderGraphTexture* InTexture)
        {
            if (InFrameIndex - InTexture->LastUsedFrame > 6)
                return true;
            return false;
        }
    );
}

void RenderGraphResourcePool::Clear()
{
    Buffers.Clear();
    Textures.Clear();
}

void RenderGraphResourcePool::AllocateBuffer(RenderGraphBuffer* InBuffer, D3D12CommandList* InCmdList)
{
    if (InBuffer->Buffer.get() == nullptr)
    {
        InBuffer->TryActive();

        auto Iterator = Buffers.FindFirstIf(
            [&InBuffer](RenderGraphBuffer* Buffer)
            {
                if (Buffer == InBuffer) return true;
                return !Buffer->Active.load() && Buffer->Desc->CanAlias(InBuffer->Desc);
            }
        );
        
        if (Iterator != InBuffer && Iterator != nullptr)
        {
            RenderGraphBuffer* AliasBuffer = Iterator->GetLastAliasedBuffer();
            if (AliasBuffer && !AliasBuffer->Active.load()) Iterator = AliasBuffer;
            
            if (Iterator->Buffer.get())
            {
                InBuffer->Buffer = std::make_unique<D3D12Buffer>(Device, *InBuffer->Desc, Iterator->Buffer.get());
                InBuffer->Buffer->SetName(StringToWString(InBuffer->Name).c_str());
                InCmdList->CreateBufferAliasingBarrier(Iterator->Buffer.get(), InBuffer->Buffer.get());

                InBuffer->AliasBuffer = Iterator;
                Iterator->AliasedBuffer = InBuffer;
            }
        }
        else
        {
            InBuffer->Buffer = std::make_unique<D3D12Buffer>(Device, *InBuffer->Desc);
            InBuffer->Buffer->CreateOwnCPUDescriptor();
            InBuffer->Buffer->SetName(StringToWString(InBuffer->Name).c_str());
        }
        
        if (InBuffer->Data) InBuffer->Buffer->UploadData(InCmdList, InBuffer->Data);
        if (InBuffer->Desc) { delete InBuffer->Desc; InBuffer->Desc = nullptr; }
    }
    else if (!InBuffer->Active.load())
    {
        InBuffer->TryActive();

        if (InBuffer->AliasBuffer)
        {
            InCmdList->CreateBufferAliasingBarrier(InBuffer->Buffer.get(), InBuffer->AliasBuffer->Buffer.get());
        }
        else if (const RenderGraphBuffer* LastAliasedBuffer = InBuffer->GetLastAliasedBuffer())
        {
            InCmdList->CreateBufferAliasingBarrier(InBuffer->Buffer.get(), LastAliasedBuffer->Buffer.get());
        }
    }
}

void RenderGraphResourcePool::AllocateTexture(RenderGraphTexture* InTexture, D3D12CommandList* InCmdList)
{
    if (InTexture->Texture.get() == nullptr)
    {
        InTexture->TryActive();
        
        auto Iterator = Textures.FindFirstIf(
            [&InTexture](RenderGraphTexture* Texture)
            {
                if (Texture == InTexture) return true;
                return !Texture->Active.load() && Texture->Desc->CanAlias(InTexture->Desc);
            }
        );
        
        if (Iterator != InTexture && Iterator != nullptr)
        {
            RenderGraphTexture* AliasTexture = Iterator->GetLastAliasedTexture();
            if (AliasTexture && !AliasTexture->Active.load()) Iterator = AliasTexture;

            if (Iterator->Texture.get())
            {
                InTexture->Texture = std::make_unique<D3D12Texture>(Device, *InTexture->Desc, Iterator->Texture.get());
                InTexture->Texture->SetName(StringToWString(InTexture->Name).c_str());
                InCmdList->CreateTextureAliasingBarrier(Iterator->Texture.get(), InTexture->Texture.get());

                InTexture->AliasTexture = Iterator;
                Iterator->AliasedTexture = InTexture;
            }
        }
        else
        {
            InTexture->Texture = std::make_unique<D3D12Texture>(Device, *InTexture->Desc);
            InTexture->Texture->CreateOwnCPUDescriptor();
            InTexture->Texture->SetName(StringToWString(InTexture->Name).c_str());
        }
        
        if (InTexture->Data) InTexture->Texture->UploadData(InCmdList, InTexture->Data);
        if (InTexture->Desc) { delete InTexture->Desc; InTexture->Desc = nullptr; }
    }
    else if (!InTexture->Active.load())
    {
        InTexture->TryActive();

        if (InTexture->AliasTexture)
        {
            InCmdList->CreateTextureAliasingBarrier(InTexture->Texture.get(), InTexture->AliasTexture->Texture.get());
        }
        else if (const RenderGraphTexture* LastAliasedTexture = InTexture->GetLastAliasedTexture())
        {
            InCmdList->CreateTextureAliasingBarrier(InTexture->Texture.get(), LastAliasedTexture->Texture.get());
        }
    }
    
}