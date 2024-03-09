#pragma once

#include "D3D12ShaderCompiler.h"

enum class ED3D12ShaderID : UINT32;

class D3D12ShaderCache
{
public:
    CLASS_NO_COPY(D3D12ShaderCache)

    D3D12ShaderCache();
    ~D3D12ShaderCache() = default;

public:
    void CompileAllShader();
    void CompileOneShader(ED3D12ShaderID InID);

    D3D12ShaderData* GetShaderData(ED3D12ShaderID InID) const { return ShaderIDData[static_cast<UINT32>(InID)].get(); }
    D3D12_INPUT_LAYOUT_DESC GetInputLayout(ED3D12ShaderID InID) { return Compiler.GetInputLayout(GetShaderData(InID)); }
    
private:
    static ED3D12ShaderTarget GetShaderTarget(ED3D12ShaderID InID);
    static std::string GetShaderEntryPoint(ED3D12ShaderID InID);
    static std::string GetShaderFileName(ED3D12ShaderID InID);
    
private:
    D3D12ShaderCompiler Compiler;
    std::vector<std::unique_ptr<D3D12ShaderData>> ShaderIDData;
};



/*--------------------------- Shader Defines -------------------------------*/

enum class ED3D12ShaderID : UINT32
{
    Invalid,
    VS_Sample,
    PS_Sample,
    Num
};

inline std::string D3D12ShaderCache::GetShaderFileName(ED3D12ShaderID InID)
{
    // 需要文件格式后缀
    switch (InID)
    {
    case ED3D12ShaderID::VS_Sample:
    case ED3D12ShaderID::PS_Sample:
        return "Sample.hlsl";
    default:
        return "";
    }
}


inline ED3D12ShaderTarget D3D12ShaderCache::GetShaderTarget(ED3D12ShaderID InID)
{
    switch (InID)
    {
    case ED3D12ShaderID::VS_Sample:
        return ED3D12ShaderTarget::Vertex;
    case ED3D12ShaderID::PS_Sample:
        return ED3D12ShaderTarget::Pixel;
        
    default:
        return ED3D12ShaderTarget::Invalid;
    }
}

inline std::string D3D12ShaderCache::GetShaderEntryPoint(ED3D12ShaderID InID)
{
    switch (InID)
    {
    case ED3D12ShaderID::VS_Sample:
        return "VertexShader";
    case ED3D12ShaderID::PS_Sample:
        return "PixelShader";
    default:
        return "Main";
    }
}

