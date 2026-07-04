#include "Material.h"

#include <algorithm>
#include <utility>

namespace Engine::Renderer
{
    void Material::SetVertexShader(std::shared_ptr<RHI::RHIShader> shader)
    {
        m_vertexShader = std::move(shader);
    }

    void Material::SetPixelShader(std::shared_ptr<RHI::RHIShader> shader)
    {
        m_pixelShader = std::move(shader);
    }

    void Material::SetBaseColor(const Math::Vector4& color)
    {
        m_baseColor = color;
    }

    void Material::SetRoughness(float roughness)
    {
        m_roughness = std::clamp(roughness, 0.0f, 1.0f);
    }

    void Material::SetMetallic(float metallic)
    {
        m_metallic = std::clamp(metallic, 0.0f, 1.0f);
    }

    void Material::SetTexture(std::shared_ptr<RHI::RHITexture> texture)
    {
        SetBaseColorTexture(std::move(texture));
    }

    void Material::SetBaseColorTexture(std::shared_ptr<RHI::RHITexture> texture)
    {
        m_baseColorTexture = std::move(texture);
    }

    void Material::SetRoughnessTexture(std::shared_ptr<RHI::RHITexture> texture)
    {
        m_roughnessTexture = std::move(texture);
    }

    void Material::SetMetallicTexture(std::shared_ptr<RHI::RHITexture> texture)
    {
        m_metallicTexture = std::move(texture);
    }

    void Material::SetNormalTexture(std::shared_ptr<RHI::RHITexture> texture)
    {
        m_normalTexture = std::move(texture);
    }

    void Material::SetName(std::string name)
    {
        m_name = std::move(name);
    }

    void Material::SetVertexShaderPath(std::wstring path)
    {
        m_vertexShaderPath = std::move(path);
    }

    void Material::SetPixelShaderPath(std::wstring path)
    {
        m_pixelShaderPath = std::move(path);
    }

    void Material::SetTexturePath(std::wstring path)
    {
        SetBaseColorTexturePath(std::move(path));
    }

    void Material::SetBaseColorTexturePath(std::wstring path)
    {
        m_baseColorTexturePath = std::move(path);
    }

    void Material::SetRoughnessTexturePath(std::wstring path)
    {
        m_roughnessTexturePath = std::move(path);
    }

    void Material::SetMetallicTexturePath(std::wstring path)
    {
        m_metallicTexturePath = std::move(path);
    }

    void Material::SetNormalTexturePath(std::wstring path)
    {
        m_normalTexturePath = std::move(path);
    }

    const std::shared_ptr<RHI::RHIShader>& Material::GetVertexShader() const
    {
        return m_vertexShader;
    }

    const std::shared_ptr<RHI::RHIShader>& Material::GetPixelShader() const
    {
        return m_pixelShader;
    }

    const Math::Vector4& Material::GetBaseColor() const
    {
        return m_baseColor;
    }

    float Material::GetRoughness() const
    {
        return m_roughness;
    }

    float Material::GetMetallic() const
    {
        return m_metallic;
    }

    const std::shared_ptr<RHI::RHITexture>& Material::GetTexture() const
    {
        return GetBaseColorTexture();
    }

    const std::shared_ptr<RHI::RHITexture>& Material::GetBaseColorTexture() const
    {
        return m_baseColorTexture;
    }

    const std::shared_ptr<RHI::RHITexture>& Material::GetRoughnessTexture() const
    {
        return m_roughnessTexture;
    }

    const std::shared_ptr<RHI::RHITexture>& Material::GetMetallicTexture() const
    {
        return m_metallicTexture;
    }

    const std::shared_ptr<RHI::RHITexture>& Material::GetNormalTexture() const
    {
        return m_normalTexture;
    }

    const std::string& Material::GetName() const
    {
        return m_name;
    }

    const std::wstring& Material::GetVertexShaderPath() const
    {
        return m_vertexShaderPath;
    }

    const std::wstring& Material::GetPixelShaderPath() const
    {
        return m_pixelShaderPath;
    }

    const std::wstring& Material::GetTexturePath() const
    {
        return GetBaseColorTexturePath();
    }

    const std::wstring& Material::GetBaseColorTexturePath() const
    {
        return m_baseColorTexturePath;
    }

    const std::wstring& Material::GetRoughnessTexturePath() const
    {
        return m_roughnessTexturePath;
    }

    const std::wstring& Material::GetMetallicTexturePath() const
    {
        return m_metallicTexturePath;
    }

    const std::wstring& Material::GetNormalTexturePath() const
    {
        return m_normalTexturePath;
    }
}
