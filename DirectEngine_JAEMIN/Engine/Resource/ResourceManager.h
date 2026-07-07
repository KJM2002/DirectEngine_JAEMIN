#pragma once

#include "Engine/Graphics/RHI/RHICommon.h"
#include "Engine/Math/MathTypes.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace Engine::RHI
{
    class RHIDevice;
    class RHIShader;
    class RHITexture;
}

namespace Engine::Asset
{
    class AssetDatabase;
}

namespace Engine::Renderer
{
    class Material;
    class Mesh;
}

namespace Engine::Resource
{
    struct TextureLoadOptions
    {
        bool sRGB = true;
        bool generateMips = true;
        RHI::TextureFilter filter = RHI::TextureFilter::Linear;
        RHI::TextureAddressMode addressMode = RHI::TextureAddressMode::Wrap;
    };

    class ResourceManager
    {
    public:
        explicit ResourceManager(RHI::RHIDevice& device);
        void SetAssetDatabase(const Asset::AssetDatabase* assetDatabase);

        std::shared_ptr<Renderer::Mesh> CreateCubeMesh(const std::string& key = "builtin:mesh:cube");
        std::shared_ptr<Renderer::Mesh> CreateSphereMesh(const std::string& key = "builtin:mesh:sphere");
        std::shared_ptr<Renderer::Mesh> CreateCylinderMesh(const std::string& key = "builtin:mesh:cylinder");
        std::shared_ptr<Renderer::Mesh> CreateConeMesh(const std::string& key = "builtin:mesh:cone");
        std::shared_ptr<Renderer::Mesh> CreatePlaneMesh(const std::string& key = "builtin:mesh:plane");
        std::shared_ptr<Renderer::Mesh> LoadOBJMesh(const std::string& key, const std::string& path);
        std::shared_ptr<Renderer::Mesh> LoadMesh(const std::wstring& path);
        std::shared_ptr<Renderer::Mesh> LoadMeshByGuid(const std::string& guid);
        std::shared_ptr<RHI::RHIShader> LoadVertexShader(const std::string& key, const std::wstring& path, const std::string& entryPoint = "main");
        std::shared_ptr<RHI::RHIShader> LoadPixelShader(const std::string& key, const std::wstring& path, const std::string& entryPoint = "main");
        std::shared_ptr<RHI::RHITexture> CreateCheckerTexture(const std::string& key = "builtin:texture:checker2x2");
        std::shared_ptr<RHI::RHITexture> LoadTexture(const std::wstring& path);
        std::shared_ptr<RHI::RHITexture> LoadTexture(const std::wstring& path, const TextureLoadOptions& options);
        std::shared_ptr<RHI::RHITexture> LoadTextureByGuid(const std::string& guid);
        std::shared_ptr<Renderer::Material> CreateMaterial(const std::string& key, const Math::Vector4& baseColor);
        std::shared_ptr<Renderer::Material> LoadMaterial(const std::wstring& path);
        std::shared_ptr<Renderer::Material> LoadMaterialByGuid(const std::string& guid);
        void ForgetMaterial(const std::wstring& path);

        void Clear();

    private:
        RHI::RHIDevice& m_device;
        const Asset::AssetDatabase* m_assetDatabase = nullptr;
        std::unordered_map<std::string, std::shared_ptr<Renderer::Mesh>> m_meshes;
        std::unordered_map<std::wstring, std::shared_ptr<Renderer::Mesh>> m_meshFiles;
        std::unordered_map<std::string, std::shared_ptr<RHI::RHIShader>> m_vertexShaders;
        std::unordered_map<std::string, std::shared_ptr<RHI::RHIShader>> m_pixelShaders;
        std::unordered_map<std::string, std::shared_ptr<RHI::RHITexture>> m_textures;
        std::unordered_map<std::wstring, std::shared_ptr<RHI::RHITexture>> m_fileTextures;
        std::unordered_map<std::string, std::shared_ptr<Renderer::Material>> m_materials;
        std::unordered_map<std::wstring, std::shared_ptr<Renderer::Material>> m_materialFiles;
    };
}
