#pragma once

#include <vector>
#include <windows.h>

#include "../Utility/ImageLoader.h"
#include "../Utility/CommonMath.h"
#include "../Utility/AlignUtil.h"



struct Vertex
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT3 Tangent;
    DirectX::XMFLOAT2 UV;

    UINT32 MeshIndex;
};

struct Mesh
{
    UINT32 MaterialIndex;

    DirectX::XMFLOAT3 Center;
    DirectX::XMFLOAT3 Extent;
    DirectX::XMFLOAT4X4 WorldMatrix = Identity4x4Matrix();
};

struct Material
{
    UINT32 DiffuseIndexInHeap = INVALID_SIZE_32;
    UINT32 RoughnessMetallicIndexInHeap = INVALID_SIZE_32;
    UINT32 NormalIndexInHeap = INVALID_SIZE_32;
    UINT32 OcclusionIndexInHeap = INVALID_SIZE_32;
    UINT32 EmissiveIndexInHeap = INVALID_SIZE_32;

    float DiffuseFactor[4];
    float RoughnessFactor;
    float MetallicFactor;
    float OcclusionFactor;
    float EmissiveFactor;
    
    bool DoubleSided;
};

struct MeshData
{
    UINT32 MaterialIndex;

    std::vector<Vertex> Vertices;
    std::vector<UINT16> Indices;

    DirectX::BoundingBox Box;
};


struct MaterialData
{
    UINT32 DiffuseIndex = INVALID_SIZE_32;
    UINT32 RoughnessMetallicIndex = INVALID_SIZE_32;
    UINT32 NormalIndex = INVALID_SIZE_32;
    UINT32 OcclusionIndex = INVALID_SIZE_32;
    UINT32 EmissiveIndex = INVALID_SIZE_32;

    float DiffuseFactor[4];
    float RoughnessFactor;
    float MetallicFactor;
    float OcclusionFactor;
    float EmissiveFactor;
    
    bool DoubleSided;
};

struct ModelData
{
    bool CameraVisible = true;
    UINT32 MaterialNum;
    UINT32 MaterialOffset;
    UINT32 ImageNum;
    UINT32 ImageOffset;
    std::vector<MeshData> MeshData;
    DirectX::XMFLOAT4X4 WorldMatrix = Identity4x4Matrix();
};
