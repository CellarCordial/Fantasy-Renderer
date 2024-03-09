#pragma once

#include "D3D12Buffer.h"

struct D3D12TextureDesc
{
    bool CanAlias(const D3D12TextureDesc* rhs) const
    {
        ThrowIfFalse(rhs != nullptr, "Try to use nullptr buffer desc to compatible.");

        // ToDo: It needs the same Size and TextureViewDesc which will be Created. Format can be compatible.
        return Width == rhs->Width && Height == rhs->Height &&
               Format == rhs->Format;
    }
    
    ED3D12TextureFlag Flag = ED3D12TextureFlag::None;
    DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
    ED3D12ResourceState State;
    UINT64 Width = 0;
    UINT32 Height = 0;
    D3D12ClearValue ClearValue;
};

class D3D12Texture
{
public:
    CLASS_NO_COPY(D3D12Texture)

    D3D12Texture(D3D12Device* InDevice, const D3D12TextureDesc& InDesc, D3D12Texture* InAliasingTexture = nullptr);
    explicit D3D12Texture(D3D12Texture* InTexture);
    ~D3D12Texture() noexcept;

public:
    void UploadData(D3D12CommandList* InCmdList, void* Data) const;

    void CreateOwnCPUDescriptor();
    void FreeOwnCPUDescriptor();

    ID3D12Resource* GetNative() const { return ResourceLocation.GetResource(); }
    D3D12TextureDesc* GetDesc() { return &Desc; }

    D3D12Descriptor GetCPUDescriptor() const { return CPUDescriptor; }

    void NoNeedToRelease() { ResourceLocation.NeedRelease = false; }
    void NeedToRelease() { ResourceLocation.NeedRelease = true; }

    void SetName(const wchar_t* InName) { ResourceLocation.GetResource()->SetName(InName); }

private:
    D3D12Device* Device; 
    D3D12TextureDesc Desc;
    D3D12ResourceLocation ResourceLocation;
    D3D12Descriptor CPUDescriptor;

    UINT64 UploadDataRequiredSize = 0;
};
