#pragma once

#include "AssetType.h"

#include <filesystem>
#include <string>

namespace Engine::Asset
{
    struct AssetMeta
    {
        std::string guid;
        AssetType type = AssetType::Unknown;
        std::filesystem::path sourcePath;
        int importerVersion = 1;
    };
}
