#include "AssetDatabase.h"

#include "Engine/Core/Log.h"
#include "GuidGenerator.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace Engine::Asset
{
    namespace
    {
        std::string ToNarrowAscii(const std::filesystem::path& path)
        {
            const std::wstring value = path.generic_wstring();
            std::string result;
            result.reserve(value.size());
            for (wchar_t c : value)
            {
                result.push_back(c <= 0x7f ? static_cast<char>(c) : '?');
            }
            return result;
        }

        std::string Trim(const std::string& value)
        {
            const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) != 0; });
            const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c) != 0; }).base();
            return first >= last ? std::string() : std::string(first, last);
        }
    }

    void AssetDatabase::ScanAssets(const std::filesystem::path& root)
    {
        Core::Log::Info("AssetDatabase scan started.");
        m_pathToMeta.clear();
        m_guidToPath.clear();

        if (!std::filesystem::exists(root))
        {
            std::filesystem::create_directories(root);
        }

        int assetCount = 0;
        for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(root))
        {
            if (!entry.is_regular_file() || ShouldSkipPath(entry.path()))
            {
                continue;
            }

            const AssetType type = AssetTypeFromPath(entry.path());
            if (type == AssetType::Unknown)
            {
                Core::Log::Warning("Unknown asset type: " + ToNarrowAscii(entry.path()));
                continue;
            }

            if (ImportAsset(entry.path()))
            {
                ++assetCount;
            }
        }

        ClearDirty();
        Core::Log::Info("AssetDatabase scan completed. Assets: " + std::to_string(assetCount));
    }

    bool AssetDatabase::ImportAsset(const std::filesystem::path& path)
    {
        if (ShouldSkipPath(path) || AssetTypeFromPath(path) == AssetType::Unknown)
        {
            return false;
        }

        CreateMetaIfMissing(path);
        AssetMeta meta;
        if (!LoadMeta(path, meta))
        {
            Core::Log::Error("Failed to load meta: " + ToNarrowAscii(GetMetaPath(path)));
            return false;
        }

        const std::filesystem::path normalizedPath = NormalizePath(path);
        if (NormalizePath(meta.sourcePath) != normalizedPath || meta.type != AssetTypeFromPath(path))
        {
            meta.sourcePath = normalizedPath;
            meta.type = AssetTypeFromPath(path);
            SaveMeta(meta);
        }

        RegisterMeta(meta);
        return true;
    }

    bool AssetDatabase::CreateMetaIfMissing(const std::filesystem::path& path)
    {
        const std::filesystem::path metaPath = GetMetaPath(path);
        if (std::filesystem::exists(metaPath))
        {
            return true;
        }

        AssetMeta meta;
        meta.guid = GenerateUniqueGuid();
        meta.type = AssetTypeFromPath(path);
        meta.sourcePath = NormalizePath(path);
        meta.importerVersion = 1;

        Core::Log::Info("Created meta: " + ToNarrowAscii(metaPath));
        MarkDirty();
        return SaveMeta(meta);
    }

    bool AssetDatabase::LoadMeta(const std::filesystem::path& path, AssetMeta& outMeta) const
    {
        std::ifstream file(GetMetaPath(path));
        if (!file.is_open())
        {
            return false;
        }

        outMeta = {};
        std::string line;
        while (std::getline(file, line))
        {
            const std::size_t separator = line.find('=');
            if (separator == std::string::npos)
            {
                continue;
            }

            const std::string key = Trim(line.substr(0, separator));
            const std::string value = Trim(line.substr(separator + 1));
            if (key == "guid")
            {
                outMeta.guid = value;
            }
            else if (key == "assetType")
            {
                outMeta.type = AssetTypeFromString(value);
            }
            else if (key == "sourcePath")
            {
                outMeta.sourcePath = NormalizePath(value);
            }
            else if (key == "importerVersion")
            {
                outMeta.importerVersion = std::stoi(value);
            }
        }

        return !outMeta.guid.empty();
    }

    bool AssetDatabase::SaveMeta(const AssetMeta& meta) const
    {
        std::filesystem::create_directories(meta.sourcePath.parent_path());
        std::ofstream file(GetMetaPath(meta.sourcePath));
        if (!file.is_open())
        {
            return false;
        }

        file << "guid=" << meta.guid << "\n";
        file << "assetType=" << AssetTypeToString(meta.type) << "\n";
        file << "sourcePath=" << meta.sourcePath.generic_string() << "\n";
        file << "importerVersion=" << meta.importerVersion << "\n";
        return true;
    }

    std::string AssetDatabase::GetGuidFromPath(const std::filesystem::path& path) const
    {
        if (const AssetMeta* meta = GetMetaFromPath(path))
        {
            return meta->guid;
        }
        return {};
    }

    std::filesystem::path AssetDatabase::GetPathFromGuid(const std::string& guid) const
    {
        if (auto it = m_guidToPath.find(guid); it != m_guidToPath.end())
        {
            return it->second;
        }
        Core::Log::Warning("GUID path lookup failed: " + guid);
        return {};
    }

    AssetType AssetDatabase::GetAssetTypeFromGuid(const std::string& guid) const
    {
        if (const AssetMeta* meta = GetMetaFromGuid(guid))
        {
            return meta->type;
        }
        return AssetType::Unknown;
    }

    bool AssetDatabase::IsValidGuid(const std::string& guid) const
    {
        return m_guidToPath.find(guid) != m_guidToPath.end();
    }

    const AssetMeta* AssetDatabase::GetMetaFromPath(const std::filesystem::path& path) const
    {
        const std::string key = NormalizePath(path).generic_string();
        if (auto it = m_pathToMeta.find(key); it != m_pathToMeta.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    const AssetMeta* AssetDatabase::GetMetaFromGuid(const std::string& guid) const
    {
        const std::filesystem::path path = GetPathFromGuid(guid);
        return path.empty() ? nullptr : GetMetaFromPath(path);
    }

    const std::unordered_map<std::string, AssetMeta>& AssetDatabase::GetAssets() const
    {
        return m_pathToMeta;
    }

    bool AssetDatabase::MoveAsset(const std::filesystem::path& oldPath, const std::filesystem::path& newPath)
    {
        if (!std::filesystem::exists(oldPath) || std::filesystem::exists(newPath))
        {
            Core::Log::Error("MoveAsset failed: invalid path.");
            return false;
        }

        AssetMeta meta;
        if (!LoadMeta(oldPath, meta))
        {
            Core::Log::Error("MoveAsset failed: missing meta.");
            return false;
        }

        std::filesystem::create_directories(newPath.parent_path());
        std::error_code error;
        std::filesystem::rename(oldPath, newPath, error);
        if (error)
        {
            Core::Log::Error("MoveAsset failed: " + error.message());
            return false;
        }

        std::filesystem::rename(GetMetaPath(oldPath), GetMetaPath(newPath), error);
        if (error)
        {
            Core::Log::Error("MoveAsset meta move failed: " + error.message());
            return false;
        }

        meta.sourcePath = NormalizePath(newPath);
        SaveMeta(meta);
        ScanAssets("Assets");
        Core::Log::Info("Moved asset: " + ToNarrowAscii(newPath));
        return true;
    }

    bool AssetDatabase::RenameAsset(const std::filesystem::path& oldPath, const std::filesystem::path& newPath)
    {
        return MoveAsset(oldPath, newPath);
    }

    bool AssetDatabase::DeleteAsset(const std::filesystem::path& path)
    {
        std::error_code error;
        std::filesystem::remove(path, error);
        if (error)
        {
            Core::Log::Error("DeleteAsset failed: " + error.message());
            return false;
        }
        std::filesystem::remove(GetMetaPath(path), error);
        ScanAssets("Assets");
        Core::Log::Info("Deleted asset: " + ToNarrowAscii(path));
        return true;
    }

    bool AssetDatabase::IsDirty() const { return m_dirty; }
    void AssetDatabase::MarkDirty() { m_dirty = true; }
    void AssetDatabase::ClearDirty() { m_dirty = false; }

    std::filesystem::path AssetDatabase::NormalizePath(const std::filesystem::path& path)
    {
        return path.lexically_normal().generic_wstring();
    }

    std::filesystem::path AssetDatabase::GetMetaPath(const std::filesystem::path& path)
    {
        return std::filesystem::path(path).concat(L".meta");
    }

    bool AssetDatabase::ShouldSkipPath(const std::filesystem::path& path)
    {
        const std::wstring filename = path.filename().wstring();
        const std::wstring extension = path.extension().wstring();
        if (extension == L".meta" || extension == L".tmp" || extension == L".bak" || filename == L".gitignore")
        {
            return true;
        }
        if (!filename.empty() && filename.front() == L'.')
        {
            return true;
        }
        for (const std::filesystem::path& part : path)
        {
            const std::wstring name = part.wstring();
            if (name == L".git" || name == L".vs" || name == L"bin" || name == L"obj" || name == L"Intermediate")
            {
                return true;
            }
        }
        return false;
    }

    std::string AssetDatabase::GenerateUniqueGuid() const
    {
        std::string guid;
        do
        {
            guid = GuidGenerator::NewGuid();
        } while (IsValidGuid(guid));
        return guid;
    }

    void AssetDatabase::RegisterMeta(const AssetMeta& meta)
    {
        const std::string pathKey = NormalizePath(meta.sourcePath).generic_string();
        if (auto existing = m_guidToPath.find(meta.guid); existing != m_guidToPath.end() && NormalizePath(existing->second) != NormalizePath(meta.sourcePath))
        {
            Core::Log::Warning("Duplicate GUID found: " + meta.guid);
        }
        m_pathToMeta[pathKey] = meta;
        m_guidToPath[meta.guid] = NormalizePath(meta.sourcePath);
    }
}
