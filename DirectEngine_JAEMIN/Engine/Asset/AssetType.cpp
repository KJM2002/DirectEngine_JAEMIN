#include "AssetType.h"

#include <algorithm>
#include <cwctype>

namespace Engine::Asset
{
    namespace
    {
        std::wstring ToLower(std::wstring value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](wchar_t c)
            {
                return static_cast<wchar_t>(std::towlower(c));
            });
            return value;
        }
    }

    const char* AssetTypeToString(AssetType type)
    {
        switch (type)
        {
        case AssetType::Mesh:
            return "Mesh";
        case AssetType::Texture:
            return "Texture";
        case AssetType::Material:
            return "Material";
        case AssetType::Scene:
            return "Scene";
        case AssetType::Prefab:
            return "Prefab";
        case AssetType::Unknown:
        default:
            return "Unknown";
        }
    }

    AssetType AssetTypeFromString(const std::string& value)
    {
        if (value == "Mesh") return AssetType::Mesh;
        if (value == "Texture") return AssetType::Texture;
        if (value == "Material") return AssetType::Material;
        if (value == "Scene") return AssetType::Scene;
        if (value == "Prefab") return AssetType::Prefab;
        return AssetType::Unknown;
    }

    AssetType AssetTypeFromPath(const std::filesystem::path& path)
    {
        const std::wstring extension = ToLower(path.extension().wstring());
        if (extension == L".obj" || extension == L".gltf" || extension == L".glb" || extension == L".fbx")
        {
            return AssetType::Mesh;
        }
        if (extension == L".png" || extension == L".jpg" || extension == L".jpeg" || extension == L".bmp" || extension == L".tga")
        {
            return AssetType::Texture;
        }
        if (extension == L".mat")
        {
            return AssetType::Material;
        }
        if (extension == L".scene")
        {
            return AssetType::Scene;
        }
        if (extension == L".prefab")
        {
            return AssetType::Prefab;
        }
        return AssetType::Unknown;
    }
}
