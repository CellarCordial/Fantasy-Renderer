#pragma once
#include <execution>

#include "D3D12Texture.h"

#include "../External/dxc/d3d12shader.h"
#include "../External/dxc/dxcapi.h"
#include "../Utility/FileUtil.h"

struct D3D12ShaderDesc
{
    std::string ShaderName;     // Need the file extension.
    std::string EntryPoint;
    ED3D12ShaderTarget Target;
};

struct D3D12ShaderData
{
    void SetByteCode(const void* InData, UINT64 InSize)
    {
        if (InData)
        {
            Data.resize(InSize);
            memcpy(Data.data(), InData, InSize);
        }
    }

    D3D12_SHADER_BYTECODE GetByteCode() const
    {
        if (Data.empty()) return D3D12_SHADER_BYTECODE{};
        return D3D12_SHADER_BYTECODE{ Data.data(), Data.size() };
    }

    bool Invalid() const { return Data.empty() || IncludeShaderFiles.empty(); }

    std::vector<UINT8> Data;
    std::vector<std::string> IncludeShaderFiles;
};

class D3D12ShaderCompiler
{
public:
    CLASS_NO_COPY(D3D12ShaderCompiler)

    D3D12ShaderCompiler();
    ~D3D12ShaderCompiler() noexcept;

public:
    D3D12ShaderData CompileShader(const D3D12ShaderDesc& InDesc);
    D3D12_INPUT_LAYOUT_DESC GetInputLayout(const D3D12ShaderData* InData);

private:
    static DXGI_FORMAT ConvertMaskToFormat(const D3D12_SIGNATURE_PARAMETER_DESC& InSignatureParameterDesc);
    static bool CheckCache(const char* InCachePath, const char* InShaderPath); 
    static void SaveToCache(const char* InCachePath, const D3D12ShaderData& InData);
    static D3D12ShaderData LoadFromCache(const char* InCachePath);

private:
    Microsoft::WRL::ComPtr<IDxcCompiler3> DxcCompiler;
    Microsoft::WRL::ComPtr<IDxcUtils> DxcUtils;
    std::vector<std::string> SemanticNames;
    std::vector<D3D12_INPUT_ELEMENT_DESC> InputElementDescs;
};


class CustomDxcIncludeHandler : public IDxcIncludeHandler
{
    friend class D3D12ShaderCompiler;
public:
    CLASS_NO_COPY(CustomDxcIncludeHandler)
    
    explicit CustomDxcIncludeHandler(IDxcUtils* InUtils) : IDxcIncludeHandler(), Utils(InUtils) {}
    virtual ~CustomDxcIncludeHandler() = default;
    HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override
    {
        Microsoft::WRL::ComPtr<IDxcBlobEncoding> ShaderSourceBlob;
        std::string FilePathToInclude = WStringToString(pFilename);

        std::transform(
            FilePathToInclude.begin(),
            FilePathToInclude.end(),
            FilePathToInclude.begin(),
            [](char& c)
            {
                if (c == '\\') c = '/';
                return c;
            }
        );
        
        if (IsFileExist(FilePathToInclude.c_str()))
        {
            ppIncludeSource = nullptr;
            return E_FAIL;
        }

        const auto Iterator = std::find_if(
            std::execution::par,
            IncludedFileNames.begin(),
            IncludedFileNames.end(),
            [&FilePathToInclude](const auto& InName)
            {
                return InName == FilePathToInclude;
            }
        );
        if (Iterator != IncludedFileNames.end())
        {
            // Return a blank blob if this file has been included before
            constexpr char nullStr[] = " ";
            Utils->CreateBlobFromPinned(nullStr, ARRAYSIZE(nullStr), DXC_CP_ACP, ShaderSourceBlob.GetAddressOf());
            *ppIncludeSource = ShaderSourceBlob.Detach();
            return S_OK;
        }

        HRESULT hr = Utils->LoadFile(pFilename, nullptr, ShaderSourceBlob.GetAddressOf());
        if (SUCCEEDED(hr))
        {
            IncludedFileNames.push_back(FilePathToInclude);
            *ppIncludeSource = ShaderSourceBlob.Detach();
        }
        else
        {
            ppIncludeSource = nullptr;
        }
        return hr;
    }
    
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override { return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE AddRef(void) override {	return 0; }
    ULONG STDMETHODCALLTYPE Release(void) override { return 0; }

private:
    std::vector<std::string> IncludedFileNames;
    IDxcUtils* Utils;
};
























