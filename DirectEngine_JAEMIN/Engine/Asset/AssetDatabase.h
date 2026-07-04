#pragma once

#include "AssetMeta.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace Engine::Asset
{
    class AssetDatabase
    {
    public:
        void ScanAssets(const std::filesystem::path& root = "Assets");
        bool ImportAsset(const std::filesystem::path& path);
        bool CreateMetaIfMissing(const std::filesystem::path& path);
        bool LoadMeta(const std::filesystem::path& path, AssetMeta& outMeta) const;
        bool SaveMeta(const AssetMeta& meta) const;

        std::string GetGuidFromPath(const std::filesystem::path& path) const;
        std::filesystem::path GetPathFromGuid(const std::string& guid) const;
        AssetType GetAssetTypeFromGuid(const std::string& guid) const;
        bool IsValidGuid(const std::string& guid) const;
        const AssetMeta* GetMetaFromPath(const std::filesystem::path& path) const;
        const AssetMeta* GetMetaFromGuid(const std::string& guid) const;
        const std::unordered_map<std::string, AssetMeta>& GetAssets() const;

        bool MoveAsset(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);
        bool RenameAsset(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);
        bool DeleteAsset(const std::filesystem::path& path);

        bool IsDirty() const;
        void MarkDirty();
        void ClearDirty();

    private:
        static std::filesystem::path NormalizePath(const std::filesystem::path& path);
        static std::filesystem::path GetMetaPath(const std::filesystem::path& path);
        static bool ShouldSkipPath(const std::filesystem::path& path);
        std::string GenerateUniqueGuid() const;
        void RegisterMeta(const AssetMeta& meta);

        std::unordered_map<std::string, AssetMeta> m_pathToMeta;
        std::unordered_map<std::string, std::filesystem::path> m_guidToPath;
        bool m_dirty = false;
    };
}
