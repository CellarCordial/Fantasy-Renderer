#include "D3D12Fence.h"
#include "D3D12Device.h"

D3D12Fence::D3D12Fence(D3D12Device* InDevice)
{
    ThrowIfFailed(InDevice->GetNative()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(Fence.GetAddressOf())));
    EventHandle = CreateEventW(nullptr, false, false, nullptr);
}

D3D12Fence::~D3D12Fence() noexcept
{
    CloseHandle(EventHandle);
}

void D3D12Fence::Wait(UINT64 InFenceValue)
{
    if (GetCompletedValue() < InFenceValue)
    {
        Fence->SetEventOnCompletion(InFenceValue, EventHandle);

        if (GetCompletedValue() > InFenceValue)
            assert(false);
        WaitForSingleObjectEx(EventHandle, INFINITE, FALSE);    // 等待EventHandle发生
    }
}

void D3D12Fence::CPUSignal(UINT64 InFenceValue)
{
    Fence->Signal(InFenceValue);
}

ID3D12Fence* D3D12Fence::GetNative() const
{
    return Fence.Get();
}

UINT64 D3D12Fence::GetCompletedValue() const
{
    return Fence->GetCompletedValue();
}
