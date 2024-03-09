#include "D3D12ShaderCache.h"

D3D12ShaderCache::D3D12ShaderCache() : ShaderIDData(static_cast<UINT32>(ED3D12ShaderID::Num))
{
    ShaderIDData[static_cast<UINT32>(ED3D12ShaderID::Invalid)] = std::make_unique<D3D12ShaderData>();
    CompileAllShader();
}

void D3D12ShaderCache::CompileOneShader(ED3D12ShaderID InID)
{
    D3D12ShaderDesc ShaderDesc;
    ShaderDesc.Target = GetShaderTarget(InID);
    ShaderDesc.EntryPoint = GetShaderEntryPoint(InID);
    ShaderDesc.ShaderName = GetShaderFileName(InID);

    ShaderIDData[static_cast<UINT32>(InID)] = std::make_unique<D3D12ShaderData>(Compiler.CompileShader(ShaderDesc));
}

void D3D12ShaderCache::CompileAllShader()
{
    constexpr UINT32 IDNum = static_cast<UINT32>(ED3D12ShaderID::Num);

    for (UINT32 ix = 1; ix < IDNum; ++ix)
    {
        CompileOneShader(static_cast<ED3D12ShaderID>(ix));
    }
}
