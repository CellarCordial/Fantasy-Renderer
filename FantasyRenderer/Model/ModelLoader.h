#pragma once

#include "ModelDefines.h"
#include "../External/tinygltf/tiny_gltf.h"


struct ModelLoadDesc
{
    ModelLoadDesc() : WorldMatrix(Identity4x4Matrix()) {}
    
    std::string ModelFilePath; 
    std::string TextureFilePath;
    DirectX::XMFLOAT4X4 WorldMatrix;
};


class ModelLoader
{
    struct WorldTransform
    {
        DirectX::XMFLOAT3 TranslationVector = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT3 ScaleVector = { 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT4 RotationVector = { 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT4X4 WorldMatirx;

        void CreateWorldMatrix()
        {
            DirectX::XMStoreFloat4x4(
                &WorldMatirx,
                DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&TranslationVector)) *
                DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3(&ScaleVector)) *
                DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&RotationVector))
            );
        }
    };
    
public:
    CLASS_NO_COPY(ModelLoader)
    
    ModelLoader() = default;
    ~ModelLoader() = default;

public:
    UINT32 LoadGLTF(const ModelLoadDesc& InModelDesc);

    UINT32 GetModelNum() const { return static_cast<UINT32>(Models.size()); }
    ModelData* GetModelData(UINT32 InModelIndex) { return &Models[InModelIndex]; }

    ImageData* GetImageData(UINT32 InImageIndex) { return &Images[InImageIndex]; }
    MaterialData* GetMaterial(UINT32 InMaterialIndex) { return &Materials[InMaterialIndex]; }

private:
    void LoadGLTFNode(const tinygltf::Model& InGLTFModel, const tinygltf::Node& InGLTFNode, const DirectX::XMMATRIX& InParentMatrix);
    static DirectX::BoundingBox CreateAABB(const std::vector<DirectX::XMFLOAT3>& InPosition);
private:
    ImageLoader ImagesLoader;
    std::vector<ModelData> Models;

    std::vector<MaterialData> Materials;
    std::vector<ImageData> Images;
};
