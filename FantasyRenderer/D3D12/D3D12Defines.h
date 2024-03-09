#pragma once

#include <optional>
#include <windows.h>
#include <wrl.h>
#include <algorithm>
#include <mutex>
#include <fstream>
#include <span>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <initguid.h>

#include <DirectXCollision.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3dcommon.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include "DirectXColors.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "dxguid.lib")

#include <variant>

#include "../Utility/Exception.h"
#include "../Utility/Macros.h"
#include "../External/d3dx12/d3dx12.h"
#include "../Defines.h"
class D3D12Device;


template<typename Enum>
requires std::is_enum_v<Enum>
constexpr bool HasFlag(Enum value, Enum flags)
{
	using T = std::underlying_type_t<Enum>;
	return (((T)value) & (T)flags) == ((T)flags);
}


enum class ED3D12DescriptorType : UINT8
{
    CBV_SRV_UAV,
    RTV,
    DSV,
    Num
};

constexpr D3D12_DESCRIPTOR_HEAP_TYPE ConvertToD3D12DescriptorHeapType(ED3D12DescriptorType InType)
{
    switch (InType)
    {
    case ED3D12DescriptorType::CBV_SRV_UAV: return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    case ED3D12DescriptorType::RTV: return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    case ED3D12DescriptorType::DSV: return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    default:
    	return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
    }
}

enum class ED3D12ResourceLocationType : UINT8
{
    Invalid,
    Texture,
    ConstantBuffer,
    DefaultBuffer,
    UploadBuffer,
};

enum class ED3D12BufferFlag : UINT32
{
    None,
	Structured,
	RWStructured
};

enum class ED3D12BufferType : UINT8
{
    Default,
    Upload 
};

inline D3D12_RESOURCE_FLAGS ConvertToD3D12ResourceFlags(ED3D12BufferFlag InFlag)
{
	switch (InFlag)
	{
	case ED3D12BufferFlag::None:
	default:
		return D3D12_RESOURCE_FLAG_NONE;
	}
}

enum class ED3D12TextureFlag : UINT32
{
	None					= 0,
	AllowRenderTarget		= 1,
	AllowDepthStencil		= 1 << 1
};

inline D3D12_RESOURCE_FLAGS ConvertToD3D12ResourceFlags(ED3D12TextureFlag InFlag)
{
	switch (InFlag)
	{
	case ED3D12TextureFlag::AllowRenderTarget: return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	case ED3D12TextureFlag::AllowDepthStencil: return D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	case ED3D12TextureFlag::None:
	default:
		return D3D12_RESOURCE_FLAG_NONE;
	}
}

enum class ED3D12ResourceState : UINT32
{
	Common,
	CopySrc,
	CopyDst,
	DepthRead,
	DepthWrite,
	GenericRead,
	PixelShader,
	NonPixelShader,
	AllShader,
	VertexBuffer,
	ConstantBuffer,
	IndexBuffer,
	Present,
	RenderTarget,
	UnorderedAccess
};

constexpr D3D12_RESOURCE_STATES ConvertToD3D12ResourceStates(ED3D12ResourceState InType)
{
	switch (InType)
	{
	case ED3D12ResourceState::CopySrc: return D3D12_RESOURCE_STATE_COPY_SOURCE;
	case ED3D12ResourceState::CopyDst: return D3D12_RESOURCE_STATE_COPY_DEST;
	case ED3D12ResourceState::DepthRead: return D3D12_RESOURCE_STATE_DEPTH_READ;
	case ED3D12ResourceState::DepthWrite: return D3D12_RESOURCE_STATE_DEPTH_WRITE;
	case ED3D12ResourceState::GenericRead: return D3D12_RESOURCE_STATE_GENERIC_READ;
	case ED3D12ResourceState::PixelShader: return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	case ED3D12ResourceState::NonPixelShader: return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	case ED3D12ResourceState::AllShader: return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	case ED3D12ResourceState::IndexBuffer: return D3D12_RESOURCE_STATE_INDEX_BUFFER;
	case ED3D12ResourceState::Present: return D3D12_RESOURCE_STATE_PRESENT;
	case ED3D12ResourceState::RenderTarget: return D3D12_RESOURCE_STATE_RENDER_TARGET;
	case ED3D12ResourceState::UnorderedAccess: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	case ED3D12ResourceState::VertexBuffer:
	case ED3D12ResourceState::ConstantBuffer:
		return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	case ED3D12ResourceState::Common:
	default:
		return D3D12_RESOURCE_STATE_COMMON;
	}	
}

constexpr ED3D12ResourceLocationType ConvertToED3D12ResourceLocationType(ED3D12BufferType InType)
{
	switch (InType)
	{
	case ED3D12BufferType::Default: return ED3D12ResourceLocationType::DefaultBuffer;
	case ED3D12BufferType::Upload: return ED3D12ResourceLocationType::UploadBuffer;
	default:
		return ED3D12ResourceLocationType::Invalid;
	}
}

enum class ED3D12ShaderTarget : UINT8
{
	Invalid,
	Vertex,
	Hull,
	Domain,
	Geometry,
	Pixel,
	Compute,
};

constexpr LPCWSTR GetTargetProfile(ED3D12ShaderTarget InTarget)
{
	switch (InTarget)
	{
	case ED3D12ShaderTarget::Vertex:
		return L"vs_6_6";
	case ED3D12ShaderTarget::Hull:
		return L"hs_6_6";
	case ED3D12ShaderTarget::Domain:
		return L"ds_6_6";
	case ED3D12ShaderTarget::Geometry:
		return L"gs_6_6";
	case ED3D12ShaderTarget::Pixel:
		return L"ps_6_6";
	case ED3D12ShaderTarget::Compute:
		return L"cs_6_6";
	default:
		assert(false && "There is no such shder target.");
		return L"";
	}
}

enum class ED3D12PipelineStateType : UINT8
{
	Invalid,
	Graphics,
	Compute
};

enum class ED3D12CommandType : UINT8
{
	Graphics,
	Compute
};

constexpr D3D12_COMMAND_LIST_TYPE ConvertToCommandListType(ED3D12CommandType InType)
{
	switch (InType)
	{
	case ED3D12CommandType::Compute: return D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;
	case ED3D12CommandType::Graphics:
	default:
		return D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;
	}
}

enum class ED3D12AccessType : UINT32
{
	InValid_InValid,
	Clear_Preserve,
	Preserve_Preserve,
	NoAccess_NoAccess
};

constexpr void SplitD3D12AccessType(ED3D12AccessType InType, D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE* OutBeginningType, D3D12_RENDER_PASS_ENDING_ACCESS_TYPE* OutEndingType)
{
	switch (InType)
	{
	case ED3D12AccessType::Clear_Preserve:
		*OutBeginningType = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
		*OutEndingType = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
		break;
	case ED3D12AccessType::Preserve_Preserve:
		*OutBeginningType = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
		*OutEndingType = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
		break;
	default:
		*OutBeginningType = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
		*OutEndingType = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
	}
}

enum class ED3D12RenderPassFlags : UINT8
{
	None,
	UseLegacy,
	AllowUAVWrite,
	DepthReadOnly,
	StencilReadOnly
};

constexpr D3D12_RENDER_PASS_FLAGS ConvertToD3D12RenderPassFlags(ED3D12RenderPassFlags InFlag)
{
	switch (InFlag)
	{
	case ED3D12RenderPassFlags::AllowUAVWrite: return D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES;
	case ED3D12RenderPassFlags::DepthReadOnly: return D3D12_RENDER_PASS_FLAG_BIND_READ_ONLY_DEPTH;
	case ED3D12RenderPassFlags::StencilReadOnly: return D3D12_RENDER_PASS_FLAG_BIND_READ_ONLY_STENCIL;
	case ED3D12RenderPassFlags::UseLegacy:
	case ED3D12RenderPassFlags::None:
	default:
		return D3D12_RENDER_PASS_FLAG_NONE;
	}
}

constexpr UINT32 GetDXGIPixelSize(DXGI_FORMAT InFormat)
{
	
    switch (InFormat)
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		return 16u;
    case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		return 12u;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
		return 8u;
    case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		return 8u;
    case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		return 4u;
    case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
		return 2u;
    case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
		return 1u;
    default:
    	return 0u;
    }
}

inline bool IsDXGIFormatCompatible(DXGI_FORMAT Format1, DXGI_FORMAT Format2)
{

	return false;
}



enum class ED3D12ClearValueType : UINT8
{
	Invalid,
	Color,
	DepthStencil
};

class D3D12ClearValue
{
public:
	CLASS_DEFAULT_COPY(D3D12ClearValue)
	
	D3D12ClearValue() : Type(ED3D12ClearValueType::Invalid), ClearValue() {}
	D3D12ClearValue(ED3D12ClearValueType InType, DXGI_FORMAT InFormat = DXGI_FORMAT_UNKNOWN) : Type(InType)
	{
		ClearValue.Format = InFormat;
		if (Type == ED3D12ClearValueType::Color)
		{
			std::ranges::fill(std::begin(ClearValue.Color), std::end(ClearValue.Color), 0.0f);
		}
		else if (Type == ED3D12ClearValueType::DepthStencil)
		{
			ClearValue.DepthStencil = { 1.0f, 0u };
		}
	}

	D3D12ClearValue(float R, float G, float B, float A, DXGI_FORMAT InFormat = DXGI_FORMAT_UNKNOWN) : Type(ED3D12ClearValueType::Color)
	{
		ClearValue.Format = InFormat;
		ClearValue.Color[0] = R;
		ClearValue.Color[1] = G;
		ClearValue.Color[2] = B;
		ClearValue.Color[3] = A;
	}

	D3D12ClearValue(std::span<const float> InValue, DXGI_FORMAT InFormat = DXGI_FORMAT_UNKNOWN)
		: D3D12ClearValue(InValue[0], InValue[1], InValue[2], InValue[3], InFormat)
	{
	}

	D3D12ClearValue(float Depth, UINT8 Stencil, DXGI_FORMAT InFormat = DXGI_FORMAT_UNKNOWN) : Type(ED3D12ClearValueType::DepthStencil)
	{
		ClearValue.DepthStencil = { Depth, Stencil };
	}

	D3D12ClearValue(D3D12_DEPTH_STENCIL_VALUE InValue, DXGI_FORMAT InFormat = DXGI_FORMAT_UNKNOWN)
		: D3D12ClearValue(InValue.Depth, InValue.Stencil, InFormat)
	{
	}

	~D3D12ClearValue() = default;

	D3D12ClearValue& operator=(std::span<const float> InValue)
	{
		D3D12ClearValue Temp(InValue);
		ClearValue = Temp.ClearValue;
		return *this;
	}

	D3D12ClearValue& operator=(const D3D12_DEPTH_STENCIL_VALUE InValue)
	{
		D3D12ClearValue Temp(InValue);
		ClearValue = Temp.ClearValue;
		return *this;
	}

	D3D12_CLEAR_VALUE* GetNative()
	{
		if (!IsValid()) return nullptr;
		return &ClearValue;
	}

	void SetFormat(DXGI_FORMAT InFormat)
	{
		ClearValue.Format = InFormat;
	}

	bool IsValid() const
	{
		if (Type == ED3D12ClearValueType::Invalid) return false;
		return true;
	}
	

private:
	ED3D12ClearValueType Type;
	D3D12_CLEAR_VALUE ClearValue;
};





