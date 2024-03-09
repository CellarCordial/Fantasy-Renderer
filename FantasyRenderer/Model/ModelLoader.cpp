#include "ModelLoader.h"

#define TINYGLTF_NOEXCEPTION
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../External/tinygltf/tiny_gltf.h"


UINT32 ModelLoader::LoadGLTF(const ModelLoadDesc& InModelDesc)
{
    tinygltf::TinyGLTF GLTFLoader;
    tinygltf::Model GLTFModel;
    std::string Error;
    std::string Warn;
    ThrowIfFalse(GLTFLoader.LoadASCIIFromFile(&GLTFModel, &Error, &Warn, InModelDesc.ModelFilePath));
    ThrowIfFalse(Error.empty() && Warn.empty(), Error + Warn);

    auto& Model = Models.emplace_back();

    Model.ImageOffset = static_cast<UINT32>(Images.size());
    Model.MaterialOffset = static_cast<UINT32>(Materials.size());
    Model.MaterialNum = static_cast<UINT32>(GLTFModel.materials.size());
    for (const auto& GLTFMaterial : GLTFModel.materials)
    {
        auto& CurrentMaterialData = Materials.emplace_back();

        CurrentMaterialData.DoubleSided = GLTFMaterial.doubleSided;

        CurrentMaterialData.DiffuseFactor[0] = static_cast<float>(GLTFMaterial.pbrMetallicRoughness.baseColorFactor[0]);
        CurrentMaterialData.DiffuseFactor[1] = static_cast<float>(GLTFMaterial.pbrMetallicRoughness.baseColorFactor[1]);
        CurrentMaterialData.DiffuseFactor[2] = static_cast<float>(GLTFMaterial.pbrMetallicRoughness.baseColorFactor[2]);
        CurrentMaterialData.DiffuseFactor[3] = static_cast<float>(GLTFMaterial.pbrMetallicRoughness.baseColorFactor[3]);
        CurrentMaterialData.RoughnessFactor = static_cast<float>(GLTFMaterial.pbrMetallicRoughness.roughnessFactor);
        CurrentMaterialData.MetallicFactor = static_cast<float>(GLTFMaterial.pbrMetallicRoughness.metallicFactor);
        CurrentMaterialData.OcclusionFactor = static_cast<float>(GLTFMaterial.occlusionTexture.strength);
        CurrentMaterialData.EmissiveFactor = static_cast<float>(GLTFMaterial.emissiveFactor[0]);

        
        if (GLTFMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0)
        {
            const auto& GLTFTexture = GLTFModel.textures[GLTFMaterial.pbrMetallicRoughness.baseColorTexture.index];
            const auto& GLTFImage = GLTFModel.images[GLTFTexture.source];
            
            CurrentMaterialData.DiffuseIndex = static_cast<UINT32>(Images.size());

            std::string ImagePath = InModelDesc.TextureFilePath + GLTFImage.uri;
            auto& ImageData = Images.emplace_back();
            ImageLoader::LoadBitMapFromFile(ImagePath.c_str(), &ImageData);
            Model.ImageNum++;
        }
        if (GLTFMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
        {
            const auto& GLTFTexture = GLTFModel.textures[GLTFMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index];
            const auto& GLTFImage = GLTFModel.images[GLTFTexture.source];

            CurrentMaterialData.RoughnessMetallicIndex = static_cast<UINT32>(Images.size());

            std::string ImagePath = InModelDesc.TextureFilePath + GLTFImage.uri;
            auto& ImageData = Images.emplace_back();
            ImageLoader::LoadBitMapFromFile(ImagePath.c_str(), &ImageData);
            Model.ImageNum++;
        }
        if (GLTFMaterial.normalTexture.index >= 0)
        {
            const auto& GLTFTexture = GLTFModel.textures[GLTFMaterial.normalTexture.index];
            const auto& GLTFImage = GLTFModel.images[GLTFTexture.source];
            
            CurrentMaterialData.NormalIndex = static_cast<UINT32>(Images.size());

            std::string ImagePath = InModelDesc.TextureFilePath + GLTFImage.uri;
            auto& ImageData = Images.emplace_back();
            ImageLoader::LoadBitMapFromFile(ImagePath.c_str(), &ImageData);
            Model.ImageNum++;
        }
        if (GLTFMaterial.occlusionTexture.index >= 0)
        {
            const auto& GLTFTexture = GLTFModel.textures[GLTFMaterial.occlusionTexture.index];
            const auto& GLTFImage = GLTFModel.images[GLTFTexture.source];
            
            CurrentMaterialData.OcclusionIndex = static_cast<UINT32>(Images.size());

            std::string ImagePath = InModelDesc.TextureFilePath + GLTFImage.uri;
            auto& ImageData = Images.emplace_back();
            ImageLoader::LoadBitMapFromFile(ImagePath.c_str(), &ImageData);
            Model.ImageNum++;
        }
        if (GLTFMaterial.emissiveTexture.index >= 0)
        {
            const auto& GLTFTexture = GLTFModel.textures[GLTFMaterial.emissiveTexture.index];
            const auto& GLTFImage = GLTFModel.images[GLTFTexture.source];
            
            CurrentMaterialData.EmissiveIndex = static_cast<UINT32>(Images.size());

            std::string ImagePath = InModelDesc.TextureFilePath + GLTFImage.uri;
            auto& ImageData = Images.emplace_back();
            ImageLoader::LoadBitMapFromFile(ImagePath.c_str(), &ImageData);
            Model.ImageNum++;
        }
    }

    const auto& GLTFScene = GLTFModel.scenes[GLTFModel.defaultScene];
    for (UINT32 ix = 0; ix < GLTFScene.nodes.size(); ++ix)
    {
        DirectX::XMMATRIX ParentMatrix = DirectX::XMLoadFloat4x4(&InModelDesc.WorldMatrix);
        LoadGLTFNode(GLTFModel, GLTFModel.nodes[GLTFScene.nodes[ix]], ParentMatrix);
    }

    return static_cast<UINT32>(Models.size()) - 1;
}

void ModelLoader::LoadGLTFNode(const tinygltf::Model& InGLTFModel, const tinygltf::Node& InGLTFNode, const DirectX::XMMATRIX& InParentMatrix)
{
    WorldTransform Transform;
    
    if (!InGLTFNode.matrix.empty())
    {
        Transform.WorldMatirx._11 = static_cast<float>(InGLTFNode.matrix[0]);
        Transform.WorldMatirx._12 = static_cast<float>(InGLTFNode.matrix[1]);
        Transform.WorldMatirx._13 = static_cast<float>(InGLTFNode.matrix[2]);
        Transform.WorldMatirx._14 = static_cast<float>(InGLTFNode.matrix[3]);
        Transform.WorldMatirx._21 = static_cast<float>(InGLTFNode.matrix[4]);
        Transform.WorldMatirx._22 = static_cast<float>(InGLTFNode.matrix[5]);
        Transform.WorldMatirx._23 = static_cast<float>(InGLTFNode.matrix[6]);
        Transform.WorldMatirx._24 = static_cast<float>(InGLTFNode.matrix[7]);
        Transform.WorldMatirx._31 = static_cast<float>(InGLTFNode.matrix[8]);
        Transform.WorldMatirx._32 = static_cast<float>(InGLTFNode.matrix[9]);
        Transform.WorldMatirx._33 = static_cast<float>(InGLTFNode.matrix[10]);
        Transform.WorldMatirx._34 = static_cast<float>(InGLTFNode.matrix[11]);
        Transform.WorldMatirx._41 = static_cast<float>(InGLTFNode.matrix[12]);
        Transform.WorldMatirx._42 = static_cast<float>(InGLTFNode.matrix[13]);
        Transform.WorldMatirx._43 = static_cast<float>(InGLTFNode.matrix[14]);
        Transform.WorldMatirx._44 = static_cast<float>(InGLTFNode.matrix[15]);
    }
    else
    {
        if (!InGLTFNode.translation.empty()) Transform.TranslationVector = { (float)InGLTFNode.translation[0], (float)InGLTFNode.translation[1], (float)InGLTFNode.translation[2] };
        if (!InGLTFNode.scale.empty()) Transform.ScaleVector = { (float)InGLTFNode.scale[0], (float)InGLTFNode.scale[1], (float)InGLTFNode.scale[2] };
        if (!InGLTFNode.rotation.empty()) Transform.RotationVector = { (float)InGLTFNode.rotation[0], (float)InGLTFNode.rotation[1], (float)InGLTFNode.rotation[2], (float)InGLTFNode.rotation[3] };

        Transform.CreateWorldMatrix();
    }

    auto MeshWorldMatrix = DirectX::XMLoadFloat4x4(&Transform.WorldMatirx) * InParentMatrix;

    auto& Model = Models.back();
    DirectX::XMStoreFloat4x4(&Model.WorldMatrix, MeshWorldMatrix);
    
    if (InGLTFNode.mesh >= 0)
    {
        const auto& GLTFMesh = InGLTFModel.meshes[InGLTFNode.mesh];
        for (const auto& GLTFPrimitives : GLTFMesh.primitives)
        {
            auto& CurrentMeshData = Model.MeshData.emplace_back();

            // Index
            
            const auto& GLTFIndicesAccessor = InGLTFModel.accessors[GLTFPrimitives.indices];
            const auto& GLTFIndicesBufferView = InGLTFModel.bufferViews[GLTFIndicesAccessor.bufferView];
            const auto& GLTFIndicesBuffer = InGLTFModel.buffers[GLTFIndicesBufferView.buffer];

            CurrentMeshData.Indices.reserve(GLTFIndicesAccessor.count);
            
            auto AddIndices = [&]<typename T>()
                {
                    const T* IndexData = reinterpret_cast<const T*>(GLTFIndicesBuffer.data.data() + GLTFIndicesBufferView.byteOffset + GLTFIndicesAccessor.byteOffset);
                    for (UINT64 ix = 0; ix < GLTFIndicesAccessor.count; ix += 3)
                    {
                        // 默认为逆时针旋转
                        CurrentMeshData.Indices.push_back(IndexData[ix + 0]);
                        CurrentMeshData.Indices.push_back(IndexData[ix + 1]);
                        CurrentMeshData.Indices.push_back(IndexData[ix + 2]);
                    }
                };

            const UINT32 IndexStride = GLTFIndicesAccessor.ByteStride(GLTFIndicesBufferView);
            switch (IndexStride)
            {
            case 1: AddIndices.operator()<UINT8>(); break;
            case 2: AddIndices.operator()<UINT16>(); break;
            case 4: AddIndices.operator()<UINT32>(); break;
            default:
                ThrowIfFalse(IndexStride == 2, "Doesn't support such stride.");
            }

            
            // Vertex
            
            UINT32 TempCounter = 0;
            UINT64 AttributeSize = 0;
            UINT32 AttributeStride[4] = { 0 };
            auto FunctionLoadAttribute = [&](const std::string& InAttributeName)
            {
                const auto Iterator = GLTFPrimitives.attributes.find(InAttributeName);
                ThrowIfFalse(Iterator != GLTFPrimitives.attributes.end(), "No such attribute name.");
                
                const auto& GLTFAttributeAccessor = InGLTFModel.accessors[Iterator->second];
                const auto& GLTFAttributeBufferView = InGLTFModel.bufferViews[GLTFAttributeAccessor.bufferView];
                const auto& GLTFAttributeBuffer = InGLTFModel.buffers[GLTFAttributeBufferView.buffer];

                if (AttributeSize != 0) ThrowIfFalse(AttributeSize == GLTFAttributeAccessor.count, "Different attribute size.");
                AttributeSize = GLTFAttributeAccessor.count;
                AttributeStride[TempCounter++] = GLTFAttributeAccessor.ByteStride(GLTFAttributeBufferView);
                ThrowIfFalse(TempCounter < 5, "Model's attribute is too more"); 

                return reinterpret_cast<size_t>(GLTFAttributeBuffer.data.data() + GLTFAttributeBufferView.byteOffset + GLTFAttributeAccessor.byteOffset);
            };

            const size_t PositionData = FunctionLoadAttribute("POSITION");
            const size_t NormalData = FunctionLoadAttribute("NORMAL");
            const size_t UVData = FunctionLoadAttribute("TEXCOORD_0");
            //const size_t TangentData = FunctionLoadAttribute("TANGENT");

            std::vector<DirectX::XMFLOAT3> PositionForAABB(AttributeSize);
            
            CurrentMeshData.Vertices.resize(AttributeSize);
            for (UINT32 ix = 0; ix < AttributeSize; ++ix)
            {
                CurrentMeshData.Vertices[ix].Position = *reinterpret_cast<DirectX::XMFLOAT3*>((PositionData) + ix * AttributeStride[0]);
                CurrentMeshData.Vertices[ix].Normal = *reinterpret_cast<DirectX::XMFLOAT3*>(NormalData + ix * AttributeStride[1]);
                CurrentMeshData.Vertices[ix].UV = *reinterpret_cast<DirectX::XMFLOAT2*>(UVData + ix * AttributeStride[2]);
                //CurrentMeshData.Vertices[ix].Tangent = *reinterpret_cast<DirectX::XMFLOAT3*>(TangentData + ix * AttributeStride[3]);
                CurrentMeshData.Vertices[ix].MeshIndex = static_cast<UINT32>(Model.MeshData.size() - 1);

                PositionForAABB[ix] = CurrentMeshData.Vertices[ix].Position;
            }
            
            CurrentMeshData.MaterialIndex = GLTFPrimitives.material;
            CurrentMeshData.Box = CreateAABB(PositionForAABB);
        }   
    }

    for (const auto ChildNodeIndex : InGLTFNode.children)
    {
        LoadGLTFNode(InGLTFModel, InGLTFModel.nodes[ChildNodeIndex], MeshWorldMatrix);
    }

}

DirectX::BoundingBox ModelLoader::CreateAABB(const std::vector<DirectX::XMFLOAT3>& InPosition)
{
    const auto XMinMax = std::minmax_element(
        InPosition.begin(),
        InPosition.end(),
        [](const auto& lhs, const auto& rhs)
        {
            return lhs.x < rhs.x;    
        }
    );
    const auto YMinMax = std::minmax_element(
        InPosition.begin(),
        InPosition.end(),
        [](const auto& lhs, const auto& rhs)
        {
            return lhs.y < rhs.y;    
        }
    );
    const auto ZMinMax = std::minmax_element(
        InPosition.begin(),
        InPosition.end(),
        [](const auto& lhs, const auto& rhs)
        {
            return lhs.z < rhs.z;    
        }
    );
    
    const DirectX::XMFLOAT3 Center(
        (XMinMax.first->x + XMinMax.second->x) / 2.0f,
        (YMinMax.first->y + YMinMax.second->y) / 2.0f,
        (ZMinMax.first->z + ZMinMax.second->z) / 2.0f
    );
    
    // Max - Center相当于(Min + Max) / 2
    const DirectX::XMFLOAT3 Extents(
        XMinMax.second->x - Center.x,
        YMinMax.second->y - Center.y,
        ZMinMax.second->z - Center.z
    );    

    return DirectX::BoundingBox{ Center, Extents };
}
