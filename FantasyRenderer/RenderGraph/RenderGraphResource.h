#pragma once
#include "RenderGraphDefines.h"


struct RenderGraphResource
{
public:
    RenderGraphResource(const char* InName, void* InData = nullptr);
    virtual ~RenderGraphResource() = default;
    
    RenderGraphResource(const RenderGraphResource& rhs);
    RenderGraphResource(RenderGraphResource&& rhs) noexcept;
    RenderGraphResource& operator=(const RenderGraphResource& rhs);
    RenderGraphResource& operator=(RenderGraphResource&& rhs) noexcept;


    bool TryActive();
    bool TryInActive();


    std::string Name;
    std::atomic<bool> Active = false;
    
    UINT64 LastUsedFrame = 0;
    RenderGraphPass* LastUsedPass = nullptr;

    void* Data;
    D3D12Descriptor GPUDescriptor;
    ERenderGraphResourceType Type = ERenderGraphResourceType::Invalid;
};


struct RenderGraphBuffer : public RenderGraphResource
{
    RenderGraphBuffer(const char* InName, D3D12BufferDesc* InDesc, void* InData = nullptr);
    RenderGraphBuffer(const char* InName, D3D12Buffer* InBuffer);
    ~RenderGraphBuffer() noexcept override;

    
    RenderGraphBuffer* GetLastAliasedBuffer() const;

    
    D3D12BufferDesc* Desc;
    std::unique_ptr<D3D12Buffer> Buffer;

    RenderGraphBuffer* AliasBuffer = nullptr;
    RenderGraphBuffer* AliasedBuffer = nullptr;
};


struct RenderGraphTexture : public RenderGraphResource
{
    RenderGraphTexture(const char* InName, D3D12TextureDesc* InDesc, void* InData = nullptr);
    RenderGraphTexture(const char* InName, D3D12Texture* InTexture);
    ~RenderGraphTexture() noexcept override;

    
    RenderGraphTexture* GetLastAliasedTexture() const;
    

    D3D12TextureDesc* Desc;
    std::unique_ptr<D3D12Texture> Texture;

    RenderGraphTexture* AliasTexture = nullptr;
    RenderGraphTexture* AliasedTexture = nullptr;
    
    union
    {
        ED3D12AccessType RenderTargetAccessType;
        std::pair<ED3D12AccessType, ED3D12AccessType> DepthStencilAccessType;
    };
};

