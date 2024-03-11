#include "SamplePass.h"

void SamplePass::Init(RenderGraph* InRenderGraph, ModelLoader* InModelLoader, LightManager* InLightManager)
{
    RenderGraphImpl = InRenderGraph;
    ModelLoaderImpl = InModelLoader;
    LightManagerImpl = InLightManager;
}

void SamplePass::Setup(const ModelLoadDesc& InModelLoadDesc, D3D12CommandList* InCommmandList)
{
    D3D12Device* Device = RenderGraphImpl->GetDevice();

    // Vertex and Index
    InCommmandList->Begin();
    
    const UINT32 ModelIndex = ModelLoaderImpl->LoadGLTF(InModelLoadDesc);
    ModelData* Model = ModelLoaderImpl->GetModelData(ModelIndex);

    std::vector<Mesh> Meshes;
    D3D12BufferDesc VertexBufferDesc{};
    D3D12BufferDesc IndexBufferDesc{};
    VertexBuffer.resize(Model->MeshData.size());
    IndexBuffer.resize(Model->MeshData.size());
    Meshes.resize(Model->MeshData.size());
    for (UINT32 ix = 0; ix < Model->MeshData.size(); ++ix)
    {
        VertexBufferDesc.Size = Model->MeshData[ix].Vertices.size() * sizeof(Vertex);
        VertexBufferDesc.State = ED3D12ResourceState::VertexBuffer;

        VertexBuffer[ix] = std::make_unique<D3D12Buffer>(Device, VertexBufferDesc);
        VertexBuffer[ix]->UploadData(InCommmandList, Model->MeshData[ix].Vertices.data());
    	
        IndexBufferDesc.Size = Model->MeshData[ix].Indices.size() * sizeof(UINT16);
        IndexBufferDesc.State = ED3D12ResourceState::IndexBuffer;

        IndexBuffer[ix] = std::make_unique<D3D12Buffer>(Device, IndexBufferDesc);
        IndexBuffer[ix]->UploadData(InCommmandList, Model->MeshData[ix].Indices.data());
        
        Meshes[ix] = Mesh{ Model->MeshData[ix].MaterialIndex, Model->MeshData[ix].Box.Center, Model->MeshData[ix].Box.Extents, Model->WorldMatrix };
    }
    
    Materials.resize(Model->MaterialNum);
    MaterialData* MaterialData = ModelLoaderImpl->GetMaterial(Model->MaterialOffset);
    for (UINT32 ix = 0; ix < Model->MaterialNum; ++ix)
    {
        Materials[ix].DiffuseFactor[0] = MaterialData[ix].DiffuseFactor[0];
        Materials[ix].DiffuseFactor[1] = MaterialData[ix].DiffuseFactor[1];
        Materials[ix].DiffuseFactor[2] = MaterialData[ix].DiffuseFactor[2];
        Materials[ix].DiffuseFactor[3] = MaterialData[ix].DiffuseFactor[3];
        Materials[ix].MetallicFactor = MaterialData[ix].MetallicFactor;
        Materials[ix].RoughnessFactor = MaterialData[ix].RoughnessFactor;
        Materials[ix].OcclusionFactor = MaterialData[ix].OcclusionFactor;
        Materials[ix].EmissiveFactor = MaterialData[ix].EmissiveFactor;
    }

    D3D12BufferDesc MeshBufferDesc;
    MeshBufferDesc.Flag = ED3D12BufferFlag::Structured;
    MeshBufferDesc.Size = Model->MeshData.size() * sizeof(Mesh);
    MeshBufferDesc.State = ED3D12ResourceState::AllShader;
    MeshBufferDesc.Stride = sizeof(Mesh);

    MeshBuffer = std::make_unique<D3D12Buffer>(Device, MeshBufferDesc);
    MeshBuffer->UploadData(InCommmandList, Meshes.data());

            
    D3D12BufferDesc MaterialBufferDesc;
    MaterialBufferDesc.Flag = ED3D12BufferFlag::Structured;
    MaterialBufferDesc.Size = Model->MaterialNum * sizeof(Material);
    MaterialBufferDesc.State = ED3D12ResourceState::GenericRead;
    MaterialBufferDesc.Stride = sizeof(Material);
    MaterialBufferDesc.Type = ED3D12BufferType::Upload;

    MaterialBuffer = std::make_unique<D3D12Buffer>(Device, MaterialBufferDesc);

    InCommmandList->Close();
    D3D12CommandList* CmdLists[] = { InCommmandList };
    Device->ExecuteGraphicsCommandLists(CmdLists);


    
    struct SamplePassData
    {
        std::vector<RGBufferRef> VertexBuffer;
        std::vector<RGBufferRef> IndexBuffer;

        RGBufferRef Mesh;
        RGBufferRef Material;
        std::vector<RGTextureRef> Textures;

        RGTextureRef DepthBuffer;
    };
    
    RenderGraphPassDesc PassDesc{};
    PassDesc.Flags = ED3D12RenderPassFlags::None;
    PassDesc.Name = "SamplePass";
    PassDesc.Width = Window::GetWidth();
    PassDesc.Height = Window::GetHeight();
    
    RenderGraphImpl->AddPass<SamplePassData>(
        PassDesc,
        [=](SamplePassData& OutData, RenderGraphBuilder* InBuilder)
        {
            OutData.VertexBuffer.resize(Model->MeshData.size());
            OutData.IndexBuffer.resize(Model->MeshData.size());
            for (UINT32 ix = 0; ix < Model->MeshData.size(); ++ix)
            {
                std::string NameA("VertexBuffer" + std::to_string(ix));
                OutData.VertexBuffer[ix] = InBuilder->ImportBuffer(NameA.c_str(), VertexBuffer[ix].get(), false);
                std::string NameB("IndexBuffer" + std::to_string(ix));
                OutData.IndexBuffer[ix] = InBuilder->ImportBuffer(NameB.c_str(), IndexBuffer[ix].get(), false);
            }
            OutData.Mesh = InBuilder->ImportBuffer("MeshBuffer", MeshBuffer.get(), true);
            OutData.Material = InBuilder->ImportBuffer("MaterialBuffer", MaterialBuffer.get(), true);

            D3D12TextureDesc TextureDesc;
            for (UINT32 ix = 0; ix < Model->ImageNum; ++ix)
            {
                const ImageData* CurrentImageData = ModelLoaderImpl->GetImageData(Model->ImageOffset + ix);
                TextureDesc.Format = CurrentImageData->Format;
                TextureDesc.Height = CurrentImageData->Height;
                TextureDesc.Width = CurrentImageData->Width;
                TextureDesc.State = ED3D12ResourceState::PixelShader;

                std::string Name("ModelTexture" + std::to_string(ix));
                OutData.Textures.emplace_back(InBuilder->DeclareReadTexture(Name.c_str(), TextureDesc, CurrentImageData->Data.get()));
            }
            
            D3D12TextureDesc FinalTextureDesc;
            FinalTextureDesc.Flag = ED3D12TextureFlag::AllowRenderTarget;
            FinalTextureDesc.Format = SWAP_CHAIN_FORMAT;
            FinalTextureDesc.Height = Window::GetHeight();
            FinalTextureDesc.Width = Window::GetWidth();
            FinalTextureDesc.State = ED3D12ResourceState::RenderTarget;
            FinalTextureDesc.ClearValue = D3D12ClearValue{ 0.690196097f, 0.768627524f, 0.870588303f, 1.f , SWAP_CHAIN_FORMAT};
				            
            RGTextureRef FinalTexture = InBuilder->DeclareWriteTexture("FinalTexture", FinalTextureDesc);
            FinalTexture->RenderTargetAccessType = ED3D12AccessType::Clear_Preserve;
			
            D3D12TextureDesc DepthBufferDesc;
            DepthBufferDesc.Flag = ED3D12TextureFlag::AllowDepthStencil;
            DepthBufferDesc.Format = SWAP_CHAIN_DEPTH_STENCIL_FORMAT;
            DepthBufferDesc.Height = Window::GetHeight();
            DepthBufferDesc.Width = Window::GetWidth();
            DepthBufferDesc.State = ED3D12ResourceState::DepthWrite;
            DepthBufferDesc.ClearValue = D3D12ClearValue(ED3D12ClearValueType::DepthStencil, SWAP_CHAIN_DEPTH_STENCIL_FORMAT);
				            
            RGTextureRef DepthBuffer = InBuilder->DeclareWriteTexture("DepthBuffer", DepthBufferDesc);
            DepthBuffer->DepthStencilAccessType = { ED3D12AccessType::Clear_Preserve, ED3D12AccessType::Clear_Preserve };
            
        },
        [=](const SamplePassData& InData, RenderGraphBuilder* InBuilder, D3D12CommandList* InCmdList)
        {
            InCmdList->SetPipelineState(Device->GetPipelineState(ED3D12PipelineStateID::Sample));

            const FrameResource* FrameResourceData = InBuilder->GetFrameResource();
            InCmdList->GetNative()->SetGraphicsRootConstantBufferView(0, FrameResourceData->CameraConstantBuffer->GetGPUAddress());

            for (UINT32 ix = 0; ix < Model->MaterialNum; ++ix)
            {
                const UINT32 DiffuseIndex = MaterialData[ix].DiffuseIndex;
                if (DiffuseIndex != INVALID_SIZE_32)
                {
                   Materials[ix].DiffuseIndexInHeap = InData.Textures[DiffuseIndex]->GPUDescriptor.GetIndex();
                }
                const UINT32 RoughnessMetallicIndex =MaterialData[ix].RoughnessMetallicIndex;
                if (RoughnessMetallicIndex != INVALID_SIZE_32)
                {
                   Materials[ix].RoughnessMetallicIndexInHeap = InData.Textures[RoughnessMetallicIndex]->GPUDescriptor.GetIndex();
                }
                
                const UINT32 NormalIndex =MaterialData[ix].NormalIndex;
                if (NormalIndex != INVALID_SIZE_32)
                {
                   Materials[ix].NormalIndexInHeap = InData.Textures[NormalIndex]->GPUDescriptor.GetIndex();
                }
                
                const UINT32 OcclusionIndex =MaterialData[ix].OcclusionIndex;
                if (OcclusionIndex != INVALID_SIZE_32)
                {
                   Materials[ix].OcclusionIndexInHeap = InData.Textures[OcclusionIndex]->GPUDescriptor.GetIndex();
                }
                
                const UINT32 EmissiveIndex =MaterialData[ix].EmissiveIndex;
                if (EmissiveIndex != INVALID_SIZE_32)
                {
                   Materials[ix].EmissiveIndexInHeap = InData.Textures[EmissiveIndex]->GPUDescriptor.GetIndex();
                }
            }

            InData.Material->Buffer->UpdateMappedData(Materials.data(), sizeof(Material) * Materials.size());

            struct SamplePassConstants
            {
                UINT32 MeshIndex;
                UINT32 MaterialIndex;
            };

            SamplePassConstants Constants{
                InData.Mesh->GPUDescriptor.GetIndex(),
                InData.Material->GPUDescriptor.GetIndex()
            };

            InCmdList->GetNative()->SetGraphicsRoot32BitConstants(3, 2, &Constants, 0);

            for (UINT32 ix = 0; ix < InData.VertexBuffer.size(); ++ix)
            {
                InCmdList->DrawIndexedInstance(InData.VertexBuffer[ix]->Buffer.get(), InData.IndexBuffer[ix]->Buffer.get(), static_cast<UINT32>(sizeof(Vertex)), static_cast<UINT32>(Model->MeshData[ix].Indices.size()));
            }
        }
    );
}
