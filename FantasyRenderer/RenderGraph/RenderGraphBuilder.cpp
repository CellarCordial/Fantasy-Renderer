#include "RenderGraphBuilder.h"

#include "RenderGraph.h"

RenderGraphBuffer* RenderGraphBuilder::ImportBuffer(const char* InName, D3D12Buffer* InBuffer, bool NeedDescriptor) const
{
    RenderGraphBuffer* Iterator = Graph->ResourcePool->Buffers.FindFirstIf(
        [&InName](const RenderGraphBuffer* InBuffer) { return InName == InBuffer->Name; }
    );

    if (Iterator == nullptr)
    {
        if (NeedDescriptor) InBuffer->CreateOwnCPUDescriptor();
        
        RenderGraphBuffer* Buffer = Graph->ResourcePool->Buffers.PushBack(InName, InBuffer);
        Buffer->Buffer->SetName(StringToWString(InName).c_str());
        Pass->ResourceStateMap[Buffer] = Buffer->Buffer->GetDesc()->State;
        Pass->ReadBuffers.push_back(Buffer);
        return Buffer;
    }
    return Iterator;
}

RenderGraphTexture* RenderGraphBuilder::ImportTexture(const char* InName, D3D12Texture* InTexture, bool NeedDescriptor) const
{
    RenderGraphTexture* Iterator = Graph->ResourcePool->Textures.FindFirstIf(
        [&InName](const RenderGraphTexture* InTexture) { return InName == InTexture->Name; }
    );

    if (Iterator == nullptr)
    {
        if (NeedDescriptor) InTexture->CreateOwnCPUDescriptor();

        RenderGraphTexture* Texture = Graph->ResourcePool->Textures.PushBack(InName, InTexture);
        Texture->Texture->SetName(StringToWString(InName).c_str());
        Pass->ResourceStateMap[Texture] = Texture->Texture->GetDesc()->State;
        Pass->ReadTextures.push_back(Texture);
        return Texture;
    }
    return Iterator;
}


RenderGraphBuffer* RenderGraphBuilder::TransitionReadBuffer(const char* InName, ED3D12ResourceState InState) const
{
    RenderGraphBuffer* Buffer = Graph->ResourcePool->Buffers.FindFirstIf(
        [&InName](const RenderGraphBuffer* InBuffer) { return InName == InBuffer->Name; }
    );
    
    ThrowIfFalse(Buffer != nullptr, "No such buffer name.");

    Pass->ResourceStateMap[Buffer] = InState;
    Pass->ReadBuffers.push_back(Buffer);

    return Buffer;
}

RenderGraphTexture* RenderGraphBuilder::TransitionReadTexture(const char* InName, ED3D12ResourceState InState) const
{
    RenderGraphTexture* Texture = Graph->ResourcePool->Textures.FindFirstIf(
        [&InName](const RenderGraphTexture* InTexture) { return InName == InTexture->Name; }
    );
    
    ThrowIfFalse(Texture != nullptr, "No such Texture name.");

    Pass->ResourceStateMap[Texture] = InState;
    Pass->ReadTextures.push_back(Texture);

    return Texture;
}

RenderGraphBuffer* RenderGraphBuilder::TransitionWriteBuffer(const char* InName, ED3D12ResourceState InState) const
{
    RenderGraphBuffer* Buffer = Graph->ResourcePool->Buffers.FindFirstIf(
        [&InName](const RenderGraphBuffer* InBuffer) { return InName == InBuffer->Name; }
    );
    
    ThrowIfFalse(Buffer != nullptr, "No such buffer name.");

    Pass->ResourceStateMap[Buffer] = InState;
    Pass->WriteBuffers.push_back(Buffer);

    return Buffer;
}

RenderGraphTexture* RenderGraphBuilder::TransitionWriteTexture(const char* InName, ED3D12ResourceState InState) const
{
    RenderGraphTexture* Texture = Graph->ResourcePool->Textures.FindFirstIf(
        [&InName](const RenderGraphTexture* InTexture) { return InName == InTexture->Name; }
    );
    
    ThrowIfFalse(Texture != nullptr, "No such Texture name.");

    Pass->ResourceStateMap[Texture] = InState;
    Pass->WriteTextures.push_back(Texture);

    return Texture;
}

RenderGraphBuffer* RenderGraphBuilder::DeclareReadBuffer(const char* InName, const D3D12BufferDesc& InDesc, void* InData/* = nullptr*/) const
{
    D3D12BufferDesc* Desc = new D3D12BufferDesc(InDesc);
    RenderGraphBuffer* Buffer = Graph->ResourcePool->Buffers.PushBack(InName, Desc, InData);
    Pass->ResourceStateMap[Buffer] = Desc->State;
    Pass->ReadBuffers.push_back(Buffer);
    return Buffer;
}

RenderGraphBuffer* RenderGraphBuilder::DeclareWriteBuffer(const char* InName, const D3D12BufferDesc& InDesc) const
{
    D3D12BufferDesc* Desc = new D3D12BufferDesc(InDesc);
    RenderGraphBuffer* Buffer = Graph->ResourcePool->Buffers.PushBack(InName, Desc);
    Pass->ResourceStateMap[Buffer] = Desc->State;
    Pass->WriteBuffers.push_back(Buffer);
    return Buffer;
}

RenderGraphTexture* RenderGraphBuilder::DeclareReadTexture(const char* InName, const D3D12TextureDesc& InDesc, void* InData/* = nullptr*/) const
{
    D3D12TextureDesc* Desc = new D3D12TextureDesc(InDesc);
    RenderGraphTexture* Texture = Graph->ResourcePool->Textures.PushBack(InName, Desc, InData);
    Pass->ResourceStateMap[Texture] = Desc->State;
    Pass->ReadTextures.push_back(Texture);
    return Texture;
}

RenderGraphTexture* RenderGraphBuilder::DeclareWriteTexture(const char* InName, const D3D12TextureDesc& InDesc) const
{
    D3D12TextureDesc* Desc = new D3D12TextureDesc(InDesc);
    RenderGraphTexture* Texture = Graph->ResourcePool->Textures.PushBack(InName, Desc);
    Pass->ResourceStateMap[Texture] = Desc->State;
    Pass->WriteTextures.push_back(Texture);
    return Texture;
}


void RenderGraphBuilder::AllocateBuffer(RenderGraphBuffer* InBuffer, D3D12CommandList* InCmdList) const
{
    Graph->ResourcePool->AllocateBuffer(InBuffer, InCmdList);
}

void RenderGraphBuilder::AllocateTexture(RenderGraphTexture* InTexture, D3D12CommandList* InCmdList) const
{
    Graph->ResourcePool->AllocateTexture(InTexture, InCmdList);
}

FrameResource* RenderGraphBuilder::GetFrameResource()
{
    return Graph->FrameResources[GetThreadIndex()].get();
}


void RenderGraphBuilder::SubmitCmdList(D3D12CommandList* InCmdList)
{
    auto& FrameResource = Graph->FrameResources[GetThreadIndex()];
    {
        std::lock_guard LockGuard(FrameResource->CmdListsMutex);
        FrameResource->CmdLists.push_back(InCmdList);
    }
}

UINT32 RenderGraphBuilder::GetThreadIndex() const
{
    return Graph->ThreadIndex;
}

UINT64 RenderGraphBuilder::GetFrameIndex() const
{
    return Graph->FrameIndex;
}


























