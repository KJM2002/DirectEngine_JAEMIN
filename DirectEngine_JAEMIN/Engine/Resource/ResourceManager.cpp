#include "ResourceManager.h"

#include "Engine/Asset/AssetDatabase.h"
#include "Engine/AssetImport/OBJLoader.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/RHI/RHIDevice.h"
#include "Engine/Graphics/RHI/RHITexture.h"
#include "Engine/Graphics/Renderer/Material.h"
#include "Engine/Graphics/Renderer/Mesh.h"
#include "Engine/Platform/Windows/WICImageLoader.h"
#include "ImageData.h"
#include "MaterialLoader.h"

#include <array>
#include <filesystem>

namespace Engine::Resource
{
    namespace
    {
        std::string ToNarrowAscii(const std::wstring& value)
        {
            std::string result;
            result.reserve(value.size());
            for (wchar_t character : value)
            {
                result.push_back(character <= 0x7f ? static_cast<char>(character) : '?');
            }
            return result;
        }

        std::wstring MakeTextureCacheKey(const std::wstring& path, const TextureLoadOptions& options)
        {
            return path
                + L"|srgb=" + std::to_wstring(options.sRGB ? 1 : 0)
                + L"|mips=" + std::to_wstring(options.generateMips ? 1 : 0)
                + L"|filter=" + std::to_wstring(static_cast<int>(options.filter))
                + L"|address=" + std::to_wstring(static_cast<int>(options.addressMode));
        }

        TextureLoadOptions MakeLinearTextureOptions()
        {
            TextureLoadOptions options;
            options.sRGB = false;
            return options;
        }
    }

    ResourceManager::ResourceManager(RHI::RHIDevice& device)
        : m_device(device)
    {
    }

    void ResourceManager::SetAssetDatabase(const Asset::AssetDatabase* assetDatabase)
    {
        m_assetDatabase = assetDatabase;
    }

    std::shared_ptr<Renderer::Mesh> ResourceManager::CreateCubeMesh(const std::string& key)
    {
        if (auto it = m_meshes.find(key); it != m_meshes.end())
        {
            return it->second;
        }

        std::shared_ptr<Renderer::Mesh> mesh = Renderer::Mesh::CreateCube(m_device);
        if (mesh)
        {
            m_meshes.emplace(key, mesh);
        }

        return mesh;
    }

    std::shared_ptr<Renderer::Mesh> ResourceManager::LoadOBJMesh(const std::string& key, const std::string& path)
    {
        if (auto it = m_meshes.find(key); it != m_meshes.end())
        {
            return it->second;
        }

        AssetImport::OBJMeshData meshData;
        if (!AssetImport::OBJLoader::Load(path, meshData))
        {
            return nullptr;
        }

        std::shared_ptr<Renderer::Mesh> mesh = Renderer::Mesh::CreateFromVertices(m_device, meshData.vertices, meshData.indices);
        if (mesh)
        {
            m_meshes.emplace(key, mesh);
        }

        return mesh;
    }

    std::shared_ptr<Renderer::Mesh> ResourceManager::LoadMesh(const std::wstring& path)
    {
        if (auto it = m_meshFiles.find(path); it != m_meshFiles.end())
        {
            return it->second;
        }

        std::shared_ptr<Renderer::Mesh> mesh = LoadOBJMesh("file:mesh:" + ToNarrowAscii(path), ToNarrowAscii(path));
        if (!mesh)
        {
            mesh = CreateCubeMesh();
        }

        if (mesh)
        {
            m_meshFiles.emplace(path, mesh);
        }

        return mesh;
    }

    std::shared_ptr<Renderer::Mesh> ResourceManager::LoadMeshByGuid(const std::string& guid)
    {
        if (!m_assetDatabase)
        {
            Core::Log::Warning("LoadMeshByGuid failed: AssetDatabase is not set.");
            return nullptr;
        }

        const std::filesystem::path path = m_assetDatabase->GetPathFromGuid(guid);
        if (path.empty())
        {
            Core::Log::Warning("LoadMeshByGuid failed: " + guid);
            return nullptr;
        }
        return LoadMesh(path.generic_wstring());
    }

    std::shared_ptr<RHI::RHIShader> ResourceManager::LoadVertexShader(const std::string& key, const std::wstring& path, const std::string& entryPoint)
    {
        if (auto it = m_vertexShaders.find(key); it != m_vertexShaders.end())
        {
            return it->second;
        }

        std::shared_ptr<RHI::RHIShader> shader = m_device.CreateVertexShader(path, entryPoint);
        if (shader)
        {
            m_vertexShaders.emplace(key, shader);
        }

        return shader;
    }

    std::shared_ptr<RHI::RHIShader> ResourceManager::LoadPixelShader(const std::string& key, const std::wstring& path, const std::string& entryPoint)
    {
        if (auto it = m_pixelShaders.find(key); it != m_pixelShaders.end())
        {
            return it->second;
        }

        std::shared_ptr<RHI::RHIShader> shader = m_device.CreatePixelShader(path, entryPoint);
        if (shader)
        {
            m_pixelShaders.emplace(key, shader);
        }

        return shader;
    }

    std::shared_ptr<RHI::RHITexture> ResourceManager::CreateCheckerTexture(const std::string& key)
    {
        if (auto it = m_textures.find(key); it != m_textures.end())
        {
            return it->second;
        }

        const std::array<std::uint32_t, 4> pixels =
        {
            0xffffffffu, 0xff202020u,
            0xff202020u, 0xffffffffu
        };

        RHI::TextureDesc desc;
        desc.width = 2;
        desc.height = 2;
        desc.format = RHI::TextureFormat::RGBA8;
        desc.initialData = pixels.data();
        desc.rowPitch = desc.width * sizeof(std::uint32_t);
        desc.filter = RHI::TextureFilter::Point;

        std::shared_ptr<RHI::RHITexture> texture = m_device.CreateTexture2D(desc);
        if (texture)
        {
            m_textures.emplace(key, texture);
        }

        return texture;
    }

    std::shared_ptr<RHI::RHITexture> ResourceManager::LoadTexture(const std::wstring& path)
    {
        TextureLoadOptions options;
        return LoadTexture(path, options);
    }

    std::shared_ptr<RHI::RHITexture> ResourceManager::LoadTexture(const std::wstring& path, const TextureLoadOptions& options)
    {
        const std::wstring cacheKey = MakeTextureCacheKey(path, options);
        if (auto it = m_fileTextures.find(cacheKey); it != m_fileTextures.end())
        {
            return it->second;
        }

        ImageData image;
        if (!Platform::Windows::WICImageLoader::LoadFromFile(path, image))
        {
            Core::Log::Warning("Texture load failed, using checker fallback: " + ToNarrowAscii(path));
            std::shared_ptr<RHI::RHITexture> fallbackTexture = CreateCheckerTexture();
            m_fileTextures.emplace(cacheKey, fallbackTexture);
            return fallbackTexture;
        }

        RHI::TextureDesc desc;
        desc.width = image.width;
        desc.height = image.height;
        desc.format = options.sRGB ? RHI::TextureFormat::RGBA8_sRGB : image.format;
        desc.initialData = image.pixels.data();
        desc.rowPitch = image.GetRowPitch();
        desc.shaderResource = true;
        desc.generateMips = options.generateMips;
        desc.filter = options.filter;
        desc.addressMode = options.addressMode;

        std::shared_ptr<RHI::RHITexture> texture = m_device.CreateTexture2D(desc);
        if (!texture)
        {
            Core::Log::Warning("Texture creation failed, using checker fallback: " + ToNarrowAscii(path));
            std::shared_ptr<RHI::RHITexture> fallbackTexture = CreateCheckerTexture();
            m_fileTextures.emplace(cacheKey, fallbackTexture);
            return fallbackTexture;
        }

        m_fileTextures.emplace(cacheKey, texture);
        return texture;
    }

    std::shared_ptr<RHI::RHITexture> ResourceManager::LoadTextureByGuid(const std::string& guid)
    {
        if (!m_assetDatabase)
        {
            Core::Log::Warning("LoadTextureByGuid failed: AssetDatabase is not set.");
            return nullptr;
        }

        const std::filesystem::path path = m_assetDatabase->GetPathFromGuid(guid);
        if (path.empty())
        {
            Core::Log::Warning("LoadTextureByGuid failed: " + guid);
            return nullptr;
        }
        return LoadTexture(path.generic_wstring());
    }

    std::shared_ptr<Renderer::Material> ResourceManager::CreateMaterial(const std::string& key, const Math::Vector4& baseColor)
    {
        if (auto it = m_materials.find(key); it != m_materials.end())
        {
            return it->second;
        }

        std::shared_ptr<RHI::RHIShader> vertexShader = LoadVertexShader("builtin:shader:basic_vs", L"Shaders/BasicVertex.hlsl", "main");
        std::shared_ptr<RHI::RHIShader> pixelShader = LoadPixelShader("builtin:shader:basic_ps", L"Shaders/BasicPixel.hlsl", "main");
        if (!vertexShader || !pixelShader)
        {
            return nullptr;
        }

        auto material = std::make_shared<Renderer::Material>();
        material->SetName(key);
        material->SetVertexShader(vertexShader);
        material->SetPixelShader(pixelShader);
        material->SetBaseColor(baseColor);
        material->SetRoughness(0.5f);
        material->SetMetallic(0.0f);
        material->SetVertexShaderPath(L"Shaders/BasicVertex.hlsl");
        material->SetPixelShaderPath(L"Shaders/BasicPixel.hlsl");
        m_materials.emplace(key, material);
        return material;
    }

    std::shared_ptr<Renderer::Material> ResourceManager::LoadMaterial(const std::wstring& path)
    {
        if (auto it = m_materialFiles.find(path); it != m_materialFiles.end())
        {
            return it->second;
        }

        MaterialDesc desc;
        if (!MaterialLoader::LoadFromFile(path, desc))
        {
            return CreateMaterial("fallback:material:default", { 0.8f, 0.8f, 0.8f, 1.0f });
        }

        const std::string materialKey = "file:material:" + ToNarrowAscii(path);
        std::shared_ptr<RHI::RHIShader> vertexShader = LoadVertexShader("file:vs:" + ToNarrowAscii(desc.vertexShaderPath), desc.vertexShaderPath, "main");
        std::shared_ptr<RHI::RHIShader> pixelShader = LoadPixelShader("file:ps:" + ToNarrowAscii(desc.pixelShaderPath), desc.pixelShaderPath, "main");
        if (!vertexShader || !pixelShader)
        {
            return CreateMaterial("fallback:material:default", desc.baseColor);
        }

        auto material = std::make_shared<Renderer::Material>();
        material->SetName(desc.name.empty() ? materialKey : desc.name);
        material->SetVertexShader(vertexShader);
        material->SetPixelShader(pixelShader);
        material->SetBaseColor(desc.baseColor);
        material->SetRoughness(desc.roughness);
        material->SetMetallic(desc.metallic);
        material->SetVertexShaderPath(desc.vertexShaderPath);
        material->SetPixelShaderPath(desc.pixelShaderPath);
        if (!desc.texturePath.empty())
        {
            material->SetBaseColorTexture(LoadTexture(desc.texturePath));
            material->SetBaseColorTexturePath(desc.texturePath);
        }
        if (!desc.roughnessTexturePath.empty())
        {
            material->SetRoughnessTexture(LoadTexture(desc.roughnessTexturePath, MakeLinearTextureOptions()));
            material->SetRoughnessTexturePath(desc.roughnessTexturePath);
        }
        if (!desc.metallicTexturePath.empty())
        {
            material->SetMetallicTexture(LoadTexture(desc.metallicTexturePath, MakeLinearTextureOptions()));
            material->SetMetallicTexturePath(desc.metallicTexturePath);
        }
        if (!desc.normalTexturePath.empty())
        {
            material->SetNormalTexture(LoadTexture(desc.normalTexturePath, MakeLinearTextureOptions()));
            material->SetNormalTexturePath(desc.normalTexturePath);
        }

        m_materialFiles.emplace(path, material);
        return material;
    }

    std::shared_ptr<Renderer::Material> ResourceManager::LoadMaterialByGuid(const std::string& guid)
    {
        if (!m_assetDatabase)
        {
            Core::Log::Warning("LoadMaterialByGuid failed: AssetDatabase is not set.");
            return nullptr;
        }

        const std::filesystem::path path = m_assetDatabase->GetPathFromGuid(guid);
        if (path.empty())
        {
            Core::Log::Warning("LoadMaterialByGuid failed: " + guid);
            return nullptr;
        }
        return LoadMaterial(path.generic_wstring());
    }

    void ResourceManager::ForgetMaterial(const std::wstring& path)
    {
        m_materialFiles.erase(path);
    }

    void ResourceManager::Clear()
    {
        m_materialFiles.clear();
        m_materials.clear();
        m_fileTextures.clear();
        m_textures.clear();
        m_pixelShaders.clear();
        m_vertexShaders.clear();
        m_meshFiles.clear();
        m_meshes.clear();
    }
}
