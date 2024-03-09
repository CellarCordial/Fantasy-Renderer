#include "D3D12ShaderCompiler.h"

#include "../Utility/Serialization.h"

D3D12ShaderCompiler::D3D12ShaderCompiler()
{
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(DxcCompiler.GetAddressOf())));
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(DxcUtils.GetAddressOf())));
}

D3D12ShaderCompiler::~D3D12ShaderCompiler() noexcept
{
    DxcCompiler.Reset();
    DxcUtils.Reset();
}

D3D12ShaderData D3D12ShaderCompiler::CompileShader(const D3D12ShaderDesc& InDesc)
{
    const std::string CachePath = ShaderCacheRootDirectory + RemoveFileExtension(InDesc.ShaderName.c_str()) + "_" + InDesc.EntryPoint + "_DEBUG.bin";
    const std::string ShaderPath = ShaderRootDirectory + InDesc.ShaderName;

    if (CheckCache(CachePath.c_str(), ShaderPath.c_str()))
    {
        return LoadFromCache(CachePath.c_str());
    }

    const std::wstring EntryPoint = StringToWString(InDesc.EntryPoint);
    std::vector<LPCWSTR> CompileArguments
    {
        L"-HV 2021",
        L"-E",
        EntryPoint.c_str(),
        L"-T",
        GetTargetProfile(InDesc.Target),
        DXC_ARG_DEBUG,
        DXC_ARG_WARNINGS_ARE_ERRORS,
        DXC_ARG_PACK_MATRIX_ROW_MAJOR
    };

    Microsoft::WRL::ComPtr<IDxcBlobEncoding> ShaderBlob;
    ThrowIfFailed(DxcUtils->LoadFile(StringToWString(ShaderPath).c_str(), nullptr, ShaderBlob.GetAddressOf()));
    const DxcBuffer ShaderBuffer(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(), 0u);

    CustomDxcIncludeHandler IncludeHandler(DxcUtils.Get());
    Microsoft::WRL::ComPtr<IDxcResult> CompileResult;
    ThrowIfFailed(DxcCompiler->Compile(
        &ShaderBuffer,
        CompileArguments.data(),
        static_cast<UINT32>(CompileArguments.size()),
        &IncludeHandler,
        IID_PPV_ARGS(CompileResult.GetAddressOf())
    ));

    Microsoft::WRL::ComPtr<IDxcBlobUtf8> Errors;
    ThrowIfFailed(CompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(Errors.GetAddressOf()), nullptr));
    if (Errors && Errors->GetStringLength() > 0)
    {
        MessageBoxA(nullptr, Errors->GetStringPointer(), nullptr, 0);
        ThrowIfFalse(false, "Compile shader failed.");
    }

    Microsoft::WRL::ComPtr<IDxcBlob> ResultShaderData;
    ThrowIfFailed(CompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(ResultShaderData.GetAddressOf()), nullptr));

    D3D12ShaderData ShaderData;
    ShaderData.SetByteCode(ResultShaderData->GetBufferPointer(), ResultShaderData->GetBufferSize());
    ShaderData.IncludeShaderFiles = std::move(IncludeHandler.IncludedFileNames);
    ShaderData.IncludeShaderFiles.push_back(InDesc.ShaderName.c_str());

    SaveToCache(CachePath.c_str(), ShaderData);

    return ShaderData;
}

bool D3D12ShaderCompiler::CheckCache(const char* InCachePath, const char* InShaderPath)
{
    if (!IsFileExist(InCachePath)) return false;
    if (CompareFileWriteTime(InShaderPath, InCachePath)) return false;
    return true;
}

void D3D12ShaderCompiler::SaveToCache(const char* InCachePath, const D3D12ShaderData& InData)
{
    ThrowIfFalse(!InData.Invalid(), "Invalid Shader Data.");

    BinaryOutput Output(InCachePath);
    Output(InData.IncludeShaderFiles);
    Output(InData.Data.size());
    Output.SaveBinaryData(InData.Data.data(), InData.Data.size());
}

D3D12ShaderData D3D12ShaderCompiler::LoadFromCache(const char* InCachePath)
{
    D3D12ShaderData ShaderData;
    
    BinaryInput Input(InCachePath);
    Input(ShaderData.IncludeShaderFiles);

    UINT64 ByteCodeSize = 0;
    Input(ByteCodeSize);
    ShaderData.Data.resize(ByteCodeSize);
    Input.LoadBinaryData(ShaderData.Data.data(), ByteCodeSize);

    return ShaderData;
}

D3D12_INPUT_LAYOUT_DESC D3D12ShaderCompiler::GetInputLayout(const D3D12ShaderData* InData)
{
    const D3D12_SHADER_BYTECODE ShaderByteCode = InData->GetByteCode();
    const DxcBuffer ShaderBuffer{ ShaderByteCode.pShaderBytecode, ShaderByteCode.BytecodeLength, 0u };

    Microsoft::WRL::ComPtr<ID3D12ShaderReflection> Reflection;
    ThrowIfFailed(DxcUtils->CreateReflection(&ShaderBuffer, IID_PPV_ARGS(Reflection.GetAddressOf())));

    D3D12_SHADER_DESC ShaderDesc;
    ThrowIfFailed(Reflection->GetDesc(&ShaderDesc));

    const UINT32 InputParameterNum = ShaderDesc.InputParameters;

    SemanticNames.resize(InputParameterNum);
    InputElementDescs.resize(InputParameterNum);
    for (UINT32 ix = 0; ix < InputParameterNum; ++ix)
    {
        D3D12_SIGNATURE_PARAMETER_DESC SignatureParameterDesc;
        Reflection->GetInputParameterDesc(ix, &SignatureParameterDesc);

        SemanticNames[ix] = (SignatureParameterDesc.SemanticName);
        InputElementDescs[ix] = D3D12_INPUT_ELEMENT_DESC{
            SemanticNames[ix].c_str(),
            SignatureParameterDesc.SemanticIndex,
            ConvertMaskToFormat(SignatureParameterDesc),
            0u,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0u
        };
    }
    return D3D12_INPUT_LAYOUT_DESC{ InputElementDescs.data(), static_cast<UINT32>(InputElementDescs.size())};
}

DXGI_FORMAT D3D12ShaderCompiler::ConvertMaskToFormat(const D3D12_SIGNATURE_PARAMETER_DESC& InSignatureParameterDesc)
{
    if (InSignatureParameterDesc.Mask == 1)
    {
        if (InSignatureParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)	    return DXGI_FORMAT_R32_UINT;
        else if (InSignatureParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)   return DXGI_FORMAT_R32_SINT;
        else if (InSignatureParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)  return DXGI_FORMAT_R32_FLOAT;
    }
    else if (InSignatureParameterDesc.Mask <= 3)
    {
        if (InSignatureParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)		return DXGI_FORMAT_R32G32_UINT;
        else if (InSignatureParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)   return DXGI_FORMAT_R32G32_SINT;
        else if (InSignatureParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)  return DXGI_FORMAT_R32G32_FLOAT;
    }
    else if (InSignatureParameterDesc.Mask <= 7)
    {
        if (InSignatureParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)		return DXGI_FORMAT_R32G32B32_UINT;
        else if (InSignatureParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)   return DXGI_FORMAT_R32G32B32_SINT;
        else if (InSignatureParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)  return DXGI_FORMAT_R32G32B32_FLOAT;
    }
    else if (InSignatureParameterDesc.Mask <= 15)
    {
        if (InSignatureParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)		return DXGI_FORMAT_R32G32B32A32_UINT;
        else if (InSignatureParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)   return DXGI_FORMAT_R32G32B32A32_SINT;
        else if (InSignatureParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)  return DXGI_FORMAT_R32G32B32A32_FLOAT;
    }
    return DXGI_FORMAT_UNKNOWN;
}




























