#pragma once

#include "Engine/Graphics/Renderer/Mesh.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Engine::AssetImport
{
    struct OBJMeshData
    {
        std::vector<Renderer::Vertex> vertices;
        std::vector<std::uint32_t> indices;
        std::wstring materialLibraryPath;
        std::string objectName;
        std::string materialName;
    };

    struct OBJMeshPart
    {
        OBJMeshData meshData;
        std::string name;
        std::string materialName;
    };

    class OBJLoader
    {
    public:
        static bool Load(const std::string& path, OBJMeshData& outMeshData);
        static bool LoadParts(const std::string& path, std::vector<OBJMeshPart>& outParts);
    };
}
