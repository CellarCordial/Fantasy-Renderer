#pragma once

#include <windows.h>

static constexpr UINT8 SWAP_CHAIN_BUFFER_COUNT = 3;
static constexpr DXGI_FORMAT SWAP_CHAIN_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
static constexpr DXGI_FORMAT SWAP_CHAIN_DEPTH_STENCIL_FORMAT = DXGI_FORMAT_D24_UNORM_S8_UINT;


static constexpr UINT64 TEXTURE_MIN_SIZE = 4ull * 1024 * 1024;
static constexpr UINT64 TEXTURE_MAX_SIZE = 4ull * 2048 * 2048;
static constexpr UINT64 HEAP_DEFAULT_SIZE = 512ull * 1024 * 1024;

static constexpr UINT32 PRESERVED_GPU_DESCRIPTOR_NUM = 1;	// For imgui
static constexpr UINT32 GPU_DESCRIPTOR_NUM = 1 << 15;	
static constexpr UINT32 CPU_DESCRIPTOR_NUM = 1 << 9;	

static constexpr char ShaderCacheRootDirectory[] = "Asset/ShaderCache/";
static constexpr char ShaderRootDirectory[] = "Shaders/";

static constexpr UINT32 RenderThreadNum = 3;

