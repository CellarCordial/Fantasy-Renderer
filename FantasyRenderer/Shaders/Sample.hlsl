#ifndef _SAMPLE_
#define _SAMPLE_


SamplerState LinearWrapSampler : register(s0);
SamplerState LinearClampSampler : register(s1);
SamplerState LinearBorderSampler : register(s2);

SamplerState PointWrapSampler : register(s3);
SamplerState PointClampSampler : register(s4);
SamplerState PointBorderSampler : register(s5);

struct Mesh
{
    uint MaterialIndex;
    float3 Center;
    float3 Extent;
    float4x4 WorldMatrix;
};

struct Material
{
    uint DiffuseIndexInHeap;
    uint RoughnessMetallicIndexInHeap;
    uint NormalIndexInHeap;
    uint OcclusionIndexInHeap;
    uint EmissiveIndexInHeap;

    float4 DiffuseFactor;
    float RoughnessFactor;
    float MetallicFactor;
    float OcclusionFactor;
    float EmissiveFactor;
    
    bool DoubleSided;
};

struct CameraConstants
{
    float4x4 View;
    float4x4 Proj;
    float4x4 ViewProj;
};

struct PassConstants
{
    uint MeshIndexInHeap;
    uint MaterialIndexInHeap;
};

ConstantBuffer<CameraConstants> CameraData : register(b0);
ConstantBuffer<PassConstants> PassData : register(b3);

struct VertexInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float2 UV : TEXCOORD;

    uint MeshIndex : MESH_INDEX;
};

struct VertexOutput
{
    float4 PositionH : SV_POSITION;
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float2 UV : TEXCOORD;

    uint MeshIndex : MESH_INDEX;
};


typedef VertexOutput PixelInput;


VertexOutput VertexShader(VertexInput Input)
{
    VertexOutput Output;
    StructuredBuffer<Mesh> MeshData = ResourceDescriptorHeap[PassData.MeshIndexInHeap];

    float4x4 WorldMatrix = MeshData[Input.MeshIndex].WorldMatrix;
    float4 PositionW = mul(float4(Input.Position, 1.0f), WorldMatrix);
    
    Output.PositionH = mul(PositionW, CameraData.ViewProj);;
    Output.Position = PositionW.xyz;
    Output.Normal = mul(Input.Normal, (float3x3)WorldMatrix);
    Output.Tangent = mul(Input.Tangent, (float3x3)WorldMatrix);
    Output.UV = Input.UV;
    Output.MeshIndex = Input.MeshIndex;
    
    return Output;
}

float4 PixelShader(PixelInput Input) : SV_TARGET
{
    float4 Color;

    StructuredBuffer<Material> MaterialData = ResourceDescriptorHeap[PassData.MaterialIndexInHeap];
    StructuredBuffer<Mesh> MeshData = ResourceDescriptorHeap[PassData.MeshIndexInHeap];

    Material Mat = MaterialData[MeshData[Input.MeshIndex].MaterialIndex];
    Texture2D Diffuse = ResourceDescriptorHeap[Mat.DiffuseIndexInHeap];
    Color = Diffuse.Sample(LinearWrapSampler, Input.UV) * Mat.DiffuseFactor;

    return Color;
}










#endif