#pragma once

#include "Engine/Math/MathTypes.h"
#include "Engine/Graphics/RHI/RHIShader.h"
#include "Engine/Graphics/RHI/RHITexture.h"

#include <memory>
#include <string>

namespace Engine::Renderer
{
    class Material
    {
    public:
        void SetVertexShader(std::shared_ptr<RHI::RHIShader> shader);
        void SetPixelShader(std::shared_ptr<RHI::RHIShader> shader);
        void SetBaseColor(const Math::Vector4& color);
        void SetRoughness(float roughness);
        void SetMetallic(float metallic);
        void SetNormalStrength(float normalStrength);
        void SetTexture(std::shared_ptr<RHI::RHITexture> texture);
        void SetBaseColorTexture(std::shared_ptr<RHI::RHITexture> texture);
        void SetRoughnessTexture(std::shared_ptr<RHI::RHITexture> texture);
        void SetMetallicTexture(std::shared_ptr<RHI::RHITexture> texture);
        void SetNormalTexture(std::shared_ptr<RHI::RHITexture> texture);
        void SetName(std::string name);
        void SetVertexShaderPath(std::wstring path);
        void SetPixelShaderPath(std::wstring path);
        void SetTexturePath(std::wstring path);
        void SetBaseColorTexturePath(std::wstring path);
        void SetRoughnessTexturePath(std::wstring path);
        void SetMetallicTexturePath(std::wstring path);
        void SetNormalTexturePath(std::wstring path);
        std::shared_ptr<Material> Clone() const;

        const std::shared_ptr<RHI::RHIShader>& GetVertexShader() const;
        const std::shared_ptr<RHI::RHIShader>& GetPixelShader() const;
        const Math::Vector4& GetBaseColor() const;
        float GetRoughness() const;
        float GetMetallic() const;
        float GetNormalStrength() const;
        const std::shared_ptr<RHI::RHITexture>& GetTexture() const;
        const std::shared_ptr<RHI::RHITexture>& GetBaseColorTexture() const;
        const std::shared_ptr<RHI::RHITexture>& GetRoughnessTexture() const;
        const std::shared_ptr<RHI::RHITexture>& GetMetallicTexture() const;
        const std::shared_ptr<RHI::RHITexture>& GetNormalTexture() const;
        const std::string& GetName() const;
        const std::wstring& GetVertexShaderPath() const;
        const std::wstring& GetPixelShaderPath() const;
        const std::wstring& GetTexturePath() const;
        const std::wstring& GetBaseColorTexturePath() const;
        const std::wstring& GetRoughnessTexturePath() const;
        const std::wstring& GetMetallicTexturePath() const;
        const std::wstring& GetNormalTexturePath() const;

    private:
        std::string m_name;
        std::wstring m_vertexShaderPath;
        std::wstring m_pixelShaderPath;
        std::wstring m_baseColorTexturePath;
        std::wstring m_roughnessTexturePath;
        std::wstring m_metallicTexturePath;
        std::wstring m_normalTexturePath;
        std::shared_ptr<RHI::RHIShader> m_vertexShader;
        std::shared_ptr<RHI::RHIShader> m_pixelShader;
        std::shared_ptr<RHI::RHITexture> m_baseColorTexture;
        std::shared_ptr<RHI::RHITexture> m_roughnessTexture;
        std::shared_ptr<RHI::RHITexture> m_metallicTexture;
        std::shared_ptr<RHI::RHITexture> m_normalTexture;
        Math::Vector4 m_baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        float m_roughness = 0.5f;
        float m_metallic = 0.0f;
        float m_normalStrength = 1.0f;
    };
}
