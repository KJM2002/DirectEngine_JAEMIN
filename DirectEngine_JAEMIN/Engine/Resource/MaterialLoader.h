#pragma once

#include "Engine/Math/MathTypes.h"

#include <string>

namespace Engine::Resource
{
    struct MaterialDesc
    {
        std::string name;
        std::wstring vertexShaderPath;
        std::wstring pixelShaderPath;
        Math::Vector4 baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        float roughness = 0.5f;
        float metallic = 0.0f;
        float normalStrength = 1.0f;
        std::wstring texturePath;
        std::wstring roughnessTexturePath;
        std::wstring metallicTexturePath;
        std::wstring normalTexturePath;
    };

    class MaterialLoader
    {
    public:
        static bool LoadFromFile(const std::wstring& path, MaterialDesc& outDesc);
        static bool SaveToFile(const std::wstring& path, const MaterialDesc& desc);
    };
}
