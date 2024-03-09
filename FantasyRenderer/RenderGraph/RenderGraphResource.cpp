#include "RenderGraphResource.h"

RenderGraphResource::RenderGraphResource(const char* InName, void* InData/* = nullptr*/) : Name(InName), Data(InData)
{
}

RenderGraphResource::RenderGraphResource(const RenderGraphResource& rhs)
    : Name(rhs.Name),
      Active(rhs.Active.load()),
      LastUsedFrame(rhs.LastUsedFrame),
      LastUsedPass(rhs.LastUsedPass),
      Data(rhs.Data)
{
}
RenderGraphResource::RenderGraphResource(RenderGraphResource&& rhs) noexcept
    : Name(std::move(rhs.Name)),
      Active(rhs.Active.load()),
      LastUsedFrame(rhs.LastUsedFrame),
      LastUsedPass(rhs.LastUsedPass),
      Data(rhs.Data)
{
}
RenderGraphResource& RenderGraphResource::operator=(const RenderGraphResource& rhs)
{
    if (this == &rhs) return *this;
    Name = rhs.Name;
    Active = rhs.Active.load();
    LastUsedFrame = rhs.LastUsedFrame;
    LastUsedPass = rhs.LastUsedPass;
    return *this;
}

RenderGraphResource& RenderGraphResource::operator=(RenderGraphResource&& rhs) noexcept
{
    Name = std::move(rhs.Name);
    Active = rhs.Active.load();
    LastUsedFrame = rhs.LastUsedFrame;
    LastUsedPass = rhs.LastUsedPass;
    return *this;
}

bool RenderGraphResource::TryActive()
{
    bool Expected = false;
    return Active.compare_exchange_strong(Expected, true);
}

bool RenderGraphResource::TryInActive()
{
    bool Expected = true;
    return Active.compare_exchange_strong(Expected, false);
}

RenderGraphBuffer::RenderGraphBuffer(const char* InName, D3D12BufferDesc* InDesc, void* InData/* = nullptr*/)
    : RenderGraphResource(InName, InData), Desc(InDesc)
{}

RenderGraphBuffer::RenderGraphBuffer(const char* InName, D3D12Buffer* InBuffer)
    : RenderGraphResource(InName, nullptr),
      Desc(nullptr),
      Buffer(std::make_unique<D3D12Buffer>(InBuffer))
{}

RenderGraphBuffer::~RenderGraphBuffer()
{
    if (Desc) { delete Desc; Desc = nullptr; }
    if (AliasBuffer) AliasBuffer->AliasedBuffer = AliasedBuffer;
    if (AliasedBuffer) AliasedBuffer->AliasBuffer = AliasBuffer;
}

RenderGraphTexture::RenderGraphTexture(const char* InName, D3D12TextureDesc* InDesc, void* InData/* = nullptr*/)
    : RenderGraphResource(InName, InData), Desc(InDesc)
{}

RenderGraphTexture::RenderGraphTexture(const char* InName, D3D12Texture* InTexture)
    : RenderGraphResource(InName, nullptr),
      Desc(nullptr),
      Texture(std::make_unique<D3D12Texture>(InTexture))
{}

RenderGraphTexture::~RenderGraphTexture()
{
    if (Desc) { delete Desc; Desc = nullptr; } 
    if (AliasTexture) AliasTexture->AliasedTexture = AliasedTexture;
    if (AliasedTexture) AliasedTexture->AliasTexture = AliasTexture;
}

RenderGraphTexture* RenderGraphTexture::GetLastAliasedTexture() const
{
    if (!AliasedTexture) return nullptr;
    RenderGraphTexture* LastAliasedResource = AliasedTexture;
    while (LastAliasedResource->AliasedTexture) LastAliasedResource = LastAliasedResource->AliasedTexture;
    return LastAliasedResource;
}

RenderGraphBuffer* RenderGraphBuffer::GetLastAliasedBuffer() const
{
    if (!AliasedBuffer) return nullptr;
    RenderGraphBuffer* LastAliasedResource = AliasedBuffer;
    while (LastAliasedResource->AliasedBuffer) LastAliasedResource = LastAliasedResource->AliasedBuffer;
    return LastAliasedResource;
}
