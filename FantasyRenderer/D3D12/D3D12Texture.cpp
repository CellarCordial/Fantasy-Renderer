#include "D3D12Texture.h"

#include "D3D12Device.h"

D3D12Texture::D3D12Texture(D3D12Device* InDevice, const D3D12TextureDesc& InDesc, D3D12Texture* InAliasingTexture/* = nullptr*/)
    : Device(InDevice),
      Desc(InDesc),
      ResourceLocation(InDevice->GetResourceAllocator(), ED3D12ResourceLocationType::Texture)
{
    UINT64 OffsetInHeap = INVALID_SIZE_64;
    if (InAliasingTexture)
    {
        if (Desc.CanAlias(InAliasingTexture->GetDesc()))
        {
            InAliasingTexture->NoNeedToRelease();
            OffsetInHeap = InAliasingTexture->ResourceLocation.GetOffset();
            
            CPUDescriptor = InAliasingTexture->CPUDescriptor;
        }
    }
    
    D3D12_RESOURCE_DESC TextureDesc;
    TextureDesc.Alignment = 0;
    TextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    TextureDesc.Flags = ConvertToD3D12ResourceFlags(Desc.Flag);
    TextureDesc.Format = Desc.Format;
    TextureDesc.Width = Desc.Width;
    TextureDesc.Height = Desc.Height;
    TextureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;;
    TextureDesc.MipLevels = 1;
    TextureDesc.SampleDesc = { 1, 0 };
    TextureDesc.DepthOrArraySize = 1;

    const UINT32 PixelSize = GetDXGIPixelSize(Desc.Format);
    const UINT32 SubresourceNum = TextureDesc.DepthOrArraySize * TextureDesc.MipLevels;
    Device->GetNative()->GetCopyableFootprints(&TextureDesc, 0, SubresourceNum, 0, nullptr, nullptr, nullptr, &UploadDataRequiredSize);
    
    D3D12ResourceLocationDesc LocationDesc;
    LocationDesc.ResourceDesc = &TextureDesc;
    LocationDesc.ResourceState = ConvertToD3D12ResourceStates(Desc.State);
    LocationDesc.ClearValue = Desc.ClearValue.GetNative();
    LocationDesc.Size = Desc.Width * Desc.Height * PixelSize;
    
    D3D12ResourceAllocator* ResourceAllocator = Device->GetResourceAllocator();
    ThrowIfFalse(ResourceAllocator->TryAllocate(&ResourceLocation, &LocationDesc, OffsetInHeap), "Memory allocate failed.");
}

D3D12Texture::D3D12Texture(D3D12Texture* InTexture) : ResourceLocation(nullptr, ED3D12ResourceLocationType::Invalid)
{
    ThrowIfFalse(InTexture != nullptr, "Try to use nullptr to initialize.");

    Device = InTexture->Device;
    Desc = InTexture->Desc;
    
    ResourceLocation.ResourceAllocator = InTexture->ResourceLocation.ResourceAllocator;
    ResourceLocation.Type = InTexture->ResourceLocation.Type;
    ResourceLocation.Resource = InTexture->ResourceLocation.Resource;        InTexture->ResourceLocation.Resource = nullptr;
    ResourceLocation.Location = InTexture->ResourceLocation.Location;
    ResourceLocation.NeedRelease = InTexture->ResourceLocation.NeedRelease;  InTexture->ResourceLocation.NeedRelease = false;

    UploadDataRequiredSize = InTexture->UploadDataRequiredSize;

    CPUDescriptor = InTexture->CPUDescriptor;
    InTexture->CPUDescriptor.Reset();

    InTexture = nullptr;
}

D3D12Texture::~D3D12Texture() noexcept
{
    FreeOwnCPUDescriptor();
}


void D3D12Texture::UploadData(D3D12CommandList* InCmdList, void* Data) const
{
    ThrowIfFalse(Data != nullptr, "Try to use nullptr data.");

    InCmdList->CreateTransitionBarrier(this->GetNative(), Desc.State, ED3D12ResourceState::CopyDst);
    InCmdList->FlushBarriers();

    const UINT32 PixelSize = GetDXGIPixelSize(Desc.Format);

    D3D12BufferDesc UploadBufferDesc{};
    UploadBufferDesc.Size = UploadDataRequiredSize;
    UploadBufferDesc.State = ED3D12ResourceState::GenericRead;
    UploadBufferDesc.Type = ED3D12BufferType::Upload;
    
    D3D12Buffer* UploadBuffer = new D3D12Buffer(Device, UploadBufferDesc);
    InCmdList->AddUploadBuffer(UploadBuffer);

    D3D12_SUBRESOURCE_DATA SubresourceData;
    SubresourceData.pData = Data;
    SubresourceData.RowPitch = static_cast<LONG_PTR>(Desc.Width) * PixelSize;
    SubresourceData.SlicePitch = SubresourceData.RowPitch * Desc.Height;

    UpdateSubresources(
        InCmdList->GetNative(),
        this->GetNative(),
        UploadBuffer->GetNative(),
        0,
        0,
        1,  // TextureDesc.DepthOrArraySize * TextureDesc.MipLevels
        &SubresourceData
    );

    InCmdList->CreateTransitionBarrier(this->GetNative(), ED3D12ResourceState::CopyDst, Desc.State);
    InCmdList->FlushBarriers();
}

void D3D12Texture::CreateOwnCPUDescriptor()
{
    FreeOwnCPUDescriptor();
    CPUDescriptor = Device->CreateTextureView(this);
}

void D3D12Texture::FreeOwnCPUDescriptor()
{
    if (CPUDescriptor.IsValid())
    {
        Device->FreeCPUDescriptor(CPUDescriptor);
    }
}

