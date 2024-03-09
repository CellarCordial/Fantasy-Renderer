#pragma once

#include "D3D12Defines.h"

class D3D12Fence
{
public:
    CLASS_NO_COPY(D3D12Fence)

    explicit D3D12Fence(D3D12Device* InDevice);
    ~D3D12Fence() noexcept;

public:
    void Wait(UINT64 InFenceValue);
    void CPUSignal(UINT64 InFenceValue);

    ID3D12Fence* GetNative() const;
    UINT64 GetCompletedValue() const;
    
private:
    HANDLE EventHandle;
    Microsoft::WRL::ComPtr<ID3D12Fence> Fence;

    bool Finish = false;
};
