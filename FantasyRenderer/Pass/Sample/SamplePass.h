#pragma once

#include "../PassDefines.h"

class LightManager;

class SamplePass
{
public:
    CLASS_NO_COPY(SamplePass)

    SamplePass() = default;
    ~SamplePass() = default;

public:
    void Init(RenderGraph* InRenderGraph, ModelLoader* InModelLoader, LightManager* InLightManager);
    void Setup(const ModelLoadDesc& InModelLoadDesc, D3D12CommandList* InCmdList);

private:
    RenderGraph* RenderGraphImpl;
    ModelLoader* ModelLoaderImpl;
    LightManager* LightManagerImpl;

    std::vector<std::unique_ptr<D3D12Buffer>> VertexBuffer;
    std::vector<std::unique_ptr<D3D12Buffer>> IndexBuffer;
    std::unique_ptr<D3D12Buffer> MeshBuffer;
    std::unique_ptr<D3D12Buffer> MaterialBuffer;

    std::vector<Material> Materials;
};
