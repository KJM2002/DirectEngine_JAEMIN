#include "OBJLoader.h"

#include "Engine/Core/Log.h"
#include "Engine/Math/MathTypes.h"

#include <fstream>
#include <cmath>
#include <sstream>
#include <unordered_map>

namespace Engine::AssetImport
{
    namespace
    {
        struct IndexTriplet
        {
            int position = 0;
            int texcoord = 0;
            int normal = 0;
        };

        int ResolveIndex(int index, std::size_t count)
        {
            if (index > 0)
            {
                return index - 1;
            }

            if (index < 0)
            {
                return static_cast<int>(count) + index;
            }

            return -1;
        }

        bool IsValidIndex(int index, std::size_t count)
        {
            return index >= 0 && static_cast<std::size_t>(index) < count;
        }

        IndexTriplet ParseFaceVertex(const std::string& token)
        {
            IndexTriplet result;
            std::stringstream stream(token);
            std::string part;

            if (std::getline(stream, part, '/') && !part.empty())
            {
                result.position = std::stoi(part);
            }
            if (std::getline(stream, part, '/') && !part.empty())
            {
                result.texcoord = std::stoi(part);
            }
            if (std::getline(stream, part, '/') && !part.empty())
            {
                result.normal = std::stoi(part);
            }

            return result;
        }

        std::string MakeVertexKey(const IndexTriplet& triplet)
        {
            return std::to_string(triplet.position) + "/" + std::to_string(triplet.texcoord) + "/" + std::to_string(triplet.normal);
        }

        std::wstring ToWideAscii(const std::string& value)
        {
            std::wstring result;
            result.reserve(value.size());
            for (char character : value)
            {
                result.push_back(static_cast<unsigned char>(character));
            }
            return result;
        }

        Math::Vector3 Subtract(const Renderer::Vertex& a, const Renderer::Vertex& b)
        {
            return
            {
                a.position[0] - b.position[0],
                a.position[1] - b.position[1],
                a.position[2] - b.position[2]
            };
        }

        Math::Vector3 Cross(const Math::Vector3& a, const Math::Vector3& b)
        {
            return
            {
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            };
        }

        float LengthSquared(const Math::Vector3& value)
        {
            return value.x * value.x + value.y * value.y + value.z * value.z;
        }

        Math::Vector3 Normalize(const Math::Vector3& value)
        {
            const float lengthSquared = LengthSquared(value);
            if (lengthSquared <= 0.000001f)
            {
                return { 0.0f, 1.0f, 0.0f };
            }

            const float invLength = 1.0f / std::sqrt(lengthSquared);
            return { value.x * invLength, value.y * invLength, value.z * invLength };
        }

        bool HasNormal(const Renderer::Vertex& vertex)
        {
            return vertex.normal[0] != 0.0f || vertex.normal[1] != 0.0f || vertex.normal[2] != 0.0f;
        }

        void AddNormal(Renderer::Vertex& vertex, const Math::Vector3& normal)
        {
            vertex.normal[0] += normal.x;
            vertex.normal[1] += normal.y;
            vertex.normal[2] += normal.z;
        }

        void AddTangent(Renderer::Vertex& vertex, const Math::Vector3& tangent)
        {
            vertex.tangent[0] += tangent.x;
            vertex.tangent[1] += tangent.y;
            vertex.tangent[2] += tangent.z;
        }

        void NormalizeNormalAndTangent(Renderer::Vertex& vertex)
        {
            const Math::Vector3 normal = Normalize({ vertex.normal[0], vertex.normal[1], vertex.normal[2] });
            const Math::Vector3 tangent = Normalize({ vertex.tangent[0], vertex.tangent[1], vertex.tangent[2] });
            vertex.normal[0] = normal.x;
            vertex.normal[1] = normal.y;
            vertex.normal[2] = normal.z;
            vertex.tangent[0] = tangent.x;
            vertex.tangent[1] = tangent.y;
            vertex.tangent[2] = tangent.z;
        }

        void GenerateMissingNormalsAndTangents(OBJMeshData& meshData)
        {
            if (meshData.indices.size() < 3)
            {
                return;
            }

            std::vector<bool> hadNormals(meshData.vertices.size(), false);
            for (std::size_t i = 0; i < meshData.vertices.size(); ++i)
            {
                hadNormals[i] = HasNormal(meshData.vertices[i]);
                if (!hadNormals[i])
                {
                    meshData.vertices[i].normal[0] = 0.0f;
                    meshData.vertices[i].normal[1] = 0.0f;
                    meshData.vertices[i].normal[2] = 0.0f;
                }
            }

            for (std::size_t i = 0; i + 2 < meshData.indices.size(); i += 3)
            {
                Renderer::Vertex& v0 = meshData.vertices[meshData.indices[i + 0]];
                Renderer::Vertex& v1 = meshData.vertices[meshData.indices[i + 1]];
                Renderer::Vertex& v2 = meshData.vertices[meshData.indices[i + 2]];
                const Math::Vector3 edge1 = Subtract(v1, v0);
                const Math::Vector3 edge2 = Subtract(v2, v0);
                const Math::Vector3 faceNormal = Normalize(Cross(edge1, edge2));

                if (!hadNormals[meshData.indices[i + 0]]) AddNormal(v0, faceNormal);
                if (!hadNormals[meshData.indices[i + 1]]) AddNormal(v1, faceNormal);
                if (!hadNormals[meshData.indices[i + 2]]) AddNormal(v2, faceNormal);

                const float du1 = v1.uv[0] - v0.uv[0];
                const float dv1 = v1.uv[1] - v0.uv[1];
                const float du2 = v2.uv[0] - v0.uv[0];
                const float dv2 = v2.uv[1] - v0.uv[1];
                const float determinant = du1 * dv2 - du2 * dv1;
                Math::Vector3 tangent = { 1.0f, 0.0f, 0.0f };
                if (std::abs(determinant) > 0.000001f)
                {
                    const float inv = 1.0f / determinant;
                    tangent =
                    {
                        (edge1.x * dv2 - edge2.x * dv1) * inv,
                        (edge1.y * dv2 - edge2.y * dv1) * inv,
                        (edge1.z * dv2 - edge2.z * dv1) * inv
                    };
                }
                AddTangent(v0, tangent);
                AddTangent(v1, tangent);
                AddTangent(v2, tangent);
            }

            for (Renderer::Vertex& vertex : meshData.vertices)
            {
                NormalizeNormalAndTangent(vertex);
            }
        }
    }

    bool OBJLoader::Load(const std::string& path, OBJMeshData& outMeshData)
    {
        std::vector<OBJMeshPart> parts;
        if (!LoadParts(path, parts))
        {
            return false;
        }

        outMeshData.vertices.clear();
        outMeshData.indices.clear();
        outMeshData.materialLibraryPath.clear();
        outMeshData.objectName.clear();
        outMeshData.materialName.clear();

        for (const OBJMeshPart& part : parts)
        {
            if (outMeshData.materialLibraryPath.empty())
            {
                outMeshData.materialLibraryPath = part.meshData.materialLibraryPath;
            }
            if (outMeshData.objectName.empty())
            {
                outMeshData.objectName = part.name;
            }
            if (outMeshData.materialName.empty())
            {
                outMeshData.materialName = part.materialName;
            }

            const std::uint32_t baseVertex = static_cast<std::uint32_t>(outMeshData.vertices.size());
            outMeshData.vertices.insert(outMeshData.vertices.end(), part.meshData.vertices.begin(), part.meshData.vertices.end());
            for (std::uint32_t index : part.meshData.indices)
            {
                outMeshData.indices.push_back(baseVertex + index);
            }
        }

        return !outMeshData.vertices.empty() && !outMeshData.indices.empty();
    }

    bool OBJLoader::LoadParts(const std::string& path, std::vector<OBJMeshPart>& outParts)
    {
        outParts.clear();

        std::ifstream file(path);
        if (!file.is_open())
        {
            Core::Log::Error("Failed to open OBJ file: " + path);
            return false;
        }

        std::vector<Math::Vector3> positions;
        std::vector<Math::Vector2> texcoords;
        std::vector<Math::Vector3> normals;
        std::unordered_map<std::string, std::uint32_t> vertexCache;
        OBJMeshPart currentPart;
        currentPart.name = "default";

        auto flushPart = [&]()
        {
            if (!currentPart.meshData.vertices.empty() && !currentPart.meshData.indices.empty())
            {
                GenerateMissingNormalsAndTangents(currentPart.meshData);
                outParts.push_back(currentPart);
            }
            const std::wstring materialLibrary = currentPart.meshData.materialLibraryPath;
            currentPart = {};
            currentPart.name = "default";
            currentPart.meshData.materialLibraryPath = materialLibrary;
            vertexCache.clear();
        };

        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
            {
                continue;
            }

            std::stringstream lineStream(line);
            std::string type;
            lineStream >> type;

            if (type == "v")
            {
                Math::Vector3 position = {};
                lineStream >> position.x >> position.y >> position.z;
                positions.push_back(position);
            }
            else if (type == "vt")
            {
                Math::Vector2 texcoord = {};
                lineStream >> texcoord.x >> texcoord.y;
                texcoords.push_back(texcoord);
            }
            else if (type == "vn")
            {
                Math::Vector3 normal = {};
                lineStream >> normal.x >> normal.y >> normal.z;
                normals.push_back(normal);
            }
            else if (type == "mtllib")
            {
                std::string materialLibrary;
                lineStream >> materialLibrary;
                currentPart.meshData.materialLibraryPath = ToWideAscii(materialLibrary);
            }
            else if (type == "o" || type == "g")
            {
                std::string name;
                lineStream >> name;
                if (!currentPart.meshData.indices.empty())
                {
                    flushPart();
                }
                currentPart.name = name.empty() ? "default" : name;
                currentPart.meshData.objectName = currentPart.name;
            }
            else if (type == "usemtl")
            {
                std::string materialName;
                lineStream >> materialName;
                if (!currentPart.meshData.indices.empty())
                {
                    flushPart();
                }
                currentPart.materialName = materialName;
                currentPart.meshData.materialName = materialName;
            }
            else if (type == "f")
            {
                std::vector<IndexTriplet> face;
                std::string token;
                while (lineStream >> token)
                {
                    face.push_back(ParseFaceVertex(token));
                }

                if (face.size() < 3)
                {
                    continue;
                }

                for (std::size_t i = 1; i + 1 < face.size(); ++i)
                {
                    const IndexTriplet triangle[] = { face[0], face[i], face[i + 1] };
                    bool validTriangle = true;
                    for (const IndexTriplet& triplet : triangle)
                    {
                        const int positionIndex = ResolveIndex(triplet.position, positions.size());
                        if (!IsValidIndex(positionIndex, positions.size()))
                        {
                            validTriangle = false;
                            break;
                        }
                    }

                    if (!validTriangle)
                    {
                        Core::Log::Warning("OBJ face skipped an invalid position index.");
                        continue;
                    }

                    for (const IndexTriplet& triplet : triangle)
                    {
                        const std::string key = MakeVertexKey(triplet);
                        if (auto it = vertexCache.find(key); it != vertexCache.end())
                        {
                            currentPart.meshData.indices.push_back(it->second);
                            continue;
                        }

                        const int positionIndex = ResolveIndex(triplet.position, positions.size());
                        const int texcoordIndex = ResolveIndex(triplet.texcoord, texcoords.size());
                        const int normalIndex = ResolveIndex(triplet.normal, normals.size());

                        Renderer::Vertex vertex = {};
                        const Math::Vector3& position = positions[static_cast<std::size_t>(positionIndex)];
                        vertex.position[0] = position.x;
                        vertex.position[1] = position.y;
                        vertex.position[2] = position.z;
                        vertex.color[0] = 1.0f;
                        vertex.color[1] = 1.0f;
                        vertex.color[2] = 1.0f;
                        vertex.color[3] = 1.0f;

                        if (IsValidIndex(normalIndex, normals.size()))
                        {
                            const Math::Vector3& normal = normals[static_cast<std::size_t>(normalIndex)];
                            vertex.normal[0] = normal.x;
                            vertex.normal[1] = normal.y;
                            vertex.normal[2] = normal.z;
                        }
                        else
                        {
                            vertex.normal[1] = 1.0f;
                        }

                        if (IsValidIndex(texcoordIndex, texcoords.size()))
                        {
                            const Math::Vector2& texcoord = texcoords[static_cast<std::size_t>(texcoordIndex)];
                            vertex.uv[0] = texcoord.x;
                            vertex.uv[1] = 1.0f - texcoord.y;
                        }

                        const std::uint32_t newIndex = static_cast<std::uint32_t>(currentPart.meshData.vertices.size());
                        currentPart.meshData.vertices.push_back(vertex);
                        vertexCache.emplace(key, newIndex);
                        currentPart.meshData.indices.push_back(newIndex);
                    }
                }
            }
        }

        flushPart();
        return !outParts.empty();
    }
}
