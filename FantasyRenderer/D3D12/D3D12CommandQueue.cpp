#include "D3D12CommandQueue.h"

#include "D3D12Device.h"

D3D12CommandQueue::D3D12CommandQueue(D3D12Device* InDevice, ED3D12CommandType InType)
    : CommandType(InType)
{
    D3D12_COMMAND_QUEUE_DESC Desc{};
    Desc.Type = ConvertToCommandListType(InType);

    ThrowIfFailed(InDevice->GetNative()->CreateCommandQueue(&Desc, IID_PPV_ARGS(CmdQueue.GetAddressOf())));
}

void D3D12CommandQueue::Signal(const D3D12Fence* InFence, UINT64 InFenceValue) const
{
    ThrowIfFailed(CmdQueue->Signal(InFence->GetNative(), InFenceValue));
}

void D3D12CommandQueue::Wait(const D3D12Fence* InFence, UINT64 InFenceValue) const
{
    ThrowIfFailed(CmdQueue->Wait(InFence->GetNative(), InFenceValue));
}

void D3D12CommandQueue::ExecuteCommandLists(std::span<D3D12CommandList*> InCmdLists) const
{
    std::vector<ID3D12CommandList*> CmdLists(InCmdLists.size());
    for (UINT32 ix = 0; ix < InCmdLists.size(); ++ix)
    {
        CmdLists[ix] = InCmdLists[ix]->GetNative();
    }
    if (CmdLists.size() != 0) CmdQueue->ExecuteCommandLists(static_cast<UINT>(InCmdLists.size()), CmdLists.data());
}
