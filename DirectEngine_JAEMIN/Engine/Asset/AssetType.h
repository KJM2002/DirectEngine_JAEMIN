#pragma once

#include <filesystem>
#include <string>

namespace Engine::Asset
{
    enum class AssetType
    {
        Unknown,
        Mesh,
        Texture,
        Material,
        Scene,
        Prefab
    };

    const char* AssetTypeToString(AssetType type);
    AssetType AssetTypeFromString(const std::string& value);
    AssetType AssetTypeFromPath(const std::filesystem::path& path);
}
