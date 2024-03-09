#include "D3D12Buffer.h"

#include "D3D12Device.h"

D3D12Buffer::D3D12Buffer(D3D12Device* InDevice, const D3D12BufferDesc& InDesc, D3D12Buffer* InAliasingBuffer/* = nullptr*/)
    : Device(InDevice),
      Desc(InDesc),
      ResourceLocation(InDevice->GetResourceAllocator(), ConvertToED3D12ResourceLocationType(InDesc.Type))
{
    UINT64 OffsetInHeap = INVALID_SIZE_64;
    if (InAliasingBuffer)
    {
        if (Desc.CanAlias(InAliasingBuffer->GetDesc()))
        {
            InAliasingBuffer->NoNeedToRelease();
            OffsetInHeap = InAliasingBuffer->ResourceLocation.GetOffset();
            
            CPUDescriptor = InAliasingBuffer->CPUDescriptor;
        }
    }
    
    D3D12_RESOURCE_DESC BufferDesc;
    BufferDesc.Alignment = 0;
    BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    BufferDesc.Flags = ConvertToD3D12ResourceFlags(Desc.Flag);
    BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    BufferDesc.Height = 1;
    BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    BufferDesc.Width = Desc.Size;
    BufferDesc.MipLevels = 1;
    BufferDesc.SampleDesc = { 1, 0 };
    BufferDesc.DepthOrArraySize = 1;

    D3D12ResourceLocationDesc LocationDesc;
    LocationDesc.ResourceDesc = &BufferDesc;
    LocationDesc.ResourceState = ConvertToD3D12ResourceStates(Desc.State);
    LocationDesc.ClearValue = Desc.ClearValue.GetNative();
    LocationDesc.Size = Desc.Size;
    
    D3D12ResourceAllocator* ResourceAllocator = Device->GetResourceAllocator();
    ThrowIfFalse(ResourceAllocator->TryAllocate(&ResourceLocation, &LocationDesc, OffsetInHeap), "Memory allocate failed.");
}

D3D12Buffer::D3D12Buffer(D3D12Buffer* InBuffer) : ResourceLocation(nullptr, ED3D12ResourceLocationType::Invalid)
{
    ThrowIfFalse(InBuffer != nullptr, "Try to use nullptr to initialize.");

    Device = InBuffer->Device;
    Desc = InBuffer->Desc;
    
    ResourceLocation.ResourceAllocator = InBuffer->ResourceLocation.ResourceAllocator;
    ResourceLocation.Type = InBuffer->ResourceLocation.Type;
    ResourceLocation.Resource = InBuffer->ResourceLocation.Resource;        InBuffer->ResourceLocation.Resource = nullptr;
    ResourceLocation.Location = InBuffer->ResourceLocation.Location;
    ResourceLocation.NeedRelease = InBuffer->ResourceLocation.NeedRelease;  InBuffer->ResourceLocation.NeedRelease = false;

    CPUDescriptor = InBuffer->CPUDescriptor;
    InBuffer->CPUDescriptor.Reset();


    InBuffer = nullptr;
}

D3D12Buffer::~D3D12Buffer() noexcept
{
    if (CPUDescriptor.IsValid())
    {
        Device->FreeCPUDescriptor(CPUDescriptor);
    }
}

void D3D12Buffer::UploadData(D3D12CommandList* InCmdList, const void* InData) const
{
    ThrowIfFalse(InData != nullptr && Desc.Type == ED3D12BufferType::Default, "Try to use nullptr data.");

    InCmdList->CreateTransitionBarrier(this->GetNative(), Desc.State, ED3D12ResourceState::CopyDst);
    InCmdList->FlushBarriers();

    D3D12BufferDesc UploadBufferDesc{};
    UploadBufferDesc.Size = Desc.Size;
    UploadBufferDesc.State = ED3D12ResourceState::GenericRead;
    UploadBufferDesc.Type = ED3D12BufferType::Upload;

    D3D12Buffer* UploadBuffer = new D3D12Buffer(Device, UploadBufferDesc);
    InCmdList->AddUploadBuffer(UploadBuffer);

    D3D12_SUBRESOURCE_DATA SubresourceData;
    SubresourceData.pData = InData;
    SubresourceData.RowPitch = static_cast<LONG_PTR>(Desc.Size);
    SubresourceData.SlicePitch = static_cast<LONG_PTR>(Desc.Size);
        
    UpdateSubresources<1>(InCmdList->GetNative(), this->GetNative(), UploadBuffer->GetNative(), 0, 0, 1, &SubresourceData);

    InCmdList->CreateTransitionBarrier(this->GetNative(), ED3D12ResourceState::CopyDst, Desc.State);
    InCmdList->FlushBarriers();
}

void D3D12Buffer::CreateOwnCPUDescriptor()
{
    CPUDescriptor = Device->CreateBufferView(this);
}

void D3D12Buffer::UpdateMappedData(void* InData, UINT32 InSize)
{
    ThrowIfFalse(InData != nullptr && Desc.Type == ED3D12BufferType::Upload, "Try to use nullptr data.");

    UINT8* MappedData;
    const CD3DX12_RANGE Range(0, 0);
    ResourceLocation.GetResource()->Map(0, &Range, reinterpret_cast<void**>(&MappedData));
    memcpy(MappedData, InData, InSize);
    ResourceLocation.GetResource()->Unmap(0, nullptr);
}
